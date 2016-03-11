cmake_minimum_required(VERSION 2.8.12)
set(ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/../..)
include(${ROOT_DIR}/cmake/common.cmake)

set(PSMOVEAPI_TRACKER_REQUIRED_LIBS)
set(PSMOVEAPI_TRACKER_PLATFORM_SRC)

# Build the OpenCV-based camera tracking module? (see psmove_tracker.h)
option(PSMOVE_BUILD_TRACKER "Build the Tracker module (needs OpenCV)" ON)

# Enable tweaks (e.g. registry settings on Windows, ...) for PS Eye
option(PSMOVE_USE_PSEYE "Enable tweaks for the PS Eye camera" ON)

# Use the CL Eye SDK to interface with the PS Eye camera (Windows only)
option(PSMOVE_USE_CL_EYE_SDK "Use the CL Eye SDK driver on Windows" OFF)

# Use the PS3EYEDriver to interface with the PS Eye camera (OS X and Windows only)
option(PSMOVE_USE_PS3EYE_DRIVER "Use the PS3EYEDriver on OS X or Windows" OFF)

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
        IF ("${OpenCV_LIBS}" STREQUAL "")
            message("OpenCV Libs was empty! Manually setting.")
            list(APPEND OpenCV_LIBS opencv_core opencv_imgproc opencv_calib3d opencv_features2d opencv_highgui)
        ENDIF()
        IF ("${OpenCV_INCLUDE_DIRS}" STREQUAL "" AND ${OpenCV_DIR})
            message("OpenCV include dirs was empty! Manually setting.")
            set (OpenCV_INCLUDE_DIRS ${OpenCV_DIR}\include)
        ENDIF()    
        list(APPEND PSMOVEAPI_TRACKER_REQUIRED_LIBS ${OpenCV_LIBS})
        include_directories(${OpenCV_INCLUDE_DIR})
        set(INFO_BUILD_TRACKER "Yes")
    ELSE()
        set(INFO_BUILD_TRACKER "No (OpenCV not found)")
        set(PSMOVE_BUILD_TRACKER OFF)
    ENDIF()
ELSE()
    set(INFO_BUILD_TRACKER "No (disabled)")
ENDIF()
IF(PSMOVE_BUILD_TRACKER)
    set(PSMOVEAPI_PKGCONFIG_LIBS "${PSMOVEAPI_PKGCONFIG_LIBS} -lpsmoveapi_tracker")
ENDIF()

# 2 - Platform-specific settings
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
        include_directories(${MSVC_INCLUDES})
        list(APPEND PSMOVEAPI_TRACKER_PLATFORM_SRC
            ${CMAKE_CURRENT_LIST_DIR}/platform/camera_control_win32.c ${MSVC_SRCS})
        list(APPEND PSMOVEAPI_TRACKER_REQUIRED_LIBS vfw32 comctl32 ${MSVC_LIBS})
        #OpenCV extra dependencies: comctl32 gdi32 ole32 setupapi ws2_32 vfw32
        IF(MINGW)
            list(APPEND PSMOVEAPI_TRACKER_REQUIRED_LIBS stdc++)
        ENDIF(MINGW)
    ELSE() #Linux
        list(APPEND PSMOVEAPI_TRACKER_PLATFORM_SRC
            ${CMAKE_CURRENT_LIST_DIR}/platform/psmove_linuxsupport.c
			${CMAKE_CURRENT_LIST_DIR}/platform/psmove_linuxsupport.h
            ${CMAKE_CURRENT_LIST_DIR}/platform/camera_control_linux.c)
    ENDIF()
ENDIF(PSMOVE_BUILD_TRACKER)

# 3 - Cameras

