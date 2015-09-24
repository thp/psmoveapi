# - Try to find the freetype library
# Once done this defines
#
#  LIBUSB_FOUND - system has libusb
#  LIBUSB_INCLUDE_DIR - the libusb include directory
#  LIBUSB_LIBRARIES - Link these to use libusb

# Copyright (c) 2006, 2008  Laurent Montel, <montel@kde.org>
#
#
# Modified on 2015/07/14 by Chadwick Boulay <chadwick.boulay@gmail.com>
# to use static local libraries only.
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.


IF (LIBUSB_INCLUDE_DIR AND LIBUSB_LIBRARIES)

    # in cache already
    set(LIBUSB_FOUND TRUE)

ELSE (LIBUSB_INCLUDE_DIR AND LIBUSB_LIBRARIES)
    # Because we want to use the static library,
    # look locally only.
    find_path(LIBUSB_INCLUDE_DIR
        NAMES
            libusb.h
        PATHS 
            ${CMAKE_CURRENT_LIST_DIR}/../external/libusb-1.0/libusb
            ${CMAKE_CURRENT_LIST_DIR}/../external/libusb-1.0/libusb-1.0
            /usr/local/include
    )
    # There are 4 platform-specific ways we might get the libraries.
    # 1 - Windows MSVC, download the source, compile with MSVC
    # 2 - Windows MSVC, download pre-compiled binaries <- Do not use; wrong CRT library
    # 3 - Windows MinGW, download pre-compiled binaries
    # 4 - OSX, download the source, build, but do not install
    # 5 - OSX, homebrew OR download the source, build, and install
    # Each of these puts the compiled library into a different folder
    # and that is also architecture-specific.

    set(LIBUSB_LIB_SEARCH_PATH ${CMAKE_CURRENT_LIST_DIR}/../external/libusb-1.0)
    IF(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
        IF(MINGW)
            set(LIBUSB_PLATFORM_PREFIX MinGW)
            IF (${CMAKE_C_SIZEOF_DATA_PTR} EQUAL 8)
                list(APPEND LIBUSB_LIB_SEARCH_PATH
                    ${CMAKE_CURRENT_LIST_DIR}/../external/libusb-1.0/MinGW64/static)
                #TODO: Add self-compiled folder for MinGW
            ELSE()
                list(APPEND LIBUSB_LIB_SEARCH_PATH
                    ${CMAKE_CURRENT_LIST_DIR}/../external/libusb-1.0/MinGW32/static)
                #TODO: Add self-compiled folder for MinGW
            ENDIF()
        ELSE()
            IF (${CMAKE_C_SIZEOF_DATA_PTR} EQUAL 8)
                list(APPEND LIBUSB_LIB_SEARCH_PATH
                    ${CMAKE_CURRENT_LIST_DIR}/../external/libusb-1.0/x64/Release/lib)
            ELSE()
                list(APPEND LIBUSB_LIB_SEARCH_PATH
                    ${CMAKE_CURRENT_LIST_DIR}/../external/libusb-1.0/Win32/Release/lib)
            ENDIF()
            set(LIBUSB_PLATFORM_PREFIX MS)
        ENDIF()
    ELSEIF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
        list(APPEND LIBUSB_LIB_SEARCH_PATH
                    ${CMAKE_CURRENT_LIST_DIR}/../external/libusb-1.0/libusb/.libs
                    /usr/local/lib)
    ENDIF()
    
    find_library(LIBUSB_LIBRARIES
        NAMES
            libusb-1.0.a libusb-1.0.lib libusb-1.0 usb-1.0 usb
        PATHS
            ${LIBUSB_LIB_SEARCH_PATH}
    )

  include(FindPackageHandleStandardArgs)
  FIND_PACKAGE_HANDLE_STANDARD_ARGS(LIBUSB DEFAULT_MSG LIBUSB_LIBRARIES LIBUSB_INCLUDE_DIR)

  MARK_AS_ADVANCED(LIBUSB_INCLUDE_DIR LIBUSB_LIBRARIES)

ENDIF (LIBUSB_INCLUDE_DIR AND LIBUSB_LIBRARIES)