cmake_minimum_required(VERSION 2.8.12)
set(ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/../..)
include(${ROOT_DIR}/cmake/common.cmake)

include_directories(${ROOT_DIR}/src)

# New command line interface
add_executable(psmove ${CMAKE_CURRENT_LIST_DIR}/psmovecli.cpp)
target_link_libraries(psmove psmoveapi)
set_property(TARGET psmove PROPERTY FOLDER "Utilities")
list(APPEND PSMOVEAPI_INSTALL_TARGETS psmove)

option(PSMOVE_USE_SIXPAIR "Enable Navigation Controller pairing" ON)

if (WIN32)
  set(PSMOVE_USE_SIXPAIR OFF CACHE BOOL "Disabling Navigation Controller pairing on Windows" FORCE)
else()
  # sixpair needs libusb
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(LIBUSB REQUIRED libusb)
  include_directories(${LIBUSB_INCLUDE_DIRS})
  target_link_libraries(psmove ${LIBUSB_LIBRARIES})
endif()
