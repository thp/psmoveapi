cmake_minimum_required(VERSION 2.8.12)
set(ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/..)
include(${ROOT_DIR}/cmake/common.cmake)

# Additional applications that can be used to test the library

option(PSMOVE_BUILD_EXAMPLES "Build the C examples" ON)
option(PSMOVE_BUILD_OPENGL_EXAMPLES "Build the OpenGL examples" ON)
option(PSMOVE_BUILD_TESTS "Build the C tests" ON)
option(PSMOVE_BUILD_TUIO_SERVER "Build the TUIO server" ON)

include_directories(${MSVC_INCLUDES})
# C examples
if(PSMOVE_BUILD_EXAMPLES)
    foreach(EXAMPLE example multiple dump_calibration battery_check)
        add_executable(${EXAMPLE} ${CMAKE_CURRENT_LIST_DIR}/c/${EXAMPLE}.c ${MSVC_SRCS})
        target_link_libraries(${EXAMPLE} psmoveapi)
		set_property(TARGET ${EXAMPLE} PROPERTY FOLDER "Examples") 
    endforeach()

    if(PSMOVE_BUILD_TRACKER AND PSMOVE_BUILD_TUIO_SERVER)
        include_directories(${ROOT_DIR}/external/TUIO_CPP/TUIO)
        include_directories(${ROOT_DIR}/external/TUIO_CPP/oscpack)
        add_executable(tuio_server ${CMAKE_CURRENT_LIST_DIR}/c/tuio_server.cpp
            external/TUIO_CPP/TUIO/TuioClient.cpp
            external/TUIO_CPP/TUIO/TuioServer.cpp
            external/TUIO_CPP/TUIO/TuioTime.cpp
            external/TUIO_CPP/oscpack/osc/OscTypes.cpp
            external/TUIO_CPP/oscpack/osc/OscOutboundPacketStream.cpp
            external/TUIO_CPP/oscpack/osc/OscReceivedElements.cpp
            external/TUIO_CPP/oscpack/osc/OscPrintReceivedElements.cpp
            external/TUIO_CPP/oscpack/ip/posix/NetworkingUtils.cpp
            external/TUIO_CPP/oscpack/ip/posix/UdpSocket.cpp
            external/TUIO_CPP/oscpack/ip/win32/NetworkingUtils.cpp
            external/TUIO_CPP/oscpack/ip/win32/UdpSocket.cpp
            ${MSVC_SRCS})
        set_target_properties(tuio_server PROPERTIES
            COMPILE_FLAGS -DOSC_HOST_LITTLE_ENDIAN)
	
	# MingW does not define WIN32 but tuio sources rely on it being there
	IF(${CMAKE_SYSTEM_NAME} MATCHES "Windows" AND MINGW)
		set_target_properties(tuio_server PROPERTIES
		    COMPILE_FLAGS -DWIN32)
	ENDIF()

        target_link_libraries(tuio_server psmoveapi psmoveapi_tracker ${MSVC_LIBS})
		set_property(TARGET tuio_server PROPERTY FOLDER "Utilities") 
    else()
        # Disable the TUIO Server if we don't build the tracker
        set(PSMOVE_BUILD_TUIO_SERVER OFF)
    endif()

    if(PSMOVE_BUILD_TRACKER)
	# C examples
        foreach(EXAMPLE distance_calibration)
            add_executable(${EXAMPLE} ${CMAKE_CURRENT_LIST_DIR}/c/${EXAMPLE}.c ${MSVC_SRCS})
            target_link_libraries(${EXAMPLE} psmoveapi psmoveapi_tracker ${MSVC_LIBS})
			set_property(TARGET ${EXAMPLE} PROPERTY FOLDER "Utilities") 
        endforeach()

	# C++ examples
        foreach(EXAMPLE tracker_camera_calibration)
            add_executable(${EXAMPLE} ${CMAKE_CURRENT_LIST_DIR}/c/${EXAMPLE}.cpp ${MSVC_SRCS})
            target_link_libraries(${EXAMPLE} psmoveapi psmoveapi_tracker ${MSVC_LIBS})
			set_property(TARGET ${EXAMPLE} PROPERTY FOLDER "Utilities") 
        endforeach()
    endif()
endif()

