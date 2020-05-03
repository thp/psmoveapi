cmake_minimum_required(VERSION 2.8.12)
set(ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/..)
include(${ROOT_DIR}/cmake/common.cmake)

# Do all the PSMOVEAPI-only work
# At the end we will call separate files for tracker and utils

set(PSMOVEAPI_VERSION_MAJOR "4")
set(PSMOVEAPI_VERSION_MINOR "0")
set(PSMOVEAPI_VERSION_PATCH "10")
set(PSMOVEAPI_VERSION "${PSMOVEAPI_VERSION_MAJOR}.${PSMOVEAPI_VERSION_MINOR}.${PSMOVEAPI_VERSION_PATCH}")

set(PSMOVEAPI_REQUIRED_LIBS)   # Container for library dependencies
set(PSMOVEAPI_PLATFORM_SRC)    # Container for source files
set(PSMOVEAPI_INSTALL_TARGETS) # Container for build targets to be installed
set(PSMOVEAPI_MATH_SRC)        # Container for math related source files

include_directories(${ROOT_DIR}/external/hidapi/hidapi)
include_directories(${ROOT_DIR}/external/glm)
include_directories(${CMAKE_BINARY_DIR})

set(INFO_LICENSE "BSD")

# Make a debug build with helpful output for debugging / maintenance
option(PSMOVE_USE_DEBUG "Build for debugging" OFF)

# Debugging output
IF(PSMOVE_USE_DEBUG)
    add_definitions(-DPSMOVE_DEBUG)
    set(INFO_USE_DEBUG "Yes")
    IF(NOT MSVC)
        add_definitions(-g -O0 -Wall)
    ENDIF(NOT MSVC)
ELSE()
    set(INFO_USE_DEBUG "No")
    IF(NOT MSVC)
        add_definitions(-O2 -Wall -Wno-unused)
    ENDIF(NOT MSVC)
ENDIF()

# Linker commands to put into the .pc file (when installing)
set(PSMOVEAPI_PKGCONFIG_LIBS "-lpsmoveapi")

IF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    find_library(FOUNDATION Foundation)
    find_library(AVFOUNDATION AVFoundation)
    find_library(IOKIT IOKit)
    find_library(COREFOUNDATION CoreFoundation)
    find_library(IOBLUETOOTH IOBluetooth)
    list(APPEND PSMOVEAPI_REQUIRED_LIBS ${FOUNDATION})
    list(APPEND PSMOVEAPI_REQUIRED_LIBS ${AVFOUNDATION})
    list(APPEND PSMOVEAPI_REQUIRED_LIBS ${IOKIT})
    list(APPEND PSMOVEAPI_REQUIRED_LIBS ${COREFOUNDATION})
    list(APPEND PSMOVEAPI_REQUIRED_LIBS ${IOBLUETOOTH})

    set(HIDAPI_SRC ${ROOT_DIR}/external/hidapi/mac/hid.c)

    list(APPEND PSMOVEAPI_PLATFORM_SRC
        ${CMAKE_CURRENT_LIST_DIR}/platform/psmove_port_osx.mm)

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -stdlib=libc++")
    set(CMAKE_STATIC_LIBRARY_FLAGS "${CMAKE_STATIC_LIBRARY_FLAGS} -stdlib=libc++")

    list(APPEND PSMOVEAPI_MOVED_SRC
        ${CMAKE_CURRENT_LIST_DIR}/daemon/moved_monitor_osx.mm)
