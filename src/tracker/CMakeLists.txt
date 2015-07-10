cmake_minimum_required(VERSION 2.8.12)
set(ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/../..)
include(${ROOT_DIR}/contrib/common.cmake)

set(PSMOVEAPI_TRACKER_REQUIRED_LIBS)
set(PSMOVEAPI_TRACKER_PLATFORM_SRC)

# Build the OpenCV-based camera tracking module? (see psmove_tracker.h)
option(PSMOVE_BUILD_TRACKER "Build the Tracker module (needs OpenCV)" ON)

# Use a custom location for OpenCV src dir
set(OpenCV_DIR ${ROOT_DIR}/external/opencv/build CACHE PATH "Path to local OpenCV source tree" FORCE)

# Enable tweaks (e.g. registry settings on Windows, ...) for PS Eye
option(PSMOVE_USE_PSEYE "Enable tweaks for the PS Eye camera" ON)

# Use the CL Eye SDK to interface with the PS Eye camera (Windows only)
option(PSMOVE_USE_CL_EYE_SDK "Use the CL Eye SDK driver on Windows" OFF)

# Use the PS3EYEDriver to interface with the PS Eye camera (OS X only)
option(PSMOVE_USE_PS3EYE_DRIVER "Use the PS3EYEDriver on OS X" OFF)

# To show the OpenCV camera capture window everytime (useful for debugging)
option(PSMOVE_USE_DEBUG_CAPTURE "Always show camera capture input" OFF)

# Write debugging images for tracking calibration to home directory
option(PSMOVE_USE_TRACKER_TRACE "Write tracker calibration images to disk" OFF)

IF(PSMOVE_USE_PS3EYE_DRIVER)
    # PS3EYEDriver is based on GPL'd code
    set(INFO_LICENSE "GPL")
ENDIF()

#
# Tracker support
#

# 1 - OpenCV
IF(PSMOVE_BUILD_TRACKER)
    FIND_PACKAGE(OpenCV QUIET)
    IF(OpenCV_FOUND)
        list(APPEND PSMOVEAPI_TRACKER_REQUIRED_LIBS ${OpenCV_LIBS})
        #include_directories(${OpenCV_INCLUDE_DIR}) #? not necessary after cmake 2.8.11
        set(INFO_BUILD_TRACKER "Yes")
        set(PSMOVEAPI_PKGCONFIG_LIBS "${PSMOVEAPI_PKGCONFIG_LIBS} -lpsmoveapi_tracker")
    ELSE()
        set(INFO_BUILD_TRACKER "No (OpenCV not found)")
        set(PSMOVE_BUILD_TRACKER OFF)
    ENDIF()
ELSE()
    set(INFO_BUILD_TRACKER "No (disabled)")
ENDIF()

