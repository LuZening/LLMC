/*
 * TLV320ADCx120.h
 *
 *  Created on: Mar 13, 2024
 *      Author: cpholzn
 */

#ifndef TLV320ADCX120_TLV320ADCX120_H_
#define TLV320ADCX120_TLV320ADCX120_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include"main.h"

#define ADCX120_I2C_ADDR (0b1001110U << 1U)

extern I2C_HandleTypeDef hi2c1;


#define ADCX120_CH_NUM 2
/* register address BEGIN */
#define ADCX120_PAGE_SELECT	0x00
#define ADCX120_SW_RESET	0x01
#define ADCX120_SLEEP_CFG 	0x02
#define ADCX120_SHDN_CFG	0x05
#define ADCX120_ASI_CFG0	0x07
#define ADCX120_ASI_CFG1	0x08
#define ADCX120_ASI_CFG2	0x09
#define ADCX120_ASI_CH1		0x0b
#define ADCX120_ASI_CH2		0x0c
#define ADCX120_ASI_CH3		0x0d
#define ADCX120_ASI_CH4		0x0e
//#define ADCX140_ASI_CH5		0x0f
//#define ADCX140_ASI_CH6		0x10
//#define ADCX140_ASI_CH7		0x11
//#define ADCX140_ASI_CH8		0x12
#define ADCX120_MST_CFG0	0x13
#define ADCX120_MST_CFG1	0x14
#define ADCX120_ASI_STS		0x15
#define ADCX120_CLK_SRC		0x16
#define ADCX120_PDMCLK_CFG	0x1f
#define ADCX120_PDMIN_CFG	0x20
#define ADCX120_GPIO_CFG0	0x21
#define ADCX120_GPO_CFG0	0x22
//#define ADCX140_GPO_CFG1	0x23
//#define ADCX140_GPO_CFG2	0x24
//#define ADCX140_GPO_CFG3	0x25
#define ADCX120_GPO_VAL		0x29
#define ADCX120_GPIO_MON	0x2a
#define ADCX120_GPI_CFG0	0x2b
//#define ADCX140_GPI_CFG1	0x2c
#define ADCX120_GPI_MON		0x2f
#define ADCX120_INT_CFG		0x32
#define ADCX120_INT_MASK0	0x33
#define ADCX120_INT_LTCH0	0x36
#define ADCX120_CM_TOL_CFG	0x3a // ADC common mode configuration register
#define ADCX120_BIAS_CFG	0x3b
#define ADCX120_CH1_CFG0	0x3c
#define ADCX120_CH1_CFG1	0x3d
#define ADCX120_CH1_CFG2	0x3e
#define ADCX120_CH1_CFG3	0x3f
#define ADCX120_CH1_CFG4	0x40
#define ADCX120_CH2_CFG0	0x41
#define ADCX120_CH2_CFG1	0x42
#define ADCX120_CH2_CFG2	0x43
#define ADCX120_CH2_CFG3	0x44
#define ADCX120_CH2_CFG4	0x45
#define ADCX120_CH3_CFG0	0x46
#define ADCX120_CH3_CFG1	0x47
#define ADCX120_CH3_CFG2	0x48
#define ADCX120_CH3_CFG3	0x49
#define ADCX120_CH3_CFG4 	0x4a
#define ADCX120_CH4_CFG0	0x4b
#define ADCX120_CH4_CFG1	0x4c
#define ADCX120_CH4_CFG2	0x4d
#define ADCX120_CH4_CFG3	0x4e
#define ADCX120_CH4_CFG4	0x4f
//#define ADCX140_CH5_CFG2	0x52
//#define ADCX140_CH5_CFG3	0x53
//#define ADCX140_CH5_CFG4	0x54
//#define ADCX140_CH6_CFG2	0x57
//#define ADCX140_CH6_CFG3	0x58
//#define ADCX140_CH6_CFG4	0x59
//#define ADCX140_CH7_CFG2	0x5c
//#define ADCX140_CH7_CFG3	0x5d
//#define ADCX140_CH7_CFG4	0x5e
//#define ADCX140_CH8_CFG2	0x61
//#define ADCX140_CH8_CFG3	0x62
//#define ADCX140_CH8_CFG4	0x63
#define ADCX120_DSP_CFG0	0x6b
#define ADCX120_DSP_CFG1	0x6c
#define ADCX120_DRE_CFG0	0x6d
#define ADCX120_AGC_CFG0	0x70
#define ADCX120_GAIN_CFG	0x71
#define ADCX120_IN_CH_EN	0x73
#define ADCX120_ASI_OUT_CH_EN	0x74
#define ADCX120_PWR_CFG		0x75
#define ADCX120_DEV_STS0	0x76
#define ADCX120_DEV_STS1	0x77
//#define ADCX120_PHASE_CALIB		0X7b
#define ADCX120_I2C_CKSUM	0x7e

#define ADCX120_RESET	BIT(0)

#define ADCX120_WAKE_DEV	BIT(0)
#define ADCX120_AREG_INTERNAL	BIT(7)

