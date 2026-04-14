cmake_minimum_required(VERSION 3.13)
project(kaleidoscope)

set(CMAKE_CXX_STANDARD 17)

# Find LLVM
find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")

# Set up LLVM
add_definitions(${LLVM_DEFINITIONS})
include_directories(${LLVM_INCLUDE_DIRS})

# Create executable
add_executable(kaleidoscope src/main.cpp)

# Link LLVM libraries
llvm_map_components_to_libnames(llvm_libs core)
target_link_libraries(kaleidoscope ${llvm_libs})
