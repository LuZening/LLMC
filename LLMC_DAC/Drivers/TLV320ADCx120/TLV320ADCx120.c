/*
 * TLV320ADCx120.c
 *
 *  Created on: Mar 13, 2024
 *      Author: cpholzn
 */


#include "TLV320ADCx120.h"
#include "main.h"
#include "freeRTOS.h"
#include <stdint.h>


#define BIT(nr)			(1U << (nr))
#define BIT_ULL(nr)		(1ULL << (nr))

adcx120_t adcx120 = {
		.pI2C = &hi2c1,
		.I2C_addr = ADCX120_I2C_ADDR,
		.bitdepth = 24,
		.format = ADCX120_FMT_I2S,
		.samplerate = ADCX120_FS_88K2_OR_96K,
};

typedef struct
{
	uint8_t regaddr;
	uint8_t v;
}reg_default_t;

static const  reg_default_t regdefaults[] = {
	{ ADCX120_PAGE_SELECT, 0x00 },
	{ ADCX120_SW_RESET, 0x00 },
	{ ADCX120_SLEEP_CFG, 0x00 },
	{ ADCX120_SHDN_CFG, 0x05 },
	{ ADCX120_ASI_CFG0, 0x30 },
	{ ADCX120_ASI_CFG1, 0x00 },
	{ ADCX120_ASI_CFG2, 0x00 },
	{ ADCX120_ASI_CH1, 0x00 },
	{ ADCX120_ASI_CH2, 0x01 },
	{ ADCX120_ASI_CH3, 0x02 },
	{ ADCX120_ASI_CH4, 0x03 },
//	{ ADCX120_ASI_CH5, 0x04 },
//	{ ADCX120_ASI_CH6, 0x05 },
//	{ ADCX120_ASI_CH7, 0x06 },
//	{ ADCX120_ASI_CH8, 0x07 },
	{ ADCX120_MST_CFG0, 0x02 },
	{ ADCX120_MST_CFG1, 0x48 },
	{ ADCX120_ASI_STS, 0xff },
	{ ADCX120_CLK_SRC, 0x10 },
	{ ADCX120_PDMCLK_CFG, 0x40 },
	{ ADCX120_PDMIN_CFG, 0x00 },
	{ ADCX120_GPIO_CFG0, 0x22 },
	{ ADCX120_GPO_CFG0, 0x00 },
//	{ ADCX120_GPO_CFG1, 0x00 },
//	{ ADCX120_GPO_CFG2, 0x00 },
//	{ ADCX120_GPO_CFG3, 0x00 },
	{ ADCX120_GPO_VAL, 0x00 },
	{ ADCX120_GPIO_MON, 0x00 },
	{ ADCX120_GPI_CFG0, 0x00 },
//	{ ADCX120_GPI_CFG1, 0x00 },
	{ ADCX120_GPI_MON, 0x00 },
	{ ADCX120_INT_CFG, 0x00 },
	{ ADCX120_INT_MASK0, 0xff },
	{ ADCX120_INT_LTCH0, 0x00 },
	{ ADCX120_BIAS_CFG, 0x00 },
	{ ADCX120_CH1_CFG0, 0x00 },
	{ ADCX120_CH1_CFG1, 0x00 },
	{ ADCX120_CH1_CFG2, 0xc9 },
	{ ADCX120_CH1_CFG3, 0x80 },
	{ ADCX120_CH1_CFG4, 0x00 },
	{ ADCX120_CH2_CFG0, 0x00 },
	{ ADCX120_CH2_CFG1, 0x00 },
	{ ADCX120_CH2_CFG2, 0xc9 },
	{ ADCX120_CH2_CFG3, 0x80 },
	{ ADCX120_CH2_CFG4, 0x00 },
	{ ADCX120_CH3_CFG0, 0x00 },
	{ ADCX120_CH3_CFG1, 0x00 },
	{ ADCX120_CH3_CFG2, 0xc9 },
	{ ADCX120_CH3_CFG3, 0x80 },
	{ ADCX120_CH3_CFG4, 0x00 },
	{ ADCX120_CH4_CFG0, 0x00 },
	{ ADCX120_CH4_CFG1, 0x00 },
	{ ADCX120_CH4_CFG2, 0xc9 },
	{ ADCX120_CH4_CFG3, 0x80 },
	{ ADCX120_CH4_CFG4, 0x00 },
//	{ ADCX120_CH5_CFG2, 0xc9 },
//	{ ADCX120_CH5_CFG3, 0x80 },
//	{ ADCX120_CH5_CFG4, 0x00 },
//	{ ADCX120_CH6_CFG2, 0xc9 },
//	{ ADCX120_CH6_CFG3, 0x80 },
//	{ ADCX120_CH6_CFG4, 0x00 },
//	{ ADCX120_CH7_CFG2, 0xc9 },
//	{ ADCX120_CH7_CFG3, 0x80 },
//	{ ADCX120_CH7_CFG4, 0x00 },
//	{ ADCX120_CH8_CFG2, 0xc9 },
//	{ ADCX120_CH8_CFG3, 0x80 },
//	{ ADCX120_CH8_CFG4, 0x00 },
	{ ADCX120_DSP_CFG0, 0x01 },
	{ ADCX120_DSP_CFG1, 0x40 },
	{ ADCX120_DRE_CFG0, 0x7b },
	{ ADCX120_AGC_CFG0, 0xe7 },
	{ADCX120_GAIN_CFG, 0},
	{ ADCX120_IN_CH_EN, 0xc0 },
	{ ADCX120_ASI_OUT_CH_EN, 0x00 },
	{ ADCX120_PWR_CFG, 0x00 },
	{ ADCX120_DEV_STS0, 0x00 },
	{ ADCX120_DEV_STS1, 0x80 },
};


