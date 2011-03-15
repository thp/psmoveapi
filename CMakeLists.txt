
cmake_minimum_required(VERSION 2.6)

project(PSMOVEAPI)

include_directories(${PSMOVEAPI_SOURCE_DIR}/hidapi/hidapi)
add_definitions(-g -Wall)

set(PSMOVE_REQUIRED_LIBS)

IF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    find_library(IOKIT IOKit)
    find_library(COREFOUNDATION CoreFoundation)
    list(APPEND PSMOVE_REQUIRED_LIBS ${IOKIT})
    list(APPEND PSMOVE_REQUIRED_LIBS ${COREFOUNDATION})

    set(HIDAPI_SRC ${PSMOVEAPI_SOURCE_DIR}/hidapi/mac/hid.c)
ELSE(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    find_package(PkgConfig REQUIRED)

    pkg_check_modules(LIBUSB REQUIRED libusb-1.0)
    include_directories(${LIBUSB_INCLUDE_DIRS})
    list(APPEND PSMOVE_REQUIRED_LIBS ${LIBUSB_LIBRARIES})

    pkg_check_modules(UDEV REQUIRED libudev)
    include_directories(${UDEV_INCLUDE_DIRS})
    list(APPEND PSMOVE_REQUIRED_LIBS ${UDEV_LIBRARIES})

    set(HIDAPI_SRC ${PSMOVEAPI_SOURCE_DIR}/hidapi/linux/hid-libusb.c)
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

set(PSMOVE_SRC ${HIDAPI_SRC} ${PSMOVEAPI_SOURCE_DIR}/psmove.c)

# C examples

add_executable(example c/example.c ${PSMOVE_SRC})
target_link_libraries(example ${PSMOVE_REQUIRED_LIBS})

# Python bindings

find_package(SWIG REQUIRED)
include(${SWIG_USE_FILE})
find_package(PythonLibs)

include_directories(${PYTHON_INCLUDE_PATH})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

set(CMAKE_SWIG_FLAGS "")

set_source_files_properties(psmove.i PROPERTIES SWIG_FLAGS "")
swig_add_module(psmove python psmove.i ${PSMOVE_SRC})
swig_link_libraries(psmove ${PYTHON_LIBRARIES} ${PSMOVE_REQUIRED_LIBS})

# Qt bindings

find_package(Qt4 REQUIRED)
add_subdirectory(qt)

