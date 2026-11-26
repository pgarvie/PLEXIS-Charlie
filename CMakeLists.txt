# =============================================================================
#  PLEXIS Labs — Charlie (synthé virtuel analogique 2 oscillateurs)
#  Construction : cmake -B build -A x64  &&  cmake --build build --config Release
# =============================================================================
cmake_minimum_required(VERSION 3.22)
project(Charlie VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")  # pas de dépendance VC++ redist

# ---- JUCE : copie locale (-DJUCE_DIR=...) ou téléchargement automatique ---------
set(JUCE_DIR "" CACHE PATH "Chemin vers une copie locale de JUCE (optionnel)")
if(JUCE_DIR)
    add_subdirectory(${JUCE_DIR} JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        8.0.4
        GIT_SHALLOW    TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

option(CHARLIE_BUILD_STANDALONE "Construire aussi l'application autonome" ON)
set(CHARLIE_FORMATS VST3)
if(CHARLIE_BUILD_STANDALONE)
    list(APPEND CHARLIE_FORMATS Standalone)
endif()

juce_add_plugin(Charlie
    COMPANY_NAME                "PLEXIS Labs"
    COMPANY_WEBSITE             ""
    BUNDLE_ID                   com.plexislabs.charlie
    PLUGIN_MANUFACTURER_CODE    Plxs
    PLUGIN_CODE                 Chrl
    PRODUCT_NAME                "Charlie"
    DESCRIPTION                 "2-OSC Virtual Analog Synthesizer"
    IS_SYNTH                    TRUE
    NEEDS_MIDI_INPUT            TRUE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    VST3_CATEGORIES             Instrument Synth
    COPY_PLUGIN_AFTER_BUILD     FALSE
    FORMATS                     ${CHARLIE_FORMATS})

target_sources(Charlie PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/Parameters.cpp)

target_include_directories(Charlie PRIVATE Source)

target_compile_definitions(Charlie PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(Charlie
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_processors
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# ---- Tests du moteur DSP (C++ pur, sans JUCE) ------------------------------------
add_executable(CharlieDspTests tests/dsp_tests.cpp)
target_include_directories(CharlieDspTests PRIVATE Source)
