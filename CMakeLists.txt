cmake_minimum_required(VERSION 3.16)
project(Peril CXX)

set(CMAKE_BUILD_TYPE Debug)

set(CMAKE_CXX_STANDARD 20)
include(FetchContent)
include(cmake/FetchLagom.cmake)

add_compile_options(-Werror=implicit-fallthrough)
add_compile_options(-Wno-literal-suffix)
add_compile_options(-fno-gnu-keywords)
add_subdirectory(LibJava)
