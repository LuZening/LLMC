/*
 * utf_convert.c
 *
 *  Created on: Apr 27, 2024
 *      Author: cpholzn
 */

#include "utf_convert.h"

/*
UTF-16		UTF-8
0000 - 007F 0xxxxxxx
0080 - 07FF 110xxxxx 10xxxxxx
0800 - FFFF 1110xxxx 10xxxxxx 10xxxxxx
*/

#define UTF8_ONE_START      (0x0001)
#define UTF8_ONE_END        (0x007F)
#define UTF8_TWO_START      (0x0080)
#define UTF8_TWO_END        (0x07FF)
#define UTF8_THREE_START    (0x0800)
#define UTF8_THREE_END      (0xFFFF)


// return bytes wriiten as utf8, excludng trailing 0
int UTF16ToUTF8(const UTF16* pUTF16Start, const UTF16* pUTF16End, UTF8* pUTF8Start, UTF8* pUTF8End)
{
     const UTF16* pTempUTF16 = pUTF16Start;
     UTF8* pTempUTF8 = pUTF8Start;

     while ((pTempUTF16 < pUTF16End) && (pUTF8End - pTempUTF8 >= 3))
     {
         if (*pTempUTF16 <= UTF8_ONE_END
             && pTempUTF8 + 1 < pUTF8End)
         {
             //0000 - 007F  0xxxxxxx
             *pTempUTF8++ = (UTF8)*pTempUTF16;
         }
         else if (*pTempUTF16 >= UTF8_TWO_START && *pTempUTF16 <= UTF8_TWO_END
             && pTempUTF8 + 2 < pUTF8End)
         {
             //0080 - 07FF 110xxxxx 10xxxxxx
             *pTempUTF8++ = (*pTempUTF16 >> 6) | 0xC0;
             *pTempUTF8++ = (*pTempUTF16 & 0x3F) | 0x80;
         }
         else if (*pTempUTF16 >= UTF8_THREE_START && *pTempUTF16 <= UTF8_THREE_END
             && pTempUTF8 + 3 < pUTF8End)
         {
             //0800 - FFFF 1110xxxx 10xxxxxx 10xxxxxx
             *pTempUTF8++ = (*pTempUTF16 >> 12) | 0xE0;
             *pTempUTF8++ = ((*pTempUTF16 >> 6) & 0x3F) | 0x80;
             *pTempUTF8++ = (*pTempUTF16 & 0x3F) | 0x80;
         }
         else
         {
             break ;
         }
         pTempUTF16++;
     }
     *pTempUTF8 = 0;
     return pTempUTF8 - pUTF8Start;
}


int UTF8ToUTF16(const UTF8* pUTF8Start, const UTF8* pUTF8End, UTF16* pUTF16Start, UTF16* pUTF16End)
{
    // little-endian by default
     UTF16* pTempUTF16 = pUTF16Start;
     const UTF8* pTempUTF8 = pUTF8Start;

     while (pTempUTF8 < pUTF8End && pTempUTF16+1 < pUTF16End)
     {
    	 *pTempUTF16 = 0;
         if (*pTempUTF8 >= 0xE0U && *pTempUTF8 <= 0xEFU) //��3���ֽڵĸ�ʽ
         {
             //0800 - FFFF 1110xxxx 10xxxxxx 10xxxxxx
             *pTempUTF16 |= ((*pTempUTF8++ & 0x0FU) << 12);
             *pTempUTF16 |= ((*pTempUTF8++ & 0x3FU) << 6);
             *pTempUTF16 |= (*pTempUTF8++ & 0x3FU);

         }
         else if (*pTempUTF8 >= 0xC0U && *pTempUTF8 <= 0xDFU) //��2���ֽڵĸ�ʽ
         {
             //0080 - 07FF 110xxxxx 10xxxxxx
             *pTempUTF16 |= ((*pTempUTF8++ & 0x1FU) << 6U);
             *pTempUTF16 |= (*pTempUTF8++ & 0x3FU);
         }
         else if (*pTempUTF8 >= 0U && *pTempUTF8 <= 0x7FU) //��1���ֽڵĸ�ʽ
         {
             //0000 - 007F  0xxxxxxx
             *pTempUTF16 = *pTempUTF8++;
         }
         else
         {
             break ;
         }

         pTempUTF16++;
     }
     *pTempUTF16 = 0;
     return (pTempUTF16 - pUTF16Start);
}


size_t strnlen_TCHAR(const TCHAR* wcs, size_t maxcnt)
{

    size_t len = 0;
    while((len < maxcnt) && wcs[len])
    {
        len++;
    }
    return len;
}


const TCHAR* strncpy_TCHAR(TCHAR* dst, const TCHAR* src, size_t maxcnt)
{
    if ((dst != src) && (maxcnt > 0))
    {
        size_t cnt = strnlen_TCHAR(src, maxcnt);
        memcpy((uint8_t*)dst, (uint8_t*)src, cnt * sizeof(TCHAR));
        if(cnt < maxcnt)
            dst[cnt] = 0; // tailing 0
        return dst + cnt;
    }
    else
        return dst;
}

const TCHAR* strnchr_TCHAR( const TCHAR* s, const TCHAR to_find, size_t maxcnt)
{
    TCHAR* found = NULL;

    while(maxcnt > 0)
    {
        if(*s == to_find)
        {
            found = s;
            break;
        }
        s++;
        maxcnt--;
    }
    return found;
}

bool endswith_TCHAR(const TCHAR* sEnd, size_t countEnd, const TCHAR* path, size_t countPath)
{

    size_t i = 0;
    // detect end of path
    i = strnlen_TCHAR(path, countPath);

    // path can be shorter than sEnd
    if (i < countEnd)
        return false;

    size_t j = 0;
    while (j < countEnd)
    {
        if (sEnd[j] != path[i - countEnd])
            return false;
        j++;
        countEnd--;
    }
    return true;
}
