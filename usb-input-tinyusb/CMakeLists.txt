cmake_minimum_required(VERSION 3.20)
set(FAMILY espressif)
set(BOARD espressif_s3_devkitc)
#set(LOG 3)

# TinyUSB cmake already include the ESP-IDF cmake
include(${CMAKE_CURRENT_SOURCE_DIR}/components/tinyusb/hw/bsp/family_support.cmake)
## Extra directory to fix `espressif__tinyusb` detecting tusb_config.h
#include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/tusb)

# gets PROJECT name for the example
family_get_project_name(PROJECT ${CMAKE_CURRENT_LIST_DIR})

project(${PROJECT} C CXX ASM)

# Checks this example is valid for the family and initializes the project
family_initialize_project(${PROJECT} ${CMAKE_CURRENT_LIST_DIR})

# Espressif has its own cmake build system
if(FAMILY STREQUAL "espressif")
  return()
endif()
