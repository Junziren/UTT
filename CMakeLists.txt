cmake_minimum_required(VERSION 3.22)
project(UTT VERSION 0.3.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    # JUCE/HarfBuzz sources are UTF-8; avoid code-page 936 warnings and mojibake
    add_compile_options(/utf-8)
endif()

add_subdirectory(juce)

# ---------------------------------------------------------------------------
# Embedded WebView frontend (offline, served via ResourceProvider)
# ---------------------------------------------------------------------------
juce_add_binary_data(UTTUI
    SOURCES resources/ui/index.html
            resources/ui/utt.css
            resources/ui/utt.js)

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
    src/gui/UTTWebView.cpp
    src/gui/FallbackPanel.cpp
)

juce_add_plugin(UTT
    COMPANY_NAME "UnpureBloom"
    PLUGIN_MANUFACTURER_CODE UT01
    PLUGIN_CODE UT02
    FORMATS VST3 Standalone
    PRODUCT_NAME "UTT"
    VERSION "0.3.0"
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    NEEDS_WEBVIEW2 TRUE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER "UnpureBloom"
    PLUGIN_DESCRIPTION "UTT - Utility Toolbox by UnpureBloom"
)

target_sources(UTT PRIVATE ${UTT_SOURCES})
target_include_directories(UTT PRIVATE src src/util)
target_link_libraries(UTT PRIVATE
    juce::juce_audio_formats
    juce::juce_audio_utils
    juce::juce_gui_extra
    UTTUI)

target_compile_definitions(UTT PRIVATE
    JUCE_WEB_BROWSER=1
    JUCE_USE_WIN_WEBVIEW2=1
    JUCE_USE_CURL=0
    JUCE_IGNORE_VST3_MISMATCHED_PARAMETER_ID_WARNING=1
)

# ---------------------------------------------------------------------------
# Console unit-test runner (DSP calibration, contract, recorder, state)
# ---------------------------------------------------------------------------
juce_add_console_app(UTTTests
    PRODUCT_NAME "UTTTests"
    COMPANY_NAME "UnpureBloom")

target_sources(UTTTests PRIVATE src/PluginProcessor.cpp src/PluginEditor.cpp
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
    src/gui/UTTWebView.cpp
    src/gui/FallbackPanel.cpp
    src/tests/UTTDspTests.cpp)
target_include_directories(UTTTests PRIVATE src src/util)
target_link_libraries(UTTTests PRIVATE
    juce::juce_audio_processors
    juce::juce_audio_formats
    juce::juce_gui_basics
    juce::juce_gui_extra
    juce::juce_webview2
    UTTUI)

target_compile_definitions(UTTTests PRIVATE
    JUCE_WEB_BROWSER=1
    JUCE_USE_WIN_WEBVIEW2=1
    JUCE_USE_CURL=0
    JUCE_IGNORE_VST3_MISMATCHED_PARAMETER_ID_WARNING=1
    UTT_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