static bool write_reg(adcx120_t *pdev, uint8_t addr, uint8_t v)
{
	const uint8_t LEN = 2;
	uint8_t data[LEN];
	data[0] = addr;
	data[1] = v;
	int r =  HAL_I2C_Master_Transmit(pdev->pI2C, pdev->I2C_addr, data, LEN, 10);
	return (r == HAL_OK);
}



static bool read_reg(adcx120_t *pdev, uint8_t addr, uint8_t* buf, size_t n)
{
	uint8_t r;
	r = HAL_I2C_Master_Transmit(pdev->pI2C, pdev->I2C_addr, &addr, 1, 10);
	if(r != HAL_OK ) goto READ_REG_FAILED;
	r = HAL_I2C_Master_Receive(pdev->pI2C, pdev->I2C_addr, buf, n, 10);
	if(r != HAL_OK ) goto READ_REG_FAILED;
	return true;
READ_REG_FAILED:
	return false;
}

bool adcx120_set_reg(adcx120_t *pdev, uint8_t regaddr, uint8_t mask, uint8_t v)
{
	uint8_t *pReg = pdev->regpage0 + regaddr;

	*pReg = (*pReg & (~mask)) | (v & mask);

	return write_reg(pdev, regaddr, *pReg);
}

static void delay_ms(int ms)
{
//	osDelay(pdMS_TO_TICKS(ms));
	HAL_Delay(ms);
}

int adcx120_reset(adcx120_t *pdev)
{
	uint8_t ret = 0;


	ret = write_reg(pdev, ADCX120_SW_RESET, ADCX120_RESET);

	/* 8.4.2: wait >= 10 ms after entering sleep mode. */


	return ret;
}

static uint8_t stored_volumes[4] = {0};
void adcx120_set_volume(adcx120_t *pdev, uint8_t ch, uint8_t volume_100)
{
	const uint8_t regaddrs[] = {ADCX120_CH1_CFG2,ADCX120_CH2_CFG2,ADCX120_CH3_CFG2,ADCX120_CH4_CFG2};
//	const int16_t volume_0db = 201; // volume_100  = 100 -> 201,  volume_100 = 0 -> 0
	// 0: muted, 1: -100db, 201: 0db volume, >201 +db volume
	if(ch >= 1 && ch <= ADCX120_CH_NUM)
	{
		uint8_t regaddr = regaddrs[ch - 1];
		write_reg(pdev, regaddr,  ((volume_100 << 1) + 1));
		stored_volumes[ch - 1] = ((volume_100 << 1) + 1);
	}
}


