cmake_minimum_required(VERSION 3.22)

project(GLAZE VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Use a local JUCE checkout if present, otherwise fetch it.
if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/CMakeLists.txt")
    add_subdirectory(JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.4
        GIT_SHALLOW ON)
    FetchContent_MakeAvailable(JUCE)
endif()

juce_add_plugin(Glaze
    COMPANY_NAME "Glaze Audio"
    BUNDLE_ID com.glazeaudio.glaze
    PLUGIN_MANUFACTURER_CODE Glzz
    PLUGIN_CODE Glz1
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "Glaze"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    MICROPHONE_PERMISSION_ENABLED TRUE
    MICROPHONE_PERMISSION_TEXT "Glaze uses the microphone in standalone mode."
    AU_MAIN_TYPE kAudioUnitType_Effect
    VST3_CATEGORIES Fx Vocals
    COPY_PLUGIN_AFTER_BUILD FALSE)

juce_add_binary_data(GlazeData SOURCES
    Resources/Righteous-Regular.ttf
    Resources/SpaceMono-Regular.ttf
    Resources/SpaceMono-Bold.ttf)

target_sources(Glaze PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_definitions(Glaze PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(Glaze
    PRIVATE
        GlazeData
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# Offline test harness (not shipped): renders test signals through the processor.
option(GLAZE_BUILD_TESTS "Build offline DSP test" OFF)
if (GLAZE_BUILD_TESTS)
    juce_add_console_app(GlazeTest PRODUCT_NAME "GlazeTest")
    target_sources(GlazeTest PRIVATE Tests/GlazeTest.cpp Source/PluginProcessor.cpp)
    target_compile_definitions(GlazeTest PRIVATE GLAZE_HEADLESS=1 JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
    target_link_libraries(GlazeTest PRIVATE juce::juce_audio_utils juce::juce_dsp juce::juce_recommended_config_flags)

    # Renders every page of the UI to PNGs (for checking the design without a DAW)
    juce_add_console_app(GlazeShot PRODUCT_NAME "GlazeShot")
    target_sources(GlazeShot PRIVATE Tests/GlazeShot.cpp Source/PluginProcessor.cpp Source/PluginEditor.cpp)
    target_compile_definitions(GlazeShot PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JucePlugin_Name="Glaze")
    target_link_libraries(GlazeShot PRIVATE GlazeData juce::juce_audio_utils juce::juce_dsp juce::juce_recommended_config_flags)
endif()
