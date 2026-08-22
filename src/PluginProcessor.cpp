#include "util/CrashLog.h"

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

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
    // The chain is stereo internally; we honestly declare only the layouts we
    // actually process end-to-end (mono input is duplicated, mono output
    // receives the processed left channel).
    const auto& in  = layouts.getMainInputChannelSet();
    const auto& out = layouts.getMainOutputChannelSet();

    if (in == juce::AudioChannelSet::stereo() && out == juce::AudioChannelSet::stereo())
        return true;
    if (in == juce::AudioChannelSet::mono() && out == juce::AudioChannelSet::stereo())
        return true;
    if (in == juce::AudioChannelSet::mono() && out == juce::AudioChannelSet::mono())
        return true;

    return false;
}

void UTTProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    UTT_TRACE();
    stereoScratch.setSize(2, juce::jmax(32, samplesPerBlock), false, false, true);

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

    resetSmoothedValues();
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

void UTTProcessor::readChainParameters()
{
    bool invL = false, invR = false;
    float phaseDeg = 0.0f;
    if (auto* p = apvts.getRawParameterValue("phaseInvertL")) invL = p->load() > 0.5f;
    if (auto* p = apvts.getRawParameterValue("phaseInvertR")) invR = p->load() > 0.5f;
    if (auto* p = apvts.getRawParameterValue("phaseOffset"))  phaseDeg = p->load();
    phaseProc.setInvert(invL, invR);
    phaseProc.setPhaseOffset(phaseDeg);

    if (auto* p = apvts.getRawParameterValue("gain"))
        gainStage.setGain(p->load());

    if (auto* p = apvts.getRawParameterValue("msMode"))
        msProc.setMode(static_cast<int>(p->load()));

    if (auto* p = apvts.getRawParameterValue("monoMode"))
        channelProc.setMonoMode(static_cast<int>(p->load()));
    if (auto* p = apvts.getRawParameterValue("stereoSeparation"))
        channelProc.setSeparation(p->load());
    if (auto* p = apvts.getRawParameterValue("monoToStereo"))
        channelProc.setMonoToStereo(static_cast<int>(p->load()));

    bool swap = false;
    if (auto* p = apvts.getRawParameterValue("channelSwap"))
        swap = p->load() > 0.5f;

    if (auto* p = apvts.getRawParameterValue("pan"))
        panner.setPan(p->load());

    float outLGain = 0.0f, outRGain = 0.0f;
    bool muteL = false, muteR = false, soloL = false, soloR = false;
    if (auto* p = apvts.getRawParameterValue("outputLGain")) outLGain = p->load();
    if (auto* p = apvts.getRawParameterValue("outputRGain")) outRGain = p->load();
    if (auto* p = apvts.getRawParameterValue("outputLMute")) muteL = p->load() > 0.5f;
    if (auto* p = apvts.getRawParameterValue("outputRMute")) muteR = p->load() > 0.5f;
    if (auto* p = apvts.getRawParameterValue("outputLSolo")) soloL = p->load() > 0.5f;
    if (auto* p = apvts.getRawParameterValue("outputRSolo")) soloR = p->load() > 0.5f;
    gainStage.setOutputStage(outLGain, outRGain, muteL, muteR, soloL, soloR);

    doSwapCurrent = swap;
}

void UTTProcessor::processStereoChunk(juce::AudioBuffer<float>& buffer, int)
{
    // 1. DC offset removal (parameter is a bypass; on = filter engaged)
    bool dcOn = false;
    if (auto* p = apvts.getRawParameterValue("dcOffsetBypass"))
        dcOn = !(p->load() > 0.5f);
    if (dcOn)
        dcOffset.process(buffer);

    readChainParameters();

    // 2. Phase invert + phase offset
    phaseProc.process(buffer);

    // 3. Input gain
    gainStage.process(buffer);

    // 4. MS encode/decode
    msProc.process(buffer);

    // 5. Mono operations
    channelProc.process(buffer);

    // 6. Stereo separation + mono-to-stereo copy
    channelProc.processPostSeparation(buffer);

    // 7. Channel swap
    if (doSwapCurrent)
        channelProc.swap(buffer);

    // 8. Pan
    panner.process(buffer);

    // 9. Output stage (gains + mute/solo)
    gainStage.processOutputStage(buffer);

    // Analysis (post-chain, never alters audio)
    metering.process(buffer);
    phaseMeter.process(buffer);
    oscilloscope.process(buffer);
    lufsMeter.process(buffer);
}

void UTTProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int totalSamples = buffer.getNumSamples();
    if (totalSamples == 0 || buffer.getNumChannels() == 0)
        return;

    // Sanitise non-finite input before anything else can latch onto it.
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        float* d = buffer.getWritePointer(ch);
        for (int i = 0; i < totalSamples; ++i)
            if (!std::isfinite(d[i]))
                d[i] = 0.0f;
    }

    // Recording arm is block-synchronous; the recorder enqueues internally
    // without touching files from this thread.
    bool recordOn = false;
    if (auto* p = apvts.getRawParameterValue("recordEnable"))
        recordOn = p->load() > 0.5f;
    recorder.setRecordEnabled(recordOn);

    const bool wasMono = buffer.getNumChannels() == 1;
    const float* const* recChannels = nullptr;

    // The chain always runs on the preallocated stereo scratch, in chunks if
    // the host block exceeds its capacity - no allocation on this thread.
    int offset = 0;
    while (offset < totalSamples)
    {
        const int n = juce::jmin(totalSamples - offset, stereoScratch.getNumSamples());

        if (wasMono)
        {
            stereoScratch.copyFrom(0, 0, buffer, 0, offset, n);
            stereoScratch.copyFrom(1, 0, buffer, 0, offset, n);
        }
        else
        {
            stereoScratch.copyFrom(0, 0, buffer, 0, offset, n);
            stereoScratch.copyFrom(1, 0, buffer, 1, offset, n);
        }

        processStereoChunk(stereoScratch, n);

        if (wasMono)
            buffer.copyFrom(0, offset, stereoScratch, 0, 0, n);
        else
        {
            buffer.copyFrom(0, offset, stereoScratch, 0, 0, n);
            buffer.copyFrom(1, offset, stereoScratch, 1, 0, n);
        }

        if (recorder.isActive() && recChannels == nullptr)
            recChannels = stereoScratch.getArrayOfReadPointers();

        if (recChannels != nullptr)
            recorder.process(recChannels, n);

        offset += n;
    }
}

void UTTProcessor::resetSmoothedValues()
{
    auto read = [this](const char* id, float fallback) {
        auto* p = apvts.getRawParameterValue(id);
        return p != nullptr ? p->load() : fallback;
    };
    auto readBool = [this](const char* id, bool fallback) {
        auto* p = apvts.getRawParameterValue(id);
        return p != nullptr ? (p->load() > 0.5f) : fallback;
    };

    gainStage.setGainImmediate(read("gain", 0.0f));
    gainStage.setOutputStageImmediate(read("outputLGain", 0.0f), read("outputRGain", 0.0f),
                                      readBool("outputLMute", false), readBool("outputRMute", false),
                                      readBool("outputLSolo", false), readBool("outputRSolo", false));
    panner.setPanImmediate(read("pan", 0.0f));
    channelProc.setSeparationImmediate(read("stereoSeparation", 100.0f));
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
    state.setProperty("uiTheme", uiTheme, nullptr);
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void UTTProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    UTT_TRACE();
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml == nullptr)
        return;

    auto tree = juce::ValueTree::fromXml(*xml);
    if (tree.hasType(apvts.state.getType()))
    {
        // Legacy v0.2 states containing meter*/themeIndex parameters are
        // tolerated: replaceState silently ignores IDs missing from the
        // current layout, and every surviving DSP parameter keeps its ID.
        apvts.replaceState(tree);
    }

    recordPath = tree.getProperty("recordPath", "");
    customThemePath = tree.getProperty("customThemePath", "");
    uiTheme = tree.getProperty("uiTheme", 0);

    recorder.setPath(recordPath);
    resetSmoothedValues();
}

void UTTProcessor::setRecordPath(const juce::String& path)
{
    recordPath = path;
    recorder.setPath(path);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    UTT::installCrashHandler();
    return new UTTProcessor();
}
