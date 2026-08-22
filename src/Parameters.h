#pragma once
#include <JuceHeader.h>

namespace UTT {

// Parameter contract v2 (see config/parameters-v2.json).
// IDs, order and defaults of the parameters below are release ABI: never
// rename, renumber or reorder existing entries. Meters and UI-local options
// are deliberately NOT host parameters - meters publish through atomics and
// the UI theme lives in a state-tree property, so the host automation list
// and saved state stay clean.
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

inline const juce::ParameterID PID_recordEnable     { "recordEnable",     1 };

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

    // Recording
    layout.add(std::make_unique<juce::AudioParameterBool>(PID_recordEnable,     "Record Enable",          false));

    return layout;
}

} // namespace UTT
