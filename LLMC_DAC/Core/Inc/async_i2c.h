/*
 * async_i2c.h
 *
 *  Created on: 2025年2月6日
 *      Author: cpholzn
 */

#ifndef INC_ASYNC_I2C_H_
#define INC_ASYNC_I2C_H_

#include <stdint.h>

uint8_t transmit_async_i2c(uint8_t slot /*0:PCM1792, 1: ADCX120*/, uint8_t* packet, uint8_t len); // implemented in main.c


#endif /* INC_ASYNC_I2C_H_ */
