cmake_minimum_required(VERSION 3.13)

# Toolchain file will set the compiler and system name
# set(CMAKE_SYSTEM_NAME Generic) # Handled by toolchain
# set(CMAKE_C_COMPILER arm-none-eabi-gcc) # Handled by toolchain

project(UWB_firmware_project C)

# Flags will be appended to those from the toolchain file
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mcpu=cortex-m4 -mthumb -O2 -Wall -ffunction-sections -fdata-sections")

set(UWB_IOT_BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../uwb-iot) # Define base for external libs
set(SDK_BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../SDK_2_6_16_QN9090) # Define base for SDK

include_directories(
    ${UWB_IOT_BASE_DIR}/uwb_api/Api
    ${UWB_IOT_BASE_DIR}/uwb_core/include
    ${UWB_IOT_BASE_DIR}/uwb_core/adaptation
    ${CMAKE_CURRENT_SOURCE_DIR}/app # local app directory

    # Correct path for uwb_board.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../boards/Host/Rhodes4
    # Path for peripherals.h, board.c, etc. used by uwb_board.h or other Host/Rhodes4 files
    ${CMAKE_CURRENT_SOURCE_DIR}/../boards/Rhodes4_SPI

    # Explicitly add SDK device path for fsl_device_registers.h (already in toolchain, but to be sure)
    ${SDK_BASE_DIR}/devices/QN9090
    # Add path for fsl_common.h and other drivers
    ${SDK_BASE_DIR}/devices/QN9090/drivers
    # Explicitly add CMSIS include path (already in toolchain, but to be sure)
    ${SDK_BASE_DIR}/CMSIS/Include
    # Path for fsl_debug_console.h
    ${SDK_BASE_DIR}/devices/QN9090/utilities/debug_console
    # Path for serial_manager.h (dependency of fsl_debug_console)
    ${SDK_BASE_DIR}/components/serial_manager
    # Path for fsl_xcvr.h (transceiver driver)
    ${SDK_BASE_DIR}/middleware/wireless/framework/XCVR/DK6

    # Paths for Secure Element Middleware
    ${CMAKE_CURRENT_SOURCE_DIR}/../se_mw/sss/inc       # For fsl_sss_ftr_default.h, fsl_sss_policy.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../se_mw/sss/ex/inc    # For ex_sss.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../se_mw/sss/port/ksdk # For fsl_sss_types.h (KSDK specific port)
    ${CMAKE_CURRENT_SOURCE_DIR}/../se_mw/se_hostlib/hostLib/inc       # For se05x_tlv.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../se_mw/se_hostlib/hostLib/fira/inc # For sus_APDU.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../se_mw/se_hostlib/hostLib/libCommon/infra # For sm_types.h

    # Path for UWB HAL implementation headers (phUwb_BuildConfig.h, uwb_iot_ftr_default.h, phUwbTypes.h)
    ${CMAKE_CURRENT_SOURCE_DIR}/../libs/halimpl/inc
    # Path for UCI headers (uci_defs.h)
    ${CMAKE_CURRENT_SOURCE_DIR}/../libs/uci-core/inc
)

file(GLOB UWB_API_SRCS
    ${UWB_IOT_BASE_DIR}/uwb_api/Api/*.c
    ${UWB_IOT_BASE_DIR}/uwb_api/Api/SR1XX/*.c
    ${UWB_IOT_BASE_DIR}/uwb_api/Api/SE051_Wrapper/*.c
    ${UWB_IOT_BASE_DIR}/uwb_api/Api/StateMachine/*.c
    ${UWB_IOT_BASE_DIR}/uwb_api/PrintUtility/*.c
    ${UWB_IOT_BASE_DIR}/uwb_api/PrintUtility/SR1XX/*.c
)
file(GLOB UWB_CORE_SRCS
    ${UWB_IOT_BASE_DIR}/uwb_core/adaptation/*.c
)
file(GLOB APP_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/app/*.c
)

add_executable(UWB_firmware_project
    ${APP_SRCS}
    ${UWB_API_SRCS}
    ${UWB_CORE_SRCS}
)

# Force-include project specific defines for FPU
set(FORCE_INCLUDE_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/project_config_defines.h)
if(CMAKE_C_COMPILER_ID STREQUAL "GNU" OR CMAKE_C_COMPILER_ID MATCHES "Clang")
    target_compile_options(UWB_firmware_project PRIVATE "-include${FORCE_INCLUDE_HEADER}")
elseif(CMAKE_C_COMPILER_ID STREQUAL "IAR")
    target_compile_options(UWB_firmware_project PRIVATE "--preinclude=${FORCE_INCLUDE_HEADER}")
endif()

# ✅ Set the full path to the linker script
set(LINKER_SCRIPT "${CMAKE_SOURCE_DIR}/linker/your_board.ld")

# ✅ Apply linker options only to this target
target_link_options(UWB_firmware_project PRIVATE
    -T${LINKER_SCRIPT}
    -nostdlib
    -Wl,--gc-sections
)

# Optional: set export of compile_commands.json for IntelliSense
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
