cmake_minimum_required(VERSION 3.30)
project(graphite VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(cmake/CPM.cmake)
include(cmake/warnings.cmake)

add_subdirectory(src)