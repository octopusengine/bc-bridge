cmake_minimum_required (VERSION 2.6)
project(jsmn)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(LIBRARY_NAME ${PROJECT_NAME})

set(JSMN_SOURCES jsmn.c)
set(JSMN_PUBLIC_INCLUDES jsmn.h)

add_library(${LIBRARY_NAME} STATIC ${JSMN_SOURCES})