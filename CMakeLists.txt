
cmake_minimum_required(VERSION 2.6)

project(PSMOVEAPI)

include_directories(${PSMOVEAPI_SOURCE_DIR}/hidapi/hidapi)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

#add_definitions(-DPSMOVE_DEBUG)
add_definitions(-g -Wall)
add_definitions(-O2)

set(PSMOVEAPI_MAJOR_VERSION "1")
set(PSMOVEAPI_MINOR_VERSION "0")
set(PSMOVEAPI_PATCH_VERSION "2")
set(PSMOVEAPI_VERSION "${PSMOVEAPI_MAJOR_VERSION}.${PSMOVEAPI_MINOR_VERSION}.${PSMOVEAPI_PATCH_VERSION}")

set(PSMOVEAPI_VERSION_SONAME "${PSMOVEAPI_MAJOR_VERSION}")

set(PSMOVE_REQUIRED_LIBS)

IF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    find_library(IOKIT IOKit)
    find_library(COREFOUNDATION CoreFoundation)
    find_library(IOBLUETOOTH IOBluetooth)
    list(APPEND PSMOVE_REQUIRED_LIBS ${IOKIT})
    list(APPEND PSMOVE_REQUIRED_LIBS ${COREFOUNDATION})
    list(APPEND PSMOVE_REQUIRED_LIBS ${IOBLUETOOTH})

    set(HIDAPI_SRC ${PSMOVEAPI_SOURCE_DIR}/hidapi/mac/hid.c)
