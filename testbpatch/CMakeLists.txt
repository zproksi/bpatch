cmake_minimum_required(VERSION 3.19)
set(pname bpatch)
project(test${pname})

# Set the default build type to Debug
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()


# Source files
set(SOURCE_FILES
    pch.cpp
    test.cpp
)
set(HEADER_FILES
    pch.h
)

include_directories(../src${pname})

# Define the executable target
add_executable(${PROJECT_NAME} ${SOURCE_FILES} ${HEADER_FILES})

# Set the name of the executable based on the platform
if(WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "${PROJECT_NAME}")
else()
    set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "${PROJECT_NAME}")
endif()

# Precompiled header
target_precompile_headers(${PROJECT_NAME} PRIVATE pch.h)

target_link_libraries(${PROJECT_NAME} PRIVATE src${pname} gtest_main gmock_main)

