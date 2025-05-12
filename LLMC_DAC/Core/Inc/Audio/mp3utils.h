/*
 * mp3utils.h
 *
 *  Created on: 2024年11月20日
 *      Author: cpholzn
 */

#ifndef INC_AUDIO_MP3UTILS_H_
#define INC_AUDIO_MP3UTILS_H_
#ifdef DECODER_SIM
#include "sim.h"
#else
#include "ff.h"
#endif
#include <stdint.h>
int parse_mp3_header(FIL* fp, uint8_t* bufFileLoader, size_t sizeBuf, size_t* firstFrame);

#endif /* INC_AUDIO_MP3UTILS_H_ */
