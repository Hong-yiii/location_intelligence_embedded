cmake_minimum_required(VERSION 3.13)
project(UWB_firmware_project C)

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mcpu=cortex-m4 -mthumb -O2 -Wall -ffunction-sections -fdata-sections")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -TUWB_firmware_project/linker/your_board.ld -nostdlib -Wl,--gc-sections")

include_directories(
    ${CMAKE_SOURCE_DIR}/uwb_api/Api
    ${CMAKE_SOURCE_DIR}/uwb_core/include
    ${CMAKE_SOURCE_DIR}/uwb_core/adaptation
    ${CMAKE_SOURCE_DIR}/app
)

file(GLOB UWB_API_SRCS
    ${CMAKE_SOURCE_DIR}/uwb_api/Api/*.c
    ${CMAKE_SOURCE_DIR}/uwb_api/Api/SR1XX/*.c
    ${CMAKE_SOURCE_DIR}/uwb_api/Api/SE051_Wrapper/*.c
    ${CMAKE_SOURCE_DIR}/uwb_api/Api/StateMachine/*.c
    ${CMAKE_SOURCE_DIR}/uwb_api/PrintUtility/*.c
    ${CMAKE_SOURCE_DIR}/uwb_api/PrintUtility/SR1XX/*.c
)
file(GLOB UWB_CORE_SRCS
    ${CMAKE_SOURCE_DIR}/uwb_core/adaptation/*.c
)
file(GLOB APP_SRCS
    ${CMAKE_SOURCE_DIR}/app/*.c
)

add_executable(UWB_firmware_project
    ${APP_SRCS}
    ${UWB_API_SRCS}
    ${UWB_CORE_SRCS}
)

set_target_properties(UWB_firmware_project PROPERTIES LINK_DEPENDS ${CMAKE_SOURCE_DIR}/linker/your_board.ld) 