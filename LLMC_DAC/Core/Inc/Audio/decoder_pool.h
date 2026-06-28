#pragma once
#include <stddef.h>

#define DECODER_POOL_SIZE  (64U * 1024U)

void  decoder_pool_reset(void);
void *decoder_pool_malloc(size_t sz);
void *decoder_pool_realloc(void *p, size_t sz);
void  decoder_pool_free(void *p);
