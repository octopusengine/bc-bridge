cmake_minimum_required(VERSION 2.8)

project(bc-bridge LANGUAGES C)
set(PROJECT_VERSION 1.0.2)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

#set(CMAKE_VERBOSE_MAKEFILE on)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -pedantic -std=gnu99")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake/)

string(TIMESTAMP FIRMWARE_DATETIME "%Y-%m-%dT%H:%M:%SZ")

add_definitions(-DFIRMWARE_RELEASE="${PROJECT_VERSION}")
add_definitions(-DFIRMWARE_DATETIME="${FIRMWARE_DATETIME}")
add_definitions(-DBRIDGE)
add_definitions(-D_BSD_SOURCE)
add_definitions(-D_REENTRANT)
add_definitions(-DTHREADSAFE)

find_package(argp REQUIRED)
find_package(Threads REQUIRED)
find_package(Libudev REQUIRED)

include_directories(jsmn)
include_directories(src)

set(SOURCE_FILES
        jsmn/jsmn.c
        src/application.c
        src/bc_bridge.c
        src/bc_i2c.c
        src/bc_i2c_pca9535.c
        src/bc_i2c_sc16is740.c
        src/bc_i2c_tca9534a.c
        src/bc_i2c_ssd1306.c
        src/bc_log.c
        src/bc_module_co2.c
        src/bc_module_relay.c
        src/bc_os.c
        src/bc_tag_barometer.c
        src/bc_tag_humidity.c
        src/bc_tag_lux_meter.c
        src/bc_tag_temperature.c
        src/bc_base64.c
        src/bc_talk.c
        src/bc_tick.c
        src/bc_gfx.c
        src/task.c
        src/task_co2.c
        src/task_humidity.c
        src/task_led.c
        src/task_lux_meter.c
        src/task_relay.c
        src/task_thermometer.c
        src/task_barometer.c
        src/task_i2c.c
        src/task_manager.c
        src/task_display_oled.c
        )

add_executable(${PROJECT_NAME} ${SOURCE_FILES} src/main.c)

target_link_libraries(${PROJECT_NAME} ${CMAKE_THREAD_LIBS_INIT} ${LIBUDEV_LIBRARIES} ${ARGP_LIBRARIES})

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)

set(CPACK_GENERATOR "DEB")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "bigclown")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libudev1")
set(CPACK_MONOLITHIC_INSTALL 1)
set(CPACK_COMPONENTS_ALL_IN_ONE_PACKAGE 1)

include(CPack)
