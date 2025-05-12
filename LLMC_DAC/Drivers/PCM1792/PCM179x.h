/*
 * PCM179x.h
 *
 *  Created on: Mar 13, 2024
 *      Author: cpholzn
 */

#ifndef PCM1792_PCM179X_H_
#define PCM1792_PCM179X_H_

#include <stdint.h>

#define PCM179X_I2C_ADDR (0b1001111U << 1) // STM32 requires shift 1

#define PCM179X_REG_DIGITAL_ATTENUATION_CONTROL_L 16U // reg16
#define PCM179X_REG_DIGITAL_ATTENUATION_CONTROL_R 17U // reg17
#define PCM179X_REG_DEVICE_ID 23U // reg23

#define PCM179X_REG_18 0b0010010U // multiple purposes: ATLD(enable or disable Attenuation refresh) FMT DMF DME SoftMUTE
#define PCM179X_REG_19 0b0010011U // multiple purposes: REV(phase reversal) ATS(Attenuation speed) OPE(DAC operation control) DFMS() FLT(filter rolloff) INZD
#define PCM179X_REG_20 0b0010100U // multiple purposes: SRST(sys reset) DSD DFTH(filter bypass) MONO CHSL(L or R for mono) OS(oversampling ratio)
#define PCM179X_REG_21 0b0010101U // multiple purposes: PCMZ(PCM zero output) DZ
#define PCM179X_REG_22 0b0010110U // multiple purposes: PCMZ(PCM zero output) DZ
#define PCM179X_REG_23 0b0010111U // ZERO detection flags

#define PCM179X_FMT_16BIT_RJ 0U
#define PCM179X_FMT_24BIT_RJ 0b101U
#define PCM179X_FMT_24BIT_LJ 0b011U
#define PCM179X_FMT_16BIT_I2S 0b100U
#define PCM179X_FMT_24BIT_I2S 0b101U // default
#define PCM179X_FMT_32BIT_I2S 0b101U // same as 24bit

typedef struct  {
	uint8_t ID;
	uint16_t addr_i2c;
	uint32_t fclk_Hz;
	uint8_t bitdepth;

	uint8_t regvals[6];

	uint32_t rate;


}	pcm179x_t;

extern pcm179x_t pcm179x;

int PCM179x_write_reg(pcm179x_t *p,  uint8_t addr, uint8_t d);

int PCM179x_read_reg(pcm179x_t *p, uint8_t addr, uint8_t* pd);

int PCM179x_set_attenuation(pcm179x_t *p, uint8_t loudness);
uint8_t PCM179x_set_attenuation_async(pcm179x_t *p, uint8_t loudness, /* out 4 bytes */ uint8_t* buf);

int PCM179x_set_format(pcm179x_t *p, uint8_t format);

int PCM179x_set_mute(pcm179x_t *p, uint8_t mute);
uint8_t PCM179x_set_mute_async(pcm179x_t *p, uint8_t mute, /* buf 2 bytes */ uint8_t* buf);

int PCM179x_set_sample_rate(pcm179x_t *p,  uint32_t fs_Hz);

int PCM179x_set_oversample_multiplier(pcm179x_t *p, uint16_t mult);


void pcm179x_hw_reset();

int pcm179x_test(pcm179x_t *p);

int pcm179x_init(pcm179x_t *p, uint32_t fclk_Hz);

#endif /* PCM1792_PCM179X_H_ */
