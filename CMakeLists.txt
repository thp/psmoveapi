cmake_minimum_required(VERSION 2.8.12)

project(PSMoveAPI LANGUAGES C CXX)

# Step into the subdirectories
include("src/CMakeLists.txt") #add_subdirectory(src ${CMAKE_BINARY_DIR})
include("examples/CMakeLists.txt") #add_subdirectory(examples)
include("bindings/CMakeLists.txt") #add_subdirectory(bindings)
