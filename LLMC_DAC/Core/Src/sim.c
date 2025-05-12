#ifdef SIM
#include "sim.h"
#include <stdlib.h>
#include <stdio.h>
#include "utf_convert.h"


int f_open(FIL* p, const TCHAR* path, char mode)
{

    char path_mbs[512];
    wcstombs(path_mbs, path, _MAX_LFN);
    if ((mode | FA_READ) > 0)
    {
        if ((mode | (FA_OPEN_ALWAYS)) > 0)
        {
            p->f = fopen(path_mbs, "rb+");
            if (p->f == NULL) // not existing
                p->f = fopen(path_mbs, "wb");
        }
        else
        {
            p->f = fopen(path_mbs, "rb");
        }

    }
    else if ((mode | FA_WRITE) > 0)
    {
        p->f = fopen(path_mbs, "wb");
    }
    else
    {
        p->f = fopen(path_mbs, "rb");
    }
    return 0;
}

int f_close(FIL* p)
{
    return fclose(p->f);
}

int f_read(FIL* p, uint8_t* buf, size_t len, size_t* lenRead)
{
    *lenRead = fread(buf, 1, len, p->f);
    return 0;
}

int f_write(FIL* p, uint8_t* buf, size_t len, size_t* lenWritten)
{
	*lenWritten = fwrite(buf, 1, len, p->f);
	return 0;
}

int f_lseek(FIL* p, size_t offset)
{
    fseek(p->f, offset, SEEK_SET);
    return 0;
}

size_t f_tell(FIL* p)
{
    return ftell(p->f);
}

size_t f_size(FIL* p)
{
    size_t old = ftell(p->f);
    fseek(p->f, 0, SEEK_END);
    size_t len = ftell(p->f);
    fseek(p->f, old, SEEK_SET);
    return len;
}


/* Implement placeholder FatFS handles BEGIN */
FRESULT f_opendir(
    DIR* dp,			/* Pointer to directory object to create */
    const TCHAR* path	/* Pointer to the directory path */
)
{
    if (wcsstr(path, L".wav"))
        return FR_INVALID_NAME;
    else
        return FR_OK;
}

FRESULT f_readdir(DIR* dp, FILINFO* fno)
{
    static counter = 0;
    const int N = 2;
    const TCHAR* fnames[] = { L"003-少女以上の恋がしたい.wav", L"028-涙×.wav"};

    if (counter < N)
    {
        size_t countFname = strnlen_TCHAR(fnames[counter], _MAX_LFN);
        memcpy(fno->fname, fnames[counter], countFname * sizeof(TCHAR));
        fno->fname[countFname] = 0U;
        ++counter;
    }
    else
    {
        counter = 0;
        memset(fno->fname, 0, sizeof(fno->fname));
    }
    return FR_OK;
}


FRESULT f_closedir(DIR* dp)
{

    return FR_OK;
}

FRESULT f_unlink(const TCHAR* path)
{
    char mbs[512];
    wcstombs(mbs, path, 256);
    return remove(mbs);
}

FRESULT f_rename(const TCHAR* path_old, const TCHAR* path_new)
{
    char mbs_old[512], mbs_new[512];
    wcstombs(mbs_old, path_old, 256);
    wcstombs(mbs_new, path_new, 256);
    return rename(mbs_old, mbs_new);
}

#endif
