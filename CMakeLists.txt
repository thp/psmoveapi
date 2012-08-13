
cmake_minimum_required(VERSION 2.6)

project(PSMOVEAPI)

include_directories(${PSMOVEAPI_SOURCE_DIR}/external/hidapi/hidapi)

add_definitions(-DPSMOVE_DEBUG)
add_definitions(-g -Wall)
add_definitions(-O2)

set(PSMOVEAPI_MAJOR_VERSION "2")
set(PSMOVEAPI_MINOR_VERSION "0")
set(PSMOVEAPI_PATCH_VERSION "0")
set(PSMOVEAPI_VERSION "${PSMOVEAPI_MAJOR_VERSION}.${PSMOVEAPI_MINOR_VERSION}.${PSMOVEAPI_PATCH_VERSION}")
set(PSMOVEAPI_VERSION_SONAME "${PSMOVEAPI_MAJOR_VERSION}")

set(PSMOVE_REQUIRED_LIBS)
set(PSMOVEAPI_PLATFORM_SRC)

# Put any other libraries that you might need in here
link_directories(${PSMOVEAPI_SOURCE_DIR}/external/libs)

# Build the OpenCV-based camera tracking module? (see psmove_tracker.h)
option(PSMOVE_BUILD_TRACKER "Build the Tracker module (needs OpenCV)" ON)

# Search for OpenCV, if we don't have it, build without tracker
FIND_PACKAGE(OpenCV QUIET)
if(OpenCV_FOUND AND PSMOVE_BUILD_TRACKER)
    list(APPEND PSMOVE_REQUIRED_LIBS ${OpenCV_LIBS})

    file(GLOB PSMOVEAPI_TRACKER_SRC
        "${PSMOVEAPI_SOURCE_DIR}/src/tracker/*.c"
        "${PSMOVEAPI_SOURCE_DIR}/external/iniparser/*.c"
    )

    set(BUILD_PSMOVE_TRACKER "Yes")
    set(PSMOVE_HAVE_TRACKER ON)
else()
    set(PSMOVEAPI_TRACKER_SRC "")

    set(BUILD_PSMOVE_TRACKER "No ")
    set(PSMOVE_HAVE_TRACKER OFF)
endif()

IF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    find_library(IOKIT IOKit)
    find_library(COREFOUNDATION CoreFoundation)
    find_library(IOBLUETOOTH IOBluetooth)
    list(APPEND PSMOVE_REQUIRED_LIBS ${IOKIT})
    list(APPEND PSMOVE_REQUIRED_LIBS ${COREFOUNDATION})
    list(APPEND PSMOVE_REQUIRED_LIBS ${IOBLUETOOTH})

    set(HIDAPI_SRC ${PSMOVEAPI_SOURCE_DIR}/external/hidapi/mac/hid.c)
    list(APPEND PSMOVEAPI_PLATFORM_SRC ${PSMOVEAPI_SOURCE_DIR}/src/platform/psmove_osxsupport.m)
