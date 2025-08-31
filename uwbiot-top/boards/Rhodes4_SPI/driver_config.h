/*
 * Rhodes4_SPI Board Configuration
 * SPI-specific configuration for UWB interface
 */

#ifndef DRIVER_CONFIG_H
#define DRIVER_CONFIG_H

#include "../Host/Rhodes4/uwb_board.h"

/* SPI Configuration for Rhodes4_SPI Board */
/* As per datasheet, taking to maximum 8Mhz of QN9090 */
#define UWB_SPI_BAUDRATE (8 * 1000 * 1000U) // 8 MHz
#define UWB_SPI_BASEADDR SPI1
#define UWB_SPI_SSEL     kSPI_Ssel0

#endif // DRIVER_CONFIG_H
