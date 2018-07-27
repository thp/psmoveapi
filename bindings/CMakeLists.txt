cmake_minimum_required(VERSION 2.8.12)
set(ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/..)
include(${ROOT_DIR}/cmake/common.cmake)

# Language bindings

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/bindings)

option(PSMOVE_BUILD_JAVA_BINDINGS "Build the Java bindings" ON)
option(PSMOVE_BUILD_CSHARP_BINDINGS "Build the C# bindings" ON)
option(PSMOVE_BUILD_PROCESSING_BINDINGS "Build the Processing bindings" ON)
option(PSMOVE_BUILD_PYTHON_BINDINGS "Build the Python bindings" ON)

# useful if multiple versions of the Python lib are installed
set(PSMOVE_PYTHON_VERSION "" CACHE STRING "Use this version of the Python lib for building Python bindings")

# Language bindings (Python, Java and Processing)
find_package(SWIG QUIET)
if(SWIG_FOUND)
    include(${SWIG_USE_FILE})

    if(PSMOVE_BUILD_PYTHON_BINDINGS)
        find_package(PythonLibs ${PSMOVE_PYTHON_VERSION} QUIET)
        if(PYTHONLIBS_FOUND)
            unset(CMAKE_SWIG_FLAGS)
            include_directories(${PYTHON_INCLUDE_PATH})
            swig_add_library(psmove LANGUAGE python SOURCES ${CMAKE_CURRENT_LIST_DIR}/psmove.i)
            swig_link_libraries(psmove psmoveapi ${PYTHON_LIBRARIES})
            if (PSMOVE_BUILD_TRACKER)
                swig_link_libraries(psmove psmoveapi_tracker)
            endif()
            set(INFO_BUILD_PYTHON_BINDINGS "Yes (version ${PYTHONLIBS_VERSION_STRING})")
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
            swig_add_library(psmove_java LANGUAGE java SOURCES ${CMAKE_CURRENT_LIST_DIR}/psmove.i)
            swig_link_libraries(psmove_java psmoveapi ${JNI_LIBRARIES})
            if (PSMOVE_BUILD_TRACKER)
                swig_link_libraries(psmove_java psmoveapi_tracker)
            endif()
            set(INFO_BUILD_JAVA_BINDINGS "Yes")

            # NOTE: We need a platform-independent copy command for the .java files, but neither the COMMAND
            #       nor CMake's "-E copy" support wildcards on Windows. We could probably whip up something
            #       more elegant which only makes the relevant line platform-dependent without copying all
            #       the other lines. But this simple branching is pretty robust and also works with older
            #       versions of CMake.
            if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
                add_custom_target(psmoveapi.jar
                    COMMAND ${CMAKE_COMMAND} -E make_directory ${PSMOVEAPI_JAVA_PKG_DIR}
                    COMMAND ${CMAKE_CURRENT_LIST_DIR}/copy_java.bat ${PSMOVEAPI_JAVA_PKG_DIR}/
                    COMMAND javac ${PSMOVEAPI_JAVA_PKG_DIR}/*.java
                    COMMAND jar cf psmoveapi.jar ${PSMOVEAPI_JAVA_PKG_DIR}/*.class
                    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                    COMMENT "Java bindings JAR archive"
                )
            else()
                add_custom_target(psmoveapi.jar
                    COMMAND ${CMAKE_COMMAND} -E make_directory ${PSMOVEAPI_JAVA_PKG_DIR}
                    COMMAND cp *.java ${PSMOVEAPI_JAVA_PKG_DIR}/
                    COMMAND javac ${PSMOVEAPI_JAVA_PKG_DIR}/*.java
                    COMMAND jar cf psmoveapi.jar ${PSMOVEAPI_JAVA_PKG_DIR}/*.class
                    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                    COMMENT "Java bindings JAR archive"
                )
            endif()

            set(PSMOVEAPI_PROCESSING_DIR "${CMAKE_CURRENT_LIST_DIR}/processing/libraries/psmove")

            IF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
                set(PSMOVEAPI_LIB_PLATFORM "macosx")
                set(PSMOVEAPI_NATIVE_LIB "libpsmove_java.jnilib")
                set(PSMOVEAPI_PAIRING_TOOL_NAME "psmove")
            ELSEIF(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
                set(PSMOVEAPI_LIB_PLATFORM "windows")
                set(PSMOVEAPI_NATIVE_LIB "psmove_java.dll")
                set(PSMOVEAPI_PAIRING_TOOL_NAME "psmove.exe")

                if(MINGW)
                    set(PSMOVEAPI_NATIVE_LIB "libpsmove_java.dll")
                endif()
            ELSE() # Linux
                set(PSMOVEAPI_LIB_PLATFORM "linux")
                set(PSMOVEAPI_NATIVE_LIB "libpsmove_java.so")
                set(PSMOVEAPI_PAIRING_TOOL_NAME "psmove")
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
                    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/${PSMOVEAPI_PAIRING_TOOL_NAME} ${PSMOVEAPI_PROCESSING_DIR}/tools/${PSMOVEAPI_LIB_PLATFORM}/${PSMOVEAPI_PAIRING_TOOL_NAME}
                    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/examples/processing ${PSMOVEAPI_PROCESSING_DIR}/examples/

                    # Cross-platform zipping :)
                    COMMAND jar cfM psmove_processing_${PSMOVEAPI_LIB_PLATFORM}.zip ${CMAKE_CURRENT_LIST_DIR}/processing

                    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                    COMMENT "Building and packaging Processing library"
                )

                # Ensure building targets in the correct order
                # (native lib->java lib->processing lib)
                add_dependencies(psmoveapi.jar psmove_java)
                add_dependencies(processing_lib psmoveapi.jar)
                add_dependencies(processing_lib psmove)

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
        swig_add_library(psmoveapi_csharp LANGUAGE csharp SOURCES ${CMAKE_CURRENT_LIST_DIR}/psmove.i)
        swig_link_libraries(psmoveapi_csharp psmoveapi)
        if (PSMOVE_BUILD_TRACKER)
            swig_link_libraries(psmoveapi_csharp psmoveapi_tracker)
        endif()
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

message("")
message("  Language bindings")
message("    Python:           " ${INFO_BUILD_PYTHON_BINDINGS})
message("    Java:             " ${INFO_BUILD_JAVA_BINDINGS})
message("    C#:               " ${INFO_BUILD_CSHARP_BINDINGS})
message("    Processing:       " ${INFO_BUILD_PROCESSING_BINDINGS})
message("")
