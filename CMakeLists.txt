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

osvr_add_plugin(com_osvr_Vive
    CPP
    com_osvr_Vive.cpp
    "${CMAKE_CURRENT_BINARY_DIR}/com_osvr_Vive_json.h")

# Build the project
add_executable(ViveLoader
    DriverLoader.cpp
    DriverLoader.h
    FindDriver.cpp
    FindDriver.h
    InterfaceTraits.h
    SearchPathExtender.h
    ViveLoader.cpp)
    
target_link_libraries(ViveLoader PRIVATE OpenVRDriver osvr::osvrUtil JsonCpp::JsonCpp)
target_include_directories(ViveLoader PRIVATE ${Boost_INCLUDE_DIRS})

if(MSVC AND NOT MSVC_VERSION LESS 1800)
	
    # We can use the experimental filesystem header here.
else()
    # Interface target for boost fs
    find_library(Boost REQUIRED COMPONENTS filesystem system)
    add_library(boost_filesystem INTERFACE)
    target_link_libraries(boost_filesystem
        INTERFACE
        ${Boost_FILESYSTEM_LIBRARIES}
        ${Boost_SYSTEM_LIBRARIES})
    target_include_directories(boost_filesystem INTERFACE ${Boost_INCLUDE_DIRS})
    target_compile_definitions(boost_filesystem INTERFACE BOOST_FILESYSTEM_VERSION=3)
    target_link_libraries(ViveLoader PRIVATE boost_filesystem)
endif()

# Copy dlls
include(CopyImportedTarget)
copy_imported_targets(ViveLoader osvr::osvrUtil)

install(FILES
    osvr_server_config.vive.sample.json
    README.md
    LICENSE
    DESTINATION .)