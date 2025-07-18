# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.0)
set(toolchainVersion 13_3_Rel1)
set(picotoolVersion 2.1.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(PICO_BOARD pico_w CACHE STRING "Board type")

include(pico_sdk_import.cmake)
set(FREERTOS_KERNEL_PATH "C:/FreeRTOS-Kernel")
include(${FREERTOS_KERNEL_PATH}/portable/ThirdParty/GCC/RP2040/FreeRTOS_Kernel_import.cmake)

project(Controle_nivel C CXX ASM)
pico_sdk_init()

add_executable(${PROJECT_NAME} Controle_nivel.c
            ${CMAKE_CURRENT_LIST_DIR}/libs/connect_wifi.c
            ${CMAKE_CURRENT_LIST_DIR}/libs/server.c
            ${CMAKE_CURRENT_LIST_DIR}/libs/init_config.c
            ${CMAKE_CURRENT_LIST_DIR}/libs/ssd1306.c
            ${CMAKE_CURRENT_LIST_DIR}/libs/global_manage.c
            ${CMAKE_CURRENT_LIST_DIR}/libs/led_matriz.c)

pico_generate_pio_header(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR}/pio_matriz.pio)

target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/include)

target_link_libraries(${PROJECT_NAME}
        pico_stdlib
        pico_cyw43_arch_lwip_threadsafe_background
        hardware_pio
        hardware_adc
        hardware_clocks
        hardware_pwm
        hardware_i2c
        FreeRTOS-Kernel
        FreeRTOS-Kernel-Heap4)

pico_enable_stdio_uart(${PROJECT_NAME} 0)
pico_enable_stdio_usb(${PROJECT_NAME} 1)

pico_add_extra_outputs(${PROJECT_NAME})
