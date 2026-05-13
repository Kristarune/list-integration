cmake_minimum_required(VERSION 3.21)

project(ListsIntegrations VERSION 1.0.0 LANGUAGES CXX)

# Force C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Mac architecture
if(APPLE)
    set(CMAKE_OSX_ARCHITECTURES "x86_64")
endif()

# Find Geode SDK
if(NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "Unable to find Geode SDK! Please define GEODE_SDK environment variable.")
endif()

message(STATUS "Found Geode SDK: $ENV{GEODE_SDK}")

# Load Geode
add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)

# Source files
file(GLOB_RECURSE SOURCES
    src/*.cpp
)

# Build mod
add_library(${PROJECT_NAME} SHARED ${SOURCES})

# Includes
target_include_directories(${PROJECT_NAME} PRIVATE
    src
    include
)

# FORCE C++20 ON TARGET
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)

# Windows fix for fmt/consteval issue
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /Zc:__cplusplus)
endif()

# Setup Geode
setup_geode_mod(${PROJECT_NAME})
