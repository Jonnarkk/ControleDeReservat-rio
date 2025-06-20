

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(PICO_BOARD pico_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

set(FREERTOS_KERNEL_PATH "C:/Dev/FreeRTOS-Kernel")
include(${FREERTOS_KERNEL_PATH}/portable/ThirdParty/GCC/RP2040/FreeRTOS_Kernel_import.cmake)

project(Controle_nivel C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(${PROJECT_NAME} Controle_nivel.c 
            ${CMAKE_CURRENT_LIST_DIR}/libs/connect_wifi.c
            ${CMAKE_CURRENT_LIST_DIR}/libs/server.c
            # ${CMAKE_CURRENT_LIST_DIR}/libs/init_config.c
            # ${CMAKE_CURRENT_LIST_DIR}/libs/ssd1306.c
            ${CMAKE_CURRENT_LIST_DIR}/libs/global_manage.c
            # ${CMAKE_CURRENT_LIST_DIR}/libs/matriz.c
            )

#pico_generate_pio_header(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR}/pio_matrix.pio)

pico_set_program_name(${PROJECT_NAME} "Controle_nivel")
pico_set_program_version(${PROJECT_NAME} "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(${PROJECT_NAME} 0)
pico_enable_stdio_usb(${PROJECT_NAME} 1)

# Add the standard library to the build
target_link_libraries(${PROJECT_NAME}
        pico_stdlib)

# Add the standard include files to the build
target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/include
        # ${PICO_SDK_PATH}/lib/lwip/src/include
        # ${PICO_SDK_PATH}/lib/lwip/src/include/arch
        # ${PICO_SDK_PATH}/lib/lwip/src/include/lwip
)

# target_sources(${PROJECT_NAME} PRIVATE
#     ${PICO_SDK_PATH}/lib/lwip/src/apps/http/httpd.c
#     ${PICO_SDK_PATH}/lib/lwip/src/apps/http/fs.c
# )

# Add any user requested libraries
target_link_libraries(${PROJECT_NAME} 
        pico_cyw43_arch_lwip_threadsafe_background
        hardware_pio
        hardware_adc
        hardware_clocks
        hardware_pwm
        hardware_i2c
        FreeRTOS-Kernel 
        FreeRTOS-Kernel-Heap4
        )

pico_add_extra_outputs(${PROJECT_NAME})

