
cmake_minimum_required(VERSION 2.6)

project(PSMOVEAPI)

include_directories(${PSMOVEAPI_SOURCE_DIR}/external/hidapi/hidapi)
include_directories(${PSMOVEAPI_SOURCE_DIR}/external/glm)

set(PSMOVEAPI_MAJOR_VERSION "3")
set(PSMOVEAPI_MINOR_VERSION "0")
set(PSMOVEAPI_PATCH_VERSION "0")
set(PSMOVEAPI_VERSION "${PSMOVEAPI_MAJOR_VERSION}.${PSMOVEAPI_MINOR_VERSION}.${PSMOVEAPI_PATCH_VERSION}")
set(PSMOVEAPI_VERSION_SONAME "${PSMOVEAPI_MAJOR_VERSION}")

set(PSMOVEAPI_REQUIRED_LIBS)
set(PSMOVEAPI_PLATFORM_SRC)

set(PSMOVEAPI_TRACKER_REQUIRED_LIBS)
set(PSMOVEAPI_TRACKER_PLATFORM_SRC)

set(PSMOVEAPI_INSTALL_TARGETS)

# Put any other libraries that you might need in here
link_directories(${PSMOVEAPI_SOURCE_DIR}/external/libs)

# Language bindings
option(PSMOVE_BUILD_QT_BINDINGS "Build the Qt bindings" OFF)
option(PSMOVE_BUILD_JAVA_BINDINGS "Build the Java bindings" OFF)
option(PSMOVE_BUILD_CSHARP_BINDINGS "Build the C# bindings" OFF)
option(PSMOVE_BUILD_PROCESSING_BINDINGS "Build the Processing bindings" OFF)
option(PSMOVE_BUILD_PYTHON_BINDINGS "Build the Python bindings" ON)


# Build the OpenCV-based camera tracking module? (see psmove_tracker.h)
option(PSMOVE_BUILD_TRACKER "Build the Tracker module (needs OpenCV)" ON)

# Additional applications that can be used to test the library
option(PSMOVE_BUILD_EXAMPLES "Build the C examples" ON)
option(PSMOVE_BUILD_OPENGL_EXAMPLES "Build the OpenGL examples" OFF)
option(PSMOVE_BUILD_TESTS "Build the C tests" ON)
option(PSMOVE_BUILD_TUIO_SERVER "Build the TUIO server" ON)


# Enable tweaks (e.g. registry settings on Windows, ...) for PS Eye
option(PSMOVE_USE_PSEYE "Enable tweaks for the PS Eye camera" ON)

# Use the CL Eye SDK to interface with the PS Eye camera (Windows only)
option(PSMOVE_USE_CL_EYE_SDK "Use the CL Eye SDK driver on Windows" OFF)

# To show the OpenCV camera capture window everytime (useful for debugging)
option(PSMOVE_USE_DEBUG_CAPTURE "Always show camera capture input" OFF)

# Make a debug build with helpful output for debugging / maintenance
option(PSMOVE_USE_DEBUG "Build for debugging" OFF)

# Write debugging images for tracking calibration to home directory
option(PSMOVE_USE_TRACKER_TRACE "Write tracker calibration images to disk" OFF)


# https://github.com/thp/psmoveapi/issues/29
add_definitions(-fPIC)

# Debugging output
if(PSMOVE_USE_DEBUG)
    add_definitions(-DPSMOVE_DEBUG)
    add_definitions(-g -O0 -Wall)
    set(INFO_USE_DEBUG "Yes")
else()
    add_definitions(-O2 -Wall -Wno-unused)
    set(INFO_USE_DEBUG "No")
endif()

# Linker commands to put into the .pc file (when installing)
set(PSMOVEAPI_PKGCONFIG_LIBS "-lpsmoveapi")

# Tracker support
if(PSMOVE_BUILD_TRACKER)
    FIND_PACKAGE(OpenCV QUIET)
    if(OpenCV_FOUND)
        list(APPEND PSMOVEAPI_TRACKER_REQUIRED_LIBS ${OpenCV_LIBS})

        set(INFO_BUILD_TRACKER "Yes")
        set(PSMOVEAPI_PKGCONFIG_LIBS "${PSMOVEAPI_PKGCONFIG_LIBS} -lpsmoveapi_tracker")
    else()
        set(INFO_BUILD_TRACKER "No (OpenCV not found)")
        set(PSMOVE_BUILD_TRACKER OFF)
    endif()
