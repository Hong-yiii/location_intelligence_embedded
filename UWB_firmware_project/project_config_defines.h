#ifndef PROJECT_CONFIG_DEFINES_H
#define PROJECT_CONFIG_DEFINES_H

// Ensure FPU is marked as present, overriding any incorrect SDK defaults
#ifdef __FPU_PRESENT
#undef __FPU_PRESENT
#endif
#define __FPU_PRESENT 1

#endif // PROJECT_CONFIG_DEFINES_H 