uint8_t adcx120_set_volume_async(adcx120_t *pdev, uint8_t ch, uint8_t volume_100, /* out */ uint8_t* buf)
{
	uint8_t i = 0;
	const uint8_t regaddrs[] = {ADCX120_CH1_CFG2,ADCX120_CH2_CFG2,ADCX120_CH3_CFG2,ADCX120_CH4_CFG2};
//	const int16_t volume_0db = 201; // volume_100  = 100 -> 201,  volume_100 = 0 -> 0
	// 0: muted, 1: -100db, 201: 0db volume, >201 +db volume
	if(ch >= 1 && ch <= ADCX120_CH_NUM)
	{
		uint8_t regaddr = regaddrs[ch - 1];
//		write_reg(pdev, regaddr,  ((volume_100 << 1) + 1));
		buf[i++] = regaddr;
		buf[i++] = (volume_100 << 1) + 1;
		stored_volumes[ch - 1] = ((volume_100 << 1) + 1);
	}
	return i;
}

void adcx120_mute(adcx120_t *pdev, uint8_t ch, uint8_t mute)
{
	const uint8_t regaddrs[] = {ADCX120_CH1_CFG2,ADCX120_CH2_CFG2,ADCX120_CH3_CFG2,ADCX120_CH4_CFG2};
	if(ch >= 1 && ch <= ADCX120_CH_NUM)
	{
		uint8_t regaddr = regaddrs[ch - 1];
		if(mute > 0)
		{
			write_reg(pdev, regaddr, 0); // 0 - muted
		}
		else //
		{
			// TODO: write_reg(pdev, regaddr, 0);
			write_reg(pdev, regaddr, stored_volumes[ch-1]); // 0 - muted
		}
	}
}


uint8_t adcx120_mute_async(adcx120_t *pdev, uint8_t ch, uint8_t mute, /* out */ uint8_t* buf)
{
	uint8_t i = 0;
	const uint8_t regaddrs[] = {ADCX120_CH1_CFG2,ADCX120_CH2_CFG2,ADCX120_CH3_CFG2,ADCX120_CH4_CFG2};

	static uint8_t stored_volumes[4] = {0};
	static uint8_t previous_cmds[4] = {0};

	if(ch >= 1 && ch <= ADCX120_CH_NUM)
	{
		uint8_t regaddr = regaddrs[ch - 1];
		buf[i++] = regaddr;

		if(mute > 0)
		{
//			write_reg(pdev, regaddr, 0); // 0 - muted
			buf[i++] = 0;
		}
		else //
		{
			// TODO: write_reg(pdev, regaddr, 0);
//			write_reg(pdev, regaddr, stored_volumes[ch-1]); // 0 - muted
			buf[i++] = stored_volumes[ch-1];
		}
	}
	return i;
}


void adcx120_set_gain(adcx120_t *pdev, uint8_t ch, uint8_t gain_db2)
{
	const uint8_t regaddrs[] = {ADCX120_CH1_CFG1,ADCX120_CH2_CFG1,ADCX120_CH3_CFG1,ADCX120_CH4_CFG1};
	const uint8_t mask = (uint8_t)(0xffU << 1);
	if(gain_db2 > 84)
		gain_db2 = 84;
	if(ch >= 1 && ch <= ADCX120_CH_NUM)
	{
		uint8_t regaddr = regaddrs[ch - 1];
		adcx120_set_reg(pdev, regaddr, mask, gain_db2);
	}
}


uint8_t adcx120_set_gain_async(adcx120_t *pdev, uint8_t ch, uint8_t gain_db2,  /* out 2B */ uint8_t* buf)
{
	uint8_t i = 0;
	const uint8_t regaddrs[] = {ADCX120_CH1_CFG1,ADCX120_CH2_CFG1,ADCX120_CH3_CFG1,ADCX120_CH4_CFG1};
	const uint8_t mask = (uint8_t)(0xffU << 1);
	if(gain_db2 > 84)
		gain_db2 = 84;
	if(ch >= 1 && ch <= ADCX120_CH_NUM)
	{
		uint8_t regaddr = regaddrs[ch - 1];
		buf[i++] = regaddr;
		buf[i++] = gain_db2 & mask;
//		adcx120_set_reg(pdev, regaddr, mask, gain_db2);
	}
	return i;
}



static void _adcx120_set_power(adcx120_t *pdev, uint8_t mask, uint8_t v)
{
	const uint8_t regaddr = ADCX120_PWR_CFG;
	uint8_t *pReg = pdev->regpage0 + regaddr;

	*pReg = (*pReg & (~mask)) | (v & mask);
	write_reg(pdev, regaddr, *pReg);

//	write_reg(pdev, ADCX120_PWR_CFG, v & mask);
}


