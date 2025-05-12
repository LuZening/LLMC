/*
 * PCM179x.c
 *
 *  Created on: Mar 13, 2024
 *      Author: cpholzn
 */


// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * PCM179X ASoC codec driver
 *
 * Copyright (c) Amarula Solutions B.V. 2013
 *
 *     Michael Trimarchi <michael@amarulasolutions.com>
 */


#include "main.h"
#include "pcm179x.h"
#include "FreeRTOS.h"

#define PCM179X_DAC_VOL_LEFT	0x10
#define PCM179X_DAC_VOL_RIGHT	0x11
#define PCM179X_FMT_CONTROL	0x12
#define PCM179X_MODE_CONTROL	0x13
#define PCM179X_SOFT_MUTE	PCM179X_FMT_CONTROL

#define PCM179X_FMT_MASK	0x70
#define PCM179X_FMT_SHIFT	4
#define PCM179X_MUTE_MASK	0x01
#define PCM179X_MUTE_SHIFT	0
#define PCM179X_ATLD_ENABLE	(1 << 7)


pcm179x_t pcm179x = {.addr_i2c = PCM179X_I2C_ADDR, .bitdepth=24, .rate = 96000,};

typedef struct
{
	uint8_t regaddr;
	uint8_t v;
}reg_default_t;

static const  reg_default_t pcm179x_reg_defaults[] = {
	{ 0x10, 0xff },
	{ 0x11, 0xff },
	{ 0x12, 0x50 },
	{ 0x13, 0x00 },
	{ 0x14, 0x00 },
	{ 0x15, 0x01 },
	{ 0x16, 0x00 },
	{ 0x17, 0x00 },
};


static bool pcm179x_accessible_reg(unsigned int reg)
{
	return reg >= 0x10 && reg <= 0x17;
}

static bool pcm179x_writeable_reg(unsigned int reg)
{
	bool accessible;

	accessible = pcm179x_accessible_reg(reg);

	return accessible && reg != 0x16 && reg != 0x17;
}




int PCM179x_set_attenuation(pcm179x_t *p, uint8_t loudness)
{
	//  loudness=255 -> 0dB (max volume)
	// loudness <= 14 -> mute
	int r;

	uint8_t mask;
	uint8_t *pRegVal;
//	// register 18: ATLD=1 allow load
//	mask = (1U << 7);
//	pRegVal = p->regvals + (18-16);
//	*pRegVal = (*pRegVal & (~mask)) | (0xffU & mask); // clear and set
//	r = PCM179x_write_reg(p, PCM179X_REG_18,  *pRegVal);

	// register 16 - L ch
	mask = 0xff;
	pRegVal = p->regvals + (16-16);
	*pRegVal = (*pRegVal & (~mask)) | (loudness & mask); // clear and set
	r = PCM179x_write_reg(p, PCM179X_REG_DIGITAL_ATTENUATION_CONTROL_L,  *pRegVal);


	// register 17 - R ch
	mask = 0xff;
	pRegVal = p->regvals + (17-16);
	*pRegVal = (*pRegVal & (~mask)) | (loudness & mask); // clear and set
	r = PCM179x_write_reg(p, PCM179X_REG_DIGITAL_ATTENUATION_CONTROL_R,  *pRegVal);

	// register 18: ATLD=1 allow load
	mask = (1U << 7);
	pRegVal = p->regvals + (18-16);
	*pRegVal = (*pRegVal & (~mask)) | (0xffU & mask); // clear and set
	r = PCM179x_write_reg(p, PCM179X_REG_18,  *pRegVal);

	if(loudness > 14)
		PCM179x_set_mute(p, 0);
	else
		PCM179x_set_mute(p, 1);
	return r;
}


uint8_t PCM179x_set_attenuation_async(pcm179x_t *p, uint8_t loudness, /* out */ uint8_t* buf)
{
	//  loudness=255 -> 0dB (max volume)
	// loudness <= 14 -> mute
	uint8_t i = 0;

	uint8_t mask;
	uint8_t *pRegVal;

	// register 16 - L ch
	mask = 0xff;
	pRegVal = p->regvals + (16-16);
	*pRegVal = (*pRegVal & (~mask)) | (loudness & mask); // clear and set
	buf[i++] = PCM179X_REG_DIGITAL_ATTENUATION_CONTROL_L; // REG16
	buf[i++] = *pRegVal;

	// register 17 - R ch
	mask = 0xff;
	pRegVal = p->regvals + (17-16);
	*pRegVal = (*pRegVal & (~mask)) | (loudness & mask); // clear and set
//	buf[i++] = PCM179X_REG_DIGITAL_ATTENUATION_CONTROL_R; // REG17 addr increments automatically
	buf[i++] = *pRegVal;

	// register 18: ATLD=1 allow load
	mask = (1U << 7);
	pRegVal = p->regvals + (18-16);
	*pRegVal = (*pRegVal & (~mask)) | (0xffU & mask); // clear and set
	// mute bit
	mask = 1U;
	uint8_t mute = 0;
	if(loudness < 14)
	{
		mute = 1;
	}
	*pRegVal = (*pRegVal & (~mask)) | (mute & mask); // clear and set
//	buf[i++] = PCM179X_REG_18; // REG18 addr increments automatically
	buf[i++] = *pRegVal;

	return i;
}


