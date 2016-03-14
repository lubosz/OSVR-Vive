#
# HTC Vive plugin for OSVR
#
cmake_minimum_required(VERSION 2.8.12)
project(com_osvr_Vive)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(osvr REQUIRED)
find_package(OpenVR REQUIRED)

osvr_convert_json(com_osvr_Vive_json
    com_osvr_Vive.json
    "${CMAKE_CURRENT_BINARY_DIR}/com_osvr_Vive_json.h")

include_directories(${CMAKE_CURRENT_BINARY_DIR}
                    ${OPENVR_INCLUDE_DIR})

osvr_add_plugin(com_osvr_Vive
    com_osvr_Vive.cpp
    "${CMAKE_CURRENT_BINARY_DIR}/com_osvr_Vive_json.h")

target_link_libraries(com_osvr_Vive
    osvr::osvrPluginKitCpp)

install(FILES
    osvr_server_config.vive.sample.json
    README.md
    LICENSE
    DESTINATION .)