void adcx120_set_power(adcx120_t *pdev, uint8_t state)
{
	_adcx120_set_power(pdev, ADCX120_PWR_CFG_ADC_PDZ | ADCX120_PWR_CFG_BIAS_PDZ | ADCX120_PWR_CFG_PLL_PDZ, state);
}





void adcx120_set_sleep(adcx120_t *pdev, uint8_t awake)
{
	const uint8_t regaddr = ADCX120_SLEEP_CFG;
	uint8_t *pReg = pdev->regpage0 + regaddr;

	// set  AREG bit, to use internal 1.8V regulator when wake up
	uint8_t mask = 1U << 7;
	if(awake > 0)
	{
		*pReg = (*pReg & (~mask)) | (0xff & mask);
	}
	else // turn off AREG when sleep
	{
		*pReg = (*pReg & (~mask)) | (0);
	}
	// set SLEEP_ENZ bit at LSB(bit0), 0 -> sleep, 1->normal
	mask = 1U;
	*pReg = (*pReg & (~mask)) | (awake & mask);

	write_reg(pdev, regaddr, *pReg);

}


void adcx120_set_format(adcx120_t *pdev, uint8_t format, uint8_t bitdepth, uint8_t samplerate)
{
	// 1： set data format
	uint8_t regaddr = ADCX120_ASI_CFG0;
	uint8_t mask = 0b11U << 6;
	uint8_t *pReg = pdev->regpage0 + regaddr;
	*pReg = (*pReg & (~mask)) | ((format << 6) & mask);

	// 1.1: set bit depth
	mask = 0b11U << 4;
	uint8_t bitdepth_flag;
	switch(bitdepth)
	{
	case 16:
		bitdepth_flag = 0; break;
	case 20:
		bitdepth_flag = 1; break;
	case 24:
		bitdepth_flag = 2; break;
	case 32:
		bitdepth_flag = 3; break;
	default:
		bitdepth_flag = 0;
	}
	*pReg = (*pReg & (~mask)) | ((bitdepth_flag << 4) & mask);
	write_reg(pdev, regaddr, *pReg);

	// 2： set sampling rate
	regaddr = ADCX120_MST_CFG1;
	mask = 0b1111U << 4;
	pReg = pdev->regpage0 + regaddr;
	*pReg = (*pReg & (~mask)) | ((samplerate << 4) & mask);
	write_reg(pdev, regaddr, *pReg);
}

void adcx120_set_channel_enable(adcx120_t *pdev, uint8_t ch, uint8_t state)
{

	uint8_t regaddr = ADCX120_IN_CH_EN;
	uint8_t *pReg = pdev->regpage0 + regaddr;
	uint8_t mask = 1U << (8-ch);
	if(ch > 0 && ch <= ADCX120_CH_NUM)
	{
		*pReg = (*pReg & (~mask)) | ((state << (8 - ch)) & mask);
		write_reg(pdev, regaddr, *pReg);
	}

	regaddr = ADCX120_ASI_OUT_CH_EN;
	pReg = pdev->regpage0 + regaddr;
	mask = 1U << (8-ch);
	if(ch > 0 && ch <= ADCX120_CH_NUM)
	{
		*pReg = (*pReg & (~mask)) | ((state << (8 - ch)) & mask);
		write_reg(pdev, regaddr, *pReg);
	}

}

void adcx120_awake(adcx120_t *pdev)
{
	/* awake the device */
	uint8_t vSleep = 0;
	adcx120_set_sleep(pdev, 0xff);
	read_reg(pdev, ADCX120_SLEEP_CFG, &vSleep, 1);
	// need a 50ms delay
}

