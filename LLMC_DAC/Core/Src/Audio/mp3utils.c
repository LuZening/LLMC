/*
 * mp3utils.c
 *
 *  Created on: 2024年11月20日
 *      Author: cpholzn
 */

#include "Audio/mp3utils.h"
#include "Helix/mp3dec.h"
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#ifndef DECODER_SIM
#include "ff.h"
#else
#include <stdio.h>
#include "sim.h"
#endif



// skip mp3 header
int parse_mp3_header(FIL* fp, uint8_t* bufFileLoader, size_t sizeBuf, size_t* firstFrame)
{
	const size_t BYTES_INITIAL = 10;
	int r = 0;
	uint8_t* readBuf = bufFileLoader;
	uint32_t head_size = 0;

	size_t file_size = f_size(fp);
	//	printf("MP3 File Size: %d \r\n", file_size);
	size_t len = 0;
	int fr = 0;
	fr = f_read(fp, readBuf, BYTES_INITIAL, &len);
	if ((fr != FR_OK) || (len < BYTES_INITIAL))
	{
		r = -1;
		printf("failed to open mp3 file");
		goto MP3_PARSE_FAILED;
	}

	/* locate header */
	//CASE: with ID3 header, parse the length of the header first and then skip it
	if(memcmp(readBuf, "ID3", 3) >= 0)
	{
		head_size |= readBuf[6]; head_size <<= 7;
		head_size |= readBuf[7]; head_size <<= 7;
		head_size |= readBuf[8]; head_size <<= 7;
		head_size |= readBuf[9];
		//printf("MP3 ID3V2 Size :%d \r\n", head_size);
		head_size += BYTES_INITIAL;
		file_size -= head_size; // mp3 data size
		fr = f_lseek(fp, head_size);
		fr = f_read(fp, readBuf, sizeBuf, &len);
		if ((fr != FR_OK) || (len == 0))
		{
			r = -2;
			//printf("no content after ID3 header");
			goto MP3_PARSE_FAILED;
		}
	}
	//CASE: no ID3 header, the whole file body is the main content
	else
	{
		//printf("MP3 no ID3V2, %02x %02x %02x \r\n", readBuf[0], readBuf[1], readBuf[2]);
		f_rewind(fp);
		fr = f_read(fp, readBuf, sizeBuf, &len);
		head_size = 0;
		if ((fr != FR_OK) || (len == 0))
		{
			r = -3;
			//printf("no content after header");
			goto MP3_PARSE_FAILED;
		}
		//printf("Read res:%d,len: %d \r\n", res, len);
	}
	size_t bytesLeft = len;
	size_t have_read_size = len;
	uint8_t* readPtr = readBuf;
	int offset = MP3FindSyncWord(readPtr, bytesLeft);
	if (offset < 0)
	{
		//printf("MP3 find syncword Fail!\r\n");
		r = -4;
		goto MP3_PARSE_FAILED;
	}
	else
	{
		// parse succeeded
		head_size += offset;
		*firstFrame = head_size;
		f_lseek(fp, head_size);
	}
MP3_PARSE_FAILED:
	return r;
}
