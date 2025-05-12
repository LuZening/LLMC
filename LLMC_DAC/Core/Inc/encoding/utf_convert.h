/*
 * utf_convert.h
 *
 *  Created on: Apr 27, 2024
 *      Author: cpholzn
 */

#ifndef INC_UTF_CONVERT_H_
#define INC_UTF_CONVERT_H_
#include <stdint.h>
#if _LFN_UNICODE
#define UNICODE 1
#endif
#include <string.h>
#include <wchar.h>
#include <stdbool.h>

#ifndef SIM
#include "ff.h"
#else
#include "sim.h"
#endif

typedef uint32_t   UTF32;  /* at least 32 bits */
typedef uint16_t UTF16;  /* at least 16 bits */
typedef uint8_t   UTF8;   /* typically 8 bits */


int UTF16ToUTF8(const UTF16* pUTF16Start, const UTF16* pUTF16End, UTF8* pUTF8Start, UTF8* pUTF8End);
int UTF8ToUTF16(const UTF8* pUTF8Start, const UTF8* pUTF8End, UTF16* pUTF16Start, UTF16* pUTF16End);


size_t strnlen_TCHAR(const TCHAR* wcs, size_t maxcnt);
const TCHAR* strncpy_TCHAR(TCHAR* dst, const TCHAR* src, size_t maxcnt);
const TCHAR* strnchr_TCHAR(const TCHAR* s, const TCHAR to_find, size_t maxcnt);

bool endswith_TCHAR(const TCHAR* sEnd, size_t countEnd, const TCHAR* path, size_t countPath);
#endif /* INC_UTF_CONVERT_H_ */
