cmake_minimum_required(VERSION 3.19)
set(pname bpatch)
project(src${pname})

# Set Release and Debug configurations
set(CMAKE_CONFIGURATION_TYPES "Release;Debug" CACHE STRING "Configurations" FORCE)

# Source files
set(SOURCE_FILES
    actionscollection.cpp
    binarylexeme.cpp
    bpatchfolders.cpp
    coloredconsole.cpp
    consoleparametersreader.cpp
    dictionary.cpp
    dictionarykeywords.cpp
    fileprocessing.cpp
    flexiblecache.cpp
    jsonparser.cpp
    processing.cpp
    stdafx.cpp
    streamreplacer.cpp
    timemeasurer.cpp
)
set(HEADER_FILES
    actionscollection.h
    binarylexeme.h
    bpatchfolders.h
    coloredconsole.h
    consoleparametersreader.h
    dictionary.h
    dictionarykeywords.h
    fileprocessing.h
    flexiblecache.h
    jsonparser.h
    processing.h
    stdafx.h
    streamreplacer.h
    timemeasurer.h
)

# Define the executable target
add_library(${PROJECT_NAME} ${SOURCE_FILES} ${HEADER_FILES})

# Precompiled header
target_precompile_headers(${PROJECT_NAME} PRIVATE stdafx.h)