else()
    set(INFO_BUILD_TRACKER "No (disabled)")
endif()

# Option to statically link OpenCV instead of using the system-wide install
option(PSMOVE_USE_LOCAL_OPENCV "Use locally-built OpenCV (static linking)" OFF)
if (PSMOVE_USE_LOCAL_OPENCV)
    # Force tracker build with a locally-built opencv library installation
    set(OPENCV_INSTALL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/opencv/build/install)
    link_directories(${OPENCV_INSTALL_DIR}/lib)
    link_directories(${OPENCV_INSTALL_DIR}/../3rdparty/lib)
    include_directories(${OPENCV_INSTALL_DIR}/include)
    set(PSMOVE_BUILD_TRACKER ON)
    set(INFO_BUILD_TRACKER "Yes (with local OpenCV)")

    if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
        find_library(QUARTZCORE QuartzCore)
        find_library(APPKIT AppKit)
        find_library(QTKIT QTKit)
        list(APPEND PSMOVEAPI_TRACKER_REQUIRED_LIBS
            opencv_core opencv_highgui opencv_imgproc
            libjpeg libpng zlib stdc++
            ${QUARTZCORE}
            ${APPKIT}
            ${QTKIT})
    elseif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
        list(APPEND PSMOVEAPI_TRACKER_REQUIRED_LIBS
            opencv_highgui249 opencv_imgproc249 opencv_core249
            libjpeg libpng zlib stdc++
            vfw32 comctl32)
    else()
        # XXX: OpenCV dependencies for Linux
    endif()
endif()

if (PSMOVE_USE_DEBUG_CAPTURE)
    add_definitions(-DCAMERA_CONTROL_DEBUG_CAPTURED_IMAGE)
endif()

set(INFO_USE_CL_EYE_SDK "No (Windows only)")

IF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    find_library(FOUNDATION Foundation)
    find_library(IOKIT IOKit)
    find_library(COREFOUNDATION CoreFoundation)
    find_library(IOBLUETOOTH IOBluetooth)
    list(APPEND PSMOVEAPI_REQUIRED_LIBS ${FOUNDATION})
    list(APPEND PSMOVEAPI_REQUIRED_LIBS ${IOKIT})
    list(APPEND PSMOVEAPI_REQUIRED_LIBS ${COREFOUNDATION})
    list(APPEND PSMOVEAPI_REQUIRED_LIBS ${IOBLUETOOTH})

    find_library(AVFOUNDATION AVFoundation)
    list(APPEND PSMOVEAPI_REQUIRED_LIBS ${AVFOUNDATION})

    set(HIDAPI_SRC ${PSMOVEAPI_SOURCE_DIR}/external/hidapi/mac/hid.c)

    # Build Universal Binaries for OS X
    set(CMAKE_OSX_ARCHITECTURES "i386;x86_64")

    list(APPEND PSMOVEAPI_PLATFORM_SRC
        ${PSMOVEAPI_SOURCE_DIR}/src/platform/psmove_osxsupport.m)

    list(APPEND PSMOVEAPI_TRACKER_PLATFORM_SRC
        ${PSMOVEAPI_SOURCE_DIR}/src/tracker/platform/psmove_osxsupport.m
        ${PSMOVEAPI_SOURCE_DIR}/src/tracker/platform/camera_control_macosx.c)
