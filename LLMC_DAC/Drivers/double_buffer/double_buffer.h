#pragma once

#include <string.h>
#include <stdint.h>
#include "kfifo_DMA.h"

typedef struct
{
//    size_t nQuartersIn[4];
	uint8_t *buf;
    size_t size;
    size_t size_half;

    uint8_t *pIn;
    uint8_t *pHalf;
    uint8_t *pOut;


} double_buffer_t;


void double_buffer_init(double_buffer_t* p, uint8_t* buf, size_t size);

int double_buffer_in_transfer_HalfCplt_cb(double_buffer_t* p);
int double_buffer_in_transfer_Cplt_cb(double_buffer_t* p);
int double_buffer_out_transfer_HalfCplt_cb(double_buffer_t* p);
int double_buffer_out_transfer_Cplt_cb(double_buffer_t* p);

