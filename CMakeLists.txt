
cmake_minimum_required(VERSION 2.6)

project(PSMOVEAPI)

include_directories(${PSMOVEAPI_SOURCE_DIR}/hidapi/hidapi)
#add_definitions(-g -Wall)
add_definitions(-O2)

set(PSMOVEAPI_MAJOR_VERSION "1")
set(PSMOVEAPI_MINOR_VERSION "0")
set(PSMOVEAPI_PATCH_VERSION "1")
set(PSMOVEAPI_VERSION "${PSMOVEAPI_MAJOR_VERSION}.${PSMOVEAPI_MINOR_VERSION}.${PSMOVEAPI_PATCH_VERSION}")

set(PSMOVEAPI_VERSION_SONAME "${PSMOVEAPI_MAJOR_VERSION}")

set(PSMOVE_REQUIRED_LIBS)

IF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    find_library(IOKIT IOKit)
    find_library(COREFOUNDATION CoreFoundation)
    find_library(IOBLUETOOTH IOBluetooth)
    list(APPEND PSMOVE_REQUIRED_LIBS ${IOKIT})
    list(APPEND PSMOVE_REQUIRED_LIBS ${COREFOUNDATION})
    list(APPEND PSMOVE_REQUIRED_LIBS ${IOBLUETOOTH})

    set(HIDAPI_SRC ${PSMOVEAPI_SOURCE_DIR}/hidapi/mac/hid.c)
ELSE(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    find_package(PkgConfig REQUIRED)

    pkg_check_modules(LIBUSB REQUIRED libusb-1.0)
    include_directories(${LIBUSB_INCLUDE_DIRS})
    list(APPEND PSMOVE_REQUIRED_LIBS ${LIBUSB_LIBRARIES})

    # Only needed when using hidraw version of hidapi
    #pkg_check_modules(UDEV REQUIRED libudev)
    #include_directories(${UDEV_INCLUDE_DIRS})
    #list(APPEND PSMOVE_REQUIRED_LIBS ${UDEV_LIBRARIES})

    set(HIDAPI_SRC ${PSMOVEAPI_SOURCE_DIR}/hidapi/linux/hid-libusb.c)
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

set(PSMOVE_SRC ${HIDAPI_SRC} ${PSMOVEAPI_SOURCE_DIR}/psmove.c)

# Shared library

add_library(psmoveapi SHARED ${PSMOVE_SRC})
target_link_libraries(psmoveapi ${PSMOVE_REQUIRED_LIBS})
set_target_properties(psmoveapi PROPERTIES VERSION ${PSMOVEAPI_VERSION} SOVERSION ${PSMOVEAPI_VERSION_SONAME})

# C utilities (those get installed on "make install")
add_executable(psmovepair c/pair.c)
target_link_libraries(psmovepair psmoveapi)

# C examples

add_executable(example c/example.c)
target_link_libraries(example psmoveapi)

add_executable(multiple c/multiple.c)
target_link_libraries(multiple psmoveapi)

# Python bindings (PS Move API code statically linked in Python module)

find_package(SWIG REQUIRED)
include(${SWIG_USE_FILE})
find_package(PythonLibs)

include_directories(${PYTHON_INCLUDE_PATH})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

set(CMAKE_SWIG_FLAGS "")

set_source_files_properties(psmove.i PROPERTIES SWIG_FLAGS "")
swig_add_module(psmove python psmove.i ${PSMOVE_SRC})
swig_link_libraries(psmove ${PYTHON_LIBRARIES} ${PSMOVE_REQUIRED_LIBS})

# For system-wide installation

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/psmoveapi.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/psmoveapi.pc
    @ONLY)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/psmoveapi.pc DESTINATION lib/pkgconfig)

install(TARGETS psmovepair psmoveapi
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
)

install(FILES psmove.h DESTINATION include/psmoveapi)
install(FILES README COPYING DESTINATION share/psmoveapi)

# Qt bindings
option(MYGPO_BUILD_TESTS "Build all unit tests" ON)

if(PSMOVEAPI_BUILD_QTAPI)
    add_subdirectory(qt)
endif(PSMOVEAPI_BUILD_QTAPI)

