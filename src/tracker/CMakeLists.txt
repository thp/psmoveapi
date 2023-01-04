cmake_minimum_required(VERSION 3.16)
set(ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/../..)
include(${ROOT_DIR}/cmake/common.cmake)

set(PSMOVEAPI_TRACKER_REQUIRED_LIBS)
set(PSMOVEAPI_TRACKER_PLATFORM_SRC)

# Build the OpenCV-based camera tracking module? (see psmove_tracker.h)
option(PSMOVE_BUILD_TRACKER "Build the Tracker module (needs OpenCV)" ON)

# Use the PS3EYEDriver to interface with the PS Eye camera
option(PSMOVE_USE_PS3EYE_DRIVER "Use the PS3EYEDriver" OFF)

# To show the OpenCV camera capture window everytime (useful for debugging)
option(PSMOVE_USE_DEBUG_CAPTURE "Always show camera capture input" OFF)


IF(PSMOVE_USE_PS3EYE_DRIVER)
    # PS3EYEDriver is based on GPL'd code
    set(INFO_LICENSE "GPL")

    include_directories(${ROOT_DIR}/external/PS3EYEDriver/src)
    find_package(USB1 REQUIRED)
    list(APPEND PSMOVEAPI_TRACKER_REQUIRED_LIBS ${LIBUSB_LIBRARIES})

    set(PS3_EYE_SRC)
    list(APPEND PS3_EYE_SRC
        ${ROOT_DIR}/external/PS3EYEDriver/src/ps3eye.cpp
        ${ROOT_DIR}/external/PS3EYEDriver/src/ps3eye_capi.cpp)

    set(PS3_EYE_HEADER)
    list(APPEND PS3_EYE_HEADER
        ${ROOT_DIR}/external/PS3EYEDriver/src/ps3eye.h
        ${ROOT_DIR}/external/PS3EYEDriver/src/ps3eye_capi.h)

    list(APPEND PSMOVEAPI_TRACKER_PLATFORM_SRC
        ${PS3_EYE_SRC}
        ${PS3_EYE_HEADER})

    source_group("Header Files\\PS3EyeDriver" FILES ${PS3_EYE_HEADER})
    source_group("Source Files\\PS3EyeDriver" FILES ${PS3_EYE_SRC})
ELSE()
    set(PSMOVE_USE_PS3EYE_DRIVER OFF)
ENDIF()


#
# Tracker support
#

# 1 - OpenCV
IF(PSMOVE_BUILD_TRACKER)
    FIND_PACKAGE(OpenCV 4 QUIET)
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
        set(INFO_BUILD_TRACKER "Yes (OpenCV version ${OpenCV_VERSION})")

        IF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
            find_library(QUARTZCORE QuartzCore)
            find_library(APPKIT AppKit)
            find_library(QTKIT QTKit)
            find_library(AVFOUNDATION AVFoundation)
            find_library(SECURITY Security)
            list(APPEND PSMOVEAPI_TRACKER_REQUIRED_LIBS
                stdc++
                ${QUARTZCORE}
                ${APPKIT}
                ${QTKIT}
                ${AVFOUNDATION}
                ${SECURITY})
        ELSEIF(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
            list(APPEND PSMOVEAPI_TRACKER_REQUIRED_LIBS vfw32 comctl32)
            #OpenCV extra dependencies: comctl32 gdi32 ole32 setupapi ws2_32 vfw32
            IF(MINGW)
                list(APPEND PSMOVEAPI_TRACKER_REQUIRED_LIBS stdc++)
            ENDIF(MINGW)
        ELSEIF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
            list(APPEND PSMOVEAPI_TRACKER_REQUIRED_LIBS v4l2)
        ENDIF()
    ELSE()
        set(INFO_BUILD_TRACKER "No (OpenCV not found)")
        set(PSMOVE_BUILD_TRACKER OFF CACHE BOOL "Disabling PS Move Tracker -- OpenCV not found" FORCE)
    ENDIF()
ELSE()
    set(INFO_BUILD_TRACKER "No (disabled)")
ENDIF()

set(INFO_CAMERA_CONTROL_DRIVER "-")

# 2 - Platform-specific settings
IF(PSMOVE_BUILD_TRACKER)
    IF(PSMOVE_USE_PS3EYE_DRIVER)
        set(INFO_CAMERA_CONTROL_DRIVER "PS3EYEDriver")
        list(APPEND PSMOVEAPI_TRACKER_PLATFORM_SRC
            ${CMAKE_CURRENT_LIST_DIR}/camera_control_driver_ps3eyedriver.cpp)
    ELSEIF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
        set(INFO_CAMERA_CONTROL_DRIVER "V4L2")
        list(APPEND PSMOVEAPI_TRACKER_PLATFORM_SRC
            ${CMAKE_CURRENT_LIST_DIR}/camera_control_driver_v4l2.cpp)
        list(APPEND PSMOVEAPI_TRACKER_REQUIRED_LIBS v4l2)
    ELSE()
        set(PSMOVE_BUILD_TRACKER OFF CACHE BOOL "Disabling PS Move Tracker -- No Camera Control driver available" FORCE)
        set(INFO_BUILD_TRACKER "No (no camera control driver)")
    ENDIF()
ENDIF(PSMOVE_BUILD_TRACKER)

# 3 - Cameras

IF(PSMOVE_USE_DEBUG_CAPTURE)
    add_definitions(-DCAMERA_CONTROL_DEBUG_CAPTURED_IMAGE)
ENDIF()

set(PSMOVEAPI_TRACKER_SRC)
list(APPEND PSMOVEAPI_TRACKER_SRC
    "${CMAKE_CURRENT_LIST_DIR}/psmove_tracker.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/psmove_fusion.cpp"

    "${CMAKE_CURRENT_LIST_DIR}/tracker_helpers.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/tracker_helpers.h"

    "${CMAKE_CURRENT_LIST_DIR}/camera_control.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/camera_control.h"

    "${CMAKE_CURRENT_LIST_DIR}/camera_control_driver.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/camera_control_driver.h"

    "${ROOT_DIR}/include/psmove_fusion.h"
    "${ROOT_DIR}/include/psmove_tracker.h"
    "${ROOT_DIR}/include/psmove_tracker_opencv.h"
)


if(PSMOVE_BUILD_TRACKER)
    add_library(psmoveapi_tracker SHARED
        ${PSMOVEAPI_TRACKER_SRC}
        ${PSMOVEAPI_TRACKER_PLATFORM_SRC})
    target_include_directories(psmoveapi_tracker PRIVATE ${LIBUSB_INCLUDE_DIR})
    target_link_libraries(psmoveapi_tracker
        psmoveapi
        ${PSMOVEAPI_REQUIRED_LIBS}
        ${PSMOVEAPI_TRACKER_REQUIRED_LIBS})
    set_target_properties(psmoveapi_tracker PROPERTIES
        COMPILE_FLAGS -DBUILDING_SHARED_LIBRARY)
    list(APPEND PSMOVEAPI_INSTALL_TARGETS psmoveapi_tracker)
endif()

message("")
message("  Tracker")
message("    Tracker library:       " ${INFO_BUILD_TRACKER})
message("    Camera control driver: " ${INFO_CAMERA_CONTROL_DRIVER})
