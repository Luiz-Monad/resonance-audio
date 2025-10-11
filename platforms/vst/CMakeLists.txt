# Copyright 2018 Google Inc. All Rights Reserved.
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
cmake_minimum_required(VERSION 3.30)
project(ResonanceAudioMonitorVst)

# ----------------------------------------------------------------------
# Helper: Create Windows resource file
# ----------------------------------------------------------------------
function(create_windows_resource_file RESOURCES)
    set(RESOURCES_LIST)
    foreach (RESOURCE_DIR ${RESOURCES})
        get_filename_component(RESOURCE_FILENAME ${RESOURCE_DIR} NAME)
        get_filename_component(RESOURCE_EXT ${RESOURCE_DIR} EXT)
        string(REPLACE "." "" RESOURCE_EXT_WITHOUT_DOT ${RESOURCE_EXT})
        string(TOUPPER ${RESOURCE_EXT_WITHOUT_DOT} UPPER_RESOURCE_EXT_WITHOUT_DOT)
        list(APPEND RESOURCES_LIST "${RESOURCE_FILENAME}\t${UPPER_RESOURCE_EXT_WITHOUT_DOT}\t\"${RESOURCE_DIR}\"\n")
    endforeach ()
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/resource.rc ${RESOURCES_LIST})
endfunction()

# ----------------------------------------------------------------------
# Include directories
# ----------------------------------------------------------------------
include_directories(${CMAKE_SOURCE_DIR}/resonance_audio)
include_directories(${VST3_SDK_INCLUDE_DIRS})

# ----------------------------------------------------------------------
# Source files
# ----------------------------------------------------------------------
set(RESONANCE_AUDIO_MONITOR_VST
    ${PROJECT_SOURCE_DIR}/binaural_renderer_factory.cc
    ${PROJECT_SOURCE_DIR}/binaural_renderer_gui.cc
    ${PROJECT_SOURCE_DIR}/binaural_renderer_gui.h
    ${PROJECT_SOURCE_DIR}/binaural_renderer_vst.cc
    ${PROJECT_SOURCE_DIR}/binaural_renderer_vst.h
)

set(RESONANCE_AUDIO_MONITOR_RESOURCES
    ${PROJECT_SOURCE_DIR}/resources/resonance_audio.png
)

# ----------------------------------------------------------------------
# Plugin target
# ----------------------------------------------------------------------
add_library(ResonanceAudioMonitorVst MODULE
    ${RESONANCE_AUDIO_MONITOR_VST}
    $<TARGET_OBJECTS:ResonanceAudioObj>
    $<TARGET_OBJECTS:SadieHrtfsObj>
    $<TARGET_OBJECTS:PffftObj>
)

target_include_directories(ResonanceAudioMonitorVst PUBLIC
    ${PROJECT_SOURCE_DIR}
)

# ----------------------------------------------------------------------
# Link against official VST3 SDK targets
# ----------------------------------------------------------------------
target_link_libraries(ResonanceAudioMonitorVst PRIVATE sdk vstgui_support)

# ----------------------------------------------------------------------
# Platform-specific settings
# ----------------------------------------------------------------------
if (WIN32)
    create_windows_resource_file("${RESONANCE_AUDIO_MONITOR_RESOURCES}")
    target_sources(ResonanceAudioMonitorVst PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}/resource.rc
    )
    add_definitions(-D_CRT_SECURE_NO_DEPRECATE=1)

    install(TARGETS ResonanceAudioMonitorVst
        DESTINATION "C:/Program Files/Common Files/VST3/"
    )

elseif(APPLE)
    set_source_files_properties(${RESONANCE_AUDIO_MONITOR_RESOURCES} PROPERTIES
        MACOSX_PACKAGE_LOCATION Resources
    )

    set(RESOURCE_FOLDER "${PROJECT_SOURCE_DIR}/resources/")
    set(PKG_INFO ${RESOURCE_FOLDER}/PkgInfo)
    set_source_files_properties("${PKG_INFO}" PROPERTIES MACOSX_PACKAGE_LOCATION .)

    target_sources(ResonanceAudioMonitorVst PRIVATE ${PKG_INFO})

    set_target_properties(ResonanceAudioMonitorVst PROPERTIES
        BUNDLE true
        BUNDLE_EXTENSION "vst"
        XCODE_ATTRIBUTE_WRAPPER_EXTENSION "vst"
        MACOSX_BUNDLE_INFO_PLIST "${RESOURCE_FOLDER}/Info.plist.in"
        MACOSX_BUNDLE_BUNDLE_NAME "ResonanceAudioMonitorVst"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.google.vr.resonance_audio_monitor"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "0.0.1"
        MACOSX_BUNDLE_COPYRIGHT "Google 2017"
    )

    target_link_libraries(ResonanceAudioMonitorVst PRIVATE
        "-framework Cocoa"
        "-framework OpenGL"
        "-framework Accelerate"
        "-framework QuartzCore"
        "-framework Carbon"
    )

    install(TARGETS ResonanceAudioMonitorVst
        DESTINATION "$ENV{HOME}/Library/Audio/Plug-Ins/VST"
    )
endif()

# ----------------------------------------------------------------------
# Misc
# ----------------------------------------------------------------------
target_sources(ResonanceAudioMonitorVst PRIVATE ${RESONANCE_AUDIO_MONITOR_RESOURCES})
