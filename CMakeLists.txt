cmake_minimum_required(VERSION 3.14)
project(mem-alloc LANGUAGES C)

include_directories(${CMAKE_SOURCE_DIR}/include)

file(GLOB SOURCES "src/*.c")

add_executable(mem-alloc ${SOURCES})