void adcx120_init(adcx120_t *pdev,  adcx120_init_t* pcfg)
{
	uint8_t mask;

	const int nregs = sizeof(regdefaults) / sizeof(reg_default_t);
	for(int ireg = 0; ireg < nregs; ++ireg)
	{
		uint8_t addr = regdefaults[ireg].regaddr;
		uint8_t v = regdefaults[ireg].v;
		pdev->regpage0[addr] = v;
	}


	/* master or slave*/
	uint8_t vMST;
	read_reg(pdev, ADCX120_MST_CFG0, &vMST, 1);

	/* GPIO1 as MCLK input */
	uint8_t vGPIO0 = 0;
	vGPIO0 |= (10U << 4); // 10d: GPIO0 as MCLK input
	vGPIO0 |= 0; // 0: GPIO0 as Hi-Z
	adcx120_set_reg(pdev, ADCX120_GPIO_CFG0, 0b11110111U, vGPIO0);
	read_reg(pdev,ADCX120_GPIO_CFG0, &vGPIO0, 1 );

	/* format */
	// format: I2S
	// 32bit
	// 96kHZ
	adcx120_set_format(pdev, ADCX120_FMT_I2S, 32, ADCX120_FS_88K2_OR_96K);



//	/* PLL */
//	uint8_t vCLK_SRC = 0;
//	mask = 0b10001000U;
//	vCLK_SRC = (1U) << 7; //MCLK (GPIO or GPIx) is used as the audio root clock source
//	vCLK_SRC |= (1U << 3); // MCLK to FS ratio is 256 (24576/96 = 256)
//	// MCLK frequency is based on the MCLK_FREQ_SEL (P0_R19)
////	vCLK_SRC = (1U) << 6;
//	adcx120_set_reg(pdev, ADCX120_CLK_SRC, mask, vCLK_SRC);
//	read_reg(pdev,ADCX120_CLK_SRC, &vCLK_SRC, 1 );

	/* bias */
	// MICBIAS: off
	// Coupling: AC Differential
	// ADC Vref: 1.25V
	uint8_t vBIAS_CFG = 0;
	mask = 0b11U;
	// VREF is set to 1.375 V to support 1 VRMS for the differential input or 0.5 VRMS for the single-ended input
	vBIAS_CFG |= (2U);
	adcx120_set_reg(pdev, ADCX120_BIAS_CFG, mask, vBIAS_CFG);


	/* set I2S channel to slot assignment*/
	mask = 0b00111111U;
	uint8_t vASI_CH1 = 32U;// put CH1(MIC2) on R-ch
	adcx120_set_reg(pdev, ADCX120_ASI_CH1, mask, vASI_CH1);
	uint8_t vASI_CH2 = 0U;// put CH2(MIC1) on L-ch
	adcx120_set_reg(pdev, ADCX120_ASI_CH2, mask, vASI_CH2);

	/* set preamp gain on-the-fly */
	adcx120_set_reg(pdev, ADCX120_GAIN_CFG, 0b11000000, 0xff); //max soft on-the-fly gain change
	/* set preamp gain */
	uint8_t vCHx_CFG1 = 0;
	uint8_t preamp_gain = 18U; // +18db
	mask = (uint8_t)(0xffU << 1);
	vCHx_CFG1 |= (pcfg->pregain_db2[0]);
	adcx120_set_reg(pdev, ADCX120_CH1_CFG1, mask, vCHx_CFG1);
	vCHx_CFG1 |= (pcfg->pregain_db2[1]);
	adcx120_set_reg(pdev, ADCX120_CH2_CFG1, mask, vCHx_CFG1);


	/* set inital volume */
	adcx120_set_volume(pdev, 1, pcfg->volume100[0]);
	adcx120_set_volume(pdev, 2, pcfg->volume100[1]);

	/* enable channels */
	adcx120_set_channel_enable(pdev, 1, 1);
	adcx120_set_channel_enable(pdev, 2, 1);
	adcx120_set_channel_enable(pdev, 3, 0);
	adcx120_set_channel_enable(pdev, 4, 0);
	/* power up  ADC, MICBIAS, and PLL */
	adcx120_set_power(pdev, 0xff);


}

int adcx120_test(adcx120_t *pdev)
{
	int r = 0;
	uint8_t v = 0;
	// check channel in enable MSB.CH1.CH2....
	r = read_reg(pdev, ADCX120_IN_CH_EN, &v, 1);
	// check channel out enable MSB
	r = read_reg(pdev, ADCX120_ASI_OUT_CH_EN, &v, 1);
	// check ADC, MICBIAS and PLL power
	r = read_reg(pdev, ADCX120_PWR_CFG, &v, 1);
	delay_ms(10);
	// check the power on stats of each channel
	r = read_reg(pdev, ADCX120_DEV_STS0, &v, 1); // CAN'T BE 0
	r = read_reg(pdev, ADCX120_DEV_STS1, &v, 1); // MSB 3bits, 4: sleep, 6: active but all channels turned off 7: partially on
	// check volume
	r = read_reg(pdev, ADCX120_CH1_CFG2, &v, 1);
	// check gain
	r = read_reg(pdev, ADCX120_CH1_CFG1, &v, 1);
	return r;

}