# PS3EYEDriver
set(INFO_USE_PS3EYE_DRIVER "No (Windows and OS X only)")
IF (PSMOVE_USE_PS3EYE_DRIVER AND (${CMAKE_SYSTEM_NAME} MATCHES "Darwin" OR ${CMAKE_SYSTEM_NAME} MATCHES "Windows"))
    add_definitions(-DCAMERA_CONTROL_USE_PS3EYE_DRIVER)
	set(PS3_EYE_SRC)
    list(APPEND PS3_EYE_SRC
                ${ROOT_DIR}/external/PS3EYEDriver/src/ps3eye.cpp
                ${ROOT_DIR}/external/PS3EYEDriver/src/ps3eye_capi.cpp)
	set(PS3_EYE_HEADER)
    list(APPEND PS3_EYE_HEADER
				${ROOT_DIR}/external/PS3EYEDriver/src/ps3eye.h
				${ROOT_DIR}/external/PS3EYEDriver/src/ps3eye_capi.h)

	list(APPEND PSMOVEAPI_TRACKER_PLATFORM_SRC ${PS3_EYE_SRC} ${PS3_EYE_HEADER})
	source_group("Header Files\\PS3EyeDriver" FILES ${PS3_EYE_HEADER})
	source_group("Source Files\\PS3EyeDriver" FILES ${PS3_EYE_SRC})
    include_directories(${ROOT_DIR}/external/PS3EYEDriver/src)
    find_package(USB1 REQUIRED)
    include_directories(${LIBUSB_INCLUDE_DIR})
    list(APPEND PSMOVEAPI_TRACKER_REQUIRED_LIBS ${LIBUSB_LIBRARIES})
    set(INFO_USE_PS3EYE_DRIVER "Yes")
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
IF(PSMOVE_USE_PSEYE
    AND PSMOVE_USE_CL_EYE_SDK
    AND ${CMAKE_SYSTEM_NAME} MATCHES "Windows"
    AND NOT(${CMAKE_C_SIZEOF_DATA_PTR} EQUAL 8))
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
else()
    SET(INFO_USE_CL_EYE_SDK "No")
ENDIF()

IF(PSMOVE_USE_DEBUG_CAPTURE)
    add_definitions(-DCAMERA_CONTROL_DEBUG_CAPTURED_IMAGE)
ENDIF()

file(GLOB PSMOVEAPI_TRACKER_SRC
    "${CMAKE_CURRENT_LIST_DIR}/*.c"
    "${CMAKE_CURRENT_LIST_DIR}/*.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/*.h"
)

list(APPEND PSMOVEAPI_TRACKER_SRC 
	"${ROOT_DIR}/include/psmove_fusion.h" 
	"${ROOT_DIR}/include/psmove_tracker.h")

file(GLOB INIPARSER_SRC
    "${ROOT_DIR}/external/iniparser/*.c"
)

file(GLOB INIPARSER_HEADER
    "${ROOT_DIR}/external/iniparser/*.h"
)

list(APPEND PSMOVEAPI_TRACKER_SRC ${INIPARSER_SRC} ${INIPARSER_HEADER})

source_group("Header Files\\iniparser" FILES ${INIPARSER_HEADER})
source_group("Source Files\\iniparser" FILES ${INIPARSER_SRC})


if(PSMOVE_BUILD_TRACKER)
	IF(MSVC)
		# Due to a bug in OpenCV 3.0+, C source files that call cvGetSize will crash when built by MSVC.
		# psmove_tracker.c is such a file, so we need to force the C++ compiler to be used for it, even though it's a C file. 
		# Once the OpenCV bug is fixed, this can/should be removed.
		#
		# See https://github.com/Itseez/opencv/issues/6221 for more info.
		set_source_files_properties( ${CMAKE_CURRENT_LIST_DIR}/psmove_tracker.c PROPERTIES LANGUAGE CXX )
	ENDIF(MSVC)
	
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
feature_use_info("PS Eye support:   " PSMOVE_USE_PSEYE)
feature_use_info("HTML tracing:     " PSMOVE_USE_TRACKER_TRACE)
message("    Use CL Eye SDK:   " ${INFO_USE_CL_EYE_SDK})
message("    Use PS3EYEDriver: " ${INFO_USE_PS3EYE_DRIVER})
