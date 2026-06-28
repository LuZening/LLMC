#pragma once
#ifdef SIM

#include <stdint.h>
#include <stdio.h>
#include <wchar.h>
#include <windows.h>


#ifndef _MAX_LFN
#define _MAX_LFN 255
#endif
#ifndef _T
#define _T(x) L##x
#endif

#define _LFN_UNICODE 1
typedef wchar_t TCHAR;

typedef struct
{
    FILE* f;
} FIL;

typedef void* DIR;

typedef struct {
    size_t	fsize;			/* File size */
    int	fdate;			/* Modified date */
    int	ftime;			/* Modified time */
    uint8_t fattrib;		/* File attribute */

    TCHAR	altname[13];			/* Alternative file name */
    TCHAR	fname[_MAX_LFN + 1];	/* Primary file name */

} FILINFO;

typedef enum {
    FR_OK = 0,				/* (0) Function succeeded */
    FR_DISK_ERR,			/* (1) A hard error occurred in the low level disk I/O layer */
    FR_INT_ERR,				/* (2) Assertion failed */
    FR_NOT_READY,			/* (3) The physical drive does not work */
    FR_NO_FILE,				/* (4) Could not find the file */
    FR_NO_PATH,				/* (5) Could not find the path */
    FR_INVALID_NAME,		/* (6) The path name format is invalid */
    FR_DENIED,				/* (7) Access denied due to a prohibited access or directory full */
    FR_EXIST,				/* (8) Access denied due to a prohibited access */
    FR_INVALID_OBJECT,		/* (9) The file/directory object is invalid */
    FR_WRITE_PROTECTED,		/* (10) The physical drive is write protected */
    FR_INVALID_DRIVE,		/* (11) The logical drive number is invalid */
    FR_NOT_ENABLED,			/* (12) The volume has no work area */
    FR_NO_FILESYSTEM,		/* (13) Could not find a valid FAT volume */
    FR_MKFS_ABORTED,		/* (14) The f_mkfs function aborted due to some problem */
    FR_TIMEOUT,				/* (15) Could not take control of the volume within defined period */
    FR_LOCKED,				/* (16) The operation is rejected according to the file sharing policy */
    FR_NOT_ENOUGH_CORE,		/* (17) LFN working buffer could not be allocated or given buffer is insufficient in size */
    FR_TOO_MANY_OPEN_FILES,	/* (18) Number of open files > FF_FS_LOCK */
    FR_INVALID_PARAMETER	/* (19) Given parameter is invalid */
} FRESULT;


#define	FA_READ				0x01
#define	FA_WRITE			0x02
#define	FA_OPEN_EXISTING	0x00
#define	FA_CREATE_NEW		0x04
#define	FA_CREATE_ALWAYS	0x08
#define	FA_OPEN_ALWAYS		0x10
#define	FA_OPEN_APPEND		0x30


#define f_rewind(p) f_lseek(p, 0)


int f_open(FIL* p, const TCHAR* path, char mode);
int f_close(FIL* p);
int f_read(FIL* p, uint8_t* buf, size_t len, size_t* lenRead);
int f_write(FIL* p, uint8_t* buf, size_t len, size_t* lenWritten);

int f_lseek(FIL* p, size_t offset);

size_t f_tell(FIL* p);
size_t f_size(FIL* p);

FRESULT f_opendir(
    DIR* dp,			/* Pointer to directory object to create */
    const TCHAR* path	/* Pointer to the directory path */
);

FRESULT f_readdir(DIR* dp, FILINFO* fno);


FRESULT f_closedir(DIR* dp);

FRESULT f_unlink(const TCHAR* path);								/* Delete an existing file or directory */
FRESULT f_rename(const TCHAR* path_old, const TCHAR* path_new);	/* Rename/Move a file or directory */

typedef enum
{
    FS_PLAY_MODE_ONE_SHOT = 0,
    FS_PLAY_MODE_ONE_LOOP,
    FS_PLAY_MODE_SEQUENTIAL,
    FS_PLAY_MODE_RANDOM,
} FS_play_mode_t;

typedef enum
{
    FS_PLAYER_STOP = 0,
    FS_PLAYER_PLAY,
    FS_PLAYER_PAUSE,
    FS_PLAYER_NEXT,
    FS_PLAYER_PREV,
} FS_player_state_t;

#endif