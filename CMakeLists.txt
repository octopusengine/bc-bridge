cmake_minimum_required (VERSION 2.6)
set(CMAKE_GENERATOR "Unix Makefiles")
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)
set(CMAKE_DISABLE_SOURCE_CHANGES  ON)

project (Bridge)

string(TIMESTAMP VERSION "%Y-%m-%dT%H:%M:%S")
add_definitions(-DVERSION="${VERSION}")

include_directories ("${PROJECT_SOURCE_DIR}/jsmn")
add_subdirectory (jsmn)
set (EXTRA_LIBS ${EXTRA_LIBS} jsmn)

include_directories ("${PROJECT_SOURCE_DIR}/ft260")
add_subdirectory (ft260)
set (EXTRA_LIBS ${EXTRA_LIBS} ft260 -ludev)

include_directories ("${PROJECT_SOURCE_DIR}/bcl/inc")
add_subdirectory (bcl)
set (EXTRA_LIBS ${EXTRA_LIBS} bcl)

include_directories("${PROJECT_BINARY_DIR}")
add_executable(Bridge "${PROJECT_BINARY_DIR}/bridge.c")
target_link_libraries (${PROJECT_NAME}  ${EXTRA_LIBS})