ELSEIF(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    list(APPEND PSMOVE_REQUIRED_LIBS setupapi bthprops kernel32 ws2_32)
    list(APPEND MOVED_CLIENT_REQUIRED_LIBS ws2_32)

    set(HIDAPI_SRC ${PSMOVEAPI_SOURCE_DIR}/hidapi/windows/hid.c)
ELSE()
    # Linux
    find_package(PkgConfig REQUIRED)
    find_package(Threads REQUIRED)

    pkg_check_modules(LIBUSB REQUIRED libusb-1.0)
    include_directories(${LIBUSB_INCLUDE_DIRS})
    list(APPEND PSMOVE_REQUIRED_LIBS ${LIBUSB_LIBRARIES})
    list(APPEND PSMOVE_REQUIRED_LIBS ${CMAKE_THREAD_LIBS_INIT})

    # Only needed when using hidraw version of hidapi
    pkg_check_modules(UDEV REQUIRED libudev)
    include_directories(${UDEV_INCLUDE_DIRS})
    list(APPEND PSMOVE_REQUIRED_LIBS ${UDEV_LIBRARIES})

    # On Linux, we require Bluez for getting the BT addr
    pkg_check_modules(BLUEZ REQUIRED bluez)
    include_directories(${BLUEZ_INCLUDE_DIRS})
    list(APPEND PSMOVE_REQUIRED_LIBS ${BLUEZ_LIBRARIES})

    set(HIDAPI_SRC ${PSMOVEAPI_SOURCE_DIR}/hidapi/linux/hid.c)
ENDIF()

set(MOVED_CLIENT_SRC ${PSMOVEAPI_SOURCE_DIR}/moved_client.c)
set(PSMOVE_SRC ${HIDAPI_SRC} ${MOVED_CLIENT_SRC} ${PSMOVEAPI_SOURCE_DIR}/psmove.c)

# Shared library

add_library(psmoveapi SHARED ${PSMOVE_SRC})
target_link_libraries(psmoveapi ${PSMOVE_REQUIRED_LIBS})
set_target_properties(psmoveapi PROPERTIES VERSION ${PSMOVEAPI_VERSION} SOVERSION ${PSMOVEAPI_VERSION_SONAME})

SET_TARGET_PROPERTIES(psmoveapi PROPERTIES COMPILE_FLAGS -DBUILDING_SHARED_LIBRARY)

# C utilities (those get installed on "make install")
add_executable(psmovepair c/pair.c)
target_link_libraries(psmovepair psmoveapi)

# C examples

add_executable(example c/example.c)
target_link_libraries(example psmoveapi)

add_executable(moved moved.c)
target_link_libraries(moved psmoveapi)

add_executable(moved-client moved_client_test.c ${MOVED_CLIENT_SRC})
target_link_libraries(moved-client ${MOVED_CLIENT_REQUIRED_LIBS})

# Disabled building of example2 now - uncomment if you have libncurses-dev
#find_package(CURSES)
#if(CURSES_FOUND)
#    add_executable(example2 c/example2.c)
#    target_link_libraries(example2 psmoveapi ${CURSES_LIBS})
#    if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
#        # Dirty hack to get example2 to compile in OS X
#        target_link_libraries(example2 ncurses)
#    endif()
#endif()

add_executable(multiple c/multiple.c)
target_link_libraries(multiple psmoveapi)

# These will be set to "Yes" below if the dependencies are found
set(BUILD_PYTHON_BINDINGS "No ")
set(BUILD_JAVA_BINDINGS "No ")
set(BUILD_QT_BINDINGS "No ")

# Language bindings (Python and Java)
find_package(SWIG)
if(SWIG_FOUND)
    include(${SWIG_USE_FILE})

    find_package(PythonLibs)
    if(PYTHONLIBS_FOUND)
        unset(CMAKE_SWIG_FLAGS)
        include_directories(${PYTHON_INCLUDE_PATH})
        swig_add_module(psmove python psmove.i)
        swig_link_libraries(psmove psmoveapi ${PYTHON_LIBRARIES})
        set(BUILD_PYTHON_BINDINGS "Yes")
    endif()

    find_package(JNI)
    if(JNI_FOUND)
        set(PSMOVEAPI_JAVA_PKG "io.thp.psmove")
        string(REPLACE "." "/" PSMOVEAPI_JAVA_PKG_DIR ${PSMOVEAPI_JAVA_PKG})

        set(CMAKE_SWIG_FLAGS -package ${PSMOVEAPI_JAVA_PKG})
        include_directories(${JNI_INCLUDE_DIRS})
        swig_add_module(psmove_java java psmove.i)
        swig_link_libraries(psmove_java psmoveapi ${JNI_LIBRARIES})
        set(BUILD_JAVA_BINDINGS "Yes")

        add_custom_target(psmoveapi.jar ALL
            COMMAND mkdir -p ${PSMOVEAPI_JAVA_PKG_DIR}
            COMMAND mv *.java ${PSMOVEAPI_JAVA_PKG_DIR}
            COMMAND javac ${PSMOVEAPI_JAVA_PKG_DIR}/*.java
            COMMAND jar cf psmoveapi.jar ${PSMOVEAPI_JAVA_PKG_DIR}/*.class
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Java bindings JAR archive"
        )
    endif()
endif()

# For system-wide installation

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/psmoveapi.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/psmoveapi.pc
    @ONLY)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/psmoveapi.pc DESTINATION lib/pkgconfig)

install(TARGETS psmovepair psmoveapi
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
)

install(FILES psmove.h DESTINATION include/psmoveapi)
install(FILES README COPYING DESTINATION share/psmoveapi)

# Qt bindings
find_package(Qt4)

if(QT_QTDECLARATIVE_FOUND)
    add_subdirectory(qt)
    set(BUILD_QT_BINDINGS "Yes")
endif(QT_QTDECLARATIVE_FOUND)

set(PADDING "                     ")
MESSAGE("")
MESSAGE("      +------ PS Move API Build Configuration ------+ ")
MESSAGE("      |  Python bindings: " ${PADDING} ${BUILD_PYTHON_BINDINGS} "  |")
MESSAGE("      |  Java bindings:   " ${PADDING} ${BUILD_JAVA_BINDINGS} "  |")
MESSAGE("      |  Qt bindings:     " ${PADDING} ${BUILD_QT_BINDINGS} "  |")
MESSAGE("      +---------------------------------------------+ ")
MESSAGE("")

