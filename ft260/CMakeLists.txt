cmake_minimum_required (VERSION 2.6)
project(ft260)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(LIBRARY_NAME ${PROJECT_NAME})

set(JSMN_SOURCES ft260.c)
set(JSMN_PUBLIC_INCLUDES ft260.h)

add_library(${LIBRARY_NAME} STATIC ${JSMN_SOURCES} )