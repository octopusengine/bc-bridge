cmake_minimum_required(VERSION 2.8)
project(bc-bridge C)

# set(CMAKE_VERBOSE_MAKEFILE on)
set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} "-Wall -pedantic -std=c99")

string(TIMESTAMP VERSION "%Y-%m-%dT%H:%M:%S")
add_definitions(-DVERSION="${VERSION}")
add_definitions(-DBRIDGE)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

find_package(Threads REQUIRED)
add_definitions(-DTHREADSAFE -D_REENTRANT)
find_package(Libudev REQUIRED)

include_directories(bcl/inc)
include_directories(ft260)
include_directories(jsmn)

set(SOURCE_FILES
        main.c
        ft260/ft260.c
        jsmn/jsmn.c
        tags.c
        bcl/src/bc/os.c
        bcl/src/bc/tag.c
        bcl/src/bc/tick.c
        bcl/src/bc/i2c/app.c
        bcl/src/bc/i2c/sys.c
        bcl/src/bc/i2c/tca9534a.c
        bcl/src/bc/i2c/sc16is740.c
        bcl/src/bc/module/co2.c
        bcl/src/bc/module/relay.c
        #bcl/src/bc/tag/accelerometer.c
        bcl/src/bc/tag/barometer.c
        bcl/src/bc/tag/humidity.c
        bcl/src/bc/tag/lux_meter.c
        bcl/src/bc/tag/temperature.c
         )

add_executable(${PROJECT_NAME} ${SOURCE_FILES})
target_link_libraries (${PROJECT_NAME} ${CMAKE_THREAD_LIBS_INIT} ${LIBUDEV_LIBRARIES})
