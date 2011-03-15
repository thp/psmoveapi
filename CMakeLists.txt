
cmake_minimum_required(VERSION 2.6)

project(PSMOVEAPI)

include_directories(${PSMOVEAPI_SOURCE_DIR}/hidapi)
add_definitions(-g -Wall)

set(CMAKE_C_FLAGS -std=c99)

find_library(IOKIT IOKit)
find_library(COREFOUNDATION CoreFoundation)

# C examples

add_executable(example hidapi/hid.c psmove.c c/example.c)
target_link_libraries(example ${IOKIT} ${COREFOUNDATION})

# Python bindings

find_package(SWIG REQUIRED)
include(${SWIG_USE_FILE})
find_package(PythonLibs)

include_directories(${PYTHON_INCLUDE_PATH})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

set(CMAKE_SWIG_FLAGS "")

set_source_files_properties(psmove.i PROPERTIES SWIG_FLAGS "")
swig_add_module(psmove python psmove.i hidapi/hid.c psmove.c)
swig_link_libraries(psmove ${PYTHON_LIBRARIES} ${IOKIT} ${COREFOUNDATION})

# Qt bindings

find_package(Qt4 REQUIRED)
add_subdirectory(qt)

