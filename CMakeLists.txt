cmake_minimum_required(VERSION 3.22)
project(PsyLabPlugins VERSION 0.2.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW ON)
FetchContent_MakeAvailable(JUCE)

set(COMMON_SOURCES
    Common/PsyUI.cpp
    Common/PresetBar.cpp)

function(add_psy_plugin TARGET CODE PROD_NAME)
    juce_add_plugin(${TARGET}
        COMPANY_NAME "PsyLab"
        PLUGIN_MANUFACTURER_CODE Psyl
        PLUGIN_CODE ${CODE}
        FORMATS VST3 Standalone
        IS_SYNTH TRUE
        NEEDS_MIDI_INPUT TRUE
        NEEDS_MIDI_OUTPUT FALSE
        PRODUCT_NAME "${PROD_NAME}"
        VST3_CATEGORIES Instrument Synth Sampler)
    target_include_directories(${TARGET} PRIVATE Common Source Guitar Prism)
    target_compile_definitions(${TARGET} PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_MODAL_LOOPS_PERMITTED=1)
    target_link_libraries(${TARGET}
        PRIVATE
            juce::juce_audio_utils
            juce::juce_audio_formats
            juce::juce_dsp
        PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_lto_flags)
endfunction()

add_psy_plugin(Eclipse Ecps "Eclipse Psy Sampler")
target_sources(Eclipse PRIVATE ${COMMON_SOURCES} Source/PluginProcessor.cpp Source/PluginEditor.cpp)

add_psy_plugin(Helios Hlgt "Helios Guitar")
target_sources(Helios PRIVATE ${COMMON_SOURCES} Guitar/GuitarProcessor.cpp Guitar/GuitarEditor.cpp)

add_psy_plugin(Prism Prsm "Prism Filter")
target_sources(Prism PRIVATE ${COMMON_SOURCES} Prism/PrismProcessor.cpp Prism/PrismEditor.cpp)

juce_add_plugin(Kino
    COMPANY_NAME "PsyLab"
    PLUGIN_MANUFACTURER_CODE Psyl
    PLUGIN_CODE Kino
    FORMATS VST3 Standalone
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    PRODUCT_NAME "Kino MIDI"
    VST3_CATEGORIES Instrument Synth Generator)
target_include_directories(Kino PRIVATE Common Kino)
target_compile_definitions(Kino PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_MODAL_LOOPS_PERMITTED=1)
target_link_libraries(Kino
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_formats
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags)
target_sources(Kino PRIVATE ${COMMON_SOURCES} Kino/KinoProcessor.cpp Kino/KinoEditor.cpp)
