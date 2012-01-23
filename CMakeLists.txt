
cmake_minimum_required(VERSION 2.6)

project(PSMOVEAPI)

include_directories(${PSMOVEAPI_SOURCE_DIR}/hidapi/hidapi)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_definitions(-DPSMOVE_DEBUG)
#add_definitions(-g -Wall)
add_definitions(-O2)

set(PSMOVEAPI_MAJOR_VERSION "1")
set(PSMOVEAPI_MINOR_VERSION "0")
set(PSMOVEAPI_PATCH_VERSION "2")
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
ELSEIF(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    list(APPEND PSMOVE_REQUIRED_LIBS setupapi bthprops kernel32)

    set(HIDAPI_SRC ${PSMOVEAPI_SOURCE_DIR}/hidapi/windows/hid.c)
ELSE()
    # Linux
    find_package(PkgConfig REQUIRED)
    find_package(Threads REQUIRED)

    pkg_check_modules(LIBUSB REQUIRED libusb-1.0)
    include_directories(${LIBUSB_INCLUDE_DIRS})
    list(APPEND PSMOVE_REQUIRED_LIBS ${LIBUSB_LIBRARIES})
    list(APPEND PSMOVE_REQUIRED_LIBS ${CMAKE_THREAD_LIBS_INIT})

    # Only needed when using hidraw version of hidapi
    #pkg_check_modules(UDEV REQUIRED libudev)
    #include_directories(${UDEV_INCLUDE_DIRS})
    #list(APPEND PSMOVE_REQUIRED_LIBS ${UDEV_LIBRARIES})

    # On Linux, we require Bluez for getting the BT addr
    pkg_check_modules(BLUEZ REQUIRED bluez)
    include_directories(${BLUEZ_INCLUDE_DIRS})
    list(APPEND PSMOVE_REQUIRED_LIBS ${BLUEZ_LIBRARIES})

    set(HIDAPI_SRC ${PSMOVEAPI_SOURCE_DIR}/hidapi/linux/hid-libusb.c)

    # Move Daemon for Linux (non-hidapi, Bluetooth-only)
    # Comment out the lines below and uncomment the above
    # lines (libudev/bluez) to build with HIDAPI support
    add_executable(moved moved.c)
    add_executable(moved-client moved_client_test.c moved_client.c)
    set(HIDAPI_SRC ${PSMOVEAPI_SOURCE_DIR}/moved_client.c)
    add_definitions(-DWITH_MOVED_CLIENT)
ENDIF()

set(PSMOVE_SRC ${HIDAPI_SRC} ${PSMOVEAPI_SOURCE_DIR}/psmove.c)

# Shared library

add_library(psmoveapi SHARED ${PSMOVE_SRC})
target_link_libraries(psmoveapi ${PSMOVE_REQUIRED_LIBS})
set_target_properties(psmoveapi PROPERTIES VERSION ${PSMOVEAPI_VERSION} SOVERSION ${PSMOVEAPI_VERSION_SONAME})

SET_TARGET_PROPERTIES(psmoveapi PROPERTIES COMPILE_FLAGS -DBUILDING_SHARED_LIBRARY)

# C utilities (those get installed on "make install")
add_executable(psmovepair c/pair.c)
target_link_libraries(psmovepair psmoveapi)

# C examples

add_executable(example c/example.c)
target_link_libraries(example psmoveapi)

# Disabled building of example2 now - uncomment if you have libncurses-dev
#find_package(CURSES)
#if(CURSES_FOUND)
#    add_executable(example2 c/example2.c)
#    target_link_libraries(example2 psmoveapi ${CURSES_LIBS})
#    if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
#        # Dirty hack to get example2 to compile in OS X
#        target_link_libraries(example2 ncurses)
#    endif()
#endif()

add_executable(multiple c/multiple.c)
target_link_libraries(multiple psmoveapi)

# Python bindings (PS Move API code statically linked in Python module)
find_package(SWIG)
if(SWIG_FOUND)
    include(${SWIG_USE_FILE})
    find_package(PythonLibs)

    include_directories(${PYTHON_INCLUDE_PATH})

    set(CMAKE_SWIG_FLAGS "")

    set_source_files_properties(psmove.i PROPERTIES SWIG_FLAGS "")
    swig_add_module(psmove python psmove.i)
    swig_link_libraries(psmove psmoveapi ${PYTHON_LIBRARIES})
endif()

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
find_package(Qt4)

if(QT_QTDECLARATIVE_FOUND)
    add_subdirectory(qt)
endif(QT_QTDECLARATIVE_FOUND)

