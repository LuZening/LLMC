/*
 * playlist.h
 *
 *  Created on: 2024年12月21日
 *      Author: cpholzn
 */

#ifndef INC_AUDIO_PLAYLIST_H_
#define INC_AUDIO_PLAYLIST_H_
#ifdef __cplusplus
extern "C" {
#endif
#ifndef SIM
#include "ff.h"
#else
#include "sim.h"
void playlist_test();
#endif


#define PLAYLIST_MAX_STR_COUNT 512
#define PLAYLIST_BUF_BYTES (PLAYLIST_MAX_STR_COUNT << 1)
#define PLAYLIST_DELETED_CHAR '-'
#define PLAYLIST_TOK_CHAR ','
#define PLAYLIST_LINE_END_CHAR '\n'

typedef struct
{
    int prevpos;
    int begin;
    size_t len;
    int nextpos;
} playlist_node_t;

typedef struct
{
    FIL* fp;
    uint8_t is_modified;
    TCHAR filepath[_MAX_LFN+1];
    TCHAR  buf[PLAYLIST_MAX_STR_COUNT];
    int posRowBegin; // point to the byte position in the file of the line head
    size_t cntRow; //row length, not counting the trailing '\n'
    int seed;
} playlist_t;

/* WARNING: all the playlist function must be protected by a mutex*/

void playlist_init(playlist_t* p, FIL* fp /* fp should be in 'rb+' mode*/, const TCHAR* filepath);

int playlist_getcur(playlist_t* p, /* out */ TCHAR* bufFname, TCHAR* bufPath, size_t maxcnt);

int playlist_isin(playlist_t* p, const TCHAR* key);

int playlist_add(playlist_t* p, const TCHAR* key, const TCHAR* path, /* out */ int *posBegin, size_t *pBytesWritten);

int playlist_remove(playlist_t* p, const TCHAR* key, /* out */ int* posRowBegin);

int playlist_prev(playlist_t* p, /*output*/ TCHAR* bufFname, TCHAR* bufPath, size_t maxcnt);

int playlist_next(playlist_t* p, /*output*/ TCHAR* bufFname, TCHAR* bufPath, size_t maxcnt);

int playlist_goto(playlist_t* p, int num, /*output*/ TCHAR *bufFname,  TCHAR* bufPath, size_t maxcnt);

size_t playlist_saveas(playlist_t* p, FIL* fp_new /* fp_new should be in wb mode */, size_t maxbytes);




//int playlist_move_to(playlist_t* p, int num, /*output*/ TCHAR* path, size_t maxlen);

//int playlist_random(playlist_t* p,/*output*/ TCHAR* path, size_t maxlen);



#ifdef __cplusplus
}
#endif

#endif /* INC_AUDIO_PLAYLIST_H_ */
