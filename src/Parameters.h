#pragma once
#include <JuceHeader.h>

namespace UTT {

    inline const juce::ParameterID PID_dcOffsetBypass   { "dcOffsetBypass",   1 };
    inline const juce::ParameterID PID_phaseInvertL     { "phaseInvertL",     1 };
    inline const juce::ParameterID PID_phaseInvertR     { "phaseInvertR",     1 };
    inline const juce::ParameterID PID_phaseOffset      { "phaseOffset",      1 };
    inline const juce::ParameterID PID_gain             { "gain",             1 };
    inline const juce::ParameterID PID_msMode           { "msMode",           1 };
    inline const juce::ParameterID PID_monoMode         { "monoMode",         1 };
    inline const juce::ParameterID PID_stereoSeparation { "stereoSeparation", 1 };
    inline const juce::ParameterID PID_monoToStereo     { "monoToStereo",     1 };
    inline const juce::ParameterID PID_channelSwap      { "channelSwap",      1 };
    inline const juce::ParameterID PID_pan              { "pan",              1 };

    inline const juce::ParameterID PID_outputLGain      { "outputLGain",      1 };
    inline const juce::ParameterID PID_outputRGain      { "outputRGain",      1 };
    inline const juce::ParameterID PID_outputLMute      { "outputLMute",      1 };
    inline const juce::ParameterID PID_outputRMute      { "outputRMute",      1 };
    inline const juce::ParameterID PID_outputLSolo      { "outputLSolo",      1 };
    inline const juce::ParameterID PID_outputRSolo      { "outputRSolo",      1 };

    inline const juce::ParameterID PID_meterLPeak       { "meterLPeak",       1 };
    inline const juce::ParameterID PID_meterRPeak       { "meterRPeak",       1 };
    inline const juce::ParameterID PID_meterLRMS        { "meterLRMS",        1 };
    inline const juce::ParameterID PID_meterRRMS        { "meterRRMS",        1 };
    inline const juce::ParameterID PID_phaseCorrelation { "phaseCorrelation", 1 };
    inline const juce::ParameterID PID_lufsMomentary    { "lufsMomentary",    1 };
    inline const juce::ParameterID PID_lufsShortTerm    { "lufsShortTerm",    1 };
    inline const juce::ParameterID PID_lufsIntegrated   { "lufsIntegrated",   1 };

    inline const juce::ParameterID PID_recordEnable     { "recordEnable",     1 };
    inline const juce::ParameterID PID_themeIndex       { "themeIndex",       1 };

    // Helper: build a StringArray safely (no initializer_list for VST3 safety)
    inline juce::StringArray makeChoices(std::initializer_list<const char*> items)
    {
        juce::StringArray arr;
        for (auto& s : items) arr.add(s);
        return arr;
    }

    inline juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
    {
        juce::AudioProcessorValueTreeState::ParameterLayout layout;

        // DSP
        layout.add(std::make_unique<juce::AudioParameterBool>(PID_dcOffsetBypass,   "DC Offset",            true));
        layout.add(std::make_unique<juce::AudioParameterBool>(PID_phaseInvertL,     "Phase Invert L",       false));
        layout.add(std::make_unique<juce::AudioParameterBool>(PID_phaseInvertR,     "Phase Invert R",       false));
        layout.add(std::make_unique<juce::AudioParameterFloat>(PID_phaseOffset,     "Phase Offset",         0.0f, 360.0f, 0.0f));
        layout.add(std::make_unique<juce::AudioParameterFloat>(PID_gain,            "Gain",                 -96.0f, 24.0f, 0.0f));
        layout.add(std::make_unique<juce::AudioParameterChoice>(PID_msMode,         "MS Mode",
            makeChoices({"Bypass", "Encode", "Decode"}), 0));
        layout.add(std::make_unique<juce::AudioParameterChoice>(PID_monoMode,       "Mono Mode",
            makeChoices({"Off", "L+R Sum", "Mid Only", "Side Only"}), 0));
        layout.add(std::make_unique<juce::AudioParameterFloat>(PID_stereoSeparation,"Stereo Separation",    0.0f, 100.0f, 100.0f));
        layout.add(std::make_unique<juce::AudioParameterChoice>(PID_monoToStereo,   "Mono to Stereo",
            makeChoices({"Off", "L->Both", "R->Both"}), 0));
        layout.add(std::make_unique<juce::AudioParameterBool>(PID_channelSwap,      "Channel Swap",          false));
        layout.add(std::make_unique<juce::AudioParameterFloat>(PID_pan,             "Pan",                   -100.0f, 100.0f, 0.0f));

        // Output Stage
        layout.add(std::make_unique<juce::AudioParameterFloat>(PID_outputLGain,     "Output L Gain",         -96.0f, 12.0f, 0.0f));
        layout.add(std::make_unique<juce::AudioParameterFloat>(PID_outputRGain,     "Output R Gain",         -96.0f, 12.0f, 0.0f));
        layout.add(std::make_unique<juce::AudioParameterBool>(PID_outputLMute,      "Output L Mute",         false));
        layout.add(std::make_unique<juce::AudioParameterBool>(PID_outputRMute,      "Output R Mute",         false));
        layout.add(std::make_unique<juce::AudioParameterBool>(PID_outputLSolo,      "Output L Solo",         false));
        layout.add(std::make_unique<juce::AudioParameterBool>(PID_outputRSolo,      "Output R Solo",         false));

        // Metering
        layout.add(std::make_unique<juce::AudioParameterFloat>(PID_meterLPeak,      "Meter L Peak",          -96.0f, 0.0f, -96.0f));
        layout.add(std::make_unique<juce::AudioParameterFloat>(PID_meterRPeak,      "Meter R Peak",          -96.0f, 0.0f, -96.0f));
        layout.add(std::make_unique<juce::AudioParameterFloat>(PID_meterLRMS,       "Meter L RMS",           -96.0f, 0.0f, -96.0f));
        layout.add(std::make_unique<juce::AudioParameterFloat>(PID_meterRRMS,       "Meter R RMS",           -96.0f, 0.0f, -96.0f));
        layout.add(std::make_unique<juce::AudioParameterFloat>(PID_phaseCorrelation, "Phase Correlation",    -1.0f, 1.0f, 0.0f));
        layout.add(std::make_unique<juce::AudioParameterFloat>(PID_lufsMomentary,   "LUFS Momentary",        -70.0f, 0.0f, -70.0f));
        layout.add(std::make_unique<juce::AudioParameterFloat>(PID_lufsShortTerm,   "LUFS Short-term",       -70.0f, 0.0f, -70.0f));
        layout.add(std::make_unique<juce::AudioParameterFloat>(PID_lufsIntegrated,  "LUFS Integrated",       -70.0f, 0.0f, -70.0f));

        // Recording
        layout.add(std::make_unique<juce::AudioParameterBool>(PID_recordEnable,     "Record Enable",          false));

        // GUI
        layout.add(std::make_unique<juce::AudioParameterChoice>(PID_themeIndex,     "Theme",
            makeChoices({"Channel Strip", "Dark Modern", "Retro Light"}), 0));

        return layout;
    }

} // namespace UTT
