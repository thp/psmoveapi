cmake_minimum_required(VERSION 2.8.12)
set(ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/../..)
include(${ROOT_DIR}/cmake/common.cmake)

include_directories(${ROOT_DIR}/src)

# New command line interface
add_executable(psmove ${CMAKE_CURRENT_LIST_DIR}/psmovecli.cpp)
target_link_libraries(psmove psmoveapi_static)
set_target_properties(psmove PROPERTIES COMPILE_FLAGS -DUSING_STATIC_LIBRARY)
set_property(TARGET psmove PROPERTY FOLDER "Utilities")
list(APPEND PSMOVEAPI_INSTALL_TARGETS psmove)
