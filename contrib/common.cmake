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

# Pretty-print if a "use" feature has been enabled
macro(feature_use_info CAPTION FEATURE)
    if(${FEATURE})
        message("    " ${CAPTION} "Yes")
    else()
        message("    " ${CAPTION} "No")
    endif()
endmacro()

LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}")