# C test programs
if(PSMOVE_BUILD_TESTS)
    foreach(TESTNAME led_update read_performance calibration responsiveness extension led_pwm_frequency)
        add_executable(test_${TESTNAME} ${CMAKE_CURRENT_LIST_DIR}/c/test_${TESTNAME}.c ${MSVC_SRCS})
        target_link_libraries(test_${TESTNAME} psmoveapi ${MSVC_LIBS})
		set_property(TARGET test_${TESTNAME} PROPERTY FOLDER "Examples") 
    endforeach(TESTNAME)

    if(PSMOVE_BUILD_TRACKER)
        foreach(TESTNAME capture_performance record_video roi_size end2end_latency)
            add_executable(test_${TESTNAME} ${CMAKE_CURRENT_LIST_DIR}/c/test_${TESTNAME}.c ${MSVC_SRCS})
            target_link_libraries(test_${TESTNAME} psmoveapi psmoveapi_tracker ${MSVC_LIBS})
			set_property(TARGET test_${TESTNAME} PROPERTY FOLDER "Examples") 
        endforeach(TESTNAME)

        foreach(TESTNAME tracker )
            add_executable(test_${TESTNAME} ${CMAKE_CURRENT_LIST_DIR}/c/test_${TESTNAME}.c ${MSVC_SRCS})
            target_link_libraries(test_${TESTNAME} psmoveapi psmoveapi_tracker ${MSVC_LIBS})
			set_property(TARGET test_${TESTNAME} PROPERTY FOLDER "Utilities") 
        endforeach(TESTNAME)
		
        if (PSMOVE_BUILD_OPENGL_EXAMPLES)
            set(OPENGL_EXAMPLES_LIBRARIES "")

            if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
		SET(ENV{SDL2DIR} ${SDL2DIR})
                find_library(OPENGL_FRAMEWORK OpenGL)
                find_library(GLUT_FRAMEWORK GLUT)
		FIND_PACKAGE(SDL2 QUIET)
		include_directories(${SDL2_INCLUDE_DIR})
                list(APPEND OPENGL_EXAMPLES_LIBRARIES ${SDL2_LIBRARY}
                    ${OPENGL_FRAMEWORK} ${GLUT_FRAMEWORK})
            ELSEIF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
                find_package(SDL2 QUIET)
                include_directories(${SDL2_INCLUDE_DIR})
                list(APPEND OPENGL_EXAMPLES_LIBRARIES ${SDL2_LIBRARY}
                    GL GLU glut)
            ELSEIF(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
               	SET(ENV{SDL2DIR} ${SDL2DIR})
                FIND_PACKAGE(SDL2 QUIET)
                include_directories(${SDL2_INCLUDE_DIR})
                list(APPEND OPENGL_EXAMPLES_LIBRARIES ${SDL2_LIBRARY})
                list(APPEND OPENGL_EXAMPLES_LIBRARIES glu32 opengl32 winmm)
                list(GET SDL2_LIBRARY 1 FIRST_SDL2_LIB_RELEASE)
				list(GET SDL2_LIBRARY 5 FIRST_SDL2_LIB_DEBUG)
                get_filename_component(SDL_BIN_PATH_RELEASE ${FIRST_SDL2_LIB_RELEASE} DIRECTORY)
				get_filename_component(SDL_BIN_PATH_DEBUG ${FIRST_SDL2_LIB_DEBUG} DIRECTORY)
            ENDIF()
			
            foreach(EXAMPLE test_opengl test_opengl2 test_opengl3)
                add_executable(${EXAMPLE} ${CMAKE_CURRENT_LIST_DIR}/c/${EXAMPLE}.cpp ${MSVC_SRCS})
                target_link_libraries(${EXAMPLE}
                    psmoveapi
                    psmoveapi_tracker
                    ${OPENGL_EXAMPLES_LIBRARIES}
                    ${MSVC_LIBS})
				IF(${CMAKE_SYSTEM_NAME} MATCHES "Windows" AND NOT MINGW)
                    add_custom_command(TARGET ${EXAMPLE} POST_BUILD
                        COMMAND ${CMAKE_COMMAND} -E copy_if_different
							"$<$<CONFIG:debug>:${SDL_BIN_PATH_DEBUG}/SDL2.dll>$<$<CONFIG:release>:${SDL_BIN_PATH_RELEASE}/SDL2.dll>"
                            "$<TARGET_FILE_DIR:${EXAMPLE}>")
                ENDIF()					
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
feature_use_info("C++ TUIO server:  " PSMOVE_BUILD_TUIO_SERVER)
