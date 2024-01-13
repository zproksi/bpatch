cmake_minimum_required(VERSION 3.19)
set(pname bpatch)
project(${pname})

##get google test
include(FetchContent)
FetchContent_Declare(googletest
    GIT_REPOSITORY https://github.com/google/googletest
    GIT_TAG release-1.12.1)
FetchContent_GetProperties(googletest)
#googletest_POPULATED
#googletest_SOURCE_DIR
#googletest_BUILD_DIR
if(NOT googletest_POPULATED)
    FetchContent_Populate(googletest)
    add_subdirectory(${googletest_SOURCE_DIR} ${googletest_BUILD_DIR})
endif()

# Set C++ standard to 20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Set the build type to Debug if not explicitly specified
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

message(STATUS "Initial COMPILER_FLAGS:" ${CMAKE_CXX_FLAGS})

#CMAKE_CXX_FLAGS_DEBUG
#CMAKE_CXX_FLAGS_RELEASE
#CMAKE_CXX_FLAGS_RELWITHDEBINFO
#CMAKE_CXX_FLAGS_MINSIZEREL

set(CMAKE_CXX_FLAGS "")
message(STATUS "CMAKE_CXX_FLAGS:" ${CMAKE_CXX_FLAGS})

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    # GCC (GNU Compiler)
    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DDEBUG -D_DEBUG -D_CONSOLE -Wno-unknown-pragmas -Wall -Werror -fno-rtti -pthread")
    elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}         -DNDEBUG -D_CONSOLE -Wno-unknown-pragmas -Wall -Werror -fno-rtti -pthread")
    else()
        message(FATAL_ERROR "Unsupported compilation mode")
    endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    # MSVC (Microsoft Visual C++)
    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /DWIN32 /D_WINDOWS /D_CONSOLE /DDEBUG /D_DEBUG /EHsc /W4 /WX /GR- /MDd")
    elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /DWIN32 /D_WINDOWS /D_CONSOLE         /DNDEBUG /EHsc /W4 /WX /GR- /MD")
    else()
        message(FATAL_ERROR "Unsupported compilation mode")
    endif()
else()
    message(FATAL_ERROR "Unsupported compiler")
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
endif()

message(STATUS "CMAKE_CXX_FLAGS:" ${CMAKE_CXX_FLAGS})

# Set Release and Debug configurations
set(CMAKE_CONFIGURATION_TYPES "Release;Debug" CACHE STRING "Configurations" FORCE)

include_directories(src${pname})

# Source files
set(SOURCE_FILES
    bpatch.cpp
)
#set(HEADER_FILES
#)

# Define the executable target
add_executable(${pname} ${SOURCE_FILES})

# Set the name of the executable based on the platform
if(WIN32)
    set_target_properties(${pname} PROPERTIES OUTPUT_NAME "${pname}")
else()
    set_target_properties(${pname} PROPERTIES OUTPUT_NAME "${pname}")
endif()

# Configuration-specific settings
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    # For GCC and Clang, use the -m64 flag to target 64-bit
set_target_properties(${pname} PROPERTIES
    COMPILE_OPTIONS "$<$<CONFIG:Release>:-O3>"
    COMPILE_OPTIONS "$<$<CONFIG:Debug>:-O0>"
)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    # For MSVC, set the generator platform to x64
set_target_properties(${pname} PROPERTIES
    COMPILE_OPTIONS "$<$<CONFIG:Release>:/Ox>"
    COMPILE_OPTIONS "$<$<CONFIG:Debug>:/Od>"
)


else()
    message(FATAL_ERROR "Unsupported compiler")

endif()

add_subdirectory(src${pname})
add_subdirectory(test${pname})

target_link_libraries(${pname} src${pname})

# Console commands for building on Windows with Visual Studio
# !!!!!!!!!!!!!
#    Use these commands in the Visual Studio Command Prompt
# !!!!!!!!!!!!!
# Navigate to the directory containing CMakeLists.txt
#   create folder 'build' and enter it
#  For visual studio 2022 option for cmake is "Visual Studio 17 2022"
#       for 32 bits compilation use:
#  cmake -G "Visual Studio 17 2022" -A Win32 ..
#       for 64 bits compilation use:
#  cmake -G "Visual Studio 17 2022" -A x64 ..
#       and execute to build
#    Release:
# cmake -DCMAKE_BUILD_TYPE=Release ..
# cmake --build . --config Release
#    Debug:
# cmake -DCMAKE_BUILD_TYPE=Debug ..
# cmake --build . --config Debug
#

# Console commands for building on Linux with GCC
# Use these commands in the terminal
# Navigate to the directory containing CMakeLists.txt
# mkdir build
# cd build
#   to build Release
# cmake -DCMAKE_BUILD_TYPE=Release ..
# cmake --build . --config Release
#   to build Debug
# cmake -DCMAKE_BUILD_TYPE=Debug ..
# cmake --build . --config Debug
#
# as a result folder should contain version of the application
# to generate 32 bits version of linux application add -m32 flag
# to CMAKE_CXX_FLAGS above in "GNU|Clang" section
