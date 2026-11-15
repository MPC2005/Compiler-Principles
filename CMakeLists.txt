cmake_minimum_required(VERSION 3.16)
project(toyc_cpp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(compiler src/Parser.cpp)
target_compile_options(compiler PRIVATE -Wall -Wextra -Wpedantic)
