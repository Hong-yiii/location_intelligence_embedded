# CMake Toolchain file for NXP QN9090 (ARM Cortex-M4F) with GCC
#
# This file tells CMake how to cross-compile for the QN9090.
#

#--------------------------------------------------------------------------------
# System and Processor
#--------------------------------------------------------------------------------
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

#--------------------------------------------------------------------------------
# Compilers
#
# Assumes arm-none-eabi-gcc, arm-none-eabi-g++, and arm-none-eabi-as are in PATH
#--------------------------------------------------------------------------------
set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc) # Use gcc for assembly files as well
set(CMAKE_AR arm-none-eabi-ar)
set(CMAKE_OBJCOPY arm-none-eabi-objcopy)
set(CMAKE_OBJDUMP arm-none-eabi-objdump)
set(CMAKE_SIZE arm-none-eabi-size)

#--------------------------------------------------------------------------------
# Compiler and Linker Flags
#
# These are typical flags for a Cortex-M4F. Adjust as needed.
# FPU_USED is typically defined by the SDK components if hardware FPU is used.
#--------------------------------------------------------------------------------
set(CPU_CORE "cortex-m4")
set(CPU_FLOAT_ABI "hard") # Use "softfp" or "soft" if not using hardware FPU or if issues arise
set(CPU_FPU "fpv4-sp-d16")

set(COMMON_FLAGS "-mcpu=${CPU_CORE} -mthumb -mfloat-abi=${CPU_FLOAT_ABI} -mfpu=${CPU_FPU}")
set(COMMON_FLAGS "${COMMON_FLAGS} -Wall -Wextra -ffunction-sections -fdata-sections -pipe")

# SDK specific defines often go here.
# For QN9090, common defines might include:
# CPU_QN9090, FSL_RTOS_FREE_RTOS (if using FreeRTOS), etc.
# These are often best handled in your main CMakeLists.txt based on components.
set(SDK_DEFINES "-DCPU_QN9090HN -DSSS_HAVE_APPLET_SE051_UWB=1 -DSSS_USE_FTR_FILE=1") # Use specific CPU variant, set SE051 UWB applet, and specify to use fsl_sss_ftr.h

set(CMAKE_C_FLAGS_INIT "${COMMON_FLAGS} ${SDK_DEFINES} -std=gnu11")
set(CMAKE_CXX_FLAGS_INIT "${COMMON_FLAGS} ${SDK_DEFINES} -std=gnu++14 -fno-rtti -fno-exceptions")
set(CMAKE_ASM_FLAGS_INIT "${COMMON_FLAGS} ${SDK_DEFINES} -x assembler-with-cpp")

# Linker flags
# set(LINKER_SCRIPT_PATH "${CMAKE_CURRENT_LIST_DIR}/SDK_2_6_16_QN9090/middleware/wireless/framework/Common/devices/QN9090/gcc/connectivity.ld") # Project will specify its own
# set(CMAKE_EXE_LINKER_FLAGS_INIT "-T\"${LINKER_SCRIPT_PATH}\" --specs=nosys.specs --specs=nano.specs -Wl,--gc-sections -Wl,-Map=\${PROJECT_NAME}.map") # Project will specify its own

# Add Newlib reentrancy flags if you are using it extensively with an RTOS
# set(CMAKE_EXE_LINKER_FLAGS_INIT "${CMAKE_EXE_LINKER_FLAGS_INIT} -Wl,--undefined=g_pfnVectors") # If using SDK's vector table relocation

#--------------------------------------------------------------------------------
# Skip CMake's default compiler checks for host system
# This is crucial for cross-compilation.
#--------------------------------------------------------------------------------
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

#--------------------------------------------------------------------------------
# Search Paths
#--------------------------------------------------------------------------------
# Set the root of your SDK. Adjust this path if your toolchain file is not in the project root.
set(NXP_SDK_ROOT "${CMAKE_CURRENT_LIST_DIR}/SDK_2_6_16_QN9090")

# Add default include directories from the SDK
# These are common paths, you might need to add more specific ones in your main CMakeLists.txt
set(CMAKE_C_IMPLICIT_INCLUDE_DIRECTORIES
    "${NXP_SDK_ROOT}/CMSIS/Include"
    "${NXP_SDK_ROOT}/devices/QN9090"
    # Add other common SDK include paths here
)
set(CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES ${CMAKE_C_IMPLICIT_INCLUDE_DIRECTORIES})
set(CMAKE_ASM_IMPLICIT_INCLUDE_DIRECTORIES ${CMAKE_C_IMPLICIT_INCLUDE_DIRECTORIES})


#--------------------------------------------------------------------------------
# Finders
#
# CMake's find_program, find_library, find_path, and find_file commands
# should search the toolchain's paths first.
#--------------------------------------------------------------------------------
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Add the SDK to CMAKE_PREFIX_PATH so find_package can find SDK components if they provide CMake configs
list(APPEND CMAKE_PREFIX_PATH "${NXP_SDK_ROOT}")

#--------------------------------------------------------------------------------
# Optional: Debugging support
#--------------------------------------------------------------------------------
# set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose build type: Debug Release RelWithDebInfo MinSizeRel")
# if(NOT CMAKE_BUILD_TYPE)
#   set(CMAKE_BUILD_TYPE Debug)
# endif()

# message(STATUS "Toolchain: NXP QN9090 (ARM Cortex-M4F) with GCC")
# message(STATUS "SDK Root: ${NXP_SDK_ROOT}")
# message(STATUS "Linker Script: ${LINKER_SCRIPT_PATH}")
# message(STATUS "C Compiler: ${CMAKE_C_COMPILER}")
# message(STATUS "C Flags: ${CMAKE_C_FLAGS_INIT}")
# message(STATUS "Linker Flags: ${CMAKE_EXE_LINKER_FLAGS_INIT}")
