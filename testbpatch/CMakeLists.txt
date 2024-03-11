cmake_minimum_required(VERSION 3.19)
set(pname bpatch)
project(test${pname})

# Set the default build type to Debug
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

include(FetchContent)
FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest
        GIT_TAG release-1.12.1)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Source files
set(SOURCE_FILES
    pch.cpp
    test.cpp
)
set(HEADER_FILES
    pch.h
)

include_directories(../src${pname})
include_directories(../wildcharacters)

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

target_link_libraries(${PROJECT_NAME} PRIVATE wildcharacters src${pname} gtest_main gmock_main)

###################################
##         for unit tests        ##
###################################
# Create the folder
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/fortesting)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/fortesting/k)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/fortesting/keb)
# List of file names
set(FILENAMES a ae aeb aebc a.txt ae.txt aeb.txt aebc.txt)

# Write each file with 'A' as its content
foreach(FILENAME IN LISTS FILENAMES)
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/fortesting/${FILENAME} "A")
endforeach()


