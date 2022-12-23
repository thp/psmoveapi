cmake_minimum_required(VERSION 3.16)

project(PSMoveAPI)

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/hidapi/hidapi/hidapi.h")
    message(FATAL_ERROR "hidapi not found, did you forget to run 'git submodule update --init'?")
endif()

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/libusb-1.0/libusb/libusb.h")
    message(FATAL_ERROR "libusb not found, did you forget to run 'git submodule update --init'?")
endif()

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/PS3EYEDriver/src/ps3eye.h")
    message(FATAL_ERROR "PS3EYEDriver not found, did you forget to run 'git submodule update --init'?")
endif()

# get rid of Visual Studio's default "Debug" and "Release" output directories
if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG   ${CMAKE_BINARY_DIR})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG   ${CMAKE_BINARY_DIR})
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG   ${CMAKE_BINARY_DIR})

    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR})
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR})
endif()

# Step into the subdirectories
include("src/CMakeLists.txt")
include("examples/CMakeLists.txt")
message("")