ELSEIF(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    list(APPEND PSMOVEAPI_REQUIRED_LIBS setupapi bthprops kernel32 ws2_32 winmm version imm32)

    IF(MINGW)
        # http://stackoverflow.com/questions/4702732/the-program-cant-start-because-libgcc-s-dw2-1-dll-is-missing
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static -static-libgcc -static-libstdc++")
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -static -static-libgcc -static-libstdc++")
        set(CMAKE_STATIC_LIBRARY_FLAGS "${CMAKE_STATIC_LIBRARY_FLAGS} -static -static-libgcc -static-libstdc++")
    ENDIF(MINGW)

    set(HIDAPI_SRC ${ROOT_DIR}/external/hidapi/windows/hid.c)

    list(APPEND PSMOVEAPI_PLATFORM_SRC
        ${CMAKE_CURRENT_LIST_DIR}/platform/psmove_port_windows.c)
ELSE()
    # Linux
    find_package(PkgConfig REQUIRED)
    find_package(Threads REQUIRED)
    list(APPEND PSMOVEAPI_REQUIRED_LIBS ${CMAKE_THREAD_LIBS_INIT})
    list(APPEND PSMOVEAPI_REQUIRED_LIBS m)

    pkg_check_modules(DBus REQUIRED dbus-1)
    include_directories(${DBus_INCLUDE_DIRS})
    list(APPEND PSMOVEAPI_REQUIRED_LIBS ${DBus_LIBRARIES})

    pkg_check_modules(UDEV REQUIRED libudev)
    include_directories(${UDEV_INCLUDE_DIRS})
    list(APPEND PSMOVEAPI_REQUIRED_LIBS ${UDEV_LIBRARIES})

    pkg_check_modules(BLUEZ REQUIRED bluez>=5)

    include_directories(${BLUEZ_INCLUDE_DIRS})
    list(APPEND PSMOVEAPI_REQUIRED_LIBS ${BLUEZ_LIBRARIES})

    set(HIDAPI_SRC ${ROOT_DIR}/external/hidapi/linux/hid.c)

    list(APPEND PSMOVEAPI_PLATFORM_SRC
        ${CMAKE_CURRENT_LIST_DIR}/platform/psmove_port_linux.cpp)

    list(APPEND PSMOVEAPI_MOVED_SRC
        ${CMAKE_CURRENT_LIST_DIR}/daemon/moved_monitor_linux.c)
ENDIF()

include_directories(${ROOT_DIR}/include)

configure_file(${ROOT_DIR}/include/psmove_config.h.in
    ${CMAKE_BINARY_DIR}/psmove_config.h
    @ONLY)


list(APPEND PSMOVEAPI_MOVED_SRC
    "${CMAKE_CURRENT_LIST_DIR}/daemon/moved_client.cpp")

file(GLOB PSMOVEAPI_MOVED_HEADER
	"${CMAKE_CURRENT_LIST_DIR}/daemon/*.h"
)

source_group("Header Files\\daemon" FILES ${PSMOVEAPI_MOVED_HEADER})
source_group("Source Files\\daemon" FILES ${PSMOVEAPI_MOVED_SRC})

file(GLOB PSMOVEAPI_SRC
    "${CMAKE_CURRENT_LIST_DIR}/*.c"
    "${CMAKE_CURRENT_LIST_DIR}/*.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/*.h"
)

file(GLOB PSMOVEAPI_HEADERS
    "${ROOT_DIR}/include/*.h"
    "${CMAKE_BINARY_DIR}/*.h"
)

file (GLOB PSMOVEAPI_MATH_HEADERS
    "${CMAKE_CURRENT_LIST_DIR}/math/*.h"
    "${CMAKE_CURRENT_LIST_DIR}/math/*.hpp"    
)

file (GLOB PSMOVEAPI_MATH_SRC
    "${CMAKE_CURRENT_LIST_DIR}/math/*.c"
    "${CMAKE_CURRENT_LIST_DIR}/math/*.cpp"
)

source_group("Header Files\\math" FILES ${PSMOVEAPI_MATH_HEADERS})
source_group("Source Files\\math" FILES ${PSMOVEAPI_MATH_SRC})

# Source files that are needed for both the shared and static library

set(PSMOVEAPI_LIBRARY_SRC)
list(APPEND PSMOVEAPI_LIBRARY_SRC ${HIDAPI_SRC})
list(APPEND PSMOVEAPI_LIBRARY_SRC ${PSMOVEAPI_SRC})
list(APPEND PSMOVEAPI_LIBRARY_SRC ${PSMOVEAPI_MOVED_SRC})
list(APPEND PSMOVEAPI_LIBRARY_SRC ${PSMOVEAPI_MOVED_HEADER})
list(APPEND PSMOVEAPI_LIBRARY_SRC ${PSMOVEAPI_PLATFORM_SRC})
list(APPEND PSMOVEAPI_LIBRARY_SRC ${PSMOVEAPI_ALGORITHM_SRC})
list(APPEND PSMOVEAPI_LIBRARY_SRC ${PSMOVEAPI_MATH_HEADERS})
list(APPEND PSMOVEAPI_LIBRARY_SRC ${PSMOVEAPI_MATH_SRC})

# Shared library

add_library(psmoveapi SHARED ${PSMOVEAPI_LIBRARY_SRC})
target_link_libraries(psmoveapi ${PSMOVEAPI_REQUIRED_LIBS})
set_target_properties(psmoveapi PROPERTIES
    COMPILE_FLAGS -DBUILDING_SHARED_LIBRARY)
list(APPEND PSMOVEAPI_INSTALL_TARGETS psmoveapi)

# For system-wide installation

if(NOT PSMOVEAPI_LIB_DEST)
    if(CMAKE_LIBRARY_ARCHITECTURE)
        set(PSMOVEAPI_LIB_DEST "lib/${CMAKE_LIBRARY_ARCHITECTURE}")
    else()
        if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
            set(PSMOVEAPI_LIB_DEST "lib")
        elseif(${CMAKE_C_SIZEOF_DATA_PTR} EQUAL 8)
            set(PSMOVEAPI_LIB_DEST "lib64")
        else()
            set(PSMOVEAPI_LIB_DEST "lib")
        endif()
    endif()
endif()

configure_file(${ROOT_DIR}/contrib/psmoveapi.pc.in
    ${CMAKE_BINARY_DIR}/psmoveapi.pc
    @ONLY)

include(${CMAKE_CURRENT_LIST_DIR}/tracker/CMakeLists.txt)
include(${CMAKE_CURRENT_LIST_DIR}/utils/CMakeLists.txt)

if (NOT (${CMAKE_SYSTEM_NAME} MATCHES "Darwin"))
    install(FILES ${CMAKE_BINARY_DIR}/psmoveapi.pc
        DESTINATION ${PSMOVEAPI_LIB_DEST}/pkgconfig)
endif()

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    # Only install the udev configuration file on Linux
    install(FILES ${ROOT_DIR}/contrib/99-psmove.rules
        DESTINATION /etc/udev/rules.d)
endif()

install(TARGETS ${PSMOVEAPI_INSTALL_TARGETS}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION ${PSMOVEAPI_LIB_DEST}
    ARCHIVE DESTINATION ${PSMOVEAPI_LIB_DEST}
)

install(FILES ${PSMOVEAPI_HEADERS} DESTINATION include/psmoveapi)
install(FILES README.md COPYING DESTINATION share/psmoveapi)

message("")
message("  Build configuration")
message("    Debug build:      " ${INFO_USE_DEBUG})
message("    Library license:  " ${INFO_LICENSE} " (see README.md for details)")
