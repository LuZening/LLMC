#ifndef _FLAC_DECODER_H
#define _FLAC_DECODER_H
 
#include "FLAC/bitstreamf.h"
#ifndef DECODER_SIM
#include "ff.h"
#else
#include <stdio.h>
#define FIL FILE
#endif
/* Maxsize in samples of one uncompressed frame
 * 这里我测试的level8文件都是4608
 * 再大内部64k也不够
 */
#define MAX_BLOCKSIZE 8192

#define MAX_FRAMESIZE 32*1024  /* Maxsize in bytes of one compressed frame */
#define FLAC_OUTPUT_DEPTH 32   /* Provide samples left-shifted to 28 bits+sign */
#define FLAC_IN_BUFFER_SIZE (4 * MAX_BLOCKSIZE)
enum decorrelation_type {
    INDEPENDENT,
    LEFT_SIDE,
    RIGHT_SIDE,
    MID_SIDE,
};


//#define INDEPENDENT  0
//#define LEFT_SIDE    1
//#define RIGHT_SIDE   2
//#define MID_SIDE     3


typedef struct FLACContext {
    GetBitContext gb;

    int min_blocksize, max_blocksize; // decompressed size of the raw audio stream (unit is sample), by default 4096 samples, sometimes 1152 samples
    int min_framesize, max_framesize; // compressed size for each frame
    int samplerate, channels;
    int blocksize;  // last_blocksize
    int bps, curr_bps; // bitdepth (per sample, i.e. 16,24,32)
    unsigned long samplenumber;
    unsigned long totalsamples;
    enum decorrelation_type decorrelation;
//	int decorrelation;

    int filesize;
    int length; // time unit: millisecond(ms)
    int bitrate; // 455kbps
    int metadatalength;
    
	int seektable;
	int seekpoints;
	
    int bitstream_size;
    int bitstream_index;

    int sample_skip;
    int framesize;

    int *decoded0;  // channel 0
    int *decoded1;  // channel 1
} FLACContext;

void init_flac_context(FLACContext* p, int* pBuf0, int* pBuf1);

void flac_dump_headers(FLACContext* s);
int parse_flac(FIL *fp, FLACContext* fc, uint8_t* bufFileLoader, size_t sizeBuf);

int flac_decode_frame(FLACContext *s, uint8_t *buf, int buf_size, int32_t *wavbuf, size_t* pcountDecoded) ICODE_ATTR_FLAC;

#endif
