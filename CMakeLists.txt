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

# Link LLVM libraries using llvm_config macro
llvm_config(kaleidoscope USE_SHARED
    core
    orcjit orcshared orctargetprocess
    executionengine jitlink object runtimedyld
    passes support targetparser native
)
