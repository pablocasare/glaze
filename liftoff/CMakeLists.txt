cmake_minimum_required(VERSION 3.22)
project(LIFTOFF VERSION 1.0.0)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/CMakeLists.txt")
    add_subdirectory(JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG 8.0.4 GIT_SHALLOW ON)
    FetchContent_MakeAvailable(JUCE)
endif()

juce_add_plugin(Liftoff
    COMPANY_NAME "Glaze Audio"
    BUNDLE_ID com.glazeaudio.liftoff
    PLUGIN_MANUFACTURER_CODE Glzz
    PLUGIN_CODE Lft1
    FORMATS VST3 AU
    PRODUCT_NAME "Liftoff"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    AU_MAIN_TYPE kAudioUnitType_Effect
    VST3_CATEGORIES Fx
    COPY_PLUGIN_AFTER_BUILD FALSE)

juce_add_binary_data(LiftoffData SOURCES
    Resources/Righteous-Regular.ttf
    Resources/SpaceMono-Bold.ttf)

target_sources(Liftoff PRIVATE Source/PluginProcessor.cpp Source/PluginEditor.cpp)
target_compile_definitions(Liftoff PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0 JUCE_DISPLAY_SPLASH_SCREEN=0)
target_link_libraries(Liftoff
    PRIVATE LiftoffData juce::juce_audio_utils juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)

option(LIFTOFF_BUILD_TESTS "Build offline tests" OFF)
if (LIFTOFF_BUILD_TESTS)
    juce_add_console_app(LiftoffTest PRODUCT_NAME "LiftoffTest")
    target_sources(LiftoffTest PRIVATE Tests/LiftoffTest.cpp Source/PluginProcessor.cpp Source/PluginEditor.cpp)
    target_compile_definitions(LiftoffTest PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JucePlugin_Name="Liftoff")
    target_link_libraries(LiftoffTest PRIVATE LiftoffData juce::juce_audio_utils juce::juce_dsp juce::juce_recommended_config_flags)
endif()
