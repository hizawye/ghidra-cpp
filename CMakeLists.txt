cmake_minimum_required(VERSION 3.20)
project(ghirda LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(GHIRDA_BUILD_GUI "Build GUI application" ON)

add_subdirectory(src)
add_subdirectory(apps)
