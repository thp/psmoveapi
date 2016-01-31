cmake_minimum_required(VERSION 2.8.12)

project(PSMoveAPI LANGUAGES C CXX)

# Step into the subdirectories
include("src/CMakeLists.txt")
include("examples/CMakeLists.txt")
include("bindings/CMakeLists.txt")
