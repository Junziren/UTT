cmake_minimum_required(VERSION 3.22)
project(UTT VERSION 0.3.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# On macOS JUCE embeds the AUv3 .appex in the Standalone .app container. The
# resulting application is the installable AUv3 product, so it is enabled in
# desktop release builds by default.
option(UTT_BUILD_AUV3 "Build the AUv3 extension embedded in the macOS Standalone app" ON)

if(MSVC)
    # JUCE/HarfBuzz sources are UTF-8; avoid code-page 936 warnings and mojibake
    add_compile_options(/utf-8)
endif()

add_subdirectory(juce)

# ---------------------------------------------------------------------------
# Embedded WebView frontend (offline, served via ResourceProvider)
# ---------------------------------------------------------------------------
set(UTT_UI_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/ui/index.html"
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/ui/styles.css"
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/ui/app.js")

# Reconfigure when an embedded UI asset changes so the cache-busting token is
# regenerated alongside BinaryData.
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${UTT_UI_SOURCES})
file(SHA256 "${CMAKE_CURRENT_SOURCE_DIR}/resources/ui/index.html" UTT_UI_HTML_HASH)
file(SHA256 "${CMAKE_CURRENT_SOURCE_DIR}/resources/ui/styles.css" UTT_UI_CSS_HASH)
file(SHA256 "${CMAKE_CURRENT_SOURCE_DIR}/resources/ui/app.js" UTT_UI_JS_HASH)
string(SHA256 UTT_UI_BUILD_HASH "${UTT_UI_HTML_HASH}${UTT_UI_CSS_HASH}${UTT_UI_JS_HASH}")
string(SUBSTRING "${UTT_UI_BUILD_HASH}" 0 12 UTT_UI_BUILD_ID)

juce_add_binary_data(UTTUI
    SOURCES ${UTT_UI_SOURCES})

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

# Format matrix:
#   Windows/Linux (this repo's first-class lane): VST3 + Standalone harness
#   macOS:  VST3 + AU + AUv3 + Standalone (JUCE embeds AUv3 in Standalone)
#   iOS:    configure with -DCMAKE_SYSTEM_NAME=iOS for AUv3 + Standalone
#           inside a container app target (UNVERIFIED, see docs/APPLE_LANE.md)
if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
    set(UTT_FORMATS AUv3 Standalone)
elseif(APPLE)
    set(UTT_FORMATS VST3 AU Standalone)
    if(UTT_BUILD_AUV3)
        if(NOT CMAKE_GENERATOR STREQUAL "Xcode")
            message(FATAL_ERROR
                "UTT_BUILD_AUV3 requires the Xcode generator so JUCE can embed UTT.appex in UTT.app.")
        endif()
        list(APPEND UTT_FORMATS AUv3)
    endif()
elseif(WIN32)
    set(UTT_FORMATS VST3 Standalone)
    set(UTT_NEEDS_WEBVIEW2 NEEDS_WEBVIEW2 TRUE)
else()
    set(UTT_FORMATS VST3 Standalone)
endif()

juce_add_plugin(UTT
    COMPANY_NAME "UnpureBloom"
    PLUGIN_MANUFACTURER_CODE UT01
    PLUGIN_CODE UT02
    FORMATS ${UTT_FORMATS}
    ${UTT_NEEDS_WEBVIEW2}
    PRODUCT_NAME "UTT"
    VERSION "${PROJECT_VERSION}"
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
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

set(UTT_COMMON_COMPILE_DEFINITIONS
    JUCE_WEB_BROWSER=1
    JUCE_USE_CURL=0
    JUCE_IGNORE_VST3_MISMATCHED_PARAMETER_ID_WARNING=1)

if(WIN32)
    list(APPEND UTT_COMMON_COMPILE_DEFINITIONS JUCE_USE_WIN_WEBVIEW2=1)
endif()

target_compile_definitions(UTT PRIVATE
    ${UTT_COMMON_COMPILE_DEFINITIONS}
    UTT_UI_BUILD_ID="${UTT_UI_BUILD_ID}")

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
    UTTUI)

if(WIN32)
    target_link_libraries(UTTTests PRIVATE juce::juce_webview2)
endif()

target_compile_definitions(UTTTests PRIVATE
    ${UTT_COMMON_COMPILE_DEFINITIONS}
    UTT_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    UTT_UI_BUILD_ID="${UTT_UI_BUILD_ID}")

# NEEDS_WEBVIEW2 links WebView2LoaderStatic.lib, while JUCE needs this
# definition to call the statically linked loader instead of searching for a
# WebView2Loader.dll beside the plugin.
if(WIN32)
    target_compile_definitions(UTT PRIVATE JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1)
    target_compile_definitions(UTTTests PRIVATE JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1)
endif()