#define ADCX120_BCLKINV_BIT	BIT(2)
#define ADCX120_FSYNCINV_BIT	BIT(3)
#define ADCX120_INV_MSK		(ADCX120_BCLKINV_BIT | ADCX120_FSYNCINV_BIT)
#define ADCX120_BCLK_FSYNC_MASTER	BIT(7)
#define ADCX120_I2S_MODE_BIT	BIT(6)
#define ADCX120_LEFT_JUST_BIT	BIT(7)
#define ADCX120_ASI_FORMAT_MSK	(ADCX120_I2S_MODE_BIT | ADCX120_LEFT_JUST_BIT)

#define ADCX120_16_BIT_WORD	0x0
#define ADCX120_20_BIT_WORD	BIT(4)
#define ADCX120_24_BIT_WORD	BIT(5)
#define ADCX120_32_BIT_WORD	(BIT(4) | BIT(5))
#define ADCX120_WORD_LEN_MSK	0x30

#define ADCX120_MAX_CHANNELS	8

#define ADCX120_MIC_BIAS_VAL_VREF	0
#define ADCX120_MIC_BIAS_VAL_VREF_1096	1
#define ADCX120_MIC_BIAS_VAL_AVDD	6
//#define ADCX120_MIC_BIAS_VAL_MSK GENMASK(6, 4)
#define ADCX120_MIC_BIAS_SHIFT		4

#define ADCX120_MIC_BIAS_VREF_275V	0
#define ADCX120_MIC_BIAS_VREF_25V	1
#define ADCX120_MIC_BIAS_VREF_1375V	2
//#define ADCX120_MIC_BIAS_VREF_MSK GENMASK(1, 0)
//
//#define ADCX120_PWR_CTRL_MSK    GENMASK(7, 5)
#define ADCX120_PWR_CFG_BIAS_PDZ	BIT(7)
#define ADCX120_PWR_CFG_ADC_PDZ		BIT(6)
#define ADCX120_PWR_CFG_PLL_PDZ		BIT(5)

//#define ADCX120_TX_OFFSET_MASK		GENMASK(4, 0)

#define ADCX120_NUM_PDM_EDGES		4
#define ADCX120_PDM_EDGE_SHIFT		7

#define ADCX120_NUM_GPI_PINS		2
#define ADCX120_GPI_SHIFT		4
#define ADCX120_GPI1_INDEX		0
#define ADCX120_GPI2_INDEX		1
#define ADCX120_GPI3_INDEX		2
#define ADCX120_GPI4_INDEX		3

#define ADCX120_NUM_GPOS		4
#define ADCX120_NUM_GPO_CFGS		2
#define ADCX120_GPO_SHIFT		4
#define ADCX120_GPO_CFG_MAX		4
#define ADCX120_GPO_DRV_MAX		5

#define ADCX120_TX_FILL    BIT(0)

#define ADCX120_NUM_GPIO_CFGS		2
#define ADCX120_GPIO_SHIFT		4
#define ADCX120_GPIO_CFG_MAX		15
#define ADCX120_GPIO_DRV_MAX		5
/* register address END */

#define ADCX120_FMT_TDM 0U
#define ADCX120_FMT_I2S 1U
#define ADCX120_FMT_LJ 2U

#define ADCX120_FS_44K1_OR_48K 4U
#define ADCX120_FS_88K2_OR_96K 5U
#define ADCX120_FS_176K4_OR_192K 6U



typedef struct {


	uint16_t I2C_addr;


	uint8_t format;
	uint8_t bitdepth;
	uint8_t samplerate;
	uint8_t regpage0[256]; // 256bytes
	I2C_HandleTypeDef* pI2C;
} adcx120_t;

typedef struct {
	uint8_t pregain_db2[2];
	uint8_t volume100[2];
} adcx120_init_t;

extern adcx120_t adcx120;
int adcx120_reset(adcx120_t *pdev); /* 8.4.2: wait >= 10 ms after entering sleep mode. */
void adcx120_awake(adcx120_t *pdev); // need a 50ms delay after
void adcx120_init(adcx120_t *pdev, adcx120_init_t* pcfg); // call after awaken

int adcx120_test(adcx120_t *pdev);

bool adcx120_set_reg(adcx120_t *pdev, uint8_t regaddr, uint8_t mask, uint8_t v);


void adcx120_set_format(adcx120_t *pdev, uint8_t format, uint8_t bitdepth, uint8_t samplerate);

void adcx120_set_power(adcx120_t *pdev, uint8_t state);
void adcx120_set_sleep(adcx120_t *pdev, uint8_t awake);
void adcx120_set_channel_enable(adcx120_t *pdev, uint8_t ch, uint8_t state);
void adcx120_set_volume(adcx120_t *pdev, uint8_t ch, uint8_t volume_100);
uint8_t adcx120_set_volume_async(adcx120_t *pdev, uint8_t ch, uint8_t volume_100, /* out 2 bytes */ uint8_t* buf);
void adcx120_mute(adcx120_t *pdev, uint8_t ch, uint8_t mute);
uint8_t adcx120_mute_async(adcx120_t *pdev, uint8_t ch, uint8_t mute, /* out 2bytes*/ uint8_t* buf);
void adcx120_set_gain(adcx120_t *pdev, uint8_t ch, uint8_t gain_db2);
uint8_t adcx120_set_gain_async(adcx120_t *pdev, uint8_t ch, uint8_t gain_db2,  /* out 2bytes */ uint8_t* buf);
#endif /* TLV320ADCX120_TLV320ADCX120_H_ */
