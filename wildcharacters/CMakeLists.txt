cmake_minimum_required(VERSION 3.19)
set(pname wildcharacters)
project(${pname})

# Set Release and Debug configurations
set(CMAKE_CONFIGURATION_TYPES "Release;Debug" CACHE STRING "Configurations" FORCE)

# Source files
set(SOURCE_FILES
    wildcharacters.cpp
)
set(HEADER_FILES
    wildcharacters.h
)

# Define the executable target
add_library(${PROJECT_NAME} ${SOURCE_FILES} ${HEADER_FILES})
