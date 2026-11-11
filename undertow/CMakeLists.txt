cmake_minimum_required(VERSION 3.22)
project(UNDERTOW VERSION 2.0.0)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/CMakeLists.txt")
    add_subdirectory(JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG 8.0.4 GIT_SHALLOW ON)
    FetchContent_MakeAvailable(JUCE)
endif()

juce_add_plugin(Undertow
    COMPANY_NAME "Token Wise"
    BUNDLE_ID com.tokenwise.undertow
    PLUGIN_MANUFACTURER_CODE Tkwz
    PLUGIN_CODE Udtw
    FORMATS VST3 AU
    PRODUCT_NAME "Undertow"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    AU_MAIN_TYPE kAudioUnitType_MusicDevice
    VST3_CATEGORIES Instrument Synth
    COPY_PLUGIN_AFTER_BUILD FALSE)

juce_add_binary_data(UndertowData SOURCES
    Resources/Inter-Black.otf Resources/Inter-Bold.otf Resources/Inter-Medium.otf Resources/SpaceMono-Bold.ttf)

target_sources(Undertow PRIVATE Source/PluginProcessor.cpp Source/PluginEditor.cpp)
target_compile_definitions(Undertow PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0 JUCE_DISPLAY_SPLASH_SCREEN=0)
target_link_libraries(Undertow
    PRIVATE UndertowData juce::juce_audio_utils
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)

# Generator invariants. No JUCE, so this can run anywhere the plugin builds.
add_executable(UndertowGenTest Tests/GenTest.cpp)
target_compile_features(UndertowGenTest PRIVATE cxx_std_17)

option(UNDERTOW_BUILD_TESTS "Build offline test/screenshot tool" OFF)
if (UNDERTOW_BUILD_TESTS)
    juce_add_console_app(UndertowShot PRODUCT_NAME "UndertowShot")
    target_sources(UndertowShot PRIVATE Tests/UndertowShot.cpp Source/PluginProcessor.cpp Source/PluginEditor.cpp)
    target_compile_definitions(UndertowShot PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JucePlugin_Name="Undertow")
    target_link_libraries(UndertowShot PRIVATE UndertowData juce::juce_audio_utils juce::juce_recommended_config_flags)
endif()

juce_add_gui_app(UndertowStage PRODUCT_NAME "Undertow" MICROPHONE_PERMISSION_ENABLED FALSE CAMERA_PERMISSION_ENABLED FALSE)
target_sources(UndertowStage PRIVATE Tests/UndertowStage.cpp Source/PluginProcessor.cpp Source/PluginEditor.cpp)
target_compile_definitions(UndertowStage PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_DISPLAY_SPLASH_SCREEN=0 JucePlugin_Name="Undertow")
target_link_libraries(UndertowStage PRIVATE UndertowData juce::juce_audio_utils juce::juce_recommended_config_flags)

option(UNDERTOW_BUILD_DEMO "Build the offline video demo renderer" OFF)
if (UNDERTOW_BUILD_DEMO)
    juce_add_console_app(UndertowDemo PRODUCT_NAME "UndertowDemo")
    target_sources(UndertowDemo PRIVATE Tests/UndertowDemo.cpp Source/PluginProcessor.cpp Source/PluginEditor.cpp)
    target_compile_definitions(UndertowDemo PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JucePlugin_Name="Undertow")
    target_link_libraries(UndertowDemo PRIVATE UndertowData juce::juce_audio_utils juce::juce_recommended_config_flags)
endif()
