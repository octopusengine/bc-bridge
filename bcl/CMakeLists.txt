cmake_minimum_required (VERSION 2.6)
project(bcl)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(LIBRARY_NAME ${PROJECT_NAME})

include_directories ("${PROJECT_SOURCE_DIR}/inc")

FILE(GLOB_RECURSE SOURCES *.c)
#message("${SOURCES}")
#list(REMOVE_ITEM SOURCES "${PROJECT_SOURCE_DIR}/src/bc/module/co2.c")


add_library(${LIBRARY_NAME} STATIC ${SOURCES} )