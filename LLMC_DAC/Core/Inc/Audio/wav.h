/*
 * wav.h
 *
 *  Created on: Apr 6, 2024
 *      Author: cpholzn
 */

#ifndef INC_AUDIO_WAV_H_
#define INC_AUDIO_WAV_H_
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
typedef struct
{
	/* 44 BEGIN */
	// 16
	char fileformat[4]; // "RIFF"
	uint32_t file_size; // len(file) - 8
	char subformat[4]; // "WAVE"
	char subformat_id[4];
	// 16
	uint16_t audio_format;    					// little or big endian
	uint16_t num_channels;     				// 2 here for left and right
	uint32_t sample_rate;						// sample_rate denotes the sampling rate.
	// 12
	uint32_t byte_rate;           					// bytes  per second
	uint16_t bytes_per_frame;  						// bytes per frame = bitdepth x nChannels / 8
	uint16_t bits_per_sample;
	/* 40 END */

	uint32_t data_size;
	uint32_t data_begin;

}  wav_header_t;

#define MIN_LEN_WAV_HEADER 40U

int wav_parse_header(wav_header_t* ph, const uint8_t* buf, size_t len);
uint32_t wav_frame_size(uint8_t nchannels, uint32_t bitdepth);
#define WAV_SAMPLE_SIZE(nCh, bitdepth) ((bitdepth << 3) * nCh);

#endif /* INC_AUDIO_WAV_H_ */
