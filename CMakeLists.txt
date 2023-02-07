cmake_minimum_required(VERSION 3.22)
project(cbounds)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_compile_options(-fno-rtti)

set(LLVM_DIR vendor/llvm-install/lib/cmake/llvm)
find_package(LLVM REQUIRED CONFIG NO_DEFAULT_PATH)
message(${LLVM_INCLUDE_DIR})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})
include_directories(SYSTEM ${LLVM_INCLUDE_DIRS})
list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")
include(AddLLVM)

set(Clang_DIR vendor/llvm-install/lib/cmake/clang)
find_package(Clang REQUIRED CONFIG NO_DEFAULT_PATH)
include_directories(SYSTEM ${CLANG_INCLUDE_DIRS})

add_subdirectory(src bin)

