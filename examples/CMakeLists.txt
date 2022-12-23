cmake_minimum_required(VERSION 2.8.12)
set(ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/..)
include(${ROOT_DIR}/cmake/common.cmake)

# Additional applications that can be used to test the library

option(PSMOVE_BUILD_EXAMPLES "Build the C examples" ON)
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
        foreach(TESTNAME tracker camera)
            add_executable(test_${TESTNAME} ${CMAKE_CURRENT_LIST_DIR}/c/test_${TESTNAME}.cpp)
            target_link_libraries(test_${TESTNAME} psmoveapi psmoveapi_tracker)
			set_property(TARGET test_${TESTNAME} PROPERTY FOLDER "Utilities") 
        endforeach(TESTNAME)
    endif()
endif()

message("")
message("  Additional targets")
feature_use_info("C example apps:   " PSMOVE_BUILD_EXAMPLES)
feature_use_info("C test programs:  " PSMOVE_BUILD_TESTS)
message("    NavCon test:      " ${INFO_BUILD_NAVCON_TEST})
