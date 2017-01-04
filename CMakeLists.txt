cmake_minimum_required(VERSION 3.5)
cmake_policy(SET CMP0048 NEW)

project(bc-bridge VERSION 1.0.2 LANGUAGES C)

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

option(BUILD_STATIC "Build statically linked executable" FALSE)

if(BUILD_STATIC)
    set(BUILD_SHARED_LIBRARIES FALSE)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -static -fno-pie")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static -no-pie")
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")

    find_library(LIBC libc.a c)
endif()

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
if(BUILD_STATIC)
    target_link_libraries(${PROJECT_NAME} ${LIBC})
endif()

if(CMAKE_BUILD_TYPE STREQUAL RelWithDebInfo AND CMAKE_OBJCOPY)
    set(strip_scope "debug")
    if(BUILD_STATIC)
        set(strip_scope "all")
    endif()

    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMENT "Detaching debugging information from ${PROJECT_NAME} into separate file ${PROJECT_NAME}.debug"
        COMMAND ${CMAKE_OBJCOPY} --only-keep-debug $<TARGET_FILE:${PROJECT_NAME}> $<TARGET_FILE:${PROJECT_NAME}>.debug
        COMMAND ${CMAKE_OBJCOPY} --strip-${strip_scope} $<TARGET_FILE:${PROJECT_NAME}>
        COMMAND ${CMAKE_OBJCOPY} --add-gnu-debuglink=$<TARGET_FILE_NAME:${PROJECT_NAME}>.debug $<TARGET_FILE:${PROJECT_NAME}>)

    set(debug_file ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.debug)

    install(FILES ${debug_file} DESTINATION bin COMPONENT debug)
    # This is used only for DEB package, see cmake/CPackProjectConfig.cmake.
    install(FILES ${debug_file} DESTINATION lib/debug/usr/bin COMPONENT dbg EXCLUDE_FROM_ALL)
endif()

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin COMPONENT main)


################################################################################
# Configure CPack                                                              #
################################################################################

# Common properties
set(CPACK_GENERATOR "TGZ;DEB")
set(CPACK_PROJECT_CONFIG_FILE ${CMAKE_SOURCE_DIR}/cmake/CPackProjectConfig.cmake)

set(CPACK_PACKAGE_CONTACT "BigClown Labs <support@bigclown.com>")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Software interface between Clown.Hub and Bridge Module")
set(CPACK_PACKAGE_VENDOR "BigClown Labs")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})

# Debian packages
set(CPACK_DEB_COMPONENT_INSTALL ON)
set(CPACK_DEBIAN_ENABLE_COMPONENT_DEPENDS TRUE)
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://github.com/bigclownlabs/bc-bridge")

set(CPACK_DEBIAN_MAIN_PACKAGE_NAME ${PROJECT_NAME})
set(CPACK_DEBIAN_MAIN_PACKAGE_SECTION comm)
if(NOT BUILD_STATIC)
    set(CPACK_DEBIAN_MAIN_PACKAGE_DEPENDS "libudev1")
endif()

set(CPACK_DEBIAN_DBG_PACKAGE_DESCRIPTION_SUMMARY "${CPACK_PACKAGE_DESCRIPTION_SUMMARY} (debug symbols)")
set(CPACK_DEBIAN_DBG_PACKAGE_PRIORITY extra)
set(CPACK_DEBIAN_DBG_PACKAGE_SECTION debug)

if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|amd64")
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE amd64)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "armhf|armv7")
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE armhf)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|armv8|arm64")
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE arm64)
endif()

include(CPack)
