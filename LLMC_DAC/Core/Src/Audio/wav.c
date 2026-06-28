/*
 * wav.c
 *
 *  Created on: 2024年8月30日
 *      Author: cpholzn
 */


#include "wav.h"
#include <string.h>
#ifndef MIN
#define MIN(a, b)  (((a) < (b)) ? (a) : (b))
#endif

#define LOAD_LE16(p) (((uint16_t)(*p)) | ((uint16_t)(*(p+1)) << 8))
#define LOAD_LE32(p) (((uint32_t)(*p)) | ((uint32_t)(*(p+1)) << 8) | ((uint32_t)(*(p+2)) << 16) | ((uint32_t)(*(p+3)) << 24))
char* find_str(const char* buf, size_t nbuf, const char* tofind, size_t ntofind)
{
    const char* end = buf + nbuf;
    const char* p = buf;
    while(p < end - ntofind)
    {
        bool match = true;
        int i;
        for(i = 0; i < ntofind; ++i)
        {
            if(*(p+i) != *(tofind + i))
            {
                match = false;
                break;
            }
        }
        if(!match)
            p+=(i+1);
        else
            break;
    }
    return p;
}
int wav_parse_header(wav_header_t* ph, const uint8_t* buf, size_t len)
{
    // detecting "data"
    const uint8_t* p = buf;
    const uint8_t* pEnd = buf + len;

    if(len < MIN_LEN_WAV_HEADER)
        goto ERROR_WAV_PARSE;

    /* locate data mark */
    uint8_t* pDataMark = (uint8_t*)find_str((const char*)buf,  MIN(len, 128), "data", 4);
    if(pDataMark == NULL)
        goto ERROR_WAV_PARSE;

    ph->data_begin = (pDataMark  + 8) - buf;
    p = pDataMark + 4;
    ph->data_size = LOAD_LE32(p);

    /* locate fmt mark */
    uint8_t* pFmtMark = (uint8_t*)find_str((const char*)buf, MIN(len, 128), "fmt", 3);
    if(pFmtMark == NULL)
        goto ERROR_WAV_PARSE;

    p = pFmtMark + 8;
    ph->audio_format = LOAD_LE16(p);

    p += 2;
    ph->num_channels = LOAD_LE16(p);

    p += 2;
    ph->sample_rate = LOAD_LE32(p);

    p += 4;
    ph->byte_rate = LOAD_LE32(p);

    p+=4;
    ph->bytes_per_frame = LOAD_LE16(p);

    p+=2;
    ph->bits_per_sample = LOAD_LE16(p);

    if((ph->sample_rate > 40000) && (ph->sample_rate < 100000) && (ph->bits_per_sample >= 16) && (ph->bits_per_sample <= 32))
        return 0;
ERROR_WAV_PARSE:
    return -1;
}
