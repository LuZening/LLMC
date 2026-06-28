#include "decoder_pool.h"
#include <stdint.h>
#include <string.h>

/* decoder_pool_arena (cacheable AXI SRAM) provides the bump-allocator backing
 * store for all decoders (dr_wav, dr_flac, dr_mp3). SIZE from decoder_pool.h.
 * Cache coherency after SDMMC DMA reads is handled in each decoder's on_read
 * callback via SCB_InvalidateDCache_by_Addr(). */
extern uint8_t decoder_pool_arena[];  /* defined in audio_player.c */
static size_t decoder_pool_offset = 0;

void decoder_pool_reset(void)
{
    decoder_pool_offset = 0;
}

void *decoder_pool_malloc(size_t sz)
{
    sz = (sz + 31) & ~(size_t)31;
    if (decoder_pool_offset + sz > DECODER_POOL_SIZE) return NULL;
    void *p = decoder_pool_arena + decoder_pool_offset;
    decoder_pool_offset += sz;
    memset(p, 0, sz);
    return p;
}

void *decoder_pool_realloc(void *p, size_t sz)
{
    sz = (sz + 31) & ~(size_t)31;
    if (sz == 0) { decoder_pool_reset(); return NULL; }
    if (p == NULL) return decoder_pool_malloc(sz);
    if (sz > DECODER_POOL_SIZE) return NULL;
    /* In-place extension: dr_libs only grows its primary buffer */
    size_t end = (uint8_t *)p - decoder_pool_arena + sz;
    if (end > decoder_pool_offset) decoder_pool_offset = end;
    return p;
}

void decoder_pool_free(void *p)
{
    (void)p;
}
