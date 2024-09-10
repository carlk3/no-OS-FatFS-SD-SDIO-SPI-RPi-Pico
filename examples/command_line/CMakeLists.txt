cmake_minimum_required(VERSION 3.5)

set(PICO_BOARD $ENV{PICO_BOARD})
set(PICO_SDK_PATH $ENV{PICO_SDK_PATH})
set(PROGRAM_NAME command_line)

# Pull in Pico SDK (must be before project)
include(${PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(${PROGRAM_NAME} C CXX ASM)

# add_compile_options(-Os) # Optimize for size (place before pico_sdk_init)

# Initialise the Pico SDK
pico_sdk_init()

add_subdirectory(../../src build)

# Add executable. Default name is the project name, version 0.1
add_executable(${PROGRAM_NAME} 
    config/hw_config.c
    main.cpp 
    src/command.cpp
    src/data_log_demo.c
    tests/app4-IO_module_function_checker.c
    tests/bench.c
    tests/big_file_test.c
    tests/CreateAndVerifyExampleFiles.c
    tests/ff_stdio_tests_with_cwd.c
    tests/simple.c
)

# https://datasheets.raspberrypi.com/pico/raspberry-pi-pico-c-sdk.pdf
target_compile_definitions(${PROGRAM_NAME} PRIVATE
  PICO_STACK_SIZE=0x1000
  PICO_CORE1_STACK_SIZE=0x800
#   PICO_HEAP_SIZE=0x20000
)

target_compile_options(${PROGRAM_NAME} PUBLIC 
  -Wall 
  -Wextra 
  -Wshadow 
  -Wstack-usage=2048 
  -fanalyzer 
)

add_compile_definitions(
    PARAM_ASSERTIONS_ENABLE_ALL=1 
    PICO_MALLOC_PANIC=1
    PICO_USE_STACK_GUARDS=1

    # This program is useless without standard input and output.
    USE_PRINTF
    #USE_DBG_PRINTF
)

# Disable CRC checking for SPI-attached cards.
# add_compile_definitions(SD_CRC_ENABLED=0)

# Use Pico's LED to show drive activity. 
# Ensure that PICO_DEFAULT_LED_PIN is set correctly.
# Note that Pico W uses GPIO 25 for SPI communication to the CYW43439.
# add_compile_definitions(USE_LED=1)

set_property(TARGET ${PROGRAM_NAME} APPEND_STRING PROPERTY LINK_FLAGS 
  "-Wl,--print-memory-usage"
)

pico_set_program_name(${PROGRAM_NAME} "${PROGRAM_NAME}")
pico_set_program_version(${PROGRAM_NAME} "3.3.1")

if ("${PICO_BOARD}" STREQUAL "pico")
    pico_set_linker_script(${PROGRAM_NAME} ${CMAKE_SOURCE_DIR}/linker/memmap.ld)
endif()

# See 4.1. Serial input and output on Raspberry Pi Pico in Getting started with Raspberry Pi Pico (https://datasheets.raspberrypi.org/pico/getting-started-with-pico.pdf)
# and 2.7.1. Standard Input/Output (stdio) Support in Raspberry Pi Pico C/C++ SDK (https://datasheets.raspberrypi.org/pico/raspberry-pi-pico-c-sdk.pdf):
#pico_enable_stdio_uart(${PROGRAM_NAME} 1)
pico_enable_stdio_usb(${PROGRAM_NAME} 1)

target_include_directories(${PROGRAM_NAME} PUBLIC 
        include/ 
)
target_link_libraries(${PROGRAM_NAME}
    no-OS-FatFS-SD-SDIO-SPI-RPi-Pico
    hardware_clocks
    hardware_adc
)

pico_add_extra_outputs(${PROGRAM_NAME})
