cmake_minimum_required(VERSION 3.22)
project(PUMP VERSION 1.0.0)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/CMakeLists.txt")
    add_subdirectory(JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG 8.0.4 GIT_SHALLOW ON)
    FetchContent_MakeAvailable(JUCE)
endif()

juce_add_plugin(Pump
    COMPANY_NAME "Glaze Audio"
    BUNDLE_ID com.glazeaudio.pump
    PLUGIN_MANUFACTURER_CODE Glzz
    PLUGIN_CODE Pmp1
    FORMATS VST3 AU
    PRODUCT_NAME "Pump"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    AU_MAIN_TYPE kAudioUnitType_Effect
    VST3_CATEGORIES Fx Dynamics
    COPY_PLUGIN_AFTER_BUILD FALSE)

juce_add_binary_data(PumpData SOURCES
    Resources/Righteous-Regular.ttf
    Resources/SpaceMono-Bold.ttf)

target_sources(Pump PRIVATE Source/PluginProcessor.cpp Source/PluginEditor.cpp)
target_compile_definitions(Pump PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0 JUCE_DISPLAY_SPLASH_SCREEN=0)
target_link_libraries(Pump
    PRIVATE PumpData juce::juce_audio_utils juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)

option(PUMP_BUILD_TESTS "Build offline tests" OFF)
if (PUMP_BUILD_TESTS)
    juce_add_console_app(PumpTest PRODUCT_NAME "PumpTest")
    target_sources(PumpTest PRIVATE Tests/PumpTest.cpp Source/PluginProcessor.cpp Source/PluginEditor.cpp)
    target_compile_definitions(PumpTest PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JucePlugin_Name="Pump")
    target_link_libraries(PumpTest PRIVATE PumpData juce::juce_audio_utils juce::juce_dsp juce::juce_recommended_config_flags)
endif()
