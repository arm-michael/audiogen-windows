#
# SPDX-FileCopyrightText: Copyright 2025 Arm Limited and/or its affiliates <open-source-office@arm.com>
#
# SPDX-License-Identifier: Apache-2.0
#

cmake_minimum_required(VERSION 3.16)

project(audiogen_runner)

include(ExternalProject)

message(STATUS "CMAKE_SYSTEM_NAME: ${CMAKE_SYSTEM_NAME}")
message(STATUS "CMAKE_HOST_SYSTEM_NAME: ${CMAKE_HOST_SYSTEM_NAME}")

set(CMAKE_CXX_STANDARD 17)

if(CMAKE_TOOLCHAIN_FILE)
  list(APPEND TOOLCHAIN_CMAKE_ARGS -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE})
endif()

if(ANDROID_ABI)
  list(APPEND TOOLCHAIN_CMAKE_ARGS -DANDROID_ABI=${ANDROID_ABI})
endif()

if(NOT TF_SRC_PATH)
  include(FetchContent)

  FetchContent_Declare(
    tensorflow_src
    GIT_REPOSITORY https://github.com/tensorflow/tensorflow.git
    GIT_TAG 5baea41aa158ce6c3396726bd84fda5cd81737a0
  )

  FetchContent_MakeAvailable(tensorflow_src)

  set(TENSORFLOW_SOURCE_DIR ${tensorflow_src_SOURCE_DIR} CACHE STRING "" FORCE)

else()
  # Normalize to forward slashes. On Windows, CMake string parsing treats
  # backslashes as escape characters (\a, \t, etc.), so a path like
  # D:\a\repo\tf_src causes "Invalid character escape '\a'" errors inside
  # TFLite's own CMakeLists.txt. file(TO_CMAKE_PATH) converts to forward
  # slashes on all platforms, making the path safe for CMake string context.
  file(TO_CMAKE_PATH "${TF_SRC_PATH}" TF_SRC_PATH_NORMALIZED)
  set(TENSORFLOW_SOURCE_DIR ${TF_SRC_PATH_NORMALIZED} CACHE STRING "" FORCE)
endif()

set(FLATBUFFERS_SOURCE_DIR ${TENSORFLOW_SOURCE_DIR}/tensorflow/lite/tools/cmake/native_tools/flatbuffers)
set(FLATBUFFERS_BIN_DIR ${CMAKE_BINARY_DIR}/flatbuffers-host-bin)
set(SENTENCEPIECE_SOURCE_DIR ${CMAKE_BINARY_DIR}/sentencepiece)

# Step 1: Build flatc ---
ExternalProject_Add(
  flatc_build
  PREFIX flatc
  DOWNLOAD_COMMAND ""
  SOURCE_DIR ${FLATBUFFERS_SOURCE_DIR}
  BINARY_DIR ${FLATBUFFERS_BIN_DIR}
  INSTALL_COMMAND ""
)

# Step 2: Build SentencePiece ---
ExternalProject_Add(
  sentencepiece_src
  GIT_REPOSITORY https://github.com/google/sentencepiece.git
  GIT_TAG        v0.2.0
  SOURCE_DIR ${SENTENCEPIECE_SOURCE_DIR}
  CMAKE_ARGS
    -DCMAKE_POLICY_VERSION_MINIMUM=3.5
    ${TOOLCHAIN_CMAKE_ARGS}
  INSTALL_COMMAND ""
)

#Get the path to the sentencepiece lib
ExternalProject_Get_Property(sentencepiece_src BINARY_DIR)
if(WIN32)
  set(SENTENCEPIECE_LIB ${BINARY_DIR}/src/Release/sentencepiece.lib)
else()
  set(SENTENCEPIECE_LIB ${BINARY_DIR}/src/libsentencepiece.a)
endif()

## Step 4: Build the audiogen app ---
# Define source
set(SRCS audiogen.cpp)

add_executable(audiogen ${SRCS})

if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64|ARM64)")
  set(XNNPACK_ENABLE_ARM_SME2 ON CACHE BOOL "" FORCE)
else()
  set(XNNPACK_ENABLE_ARM_SME2 OFF CACHE BOOL "" FORCE)
endif()
set(TFLITE_HOST_TOOLS_DIR "${FLATBUFFERS_BIN_DIR}/_deps/flatbuffers-build" CACHE PATH "Host tools directory")

# Because flatc is not available at configure time,
# this workaround places a placeholder at the expected install location.
# On Windows, the MSVC build produces flatc.exe; on Unix it produces flatc.
if(WIN32)
  set(FLATC_PLACEHOLDER ${FLATBUFFERS_BIN_DIR}/_deps/flatbuffers-build/flatc.exe)
else()
  set(FLATC_PLACEHOLDER ${FLATBUFFERS_BIN_DIR}/_deps/flatbuffers-build/flatc)
endif()
file(WRITE ${FLATC_PLACEHOLDER} "")

# LiteRT CMake configure stage
add_subdirectory(
  "${TENSORFLOW_SOURCE_DIR}/tensorflow/lite"
  "${CMAKE_CURRENT_BINARY_DIR}/tensorflow-lite"
  EXCLUDE_FROM_ALL
)

# Delete the placeholder flatc file after the LiteRT CMake configuration stage is complete.
file(REMOVE ${FLATC_PLACEHOLDER} "")

add_dependencies(tensorflow-lite flatc_build)


# Include headers
target_include_directories(audiogen PRIVATE
  ${TENSORFLOW_SOURCE_DIR}/tensorflow/lite
  ${SENTENCEPIECE_SOURCE_DIR}/src
)

# Link with dependencies
target_link_libraries(audiogen
  tensorflow-lite
  ${SENTENCEPIECE_LIB}
)

# Ensure dependency build order
add_dependencies(audiogen flatc_build sentencepiece_src)


