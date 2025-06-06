/*
 * Copyright  2019 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/***********************************************************************************************************************
 * This file was generated by the MCUXpresso Config Tools. Any manual edits made to this file
 * will be overwritten if the respective MCUXpresso Config Tools is used to update this file.
 **********************************************************************************************************************/

#ifndef _PIN_MUX_H_
#define _PIN_MUX_H_

/*!
 * @addtogroup pin_mux
 * @{
 */

/***********************************************************************************************************************
 * API
 **********************************************************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif

/*!
 * @brief Calls initialization functions.
 *
 */
void BOARD_InitBootPins(void);

/*!
 * @brief Enables digital function */
#define IOCON_PIO_DIGITAL_EN 0x80u
/*!
 * @brief IO is an open drain cell */
#define IOCON_PIO_ECS_DI 0x00u
/*!
 * @brief I2C mode */
#define IOCON_PIO_EGP_I2C 0x00u
/*!
 * @brief High speed IO for GPIO mode, IIC not */
#define IOCON_PIO_EHS_DI 0x00u
/*!
 * @brief IIC mode:Noise pulses below approximately 50ns are filtered out. GPIO mode:a 3ns filter */
#define IOCON_PIO_FSEL_DI 0x00u
/*!
 * @brief Selects pin function 2 */
#define IOCON_PIO_FUNC2 0x02u
/*!
 * @brief Selects pin function 5 */
#define IOCON_PIO_FUNC5 0x05u
/*!
 * @brief Input filter disabled */
#define IOCON_PIO_INPFILT_OFF 0x0100u
/*!
 * @brief Input function is not inverted */
#define IOCON_PIO_INV_DI 0x00u
/*!
 * @brief IO_CLAMP disabled */
#define IOCON_PIO_IO_CLAMP_DI 0x00u
/*!
 * @brief Selects pull-up function */
#define IOCON_PIO_MODE_PULLUP 0x00u
/*!
 * @brief Open drain is disabled */
#define IOCON_PIO_OPENDRAIN_DI 0x00u
/*!
 * @brief Open drain is enabled */
#define IOCON_PIO_OPENDRAIN_EN 0x0400u
/*!
 * @brief Standard mode, output slew rate control is disabled */
#define IOCON_PIO_SLEW0_STANDARD 0x00u
/*!
 * @brief Standard mode, output slew rate control is disabled */
#define IOCON_PIO_SLEW1_STANDARD 0x00u
/*!
 * @brief SSEL is disabled */
#define IOCON_PIO_SSEL_DI 0x00u

/*!
 * @brief Configures pin routing and optionally pin electrical features.
 *
 */
void BOARD_InitPins(void); /* Function assigned for the Cortex-M4 */

#if defined(__cplusplus)
}
#endif

/*!
 * @}
 */
#endif /* _PIN_MUX_H_ */

/***********************************************************************************************************************
 * EOF
 **********************************************************************************************************************/
