cmake_minimum_required(VERSION 2.8)
project(bc-bridge C)

# set(CMAKE_VERBOSE_MAKEFILE on)
set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} "-Wall -pedantic -std=c99 -D_POSIX_C_SOURCE=199309L")

string(TIMESTAMP VERSION "%Y-%m-%dT%H:%M:%S")
add_definitions(-DVERSION="${VERSION}")
add_definitions(-DBRIDGE)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

find_package(Threads REQUIRED)
add_definitions(-D_BSD_SOURCE -DTHREADSAFE -D_REENTRANT)
find_package(Libudev REQUIRED)
find_package(argp REQUIRED)

include_directories(src)
include_directories(jsmn)

set(SOURCE_FILES
        jsmn/jsmn.c
        src/bc_bridge.c
        src/bc_module_co2.c
        src/bc_module_relay.c
        src/bc_os.c
        #src/bc_tag_accelerometer.c
        src/bc_tag_barometer.c
        src/bc_i2c.c
        src/bc_tag_humidity.c
        src/bc_tag_lux_meter.c
        src/bc_tag_temperature.c
        src/bc_tick.c
        src/bc_i2c_sc16is740.c
        src/tags.c
        src/bc_i2c_tca9534a.c
        src/bc_i2c_pca9535.c
        src/bc_log.c
        src/task_thermometer.c
        src/task_lux_meter.c
        src/task_co2.c
        src/task_relay.c
        src/task_led.c
        src/task.c
        src/bc_talk.c
        src/application.c
        )

add_executable(${PROJECT_NAME} ${SOURCE_FILES} src/main.c)
target_link_libraries (${PROJECT_NAME} ${CMAKE_THREAD_LIBS_INIT} ${LIBUDEV_LIBRARIES} ${ARGP_LIBRARIES} -ludev)