ELSEIF(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    list(APPEND PSMOVE_REQUIRED_LIBS setupapi bthprops kernel32 ws2_32)
    list(APPEND MOVED_CLIENT_REQUIRED_LIBS ws2_32)

    option(PSMOVE_USE_CL_EYE_SDK "Use the CL Eye SDK driver on Windows" OFF)

    set(BUILD_WITH_CL_SDK "No ")
    IF(PSMOVE_USE_CL_EYE_SDK)
        find_path(CL_EYE_SDK_PATH Bin/CLEyeMulticam.dll
            HINTS "C:/Program Files/Code Laboratories/CL-Eye Platform SDK"
            "C:/Program Files (x86)/Code Laboratories/CL-Eye Platform SDK")
        IF(CL_EYE_SDK_PATH)
            add_definitions(-DCAMERA_CONTROL_USE_CL_DRIVER)
            list(APPEND PSMOVE_REQUIRED_LIBS CLEyeMulticam)
            file(COPY ${CL_EYE_SDK_PATH}/Bin/CLEyeMulticam.dll
                DESTINATION ${PSMOVEAPI_SOURCE_DIR}/external/libs)
            set(BUILD_WITH_CL_SDK "Yes")

            # FIXME: Figure out why the optimizations result in a bad build
            # and remove this workaround (we always want to build with -O2)
            remove_definitions(-O2)
            add_definitions(-O0)
            MESSAGE("-- WARNING: Compiler optimizations are disabled when using 'PSMOVE_USE_CL_EYE_SDK'.")
        ENDIF()
    ENDIF()

    set(HIDAPI_SRC ${PSMOVEAPI_SOURCE_DIR}/external/hidapi/windows/hid.c)
ELSE()
    # Linux
    find_package(PkgConfig REQUIRED)
    find_package(Threads REQUIRED)
    list(APPEND PSMOVE_REQUIRED_LIBS ${CMAKE_THREAD_LIBS_INIT})

    pkg_check_modules(UDEV REQUIRED libudev)
    include_directories(${UDEV_INCLUDE_DIRS})
    list(APPEND PSMOVE_REQUIRED_LIBS ${UDEV_LIBRARIES})

    pkg_check_modules(BLUEZ REQUIRED bluez)
    include_directories(${BLUEZ_INCLUDE_DIRS})
    list(APPEND PSMOVE_REQUIRED_LIBS ${BLUEZ_LIBRARIES})

    set(HIDAPI_SRC ${PSMOVEAPI_SOURCE_DIR}/external/hidapi/linux/hid.c)
    list(APPEND PSMOVEAPI_PLATFORM_SRC ${PSMOVEAPI_SOURCE_DIR}/src/platform/psmove_linuxsupport.c)
ENDIF()

include_directories(${PSMOVEAPI_SOURCE_DIR}/include)

file(GLOB PSMOVEAPI_MOVED_SRC
    "${PSMOVEAPI_SOURCE_DIR}/src/daemon/*.c"
)

file(GLOB PSMOVEAPI_SRC
    "${PSMOVEAPI_SOURCE_DIR}/src/*.c"
)

file(GLOB PSMOVEAPI_HEADERS
    "${PSMOVEAPI_SOURCE_DIR}/include/*.h"
)

set(PSMOVEAPI_ALGORITHM_SRC
    ${PSMOVEAPI_SOURCE_DIR}/external/MadgwickAHRS/MadgwickAHRS.c
)

set(PSMOVE_SRC
    ${HIDAPI_SRC}
    ${PSMOVEAPI_SRC}
    ${PSMOVEAPI_TRACKER_SRC}
    ${PSMOVEAPI_MOVED_SRC}
    ${PSMOVEAPI_PLATFORM_SRC}
    ${PSMOVEAPI_ALGORITHM_SRC}
)

# Shared library

add_library(psmoveapi SHARED ${PSMOVE_SRC})
target_link_libraries(psmoveapi ${PSMOVE_REQUIRED_LIBS})
set_target_properties(psmoveapi PROPERTIES VERSION ${PSMOVEAPI_VERSION} SOVERSION ${PSMOVEAPI_VERSION_SONAME})
set_target_properties(psmoveapi PROPERTIES COMPILE_FLAGS -DBUILDING_SHARED_LIBRARY)

# C examples

add_executable(moved examples/c/moved.c)
target_link_libraries(moved psmoveapi)

add_executable(moved_client_test examples/c/moved_client_test.c src/daemon/moved_client.c)
target_link_libraries(moved_client_test ${MOVED_CLIENT_REQUIRED_LIBS})

# C examples
foreach(PSMOVE_EXAMPLE example multiple psmovepair dump_calibration linmcfake)
    add_executable(${PSMOVE_EXAMPLE} examples/c/${PSMOVE_EXAMPLE}.c)
    target_link_libraries(${PSMOVE_EXAMPLE} psmoveapi)
endforeach(PSMOVE_EXAMPLE)

# C test programs
foreach(PSMOVE_TEST led_update read_performance calibration tracker)
    add_executable(test_${PSMOVE_TEST} examples/c/test_${PSMOVE_TEST}.c)
    target_link_libraries(test_${PSMOVE_TEST} psmoveapi)
endforeach(PSMOVE_TEST)

# These will be set to "Yes" below if the dependencies are found
set(BUILD_PYTHON_BINDINGS "No ")
set(BUILD_JAVA_BINDINGS "No ")
set(BUILD_QT_BINDINGS "No ")

# Language bindings (Python and Java)
find_package(SWIG)
if(SWIG_FOUND)
    include(${SWIG_USE_FILE})

    file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bindings/swig)

    find_package(PythonLibs)
    if(PYTHONLIBS_FOUND)
        unset(CMAKE_SWIG_FLAGS)
        include_directories(${PYTHON_INCLUDE_PATH})
        swig_add_module(psmove python bindings/swig/psmove.i)
        swig_link_libraries(psmove psmoveapi ${PYTHON_LIBRARIES})
        set(BUILD_PYTHON_BINDINGS "Yes")
    endif()

    find_package(JNI)
    if(JNI_FOUND)
        set(PSMOVEAPI_JAVA_PKG "io.thp.psmove")
        string(REPLACE "." "/" PSMOVEAPI_JAVA_PKG_DIR ${PSMOVEAPI_JAVA_PKG})

        set(CMAKE_SWIG_FLAGS -package ${PSMOVEAPI_JAVA_PKG})
        include_directories(${JNI_INCLUDE_DIRS})
        swig_add_module(psmove_java java bindings/swig/psmove.i)
        swig_link_libraries(psmove_java psmoveapi ${JNI_LIBRARIES})
        set(BUILD_JAVA_BINDINGS "Yes")

        add_custom_target(psmoveapi.jar ALL
            COMMAND mkdir -p ${PSMOVEAPI_JAVA_PKG_DIR}
            COMMAND mv *.java ${PSMOVEAPI_JAVA_PKG_DIR} || true
            COMMAND javac ${PSMOVEAPI_JAVA_PKG_DIR}/*.java
            COMMAND jar cf psmoveapi.jar ${PSMOVEAPI_JAVA_PKG_DIR}/*.class
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Java bindings JAR archive"
        )
    endif()
endif()

# For system-wide installation

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/contrib/psmoveapi.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/psmoveapi.pc
    @ONLY)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/include/psmove_config.h.in
    ${CMAKE_CURRENT_SOURCE_DIR}/include/psmove_config.h
    @ONLY)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/psmoveapi.pc DESTINATION lib/pkgconfig)

install(TARGETS psmovepair psmoveapi
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
)


install(FILES ${PSMOVEAPI_HEADERS} DESTINATION include/psmoveapi)
install(FILES README COPYING LAYOUT INSTALL DESTINATION share/psmoveapi)

# Qt bindings
find_package(Qt4)

if(QT_QTDECLARATIVE_FOUND)
    add_subdirectory(bindings/qt)
    set(BUILD_QT_BINDINGS "Yes")
endif(QT_QTDECLARATIVE_FOUND)

set(PADDING "                     ")
MESSAGE("")
MESSAGE("      +------ PS Move API Build Configuration ------+ ")
MESSAGE("      |  Python bindings: " ${PADDING} ${BUILD_PYTHON_BINDINGS} "  |")
MESSAGE("      |  Java bindings:   " ${PADDING} ${BUILD_JAVA_BINDINGS} "  |")
MESSAGE("      |  Qt bindings:     " ${PADDING} ${BUILD_QT_BINDINGS} "  |")
IF(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
MESSAGE("      |  Use CL Eye SDK:  " ${PADDING} ${BUILD_WITH_CL_SDK} "  |")
ENDIF()
MESSAGE("      +---------------------------------------------+ ")
MESSAGE("      |  Tracker module:  " ${PADDING} ${BUILD_PSMOVE_TRACKER} "  |")
MESSAGE("      +---------------------------------------------+ ")
MESSAGE("")
if(NOT OpenCV_FOUND)
    MESSAGE(" If you want to build the tracker module, install OpenCV!")
endif()
MESSAGE("")
MESSAGE("")

