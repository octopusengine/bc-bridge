cmake_minimum_required(VERSION 2.6)
project(bc-bridge C)
# set(CMAKE_VERBOSE_MAKEFILE on)
set(CMAKE_C_FLAGS "-Wall -pedantic -std=c99")
string(TIMESTAMP VERSION "%Y-%m-%dT%H:%M:%S")
add_definitions(-DVERSION="${VERSION}")
include_directories(bcl/inc)
include_directories(ft260)
include_directories(jsmn)
set(SOURCE_FILES
        bridge.c
        ft260/ft260.c
        jsmn/jsmn.c
        bcl/src/bc/os.c
        bcl/src/bc/tag.c
        bcl/src/bc/tick.c
        bcl/src/bc/i2c/app.c
        bcl/src/bc/i2c/sys.c
        bcl/src/bc/i2c/tca9534a.c
        bcl/src/bc/module/co2.c
        bcl/src/bc/module/relay.c
        bcl/src/bc/tag/accelerometer.c
        bcl/src/bc/tag/barometer.c
        bcl/src/bc/tag/humidity.c
        bcl/src/bc/tag/lux_meter.c
        #bcl/src/bc/tag/temperature.c
)
add_executable(${PROJECT_NAME} ${SOURCE_FILES})
target_link_libraries(${PROJECT_NAME} -ludev)
