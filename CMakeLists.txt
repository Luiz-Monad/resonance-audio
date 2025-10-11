# Copyright 2018 Google Inc. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS-IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# Static/dynamic runtime can be selected via -DSTATIC_MSVC_RUNTIME=ON|OFF
macro(configure_msvc_runtime)
    if (MSVC AND NOT BUILD_WWISE_AUTHORING_PLUGIN)
        option(STATIC_MSVC_RUNTIME "Use static MSVC runtime (/MT, /MTd)" ON)
        if (STATIC_MSVC_RUNTIME)
            set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE STRING "" FORCE)
        else()
            set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL$<$<CONFIG:Debug>:Debug>" CACHE STRING "" FORCE)
        endif()
    endif()
    if (MSVC)
        message(STATUS "MSVC runtime: ${CMAKE_MSVC_RUNTIME_LIBRARY}")
    endif()
endmacro()

macro(use_cxx17)
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)
endmacro(use_cxx17)

# Build options
option(BUILD_RESONANCE_AUDIO_API "Build Resonance Audio API." OFF)
option(BUILD_RESONANCE_AUDIO_TESTS "Build Resonance Audio Unit Tests" OFF)
option(BUILD_UNITY_PLUGIN "Build Unity Resonance Audio Plugin" OFF)
option(BUILD_GEOMETRICAL_ACOUSTICS_TESTS "Build Resonance Audio's Geometrical Acoustics Tests" OFF)
option(BUILD_WWISE_AUTHORING_PLUGIN "Build Resonance Audio WWise Authoring Plugin" OFF)
option(BUILD_WWISE_SOUND_ENGINE_PLUGIN "Build Resonance Audio WWise Sound Engine Plugin." OFF)
option(BUILD_FMOD_PLUGIN "Build FMOD Resonance Audio Plugin" OFF)
option(BUILD_VST_MONITOR_PLUGIN "Build Resonance Audio VST Monitor Plugin" ON)

if (IOS_PLATFORM STREQUAL "OS" OR IOS_PLATFORM MATCHES "SIMULATOR")
    message(STATUS "iOS toolchain detected")
    set (IOS_DETECTED TRUE CACHE BOOL "IOS platform detected")
    add_definitions(-DPLATFORM_IOS)
elseif(ANDROID)
    add_definitions(-DPLATFORM_ANDROID)
endif ()

use_cxx17()
configure_msvc_runtime()
cmake_minimum_required(VERSION 3.30)
project(ResonanceAudio)

set(CMAKE_COLOR_MAKEFILE ON)

set(INSTALL_DIR "${PROJECT_SOURCE_DIR}/install" CACHE PATH "Install path")

set(RA_SOURCE_DIR ${PROJECT_SOURCE_DIR}/resonance_audio/)

if (BUILD_RESONANCE_AUDIO_TESTS OR BUILD_GEOMETRICAL_ACOUSTICS_TESTS)
    enable_testing()

    set(GTEST_DIR "${PROJECT_SOURCE_DIR}/third_party/googletest/" CACHE PATH "Path to GTest library")
    # Compile Google Test as an object library
    add_library(gtest OBJECT "${GTEST_DIR}/googlemock/src/gmock-all.cc"
            "${GTEST_DIR}/googletest/src/gtest-all.cc"
            "${GTEST_DIR}/googletest/src/gtest_main.cc")
    target_include_directories(gtest PUBLIC "${GTEST_DIR}/googlemock/")
    target_include_directories(gtest PUBLIC "${GTEST_DIR}/googlemock/include")
    target_include_directories(gtest PUBLIC "${GTEST_DIR}/googletest/")
    target_include_directories(gtest PUBLIC "${GTEST_DIR}/googletest/include")
endif (BUILD_RESONANCE_AUDIO_TESTS OR BUILD_GEOMETRICAL_ACOUSTICS_TESTS)

if (WIN32)
    add_definitions(-D_USE_MATH_DEFINES)
    add_definitions(-DNOMINMAX)
    # Disable type conversion warnings.
    add_definitions(/wd4244)
    add_definitions(/wd4305)
    add_definitions(/wd4722)
elseif (APPLE)
elseif (ANDROID)
    set(NEON ON)
elseif (UNIX)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
    set(CMAKE_CXX_FLAGS "-fPIC -O3 -Wno-vla -msse -msse2 -msse3 -mfpmath=sse")
endif ()

# Add Eigen3.
set(EIGEN3_DIR "${PROJECT_SOURCE_DIR}/third_party/eigen/" CACHE PATH "Path to eigen3 library")
set(EIGEN3_INCLUDE_DIR ${EIGEN3_DIR})
add_definitions(-DEIGEN_MPL2_ONLY)

include_directories(${PROJECT_SOURCE_DIR})
include_directories(${EIGEN3_INCLUDE_DIR})

add_subdirectory(resonance_audio)
add_subdirectory(platforms)

# ----------------------------------------------------------------------
# Import Steinberg VST3 SDK
# ----------------------------------------------------------------------
if (BUILD_VST_MONITOR_PLUGIN)
    set(VST3_SDK_PATH "${CMAKE_SOURCE_DIR}/third_party/VST_SDK" CACHE PATH "Path to VST3 SDK")

    # Include VST3 SDK submodules (headers and sources)
    set(VST3_SDK_INCLUDE_DIRS
        ${VST3_SDK_PATH}
        ${VST3_SDK_PATH}/base
        ${VST3_SDK_PATH}/pluginterfaces
        ${VST3_SDK_PATH}/public.sdk
        ${VST3_SDK_PATH}/public.sdk/source/vst
        ${VST3_SDK_PATH}/public.sdk/source/common
        ${VST3_SDK_PATH}/public.sdk/source/vst/vstgui
        ${VST3_SDK_PATH}/public.sdk/source/vst/vst2wrapper
        ${VST3_SDK_PATH}/vstgui4
        ${VST3_SDK_PATH}/vstgui4/vstgui
        ${VST3_SDK_PATH}/vstgui4/vstgui/lib
        ${VST3_SDK_PATH}/vstgui4/vstgui/uidescription
    )

    # Disable unwanted VST3 example builds
    set(SMTG_ENABLE_VST3_PLUGIN_EXAMPLES OFF CACHE BOOL "Enable VST 3 Plug-in Examples")
    set(SMTG_ENABLE_VST3_HOSTING_EXAMPLES OFF CACHE BOOL "Enable VST 3 Hosting Examples")
    set(SMTG_ENABLE_VSTGUI_SUPPORT ON CACHE BOOL "Enable VSTGUI Support")
    add_subdirectory(${VST3_SDK_PATH})

endif (BUILD_VST_MONITOR_PLUGIN)
