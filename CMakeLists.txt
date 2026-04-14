cmake_minimum_required(VERSION 3.13)
project(kaleidoscope)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(GLOB SOURCES src/*.cpp)

# Find LLVM
find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")

# Set up LLVM
add_definitions(${LLVM_DEFINITIONS})
include_directories(${LLVM_INCLUDE_DIRS} include src)

# Create executable
add_executable(kaleidoscope ${SOURCES})

# Link LLVM libraries
llvm_map_components_to_libnames(llvm_libs core)
target_link_libraries(kaleidoscope ${llvm_libs})
