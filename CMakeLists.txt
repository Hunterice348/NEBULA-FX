cmake_minimum_required(VERSION 3.20)
project(NEBULA_FX VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.6
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(NEBULA_FX
    COMPANY_NAME "Hunterice Records"
    BUNDLE_ID "com.hunterice.nebula.fx"
    PLUGIN_MANUFACTURER_CODE Hntr
    PLUGIN_CODE Nfx1
    PRODUCT_NAME "NEBULA FX"
    DESCRIPTION "Modular Industrial Audio Effects"
    FORMATS VST3 Standalone
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD TRUE
)

target_sources(NEBULA_FX PRIVATE Source/Plugin.cpp)

target_link_libraries(NEBULA_FX PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_gui_extra
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
)

target_compile_definitions(NEBULA_FX PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
)
