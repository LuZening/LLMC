#include "double_buffer.h"


void double_buffer_init(double_buffer_t* p, uint8_t* buf, size_t size)
{
	p->buf = buf;
	memset(buf, 0, size);
	p->size = size;
	p->size_half = size >> 1;
    
	p->pIn = buf;
	p->pHalf = buf + p->size_half;
	p->pOut = buf;
	//    kfifo_DMA_static_init(&p->fifoOut, bufOut, (32U - __clz(lenBufOut) - 1U));

}


int double_buffer_in_transfer_HalfCplt_cb(double_buffer_t* p)
{
	int r = 0;

	p->pIn = p->pHalf;
	p->pOut = p->buf;

	return r;
}

int double_buffer_in_transfer_Cplt_cb(double_buffer_t* p)
{
	int r = 0;

	p->pIn = p->buf;
	p->pOut = p->pHalf;

	return r;
}

int double_buffer_out_transfer_HalfCplt_cb(double_buffer_t* p)
{
	int r = 0;

	p->pIn = p->buf;
	p->pOut = p->pHalf;

	return r;
}

int double_buffer_out_transfer_Cplt_cb(double_buffer_t* p)
{
	int r = 0;

	p->pIn = p->pHalf;
	p->pOut = p->buf;

	return r;
}
