cmake_minimum_required(VERSION 2.8.12)
set(ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/..)
include(${ROOT_DIR}/cmake/common.cmake)

# Additional applications that can be used to test the library

option(PSMOVE_BUILD_EXAMPLES "Build the C examples" ON)
option(PSMOVE_BUILD_OPENGL_EXAMPLES "Build the OpenGL examples" ON)
option(PSMOVE_BUILD_TESTS "Build the C tests" ON)
option(PSMOVE_BUILD_NAVCON_TEST "Build the navcon C test" ON)

# C examples
if(PSMOVE_BUILD_EXAMPLES)
    foreach(EXAMPLE example multiple)
        add_executable(${EXAMPLE} ${CMAKE_CURRENT_LIST_DIR}/c/${EXAMPLE}.c)
        target_link_libraries(${EXAMPLE} psmoveapi)
		set_property(TARGET ${EXAMPLE} PROPERTY FOLDER "Examples") 
    endforeach()

    foreach(EXAMPLE example_new_api)
        add_executable(${EXAMPLE} ${CMAKE_CURRENT_LIST_DIR}/c/${EXAMPLE}.cpp)
        target_link_libraries(${EXAMPLE} psmoveapi)
        set_property(TARGET ${EXAMPLE} PROPERTY FOLDER "Examples")
    endforeach()

    if(PSMOVE_BUILD_TRACKER)
        # Build GLFW as dependency
        set(GLFW_BUILD_DOCS OFF CACHE BOOL "")
        set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "")
        set(GLFW_BUILD_TESTS OFF CACHE BOOL "")
        add_subdirectory(${ROOT_DIR}/external/glfw EXCLUDE_FROM_ALL)

	# C++ examples
        foreach(EXAMPLE tracker_camera_calibration distance_calibration)
            add_executable(${EXAMPLE} ${CMAKE_CURRENT_LIST_DIR}/c/${EXAMPLE}.cpp)
            target_link_libraries(${EXAMPLE} psmoveapi psmoveapi_tracker)
			set_property(TARGET ${EXAMPLE} PROPERTY FOLDER "Utilities") 
        endforeach()
    endif()
endif()

if(PSMOVE_BUILD_NAVCON_TEST)
    find_package(SDL2 QUIET)

    if (SDL2_FOUND)
        include_directories(${SDL2_INCLUDE_DIRS})
        add_executable(test_navcon ${CMAKE_CURRENT_LIST_DIR}/c/test_navcon.cpp)
        target_link_libraries(test_navcon ${SDL2_LIBRARIES})
        set(INFO_BUILD_NAVCON_TEST "Yes (using SDL2 joystick API)")
    else()
        set(INFO_BUILD_NAVCON_TEST "No (SDL2 not found)")
        set(PSMOVE_BUILD_NAVCON_TEST OFF CACHE BOOL "Disabling NavCon test -- SDL2 not found" FORCE)
    endif()
else()
    set(INFO_BUILD_NAVCON_TEST "No (disabled)")
endif()

# C test programs
if(PSMOVE_BUILD_TESTS)
    if(PSMOVE_BUILD_TRACKER)
        foreach(TESTNAME tracker )
            add_executable(test_${TESTNAME} ${CMAKE_CURRENT_LIST_DIR}/c/test_${TESTNAME}.cpp)
            target_link_libraries(test_${TESTNAME} psmoveapi psmoveapi_tracker)
			set_property(TARGET test_${TESTNAME} PROPERTY FOLDER "Utilities") 
        endforeach(TESTNAME)
		
        if (PSMOVE_BUILD_OPENGL_EXAMPLES)
            set(OPENGL_EXAMPLES_LIBRARIES "")

            if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
                find_library(OPENGL_FRAMEWORK OpenGL)
                find_library(GLUT_FRAMEWORK GLUT)
                list(APPEND OPENGL_EXAMPLES_LIBRARIES ${OPENGL_FRAMEWORK} ${GLUT_FRAMEWORK})
            ELSEIF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
                list(APPEND OPENGL_EXAMPLES_LIBRARIES GL GLU glut)
            ELSEIF(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
                list(APPEND OPENGL_EXAMPLES_LIBRARIES glu32 opengl32 winmm)
            ENDIF()
			
            foreach(EXAMPLE test_opengl test_opengl2 test_opengl3)
                add_executable(${EXAMPLE} ${CMAKE_CURRENT_LIST_DIR}/c/${EXAMPLE}.cpp)
                target_link_libraries(${EXAMPLE}
                    psmoveapi
                    psmoveapi_tracker
                    glfw
                    ${OPENGL_EXAMPLES_LIBRARIES})
                set_property(TARGET ${EXAMPLE} PROPERTY FOLDER "Examples")
            endforeach()
        endif()
    endif()
endif()

message("")
message("  Additional targets")
feature_use_info("C example apps:   " PSMOVE_BUILD_EXAMPLES)
feature_use_info("OpenGL examples:  " PSMOVE_BUILD_OPENGL_EXAMPLES)
feature_use_info("C test programs:  " PSMOVE_BUILD_TESTS)
message("    NavCon test:      " ${INFO_BUILD_NAVCON_TEST})