int PCM179x_set_format(pcm179x_t *p, uint8_t format)
{
	int r;

	uint8_t mask = (0b111U) << 4 ;

	// register 16
	uint8_t *pRegVal = p->regvals + (18-16);
	*pRegVal = (*pRegVal & (~mask)) | ((format << 4) & mask); // clear and set
	r = PCM179x_write_reg(p, PCM179X_REG_DIGITAL_ATTENUATION_CONTROL_L,  *pRegVal);

	return r;
}

int PCM179x_set_mute(pcm179x_t *p, uint8_t mute)
{
	int r;

	uint8_t mask = 1U ;


	// register 16
	uint8_t *pRegVal = p->regvals + (18-16);

	*pRegVal = (*pRegVal & (~mask)) | (mute & mask); // clear and set
	r = PCM179x_write_reg(p, 18,  *pRegVal);


	return r;
}


uint8_t PCM179x_set_mute_async(pcm179x_t *p, uint8_t mute, /* buf */ uint8_t* buf)
{
	uint8_t i =0;

	uint8_t mask = 1U ;


	// register 16
	uint8_t *pRegVal = p->regvals + (18-16);

	*pRegVal = (*pRegVal & (~mask)) | (mute & mask); // clear and set
//	PCM179x_write_reg(p, 18,  *pRegVal);
	buf[i++] = 18;
	buf[i++] = *pRegVal;

	return i;
}


int PCM179x_set_sample_rate(pcm179x_t *p,  uint32_t fs_Hz)
{
	return 0;
}


int PCM179x_set_oversample_multiplier(pcm179x_t *p, uint16_t mult)
{
	int r;

	uint8_t mask = 0b11U ;

	// register 16
	uint8_t *pRegVal = p->regvals + (20-16);
	uint8_t bitflags = 0;
	if(mult == 64)
		bitflags = 0;
	else if(mult == 32)
		bitflags = 0b01;
	else if(mult == 128)
		bitflags = 0b10;
	*pRegVal = (*pRegVal & (~mask)) | (bitflags & mask); // clear and set
	r = PCM179x_write_reg(p, PCM179X_REG_20,  *pRegVal);

	return r;
}

void pcm179x_hw_reset()
{
	// reset pcm179x
	HAL_GPIO_WritePin(DECODER_RSTX_GPIO_Port, DECODER_RSTX_Pin, 0);
	HAL_GPIO_WritePin(DECODER_RSTX_GPIO_Port, DECODER_RSTX_Pin, 0);
	HAL_GPIO_WritePin(DECODER_RSTX_GPIO_Port, DECODER_RSTX_Pin, 0);
	HAL_GPIO_WritePin(DECODER_RSTX_GPIO_Port, DECODER_RSTX_Pin, 0);
	HAL_GPIO_WritePin(DECODER_RSTX_GPIO_Port, DECODER_RSTX_Pin, 0);
	HAL_GPIO_WritePin(DECODER_RSTX_GPIO_Port, DECODER_RSTX_Pin, 0);
	HAL_GPIO_WritePin(DECODER_RSTX_GPIO_Port, DECODER_RSTX_Pin, 0);
	HAL_GPIO_WritePin(DECODER_RSTX_GPIO_Port, DECODER_RSTX_Pin, 0);
	HAL_GPIO_WritePin(DECODER_RSTX_GPIO_Port, DECODER_RSTX_Pin, 1);
	// NOTE: need 1024 system MCLKs to get system prepared, enable I2S transmission for a while after hw reset
}


int pcm179x_test(pcm179x_t *p)
{
	int r;

	//todo
	// read register: device ID
	r = PCM179x_read_reg(p, PCM179X_REG_DEVICE_ID, &p->ID);

	// write register: Reg 18
	uint8_t v = 0;
	r = PCM179x_read_reg(p, 18, &v );
	r = PCM179x_write_reg(p , 18 , 0b11010001); // Load ATT, Mute enabled
	r = PCM179x_read_reg(p, 18, &v);
	return r;

}


int pcm179x_init(pcm179x_t *p, uint32_t fclk_Hz)
{
	int r;

	p->addr_i2c = PCM179X_I2C_ADDR;

	p->regvals[0] = 0xffU; // reg 16 default value
	p->regvals[1] = 0xffU; // reg 17 default value
	p->regvals[2] = 0b01010000U; // reg 18 default value
	p->regvals[3] = 0; // reg 19 default value
	p->regvals[4] = 0; // reg 20
	p->regvals[5] = 1; // reg 21
	p->fclk_Hz = fclk_Hz; // master clock freq
	// set registers
	PCM179x_set_attenuation(p, 100);
	r = PCM179x_set_format(p, PCM179X_FMT_32BIT_I2S);

	return r;
}