# 2 - Platform-specific settings for OpenCV, PS3EYEDriver, maybe others
IF(PSMOVE_BUILD_TRACKER)
    IF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
        list(APPEND PSMOVEAPI_TRACKER_PLATFORM_SRC
            ${CMAKE_CURRENT_LIST_DIR}/platform/psmove_osxsupport.m
            ${CMAKE_CURRENT_LIST_DIR}/platform/camera_control_macosx.c)
        find_library(QUARTZCORE QuartzCore)
        find_library(APPKIT AppKit)
        find_library(QTKIT QTKit)
        find_library(AVFOUNDATION AVFoundation)
        list(APPEND PSMOVEAPI_TRACKER_REQUIRED_LIBS
            stdc++
            ${QUARTZCORE}
            ${APPKIT}
            ${QTKIT}
            ${AVFOUNDATION})
    ELSEIF(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
        list(APPEND PSMOVEAPI_TRACKER_PLATFORM_SRC
            ${CMAKE_CURRENT_LIST_DIR}/platform/camera_control_win32.c)
        list(APPEND PSMOVEAPI_TRACKER_REQUIRED_LIBS stdc++ vfw32 comctl32)
    ELSE() #Linux
        list(APPEND PSMOVEAPI_TRACKER_PLATFORM_SRC
            ${CMAKE_CURRENT_LIST_DIR}/platform/psmove_linuxsupport.c
            ${CMAKE_CURRENT_LIST_DIR}/platform/camera_control_linux.c)
    ENDIF()
ENDIF(PSMOVE_BUILD_TRACKER)

# 3 - Cameras

# PS3EYEDriver
set(INFO_USE_PS3EYE_DRIVER "No (Windows and OS X only)")
IF (PSMOVE_USE_PS3EYE_DRIVER AND (${CMAKE_SYSTEM_NAME} MATCHES "Darwin" OR ${CMAKE_SYSTEM_NAME} MATCHES "Windows"))
    add_definitions(-DCAMERA_CONTROL_USE_PS3EYE_DRIVER)
    list(APPEND PSMOVEAPI_TRACKER_PLATFORM_SRC
                ${ROOT_DIR}/external/PS3EYEDriver/src/ps3eye.cpp
                ${ROOT_DIR}/external/PS3EYEDriver/src/ps3eyedriver.cpp)
    include_directories(${ROOT_DIR}/external/PS3EYEDriver/src)
    IF(PSMOVE_USE_LOCAL_LIBUSB)
        set(INFO_USE_PS3EYE_DRIVER "Yes (static libusb)")
        #Annoyingly, each platform builds to a slightly different directory
        IF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
            # Assume the user has built libusb in external/libusb-1.0.9/
            include_directories(${CMAKE_SOURCE_DIR}/external/libusb-1.0.9/libusb)
            list(APPEND PSMOVEAPI_TRACKER_REQUIRED_LIBS
                ${ROOT_DIR}/external/libusb-1.0.9/build/libusb/.libs/libusb-1.0.a)
        ELSEIF(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
            set(PSMOVE_USE_PSEYE OFF)  # So we don't trigger the registry code paths
            # Assume the user has built libusb in external/libusb-1.0/
            include_directories(${CMAKE_SOURCE_DIR}/external/libusb-1.0/include/libusb-1.0)
            IF (${CMAKE_C_SIZEOF_DATA_PTR} EQUAL 8)
                list(APPEND PSMOVEAPI_TRACKER_REQUIRED_LIBS
                    ${ROOT_DIR}/external/libusb-1.0/MinGW64/static/libusb-1.0.a)
            ELSE()  # TODO: Detect target bitness when cross-compiling?
                list(APPEND PSMOVEAPI_TRACKER_REQUIRED_LIBS
                    ${ROOT_DIR}/external/libusb-1.0/MinGW32/static/libusb-1.0.a)
            ENDIF()
        ENDIF()
    ELSE() #using system libusb, e.g., homebrew
        set(INFO_USE_PS3EYE_DRIVER "Yes")
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(LIBUSB REQUIRED libusb-1.0)
        list(APPEND PSMOVEAPI_TRACKER_REQUIRED_LIBS ${LIBUSB_LIBRARIES})
        include_directories(${LIBUSB_INCLUDE_DIRS})
        #link_directories(${LIBUSB_LIBDIR})
    ENDIF() #PSMOVE_USE_LOCAL_LIBUSB
ELSE()
    set(PSMOVE_USE_PS3EYE_DRIVER OFF)
    set(INFO_USE_PS3EYE_DRIVER "No (disabled)")
ENDIF()

IF(PSMOVE_USE_PS3EYE_DRIVER)
    # PS3EYEDriver is based on GPL'd code
    set(INFO_LICENSE "GPL")
ENDIF()

#CL_EYE_DRIVER
set(INFO_USE_CL_EYE "No (Windows 32-bit only)")
IF(PSMOVE_USE_PSEYE AND PSMOVE_USE_CL_EYE_SDK AND ${CMAKE_SYSTEM_NAME} MATCHES "Windows") #TODO: Check for 32-bit
    find_path(CL_EYE_SDK_PATH Bin/CLEyeMulticam.dll
        HINTS "C:/Program Files/Code Laboratories/CL-Eye Platform SDK"
        "C:/Program Files (x86)/Code Laboratories/CL-Eye Platform SDK")
    IF(CL_EYE_SDK_PATH)
        add_definitions(-DCAMERA_CONTROL_USE_CL_DRIVER)
        list(APPEND PSMOVEAPI_TRACKER_REQUIRED_LIBS CLEyeMulticam)
        file(COPY ${CL_EYE_SDK_PATH}/Bin/CLEyeMulticam.dll
            DESTINATION ${ROOT_DIR}/external/libs)
        set(INFO_USE_CL_EYE_SDK "Yes (SDK version)")
        # XXX: If this crashes, disable compiler optimizations
    ELSE()  # Fall back to registry version
        set(INFO_USE_CL_EYE_SDK "Yes (registry version)")
    ENDIF()
ENDIF()

IF(PSMOVE_USE_DEBUG_CAPTURE)
    add_definitions(-DCAMERA_CONTROL_DEBUG_CAPTURED_IMAGE)
ENDIF()

file(GLOB PSMOVEAPI_TRACKER_SRC
    "${CMAKE_CURRENT_LIST_DIR}/*.c"
    "${CMAKE_CURRENT_LIST_DIR}/*.cpp"
    "${ROOT_DIR}/external/iniparser/*.c"
)

if(PSMOVE_BUILD_TRACKER)
    include_directories(${ROOT_DIR}/external/glm)
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

message("")
message("  Tracker")
message("    Tracker library:  " ${INFO_BUILD_TRACKER})
message("    AHRS algorithm:   " ${INFO_AHRS_ALGORITHM})
feature_use_info("PS Eye support:   " PSMOVE_USE_PSEYE)
feature_use_info("HTML tracing:     " PSMOVE_USE_TRACKER_TRACE)
message("    Use CL Eye SDK:   " ${INFO_USE_CL_EYE_SDK})
message("    Use PS3EYEDriver: " ${INFO_USE_PS3EYE_DRIVER})
