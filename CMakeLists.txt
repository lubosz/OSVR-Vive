
cmake_minimum_required(VERSION 3.1.0)
project(ViveLoader)


list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(osvr REQUIRED)

find_package(JsonCpp REQUIRED)
