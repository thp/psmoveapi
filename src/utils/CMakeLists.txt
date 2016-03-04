cmake_minimum_required(VERSION 2.8.12)
set(ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/../..)
include(${ROOT_DIR}/cmake/common.cmake)

# Essential utilities
include_directories(${MSVC_INCLUDES})
include_directories(${ROOT_DIR}/src)
foreach(UTILITY moved psmovepair magnetometer_calibration psmove_dfu_mode psmove_auth_response psmove_get_firmware_info)
    add_executable(${UTILITY} ${CMAKE_CURRENT_LIST_DIR}/${UTILITY}.c ${MSVC_SRCS})
    target_link_libraries(${UTILITY} psmoveapi_static ${MSVC_LIBS})
    set_target_properties(${UTILITY} PROPERTIES
        COMPILE_FLAGS -DUSING_STATIC_LIBRARY)
	set_property(TARGET ${UTILITY} PROPERTY FOLDER "Utilities") 
    list(APPEND PSMOVEAPI_INSTALL_TARGETS ${UTILITY})
endforeach()