ELSEIF(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    list(APPEND PSMOVEAPI_REQUIRED_LIBS setupapi bthprops kernel32 ws2_32 winmm)

    # http://stackoverflow.com/questions/4702732/the-program-cant-start-because-libgcc-s-dw2-1-dll-is-missing
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static -static-libgcc -static-libstdc++")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -static -static-libgcc -static-libstdc++")
    set(CMAKE_STATIC_LIBRARY_FLAGS "${CMAKE_STATIC_LIBRARY_FLAGS} -static -static-libgcc -static-libstdc++")

    if (NOT CMAKE_TOOLCHAIN_FILE)
        # Only include these if we are NOT cross-compiling on Linux
        include_directories(${CMAKE_CURRENT_SOURCE_DIR}/external/mingw-w64-headers)
        link_directories(${CMAKE_CURRENT_SOURCE_DIR}/external/mingw-w64-headers)
    endif()

    IF(PSMOVE_USE_CL_EYE_SDK)
        find_path(CL_EYE_SDK_PATH Bin/CLEyeMulticam.dll
            HINTS "C:/Program Files/Code Laboratories/CL-Eye Platform SDK"
            "C:/Program Files (x86)/Code Laboratories/CL-Eye Platform SDK")
        IF(CL_EYE_SDK_PATH)
            add_definitions(-DCAMERA_CONTROL_USE_CL_DRIVER)
            list(APPEND PSMOVEAPI_TRACKER_REQUIRED_LIBS CLEyeMulticam)
            file(COPY ${CL_EYE_SDK_PATH}/Bin/CLEyeMulticam.dll
                DESTINATION ${PSMOVEAPI_SOURCE_DIR}/external/libs)
            set(INFO_USE_CL_EYE_SDK "Yes")

            # XXX: If this crashes, disable compiler optimizations
        else()
            set(INFO_USE_CL_EYE_SDK "No (SDK not found)")
        ENDIF()
    else()
        set(INFO_USE_CL_EYE_SDK "No (disabled)")
    ENDIF()

    set(HIDAPI_SRC ${PSMOVEAPI_SOURCE_DIR}/external/hidapi/windows/hid.c)

    list(APPEND PSMOVEAPI_TRACKER_PLATFORM_SRC
        ${PSMOVEAPI_SOURCE_DIR}/src/tracker/platform/camera_control_win32.c)
ELSE()
    # Linux
    find_package(PkgConfig REQUIRED)
    find_package(Threads REQUIRED)
    list(APPEND PSMOVEAPI_REQUIRED_LIBS ${CMAKE_THREAD_LIBS_INIT})
    list(APPEND PSMOVEAPI_REQUIRED_LIBS m)

    pkg_check_modules(UDEV REQUIRED libudev)
    include_directories(${UDEV_INCLUDE_DIRS})
    list(APPEND PSMOVEAPI_REQUIRED_LIBS ${UDEV_LIBRARIES})

    pkg_check_modules(BLUEZ REQUIRED bluez)
    include_directories(${BLUEZ_INCLUDE_DIRS})
    list(APPEND PSMOVEAPI_REQUIRED_LIBS ${BLUEZ_LIBRARIES})

    set(HIDAPI_SRC ${PSMOVEAPI_SOURCE_DIR}/external/hidapi/linux/hid.c)

    list(APPEND PSMOVEAPI_PLATFORM_SRC
        ${PSMOVEAPI_SOURCE_DIR}/src/platform/psmove_linuxsupport.c)

    list(APPEND PSMOVEAPI_TRACKER_PLATFORM_SRC
        ${PSMOVEAPI_SOURCE_DIR}/src/tracker/platform/psmove_linuxsupport.c
        ${PSMOVEAPI_SOURCE_DIR}/src/tracker/platform/camera_control_linux.c)
ENDIF()

include_directories(${PSMOVEAPI_SOURCE_DIR}/include)

file(GLOB PSMOVEAPI_MOVED_SRC
    "${PSMOVEAPI_SOURCE_DIR}/src/daemon/*.c"
)

