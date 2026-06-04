#pragma once
#include <JuceHeader.h>
#include "Parameters.h"
#include "dsp/DCOffset.h"
#include "dsp/PhaseProcessor.h"
#include "dsp/GainStage.h"
#include "dsp/MSProcessor.h"
#include "dsp/ChannelProcessor.h"
#include "dsp/Panner.h"
#include "dsp/Metering.h"
#include "dsp/PhaseMeter.h"
#include "dsp/Oscilloscope.h"
#include "dsp/LUFSMeter.h"
#include "recorder/AudioRecorder.h"

class UTTProcessor : public juce::AudioProcessor
{
public:
    UTTProcessor();
    ~UTTProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    bool isBusesLayoutSupported(const juce::AudioProcessor::BusesLayout& layouts) const override;
    const juce::String getName() const override { return "UTT"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return "Default"; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    void setRecordPath(const juce::String& path) { recordPath = path; }
    juce::String getRecordPath() const { return recordPath; }
    void setCustomThemePath(const juce::String& path) { customThemePath = path; }
    juce::String getCustomThemePath() const { return customThemePath; }

    // DSP accessors for GUI
    const UTT::Metering&       getMetering() const       { return metering; }
    const UTT::PhaseMeter&     getPhaseMeter() const     { return phaseMeter; }
    const UTT::Oscilloscope&   getOscilloscope() const   { return oscilloscope; }
    const UTT::LUFSMeter&      getLUFSMeter() const      { return lufsMeter; }
    UTT::AudioRecorder&        getRecorder()             { return recorder; }

    juce::AudioProcessorValueTreeState apvts;

private:
    juce::String recordPath;
    juce::String customThemePath;

    UTT::DCOffset           dcOffset;
    UTT::PhaseProcessor     phaseProc;
    UTT::GainStage          gainStage;
    UTT::MSProcessor        msProc;
    UTT::ChannelProcessor   channelProc;
    UTT::Panner             panner;
    UTT::Metering           metering;
    UTT::PhaseMeter         phaseMeter;
    UTT::Oscilloscope       oscilloscope;
    UTT::LUFSMeter          lufsMeter;
    UTT::AudioRecorder      recorder;

    JUCE_DECLARE_NON_COPYABLE(UTTProcessor)
};
