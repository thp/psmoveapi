# Some platform-specific but target-agnostic settings

# We recommend using MinGW-w64 for the Windows builds which generates
# position-independent code by default, so skip this for Windows builds.
IF(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    # https://github.com/thp/psmoveapi/issues/29
    add_definitions(-fPIC)
ENDIF()

# Required by pthread, must be defined before include of any standard header.
# It's easier to do this here once instead of multiple times in different source/header files.
IF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    add_definitions(-D_GNU_SOURCE)
ENDIF()

# Prevent windows.h from automatically including a bunch of header files
IF(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    add_definitions(-DWIN32_LEAN_AND_MEAN)
ENDIF()

IF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    # Build Universal Binaries for OS X
    set(CMAKE_OSX_ARCHITECTURES "x86_64")
ENDIF()

# This should be on by default in latest MinGW and MSVC >= 2013, but just in case.
IF(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
ENDIF()

# Windows' math include does not define constants by default.
# Set this definition so it does.
# Also set NOMINMAX so the min and max functions are not overwritten with macros.
IF(MSVC)
    add_definitions(-D_USE_MATH_DEFINES)
    add_definitions(-DNOMINMAX)
ENDIF()

# Targets requiring symbols not present by default in MSVC
# can include ${MSVC_INCLUDES}, link ${MSVC_LIBS}, and
# add ${MSVC_SRCS} to their list of sources.
IF(MSVC_INCLUDES AND MSVC_LIBS AND MSVC_SRCS) #pragma once guard
ELSE()
    set(MSVC_INCLUDES)
    set(MSVC_LIBS)
    set(MSVC_SRCS)
    IF(MSVC)
        list(APPEND MSVC_INCLUDES ${CMAKE_CURRENT_LIST_DIR}/../external/pthreads-w32/include ${CMAKE_CURRENT_LIST_DIR}/../external/msvc-support)
        list(APPEND MSVC_SRCS ${CMAKE_CURRENT_LIST_DIR}/../external/msvc-support/getopt.c ${CMAKE_CURRENT_LIST_DIR}/../external/msvc-support/unistd.c)
        IF(CMAKE_SIZEOF_VOID_P EQUAL 8)
            find_library(PThreadLib
                NAMES pthreadVC2
                PATHS ${CMAKE_CURRENT_LIST_DIR}/../external/pthreads-w32/lib/x64)
        ELSE()
            find_library(PThreadLib
                NAMES pthreadVC2
                PATHS ${CMAKE_CURRENT_LIST_DIR}/../external/pthreads-w32/lib/x86)
        ENDIF()
        list(APPEND MSVC_LIBS ${PThreadLib})
    ENDIF(MSVC)
ENDIF()

# Pretty-print if a "use" feature has been enabled
macro(feature_use_info CAPTION FEATURE)
    if(${FEATURE})
        message("    " ${CAPTION} "Yes")
    else()
        message("    " ${CAPTION} "No")
    endif()
endmacro()

LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}")