file(GLOB PSMOVEAPI_TRACKER_SRC
    "${PSMOVEAPI_SOURCE_DIR}/src/tracker/*.c"
    "${PSMOVEAPI_SOURCE_DIR}/src/tracker/*.cpp"
    "${PSMOVEAPI_SOURCE_DIR}/external/iniparser/*.c"
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

# Source files that are needed for both the shared and static library

set(PSMOVEAPI_LIBRARY_SRC)
list(APPEND PSMOVEAPI_LIBRARY_SRC ${HIDAPI_SRC})
list(APPEND PSMOVEAPI_LIBRARY_SRC ${PSMOVEAPI_SRC})
list(APPEND PSMOVEAPI_LIBRARY_SRC ${PSMOVEAPI_MOVED_SRC})
list(APPEND PSMOVEAPI_LIBRARY_SRC ${PSMOVEAPI_PLATFORM_SRC})
list(APPEND PSMOVEAPI_LIBRARY_SRC ${PSMOVEAPI_ALGORITHM_SRC})

# Shared library

add_library(psmoveapi SHARED ${PSMOVEAPI_LIBRARY_SRC})
target_link_libraries(psmoveapi ${PSMOVEAPI_REQUIRED_LIBS})
set_target_properties(psmoveapi PROPERTIES
    VERSION ${PSMOVEAPI_VERSION}
    SOVERSION ${PSMOVEAPI_VERSION_SONAME})
set_target_properties(psmoveapi PROPERTIES
    COMPILE_FLAGS -DBUILDING_SHARED_LIBRARY)
list(APPEND PSMOVEAPI_INSTALL_TARGETS psmoveapi)

# Static library

add_library(psmoveapi_static STATIC ${PSMOVEAPI_LIBRARY_SRC})
target_link_libraries(psmoveapi_static ${PSMOVEAPI_REQUIRED_LIBS})
set_target_properties(psmoveapi_static PROPERTIES
    COMPILE_FLAGS -DBUILDING_STATIC_LIBRARY)

if(PSMOVE_BUILD_TRACKER)
    add_library(psmoveapi_tracker SHARED
        ${PSMOVEAPI_TRACKER_SRC}
        ${PSMOVEAPI_TRACKER_PLATFORM_SRC})
    target_link_libraries(psmoveapi_tracker
        psmoveapi
        ${PSMOVEAPI_REQUIRED_LIBS}
        ${PSMOVEAPI_TRACKER_REQUIRED_LIBS})
    set_target_properties(psmoveapi_tracker PROPERTIES
        VERSION ${PSMOVEAPI_VERSION}
        SOVERSION ${PSMOVEAPI_VERSION_SONAME})
    set_target_properties(psmoveapi_tracker PROPERTIES
        COMPILE_FLAGS -DBUILDING_SHARED_LIBRARY)
    list(APPEND PSMOVEAPI_INSTALL_TARGETS psmoveapi_tracker)

    add_library(psmoveapi_tracker_static STATIC
        ${PSMOVEAPI_TRACKER_SRC}
        ${PSMOVEAPI_TRACKER_PLATFORM_SRC})
    target_link_libraries(psmoveapi_tracker_static
        psmoveapi_static
        ${PSMOVEAPI_REQUIRED_LIBS}
        ${PSMOVEAPI_TRACKER_REQUIRED_LIBS})
    set_target_properties(psmoveapi_tracker_static PROPERTIES
        COMPILE_FLAGS -DBUILDING_STATIC_LIBRARY)
endif()

# Essential utilities
foreach(UTILITY moved psmovepair magnetometer_calibration)
    add_executable(${UTILITY} src/utils/${UTILITY}.c)
    target_link_libraries(${UTILITY} psmoveapi_static)
    set_target_properties(${UTILITY} PROPERTIES
        COMPILE_FLAGS -DUSING_STATIC_LIBRARY)
    list(APPEND PSMOVEAPI_INSTALL_TARGETS ${UTILITY})
endforeach()


# C examples
if(PSMOVE_BUILD_EXAMPLES)
    foreach(EXAMPLE example multiple dump_calibration)
        add_executable(${EXAMPLE} examples/c/${EXAMPLE}.c)
        target_link_libraries(${EXAMPLE} psmoveapi)
    endforeach()

    if(PSMOVE_BUILD_TRACKER AND PSMOVE_BUILD_TUIO_SERVER)
        include_directories(${PSMOVEAPI_SOURCE_DIR}/external/TUIO_CPP/TUIO)
        include_directories(${PSMOVEAPI_SOURCE_DIR}/external/TUIO_CPP/oscpack)
        add_executable(tuio_server examples/c/tuio_server.cpp
            external/TUIO_CPP/TUIO/TuioClient.cpp
            external/TUIO_CPP/TUIO/TuioServer.cpp
            external/TUIO_CPP/TUIO/TuioTime.cpp
            external/TUIO_CPP/oscpack/osc/OscTypes.cpp
            external/TUIO_CPP/oscpack/osc/OscOutboundPacketStream.cpp
            external/TUIO_CPP/oscpack/osc/OscReceivedElements.cpp
            external/TUIO_CPP/oscpack/osc/OscPrintReceivedElements.cpp
            external/TUIO_CPP/oscpack/ip/posix/NetworkingUtils.cpp
            external/TUIO_CPP/oscpack/ip/posix/UdpSocket.cpp
            external/TUIO_CPP/oscpack/ip/win32/NetworkingUtils.cpp
            external/TUIO_CPP/oscpack/ip/win32/UdpSocket.cpp)
        set_target_properties(tuio_server PROPERTIES
            COMPILE_FLAGS -DOSC_HOST_LITTLE_ENDIAN)
        target_link_libraries(tuio_server psmoveapi psmoveapi_tracker)
    else()
        # Disable the TUIO Server if we don't build the tracker
        set(PSMOVE_BUILD_TUIO_SERVER OFF)
    endif()

    if(PSMOVE_BUILD_TRACKER)
        foreach(EXAMPLE distance_calibration)
            add_executable(${EXAMPLE} examples/c/${EXAMPLE}.c)
            target_link_libraries(${EXAMPLE} psmoveapi psmoveapi_tracker)
        endforeach()
    endif()
endif()


# C test programs
if(PSMOVE_BUILD_TESTS)
    foreach(TESTNAME led_update read_performance calibration)
        add_executable(test_${TESTNAME} examples/c/test_${TESTNAME}.c)
        target_link_libraries(test_${TESTNAME} psmoveapi)
    endforeach(TESTNAME)

    if(PSMOVE_BUILD_TRACKER)
        foreach(TESTNAME tracker capture_performance record_video roi_size end2end_latency)
            add_executable(test_${TESTNAME} examples/c/test_${TESTNAME}.c)
            target_link_libraries(test_${TESTNAME} psmoveapi psmoveapi_tracker)
        endforeach(TESTNAME)

        if (PSMOVE_BUILD_OPENGL_EXAMPLES)
            foreach(EXAMPLE test_opengl test_opengl2 test_opengl3)
                add_executable(${EXAMPLE} examples/c/${EXAMPLE}.cpp)
                target_link_libraries(${EXAMPLE}
                    psmoveapi
                    psmoveapi_tracker
                    GL glut SDL)
            endforeach()
        endif()
    endif()
endif()


# Language bindings (Python, Java and Processing)
find_package(SWIG QUIET)
if(SWIG_FOUND)
    include(${SWIG_USE_FILE})

    file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bindings/swig)

    if(PSMOVE_BUILD_PYTHON_BINDINGS)
        find_package(PythonLibs QUIET)
        if(PYTHONLIBS_FOUND)
            unset(CMAKE_SWIG_FLAGS)
            include_directories(${PYTHON_INCLUDE_PATH})
            swig_add_module(psmove python bindings/swig/psmove.i)
            swig_link_libraries(psmove psmoveapi_static ${PYTHON_LIBRARIES})
            if (PSMOVE_BUILD_TRACKER)
                swig_link_libraries(psmove psmoveapi_tracker_static)
            endif()
            set_target_properties(_psmove PROPERTIES
                COMPILE_FLAGS -DUSING_STATIC_LIBRARY)
            set(INFO_BUILD_PYTHON_BINDINGS "Yes")
        else()
            set(INFO_BUILD_PYTHON_BINDINGS "No (libpython not found)")
        endif()
    else()
        set(INFO_BUILD_PYTHON_BINDINGS "No (disabled)")
    endif()

    if(PSMOVE_BUILD_JAVA_BINDINGS)
        find_package(JNI QUIET)
        if(JNI_FOUND)
            set(PSMOVEAPI_JAVA_PKG "io.thp.psmove")
            string(REPLACE "." "/" PSMOVEAPI_JAVA_PKG_DIR ${PSMOVEAPI_JAVA_PKG})

            set(CMAKE_SWIG_FLAGS -package ${PSMOVEAPI_JAVA_PKG})
            include_directories(${JNI_INCLUDE_DIRS})
            swig_add_module(psmove_java java bindings/swig/psmove.i)
            swig_link_libraries(psmove_java psmoveapi_static ${JNI_LIBRARIES})
            if (PSMOVE_BUILD_TRACKER)
                swig_link_libraries(psmove_java psmoveapi_tracker_static)
            endif()
            set_target_properties(psmove_java PROPERTIES
                COMPILE_FLAGS -DUSING_STATIC_LIBRARY)
            set(INFO_BUILD_JAVA_BINDINGS "Yes")

            add_custom_target(psmoveapi.jar ALL
                COMMAND ${CMAKE_COMMAND} -E make_directory ${PSMOVEAPI_JAVA_PKG_DIR}
                COMMAND cp *.java ${PSMOVEAPI_JAVA_PKG_DIR}/
                COMMAND javac ${PSMOVEAPI_JAVA_PKG_DIR}/*.java
                COMMAND jar cf psmoveapi.jar ${PSMOVEAPI_JAVA_PKG_DIR}/*.class
                WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                COMMENT "Java bindings JAR archive"
            )

            set(PSMOVEAPI_PROCESSING_DIR "processing/libraries/psmove")
            set(PROCESSING_LIB_DIR "${PSMOVEAPI_PROCESSING_DIR}/library")

            IF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
                set(PSMOVEAPI_LIB_PLATFORM "macosx")
                set(PSMOVEAPI_NATIVE_LIB "libpsmove_java.jnilib")
                set(PSMOVEAPI_PAIRING_TOOL_NAME "psmovepair")
            ELSEIF(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
                set(PSMOVEAPI_LIB_PLATFORM "windows")
                set(PSMOVEAPI_NATIVE_LIB "libpsmove_java.dll")
                set(PSMOVEAPI_PAIRING_TOOL_NAME "psmovepair.exe")
            ELSE() # Linux
                set(PSMOVEAPI_LIB_PLATFORM "linux")
                set(PSMOVEAPI_NATIVE_LIB "libpsmove_java.so")
                set(PSMOVEAPI_PAIRING_TOOL_NAME "psmovepair")
            ENDIF()

            if(PSMOVE_BUILD_PROCESSING_BINDINGS)
                add_custom_target(processing_lib ALL
                    # Create Folder Structure
                    COMMAND ${CMAKE_COMMAND} -E make_directory ${PSMOVEAPI_PROCESSING_DIR}/examples
                    COMMAND ${CMAKE_COMMAND} -E make_directory ${PSMOVEAPI_PROCESSING_DIR}/library
                    COMMAND ${CMAKE_COMMAND} -E make_directory ${PSMOVEAPI_PROCESSING_DIR}/library/linux
                    COMMAND ${CMAKE_COMMAND} -E make_directory ${PSMOVEAPI_PROCESSING_DIR}/library/macosx
                    COMMAND ${CMAKE_COMMAND} -E make_directory ${PSMOVEAPI_PROCESSING_DIR}/library/windows
                    COMMAND ${CMAKE_COMMAND} -E make_directory ${PSMOVEAPI_PROCESSING_DIR}/tools
                    COMMAND ${CMAKE_COMMAND} -E make_directory ${PSMOVEAPI_PROCESSING_DIR}/tools/linux
                    COMMAND ${CMAKE_COMMAND} -E make_directory ${PSMOVEAPI_PROCESSING_DIR}/tools/macosx
                    COMMAND ${CMAKE_COMMAND} -E make_directory ${PSMOVEAPI_PROCESSING_DIR}/tools/windows

                    # Copy java library, native library, pairing tool, and example app into folder structure
                    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/psmoveapi.jar ${PSMOVEAPI_PROCESSING_DIR}/library/psmove.jar
                    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/${PSMOVEAPI_NATIVE_LIB} ${PSMOVEAPI_PROCESSING_DIR}/library/${PSMOVEAPI_LIB_PLATFORM}/${PSMOVEAPI_NATIVE_LIB}
                    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/${PSMOVEAPI_PAIRING_TOOL_NAME} ${PSMOVEAPI_PROCESSING_DIR}/tools/${PSMOVEAPI_LIB_PLATFORM}/${PSMOVEAPI_PAIRING_TOOL_NAME}
                    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/examples/processing ${PSMOVEAPI_PROCESSING_DIR}/examples/

                    # Cross-platform zipping :)
                    COMMAND jar cfM psmove_processing_${PSMOVEAPI_LIB_PLATFORM}.zip processing

                    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                    COMMENT "Building and packaging Processing library"
                )

                # Ensure building targets in the correct order
                # (native lib->java lib->processing lib)
                add_dependencies(psmoveapi.jar psmove_java)
                add_dependencies(processing_lib psmoveapi.jar)
                add_dependencies(processing_lib psmovepair)

                set(INFO_BUILD_PROCESSING_BINDINGS "Yes")
            else()
                set(INFO_BUILD_PROCESSING_BINDINGS "No (disabled)")
            endif()
        else()
            set(INFO_BUILD_JAVA_BINDINGS "No (JNI not found)")
            set(INFO_BUILD_PROCESSING_BINDINGS "No (JNI not found)")
        endif()
    else()
        set(INFO_BUILD_JAVA_BINDINGS "No (disabled)")
        set(INFO_BUILD_PROCESSING_BINDINGS "No (disabled)")
    endif()

    if(PSMOVE_BUILD_CSHARP_BINDINGS)
        set(PSMOVEAPI_CSHARP_NS "io.thp.psmove")

        set(CMAKE_SWIG_FLAGS -namespace ${PSMOVEAPI_CSHARP_NS})
        swig_add_module(psmoveapi_csharp csharp bindings/swig/psmove.i)
        swig_link_libraries(psmoveapi_csharp psmoveapi_static)
        if (PSMOVE_BUILD_TRACKER)
            swig_link_libraries(psmoveapi_csharp psmoveapi_tracker_static)
        endif()
        set_target_properties(psmoveapi_csharp PROPERTIES
            COMPILE_FLAGS -DUSING_STATIC_LIBRARY)
        set(INFO_BUILD_CSHARP_BINDINGS "Yes")
    else()
        set(INFO_BUILD_CSHARP_BINDINGS "No (disabled)")
    endif()
else()
    set(INFO_BUILD_PYTHON_BINDINGS "No (SWIG not found)")
    set(INFO_BUILD_JAVA_BINDINGS "No (SWIG not found)")
    set(INFO_BUILD_CSHARP_BINDINGS "No (SWIG not found)")
    set(INFO_BUILD_PROCESSING_BINDINGS "No (SWIG not found)")
endif()


# For system-wide installation

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/contrib/psmoveapi.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/psmoveapi.pc
    @ONLY)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/include/psmove_config.h.in
    ${CMAKE_CURRENT_SOURCE_DIR}/include/psmove_config.h
    @ONLY)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/psmoveapi.pc
    DESTINATION lib/pkgconfig)

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    # Only install the udev configuration file on Linux
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/contrib/99-psmove.rules
        DESTINATION /etc/udev/rules.d)
endif()

install(TARGETS ${PSMOVEAPI_INSTALL_TARGETS}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(FILES ${PSMOVEAPI_HEADERS} DESTINATION include/psmoveapi)
install(FILES README COPYING LAYOUT INSTALL DESTINATION share/psmoveapi)


# Qt bindings
if(PSMOVE_BUILD_QT_BINDINGS)
    find_package(Qt4 QUIET)

    if(QT_QTDECLARATIVE_FOUND)
        add_subdirectory(bindings/qt)
        set(INFO_BUILD_QT_BINDINGS "Yes")
    else()
        set(INFO_BUILD_QT_BINDINGS "No (Qt not found)")
    endif()
else()
    set(INFO_BUILD_QT_BINDINGS "No (disabled)")
endif()


# Pretty-print if a "use" feature has been enabled
macro(feature_use_info CAPTION FEATURE)
    if(${FEATURE})
        message("    " ${CAPTION} "Yes")
    else()
        message("    " ${CAPTION} "No")
    endif()
endmacro()


# Configuration report
message("")
message("  Build configuration")
message("    Debug build:      " ${INFO_USE_DEBUG})
message("    Tracker library:  " ${INFO_BUILD_TRACKER})
message("")
message("  Language bindings")
message("    Python:           " ${INFO_BUILD_PYTHON_BINDINGS})
message("    Java:             " ${INFO_BUILD_JAVA_BINDINGS})
message("    C#:               " ${INFO_BUILD_CSHARP_BINDINGS})
message("    Processing:       " ${INFO_BUILD_PROCESSING_BINDINGS})
message("    Qt:               " ${INFO_BUILD_QT_BINDINGS})
message("")
message("  Tracker")
feature_use_info("PS Eye support:   " PSMOVE_USE_PSEYE)
feature_use_info("HTML tracing:     " PSMOVE_USE_TRACKER_TRACE)
message("    Use CL Eye SDK:   " ${INFO_USE_CL_EYE_SDK})
message("")
message("  Additional targets")
feature_use_info("C example apps:   " PSMOVE_BUILD_EXAMPLES)
feature_use_info("OpenGL examples:  " PSMOVE_BUILD_OPENGL_EXAMPLES)
feature_use_info("C test programs:  " PSMOVE_BUILD_TESTS)
feature_use_info("C++ TUIO server:  " PSMOVE_BUILD_TUIO_SERVER)
message("")

