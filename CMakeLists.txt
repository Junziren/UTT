cmake_minimum_required(VERSION 3.22)
project(UTT VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(juce)

set(UTT_SOURCES
    src/PluginProcessor.cpp
    src/PluginEditor.cpp
    src/dsp/DCOffset.cpp
    src/dsp/PhaseProcessor.cpp
    src/dsp/GainStage.cpp
    src/dsp/MSProcessor.cpp
    src/dsp/ChannelProcessor.cpp
    src/dsp/Panner.cpp
    src/dsp/Metering.cpp
    src/dsp/PhaseMeter.cpp
    src/dsp/Oscilloscope.cpp
    src/dsp/LUFSMeter.cpp
    src/recorder/AudioRecorder.cpp
    src/gui/theme/UTTLookAndFeel.cpp
    src/gui/theme/ThemeLoader.cpp
)

juce_add_plugin(UTT
    COMPANY_NAME "UTT"
    PLUGIN_MANUFACTURER_CODE UT01
    PLUGIN_CODE UT02
    FORMATS VST3
    PRODUCT_NAME "UTT"
    VERSION "0.1.0"
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER "UTT Tools"
    PLUGIN_DESCRIPTION "Utility Toolbox"
)

target_sources(UTT PRIVATE ${UTT_SOURCES})
target_include_directories(UTT PRIVATE src)
target_link_libraries(UTT PRIVATE juce::juce_audio_formats)

target_compile_definitions(UTT PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_IGNORE_VST3_MISMATCHED_PARAMETER_ID_WARNING=1
)
