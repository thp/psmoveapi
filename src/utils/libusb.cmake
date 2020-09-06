################################################################################
# FUNCTION dl
################################################################################

function(dl url dstdir)
    if (EXISTS "${dstdir}")
        return()
    endif()

    message(STATUS "Downloading: " ${url})

    set(fn "${CMAKE_BINARY_DIR}/x.tar.gz")
    file(DOWNLOAD
        "${url}"
        "${fn}"
        SHOW_PROGRESS
        INACTIVITY_TIMEOUT 300 # seconds
    )
    execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory "${dstdir}")
    execute_process(COMMAND ${CMAKE_COMMAND} -E tar xz "${fn}"
        WORKING_DIRECTORY "${dstdir}"
        RESULT_VARIABLE __result
    )
    if(NOT __result EQUAL 0)
        message(FATAL_ERROR "error ${__result}")
    endif()
endfunction()

################################################################################
# FUNCTION replace_in_file_once
################################################################################

# TODO: create files in BDIR
function(replace_in_file_once f from to)
    string(SHA1 h "${f}${from}${to}")
    string(SUBSTRING "${h}" 0 5 h)

    # cannot set this file to bdir because multiple configs use
    # different bdirs and will do multiple replacements
    set(h ${f}.${h})
    set(ch ${f}.cppan.hash)

    if (EXISTS ${h} AND EXISTS ${ch})
        file(READ ${ch} h2)
        file(READ ${f} fc)
        string(SHA1 h3 "${fc}")
        if ("${h2}" STREQUAL "${h3}")
            return()
        endif()
    endif()

    set(lock ${f}.lock)
    file(
        LOCK ${lock}
        GUARD FUNCTION # CMake bug workaround https://gitlab.kitware.com/cmake/cmake/issues/16295
        RESULT_VARIABLE lock_result
    )
    if (NOT ${lock_result} EQUAL 0)
        message(FATAL_ERROR "Lock error: ${lock_result}")
    endif()

    # double check
    if (EXISTS ${h} AND EXISTS ${ch})
        file(READ ${ch} h2)
        file(READ ${f} fc)
        string(SHA1 h3 "${fc}")
        if ("${h2}" STREQUAL "${h3}")
            return()
        endif()
    endif()

    file(READ ${f} fc)
    string(REPLACE "${from}" "${to}" fc "${fc}")
    string(SHA1 h2 "${fc}")
    file(WRITE "${f}" "${fc}")

    # create flag file
    file(WRITE ${h} "")
    file(WRITE ${ch} "${h2}")
endfunction(replace_in_file_once)

################################################################################
# dirent
################################################################################

dl(
    "https://github.com/tronkko/dirent/archive/1.23.2.tar.gz"
    "${CMAKE_BINARY_DIR}/dl/dirent"
)
set(dir "${CMAKE_BINARY_DIR}/dl/dirent/dirent-1.23.2")

add_library(dirent INTERFACE)
target_include_directories(dirent INTERFACE "${dir}/include")

################################################################################
# usb
################################################################################

dl(
    "https://github.com/libusb/libusb/archive/v1.0.23.tar.gz"
    "${CMAKE_BINARY_DIR}/dl/usb"
)
set(dir "${CMAKE_BINARY_DIR}/dl/usb/libusb-1.0.23")

add_library(usb STATIC)
file(GLOB files1 "${dir}/libusb/*.c")
file(GLOB files2 "${dir}/libusb/os/*windows*")
if (MSVC)
    file(REMOVE "${dir}/msvc/errno.h")
    target_include_directories(usb PUBLIC "${dir}/msvc")
elseif (MINGW)
    target_compile_definitions(usb PRIVATE OS_WINDOWS)
    if (NOT EXISTS "${dir}/libusb/config.h")
        file(WRITE "${dir}/libusb/config.h" "
            /* Define to 1 to enable message logging. */
            #define ENABLE_LOGGING 1
            /* Define to 1 if using the Windows poll() implementation. */
            #define POLL_WINDOWS 1
            /* Define to 1 if using Windows threads. */
            #define THREADS_WINDOWS 1
            #define DEFAULT_VISIBILITY// USB_API
            #define POLL_NFDS_TYPE int
        ")
    endif()
else()
    message(FATAL_ERROR "Wrong platform")
endif()
target_include_directories(usb PUBLIC "${dir}/libusb")
target_sources(usb PRIVATE ${files1} ${files2})

################################################################################
# usbcompat
################################################################################

dl(
    "https://github.com/libusb/libusb-compat-0.1/archive/v0.1.7.tar.gz"
    "${CMAKE_BINARY_DIR}/dl/usbcompat"
)
set(dir "${CMAKE_BINARY_DIR}/dl/usbcompat/libusb-compat-0.1-0.1.7")

add_library(usbcompat STATIC)
target_sources(usbcompat PRIVATE "${dir}/libusb/core.c")
target_compile_definitions(usbcompat PRIVATE API_EXPORTED=)
target_include_directories(usbcompat PUBLIC "${dir}/libusb")
target_link_libraries(usbcompat usb dirent)
if (NOT EXISTS "${dir}/libusb/unistd.h")
    file(WRITE "${dir}/libusb/unistd.h" "")
endif()
replace_in_file_once("${dir}/libusb/core.c" "fmt..." "fmt, ...")
replace_in_file_once("${dir}/libusb/core.c" "__attribute__ ((constructor))" "")
replace_in_file_once("${dir}/libusb/core.c" "__attribute__ ((destructor))" "")

set(LIBUSB_LIBRARIES usbcompat)

################################################################################
