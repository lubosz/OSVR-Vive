#
# HTC Vive plugin for OSVR
#
cmake_minimum_required(VERSION 3.1.0)
project(com_osvr_Vive)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(WIN32 OR ANDROID)
    option(Boost_USE_STATIC_LIBS "Build with Boost's static libraries?" ON)
endif()
find_package(osvr REQUIRED)
find_package(JsonCpp REQUIRED)
find_package(Boost)

# Interface target for the openvr_driver.h header we'll use to interact with the target driver.
add_library(OpenVRDriver INTERFACE)
target_include_directories(OpenVRDriver INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/vendor/openvr/headers")

# Interface target for some sort of filesystem functionality
if(MSVC AND NOT MSVC_VERSION LESS 1800)
    # We can use the experimental filesystem header here.
    add_library(filesystem_lib INTERFACE)
    target_compile_definitions(filesystem_lib INTERFACE OSVR_USING_FILESYSTEM_TR2)
else()
    # Interface target for boost fs
    find_library(Boost REQUIRED COMPONENTS filesystem system)
    add_library(filesystem_lib INTERFACE)
    target_link_libraries(filesystem_lib
        INTERFACE
        ${Boost_FILESYSTEM_LIBRARIES}
        ${Boost_SYSTEM_LIBRARIES})
    target_include_directories(filesystem_lib INTERFACE ${Boost_INCLUDE_DIRS})
    target_compile_definitions(filesystem_lib INTERFACE BOOST_FILESYSTEM_VERSION=3 OSVR_USING_BOOST_FILESYSTEM)
endif()

# Build the plugin

osvr_convert_json(com_osvr_Vive_json
    com_osvr_Vive.json
    "${CMAKE_CURRENT_BINARY_DIR}/com_osvr_Vive_json.h")
osvr_add_plugin(com_osvr_Vive
    CPP
    com_osvr_Vive.cpp
    "${CMAKE_CURRENT_BINARY_DIR}/com_osvr_Vive_json.h")

target_link_libraries(com_osvr_Vive OpenVRDriver)

# Build the executable
add_executable(ViveLoader
    DriverLoader.cpp
    DriverLoader.h
    DriverProviderOwner.h
    FindDriver.cpp
    FindDriver.h
    InterfaceTraits.h
    SearchPathExtender.h
    ViveLoader.cpp)

target_link_libraries(ViveLoader PRIVATE OpenVRDriver osvr::osvrUtil JsonCpp::JsonCpp filesystem_lib)
target_include_directories(ViveLoader PRIVATE ${Boost_INCLUDE_DIRS})

# Copy dlls
include(CopyImportedTarget)
copy_imported_targets(ViveLoader osvr::osvrUtil)

# Install files for the plugin.
install(FILES
    osvr_server_config.vive.sample.json
    README.md
    LICENSE
    DESTINATION .)
