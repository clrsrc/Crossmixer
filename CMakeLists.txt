cmake_minimum_required(VERSION 3.22)
project(Crossmixer VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(JUCE)

juce_add_gui_app(Crossmixer
    PRODUCT_NAME "Crossmixer"
    COMPANY_NAME "Crossmixer"
    BUNDLE_ID "com.crossmixer.app"
)

target_sources(Crossmixer PRIVATE
    Source/Main.cpp
    Source/MainWindow.cpp
    Source/Audio/WasapiLoopbackCapture.cpp
    Source/Audio/WasapiMultichannelOutput.cpp
    Source/Audio/AudioEngine.cpp
    Source/Audio/ChannelProcessor.cpp
    Source/Audio/StereoTo51Router.cpp
    Source/Audio/DeviceChangeNotifier.cpp
    Source/GUI/MainComponent.cpp
    Source/GUI/ChannelStrip.cpp
    Source/GUI/MasterSection.cpp
    Source/GUI/DeviceSelector.cpp
    Source/GUI/PresetBar.cpp
    Source/GUI/SpectrumAnalyzer.cpp
    Source/GUI/PeakMeter.cpp
    Source/GUI/CrossmixerLookAndFeel.cpp
    Source/Preset/PresetManager.cpp
    Source/Preset/ParameterLayout.cpp
)

target_include_directories(Crossmixer PRIVATE Source)

target_compile_definitions(Crossmixer PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:Crossmixer,JUCE_PRODUCT_NAME>"
    JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:Crossmixer,JUCE_VERSION>"
)

target_link_libraries(Crossmixer PRIVATE
    juce::juce_core
    juce::juce_data_structures
    juce::juce_events
    juce::juce_graphics
    juce::juce_gui_basics
    juce::juce_gui_extra
    juce::juce_dsp
    juce::juce_audio_basics
    juce::juce_audio_formats
    ole32
    uuid
    avrt
)

if(MSVC)
    target_compile_options(Crossmixer PRIVATE /W3 /MP)
endif()
