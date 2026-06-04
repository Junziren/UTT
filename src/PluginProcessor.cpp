#include "util/CrashLog.h"
#include "util/TraceLog.h"
#include "PluginProcessor.h"
#include "PluginEditor.h"

UTTProcessor::UTTProcessor()
    : juce::AudioProcessor(BusesProperties()
          .withInput("Input", juce::AudioChannelSet::stereo(), true)
          .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "UTT_PARAMS", UTT::createParameterLayout())
{
    UTT_TRACE();
}

UTTProcessor::~UTTProcessor() = default;

bool UTTProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    // Accept any layout with at least 1 channel per bus
    // Our processBlock handles mono internally by duplicating to stereo
    auto inputChans  = layouts.getMainInputChannels();
    auto outputChans = layouts.getMainOutputChannels();

    // Need at least 1 channel on each side
    if (inputChans == 0 || outputChans == 0)
        return false;

    // Accept any channel count from 1-8
    if (inputChans > 8 || outputChans > 8)
        return false;

    // Must be same channel count on both sides OR mono->stereo OK
    return true;
}

void UTTProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    UTT_TRACE();
    dcOffset.prepare(sampleRate, samplesPerBlock);
    phaseProc.prepare(sampleRate, samplesPerBlock);
    gainStage.prepare(sampleRate, samplesPerBlock);
    msProc.prepare(sampleRate, samplesPerBlock);
    channelProc.prepare(sampleRate, samplesPerBlock);
    panner.prepare(sampleRate, samplesPerBlock);
    metering.prepare(sampleRate, samplesPerBlock);
    phaseMeter.prepare(sampleRate, samplesPerBlock);
    oscilloscope.prepare(sampleRate, samplesPerBlock);
    lufsMeter.prepare(sampleRate, samplesPerBlock);
    recorder.prepare(sampleRate, samplesPerBlock);
}

void UTTProcessor::releaseResources()
{
    dcOffset.reset();
    phaseProc.reset();
    gainStage.reset();
    msProc.reset();
    channelProc.reset();
    panner.reset();
    metering.reset();
    phaseMeter.reset();
    oscilloscope.reset();
    lufsMeter.reset();
    recorder.reset();
}

void UTTProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    auto numChannels = buffer.getNumChannels();
    if (numChannels == 0)
        return;

    // Handle mono input: create a temporary stereo buffer for internal processing
    bool wasMono = (numChannels < 2);
    juce::AudioBuffer<float> stereoBuffer;
    juce::AudioBuffer<float>* procBuf = &buffer;

    if (wasMono)
    {
        // Duplicate the mono input to both L and R for stereo DSP chain
        stereoBuffer.setSize(2, buffer.getNumSamples(), false, false, true);
        stereoBuffer.copyFrom(0, 0, buffer, 0, 0, buffer.getNumSamples());
        stereoBuffer.copyFrom(1, 0, buffer, 0, 0, buffer.getNumSamples());
        procBuf = &stereoBuffer;
    }

    auto& params = apvts;

    // 1. DC Offset
    if (auto* p = params.getRawParameterValue("dcOffsetBypass"))
        if (!p->load()) dcOffset.process(*procBuf);

    // 2. Phase
    bool invL = false, invR = false;
    float phaseDeg = 0.0f;
    if (auto* p = params.getRawParameterValue("phaseInvertL")) invL = p->load() > 0.5f;
    if (auto* p = params.getRawParameterValue("phaseInvertR")) invR = p->load() > 0.5f;
    if (auto* p = params.getRawParameterValue("phaseOffset"))  phaseDeg = p->load();
    phaseProc.setInvert(invL, invR);
    phaseProc.setPhaseOffset(phaseDeg);
    phaseProc.process(*procBuf);

    // 3. Gain
    if (auto* p = params.getRawParameterValue("gain"))
    {
        gainStage.setGain(p->load());
        gainStage.process(*procBuf);
    }

    // 4. MS Encode/Decode
    if (auto* p = params.getRawParameterValue("msMode"))
    {
        msProc.setMode(static_cast<int>(p->load()));
        msProc.process(*procBuf);
    }

    // 5-7. Channel processing
    if (auto* p = params.getRawParameterValue("monoMode"))
    {
        channelProc.setMonoMode(static_cast<int>(p->load()));
        channelProc.process(*procBuf);
    }
    if (auto* p = params.getRawParameterValue("stereoSeparation"))
        channelProc.setSeparation(p->load());
    if (auto* p = params.getRawParameterValue("monoToStereo"))
    {
        channelProc.setMonoToStereo(static_cast<int>(p->load()));
        channelProc.processPostSeparation(*procBuf);
    }

    // 8. Channel swap
    if (auto* p = params.getRawParameterValue("channelSwap"))
        if (p->load() > 0.5f) channelProc.swap(*procBuf);

    // 9. Pan
    if (auto* p = params.getRawParameterValue("pan"))
    {
        panner.setPan(p->load());
        panner.process(*procBuf);
    }

    // 10. Output stage
    float outLGain = 0.0f, outRGain = 0.0f;
    bool muteL = false, muteR = false, soloL = false, soloR = false;
    if (auto* p = params.getRawParameterValue("outputLGain")) outLGain = p->load();
    if (auto* p = params.getRawParameterValue("outputRGain")) outRGain = p->load();
    if (auto* p = params.getRawParameterValue("outputLMute")) muteL = p->load() > 0.5f;
    if (auto* p = params.getRawParameterValue("outputRMute")) muteR = p->load() > 0.5f;
    if (auto* p = params.getRawParameterValue("outputLSolo")) soloL = p->load() > 0.5f;
    if (auto* p = params.getRawParameterValue("outputRSolo")) soloR = p->load() > 0.5f;
    gainStage.setOutputStage(outLGain, outRGain, muteL, muteR, soloL, soloR);
    gainStage.processOutputStage(*procBuf);

    // Metering (always process)
    metering.process(*procBuf);
    phaseMeter.process(*procBuf);
    oscilloscope.process(*procBuf);
    lufsMeter.process(*procBuf);

    // Recover mono output: copy processed L channel back to mono buffer
    if (wasMono)
        buffer.copyFrom(0, 0, *procBuf, 0, 0, buffer.getNumSamples());

    // Write metering values to parameters (with null checks)
    if (auto* p = params.getRawParameterValue("meterLPeak"))       p->store(metering.getLPeak());
    if (auto* p = params.getRawParameterValue("meterRPeak"))       p->store(metering.getRPeak());
    if (auto* p = params.getRawParameterValue("meterLRMS"))        p->store(metering.getLRMS());
    if (auto* p = params.getRawParameterValue("meterRRMS"))        p->store(metering.getRRMS());
    if (auto* p = params.getRawParameterValue("phaseCorrelation")) p->store(phaseMeter.getCorrelation());
    if (auto* p = params.getRawParameterValue("lufsMomentary"))    p->store(lufsMeter.getMomentary());
    if (auto* p = params.getRawParameterValue("lufsShortTerm"))    p->store(lufsMeter.getShortTerm());
    if (auto* p = params.getRawParameterValue("lufsIntegrated"))   p->store(lufsMeter.getIntegrated());

    // Recording
    if (auto* p = params.getRawParameterValue("recordEnable"))
    {
        if (p->load() > 0.5f && recordPath.isNotEmpty())
        {
            recorder.setPath(juce::File(recordPath));
            recorder.startRecording();
            recorder.process(*procBuf);
        }
        else
        {
            recorder.stopRecording();
        }
    }
}

juce::AudioProcessorEditor* UTTProcessor::createEditor()
{
    UTT_TRACE();
    return new UTTEditor(*this);
}

void UTTProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty("recordPath", recordPath, nullptr);
    state.setProperty("customThemePath", customThemePath, nullptr);
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void UTTProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    UTT_TRACE();
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml && xml->hasTagName(apvts.state.getType()))
    {
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
    }
    if (xml)
    {
        auto state = juce::ValueTree::fromXml(*xml);
        recordPath = state.getProperty("recordPath", "");
        customThemePath = state.getProperty("customThemePath", "");
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    UTT::installCrashHandler();
    return new UTTProcessor();
}
