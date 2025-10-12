# Copyright 2025 Luiz Felipe Stangarlin
# Licensed under the Apache License, Version 2.0 (the "License");
# You may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#     http://www.apache.org/licenses/LICENSE-2.0
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS-IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# limitations under the License.
#

# ----------------------------------------------------------------------
# Project setup
# ----------------------------------------------------------------------
cmake_minimum_required(VERSION 3.30.0)

project(ResonanceAudioMonitorVst
    VERSION 1.0.0
    DESCRIPTION "Resonance Audio Monitor VST3 Plugin"
)

if(NOT SMTG_ENABLE_VSTGUI_SUPPORT)
    message(FATAL_ERROR "VSTGUI support is required for ResonanceAudioMonitorVst")
endif()

set(VST3_SDK_PATH "${CMAKE_SOURCE_DIR}/third_party/VST_SDK" CACHE PATH "Path to VST3 SDK")
set(CMAKE_MODULE_PATH ${VST3_SDK_PATH}/cmake/modules)
include(SMTG_VST3_SDK)

# ----------------------------------------------------------------------
# Source files
# ----------------------------------------------------------------------
set(resonance_audio_monitor_sources
    ${PROJECT_SOURCE_DIR}/binaural_renderer_factory.cc
    ${PROJECT_SOURCE_DIR}/binaural_renderer_gui.cc
    ${PROJECT_SOURCE_DIR}/binaural_renderer_gui.h
    ${PROJECT_SOURCE_DIR}/binaural_renderer_controller.cc
    ${PROJECT_SOURCE_DIR}/binaural_renderer_controller.h
    ${PROJECT_SOURCE_DIR}/binaural_renderer_processor.cc
    ${PROJECT_SOURCE_DIR}/binaural_renderer_processor.h
)

# ----------------------------------------------------------------------
# Plugin target using SDK helper
# ----------------------------------------------------------------------
smtg_add_vst3plugin(ResonanceAudioMonitorVst 
    SOURCES_LIST ${resonance_audio_monitor_sources}
)

smtg_target_configure_version_file(ResonanceAudioMonitorVst)

# Add object libraries as dependencies
target_sources(ResonanceAudioMonitorVst PRIVATE
    $<TARGET_OBJECTS:ResonanceAudioObj>
    $<TARGET_OBJECTS:SadieHrtfsObj>
    $<TARGET_OBJECTS:PffftObj>
)

# ----------------------------------------------------------------------
# Include directories
# ----------------------------------------------------------------------
target_include_directories(ResonanceAudioMonitorVst PUBLIC
    ${PROJECT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/resonance_audio
)

# ----------------------------------------------------------------------
# Resources
# ----------------------------------------------------------------------
smtg_target_add_plugin_resources(ResonanceAudioMonitorVst
    RESOURCES
        ${PROJECT_SOURCE_DIR}/resources/resonance_audio.png
)

# ----------------------------------------------------------------------
# Compiler features and linking
# ----------------------------------------------------------------------
target_compile_features(ResonanceAudioMonitorVst
    PUBLIC
        cxx_std_17
)

target_link_libraries(ResonanceAudioMonitorVst
    PRIVATE 
        sdk
        vstgui_support
        base
)

# ----------------------------------------------------------------------
# Targets
# ----------------------------------------------------------------------
if(SMTG_WIN)
    # Let SDK handle Windows resources automatically
    # Remove your create_windows_resource_file calls
    
    install(TARGETS ResonanceAudioMonitorVst
        DESTINATION "$ENV{ProgramFiles}/Common Files/VST3"
    )
endif()

if(SMTG_MAC)
    smtg_target_set_bundle(ResonanceAudioMonitorVst
        BUNDLE_IDENTIFIER "com.google.vr.resonance_audio_monitor"
        COMPANY_NAME "Google"
        BUNDLE_NAME "Resonance Audio Monitor"
    )

    target_link_libraries(ResonanceAudioMonitorVst PRIVATE
        "-framework Cocoa"
        "-framework OpenGL"
        "-framework Accelerate"
        "-framework QuartzCore"
        "-framework Carbon"
    )

    install(TARGETS ResonanceAudioMonitorVst
        DESTINATION "$ENV{HOME}/Library/Audio/Plug-Ins/VST3"
    )
endif()
