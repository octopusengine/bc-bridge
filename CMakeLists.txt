cmake_minimum_required (VERSION 2.6)
set(CMAKE_GENERATOR "Unix Makefiles")

project (Bridge)
set (Bridge_VERSION_MAJOR 0)
set (Bridge_VERSION_MINOR 1)
configure_file (
  "${PROJECT_SOURCE_DIR}/bridge/bridge.h.in"
  "${PROJECT_BINARY_DIR}/bridge/bridge.h"
  )

include_directories ("${PROJECT_SOURCE_DIR}/jsmn")
add_subdirectory (jsmn)
set (EXTRA_LIBS ${EXTRA_LIBS} jsmn)

include_directories("${PROJECT_BINARY_DIR}/bridge")
add_executable(Bridge "${PROJECT_BINARY_DIR}/bridge/bridge.c")
target_link_libraries (Bridge  ${EXTRA_LIBS})