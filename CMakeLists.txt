cmake_minimum_required(VERSION 2.8)
project(bc-bridge C)

# set(CMAKE_VERBOSE_MAKEFILE on)
set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} "-Wall -pedantic -std=c99")

string(TIMESTAMP VERSION "%Y-%m-%dT%H:%M:%S")
add_definitions(-DVERSION="${VERSION}")
add_definitions(-DBRIDGE)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

find_package(Threads REQUIRED)
add_definitions(-D_BSD_SOURCE -DTHREADSAFE -D_REENTRANT)
find_package(Libudev REQUIRED)

include_directories(bcl/inc)
include_directories(ft260)
include_directories(jsmn)

set(SOURCE_FILES

        src/bc_bridge.c
        src/bc_i2c_app.c
        src/bc_i2c_sys.c
        src/bc_module_co2.c
        src/bc_module_relay.c
        src/bc_os.c
        #src/bc_tag_accelerometer.c
        src/bc_tag_barometer.c
        src/bc_tag.c
        src/bc_tag_humidity.c
        src/bc_tag_lux_meter.c
        src/bc_tag_temperature.c
        src/bc_tick.c
        src/main.c
        src/bc_i2c_sc16is740.c
        src/tags.c
        src/bc_i2c_tca9534a.c

        jsmn/jsmn.c
)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})
target_link_libraries (${PROJECT_NAME} ${CMAKE_THREAD_LIBS_INIT} ${LIBUDEV_LIBRARIES})
