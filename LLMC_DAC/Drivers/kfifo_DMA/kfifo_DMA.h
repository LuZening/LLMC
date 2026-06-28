#pragma once
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "main.h"
#include <assert.h>
#define KFIFO_DMA_OK 0
#define KFIFO_DMA_UNDERRUN 1
//#define KFIFO_DMA_OVERRUN 2
#define IS_POWER_OF_TWO_U(x)  ((x) != 0 && (((x) & ((x) - 1)) == 0))
//声明 一个 结构体 kfifo
struct KFIFO_DMA
{
    unsigned char *buffer;    /* the buffer holding the data */
    size_t size;            /* the size of the allocated buffer, must be power of 2 */
    uint32_t mask_size;
    size_t half;
    volatile unsigned int in;                /* data is added at offset (in % size) */
    volatile unsigned int out;                /* data is extracted from off. (out % size) */
    size_t bytesWritten;
    size_t bytesRead;
	//    volatile unsigned int *lock; /* protects concurrent modifications */
#ifdef USE_DMA_ON_KFIFO
    volatile unsigned int dmacur; /* DMA cursor position */
    size_t dmastep; // turn kfifo buffer to multiple segments for DMA transfer, each segment is a minial part for DMA
    uint8_t pow_of_two;Q
#endif
};


typedef struct KFIFO_DMA KFIFO_DMA;

void kfifo_DMA_static_init(KFIFO_DMA* p, uint8_t* buf, size_t size /* must be power of 2*/, size_t dmastep);


int kfifo_DMA_HalfCplt_cb(KFIFO_DMA* p);

int kfifo_DMA_FullCplt_cb(KFIFO_DMA* p, DMA_HandleTypeDef *pDMA);


size_t __kfifo_put(struct KFIFO_DMA *fifo, const uint8_t *buffer, size_t len);

size_t __kfifo_get(struct KFIFO_DMA *fifo, uint8_t *buffer, size_t len);
//size_t kfifo_get_inplace(struct KFIFO_DMA *fifo, unsigned char **ptrBuffer, unsigned int len);

#define kfifo_get(fifo, buf, len) __kfifo_get(fifo,buf,len)
#define kfifo_put(fifo, buf, len)  __kfifo_put(fifo,buf,len)
void kfifo_invalidate(KFIFO_DMA* fifo);

size_t kfifo_get_free_space( KFIFO_DMA *fifo);

size_t kfifo_get_filled_size( KFIFO_DMA *fifo);

