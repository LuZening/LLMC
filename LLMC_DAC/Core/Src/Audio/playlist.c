/*
 * playlist.c
 *
 *  Created on: 2024年12月21日
 *      Author: cpholzn
 */


#include "playlist.h"
#include "utf_convert.h"
#include <string.h>

#define MAX(x,y) (((x)>(y))?(x):(y))

void playlist_init(playlist_t* p, FIL* fp /* fp should be in 'rb+' mode*/, const TCHAR* filepath)
{
	memset(p, 0, sizeof(playlist_t));
	strncpy_TCHAR(p->filepath, filepath, _MAX_LFN);
	p->fp = fp;
	p->posRowBegin = -1;
}


int playlist_getcur(playlist_t* p, /* out */ TCHAR* bufFname, TCHAR* bufPath, size_t maxcnt)
{
	int r = 0;
	if (p->posRowBegin < 0)
	{
		r = -1; // not found
		goto PLAYLIST_GETCUR_END;
	}

	f_lseek(p->fp, p->posRowBegin);
	size_t bytesRead;
	f_read(p->fp, p->buf, sizeof(p->buf), &bytesRead);
	if ((bytesRead <= 2) || (p->buf[0] == PLAYLIST_DELETED_CHAR))
	{
		r = -1;
		goto PLAYLIST_GETCUR_END;
	}

	const TCHAR* pTok = strnchr_TCHAR(p->buf, PLAYLIST_TOK_CHAR, bytesRead / 2);
	const TCHAR* pEnd = strnchr_TCHAR(p->buf, PLAYLIST_LINE_END_CHAR, bytesRead / 2);
	int cntKey = pTok - p->buf;
	int cntPath = pEnd - pTok - 1;
	if ((cntKey == 0) || (cntPath <= 1))
	{
		r = -1;
		goto PLAYLIST_GETCUR_END;
	}
	
	strncpy_TCHAR(bufFname, p->buf, cntKey);
	bufFname[cntKey] = 0;

	strncpy_TCHAR(bufPath, pTok + 1, cntPath);
	bufPath[cntPath] = 0;


PLAYLIST_GETCUR_END:
	return r;
}

int playlist_isin(playlist_t* p, const TCHAR* key)
{
	// return the row id of the record found
	int i = -1;
	// preserve
	size_t cntRow_old = p->cntRow;
	int posRowBegin_old = p->posRowBegin;
	// verify key length
	size_t cntKey = strnlen_TCHAR(key, PLAYLIST_MAX_STR_COUNT / 2);

	if (cntKey > 0)
	{		
		// rewind
		p->posRowBegin = -1;
		// iterate
		int r = 0;
		int j = -1;
		while (r == 0)
		{
			r = playlist_next(p, NULL, NULL, 0);
			j++;
			if (r == 0)
			{
				if ((p->cntRow > cntKey) && (memcmp(p->buf, key, cntKey) == 0))
				{
					i = j;
					goto PLAYLIST_ISIN_END;
				}
			}
		}
	}
PLAYLIST_ISIN_END:
	// recover
	p->posRowBegin = posRowBegin_old;
	p->cntRow = cntRow_old;
	return i;
}

int playlist_goto(playlist_t* p, int num, /*output*/ TCHAR *bufFname,  TCHAR* bufPath, size_t maxcnt)
{
	p->posRowBegin = -1;
	int i = -1; // mark the actual number of line we found
	while (i < num)
	{
		int r = playlist_next(p, bufFname, bufPath, maxcnt);
		if (r != 0)
			break;
		i++;
	}
	return i;
}



int playlist_add(playlist_t* p, const TCHAR* key, const TCHAR* path, /* out */ int *posBegin, size_t *pBytesWritten)
{
	int r = 0;
	if((p == NULL) || (p->fp == NULL))
		goto PLAYLIST_ADD_ERROR;

	// move to the head of file, check for repeated record
	playlist_goto(p, -1, NULL, NULL, 0);
	int i = 0;
	size_t cntKey = strnlen_TCHAR(key, PLAYLIST_MAX_STR_COUNT);
	if ( (cntKey < 1) || (cntKey > (PLAYLIST_MAX_STR_COUNT >> 1) - 1))
	{
		r = -1; // key too short or long
		goto PLAYLIST_ADD_ERROR;
	}
	// check repeat
	int nIn = false;
	nIn = playlist_isin(p, key);
	if (nIn >= 0)
	{
		r = -2; // already in
		goto PLAYLIST_ADD_ERROR;
	}
	// move to the end of file
	size_t lenFile = f_size(p->fp);
	f_lseek(p->fp, lenFile);

	// verify content
	size_t cnt;

	cnt = strnlen_TCHAR(path, PLAYLIST_MAX_STR_COUNT);
	if(cnt > (PLAYLIST_MAX_STR_COUNT >> 1) - 1)
	{
		r = -2; // path too long
		goto PLAYLIST_ADD_ERROR;
	}
	// make content
	TCHAR* pcur = p->buf;
	pcur = strncpy_TCHAR(pcur, key, (PLAYLIST_MAX_STR_COUNT >> 1) - 1);
	*(pcur++) = ',';
	pcur = strncpy_TCHAR(pcur, path, (PLAYLIST_MAX_STR_COUNT >> 1) - 1);
	*(pcur++) = '\n';
	// write to file
	size_t bytesToWrite = ((uint8_t*)pcur) - ((uint8_t*)p->buf);
	f_write(p->fp, (uint8_t*)p->buf, bytesToWrite, pBytesWritten);
	*posBegin = lenFile;
	p->is_modified = 1;

PLAYLIST_ADD_ERROR:
	return r;
}

int playlist_prev(playlist_t* p, /*output*/ TCHAR* bufFname, TCHAR* bufPath, size_t maxcnt)
{
	/*
	p->buf stores the full content of the found line, but
	note: will modify p->buf, by replaceing the end of the line, '\n',  with 0
	note: p->posRowBegin is in bytes
	note: p->cntRow does not count the trailing '\n'
	*/
	int r = 0;
	// find the previous record
	TCHAR* pBuf = p->buf;
	int bytesFile = f_size(p->fp);
	int posRowBegin;
	const TCHAR* pFound = NULL;
	bool isFound = false;
	if (p->posRowBegin < 0) // looking for the last line
	{
		posRowBegin = bytesFile;
	}
	else // looking for the previous line
	{
		posRowBegin = p->posRowBegin;
	}
	while (posRowBegin > 0)
	{
		size_t b;
		int posStartReading = MAX(0, (posRowBegin - PLAYLIST_BUF_BYTES));
		size_t bToRead = posRowBegin - posStartReading;
		f_lseek(p->fp, posStartReading);
		f_read(p->fp, (uint8_t*)pBuf, bToRead, &b);
		size_t c = b / sizeof(TCHAR);
		if (c > 1)
		{
			// find the right-most \n mark on the buffer
			pFound = pBuf;
			int idx = 0;
			int cntSkip = 0;
			while (idx < c)
			{
				const TCHAR* ptmp = strnchr_TCHAR(pFound, PLAYLIST_LINE_END_CHAR, c - idx);
				if (ptmp != NULL)
				{
					cntSkip = ptmp - pFound; // cntSkip indicates the distance between the \n mark and the head of the line
					idx += cntSkip + 1;
					pFound = ptmp + 1; // now pFound points to the symbol after the \n, which is the head of the next line
				}
				else
				{
					break;
				}
			}
			if (cntSkip > 0) // found the line
			{
				pFound -= cntSkip + 1; // next line does not exist, roll back to the previous line, which is the last line in the buffer
				c = (c - (pFound - pBuf));
				b = c * sizeof(TCHAR); // b indicates the bytes between the found line head and the end of the buffer 
				// make sure the line has not yet been deleted
				if (*pFound != PLAYLIST_DELETED_CHAR)
				{
					p->posRowBegin = posRowBegin - b;
					p->cntRow = c - 1; // excluding the length of the \n symbol inbetween
					isFound = true;
					if (bufFname)
					{
						const TCHAR* pTok_path = strnchr_TCHAR(pFound, PLAYLIST_TOK_CHAR, c); // tokenize the path part
						if (pTok_path > pFound)
						{
							size_t cntFname = pTok_path - pFound;
							strncpy_TCHAR(bufFname, pFound, cntFname); // leave a single more room to replace '\n' by 0
							bufFname[cntFname] = 0;
						}
					}
					if (bufPath)
					{
						const TCHAR* pTok_path = strnchr_TCHAR(pFound, PLAYLIST_TOK_CHAR, c); // tokenize the path part
						if (pTok_path > pFound) // the splitter has some TCHARs after
						{
							size_t cntPath = c - (pTok_path - pFound) - 2; // -2 excludes the ',' and '\n' mark
							strncpy_TCHAR(bufPath, pTok_path + 1, cntPath);
							bufPath[cntPath] = 0;
						}
					}
					break;
				}
			}
			else
			{
				b = c * sizeof(TCHAR); // \n does not show up even if we have exhaust the buffer, thus use the buffer's full length
			}

			posRowBegin -= b; // skip the trailing '\n'
		}
	}
	if (!isFound)
	{
		r = -1; // not found
	}
	return r;
}

int playlist_next(playlist_t* p, /*output*/ TCHAR* bufFname, TCHAR* bufPath, size_t maxcnt)
{
	/*
	p->buf stores the full content of the found line, but
	note: will modify p->buf, by replaceing the end of the line, '\n',  with 0
	note: p->posRowBegin is in bytes
	note: p->cntRow does not count the trailing '\n'
	*/
	int r = 0;
	// find the first record
	TCHAR* pBuf = p->buf;
	size_t bytesRead;
	int posRowBegin;
	const TCHAR* pFound = NULL;
	bool isFound = false;
	if(p->posRowBegin < 0) // looking for the first line
	{
		bytesRead = 0;
		posRowBegin = 0;
	}
	else // lookng for the next line
	{
		bytesRead = p->posRowBegin + (p->cntRow + 1) * sizeof(TCHAR);
		posRowBegin = bytesRead;
	}
	while(bytesRead < f_size(p->fp))
	{
		size_t b;
		f_lseek(p->fp, bytesRead);
		f_read(p->fp, (uint8_t*)pBuf, PLAYLIST_BUF_BYTES, &b);
		size_t c = b / sizeof(TCHAR);
		if(c > 1)
		{
			pFound = strnchr_TCHAR(pBuf, PLAYLIST_LINE_END_CHAR, c);
			if(pFound)
			{
				c = pFound - pBuf; // c refers to the coun of the line, excluding the trailing '\n'
				b = (c+1) * sizeof(TCHAR); // bytes scanned in the round
				// make sure the line has not yet been deleted
				if(*pBuf != PLAYLIST_DELETED_CHAR)
				{
					p->posRowBegin = posRowBegin;
					p->cntRow = c;
					isFound = true;
					if (bufFname)
					{
						const TCHAR* pTok_path = strnchr_TCHAR(pBuf, PLAYLIST_TOK_CHAR, c); // tokenize the path part
						if (pTok_path < pFound - 1) // the splitter has some TCHARs after
						{
							size_t cntFname = pTok_path - pBuf;
							strncpy_TCHAR(bufFname, pBuf, cntFname); // leave a single more room to replace '\n' by 0
							bufFname[cntFname] = 0;
						}
					}
					if(bufPath)
					{
						const TCHAR* pTok_path = strnchr_TCHAR(pBuf, PLAYLIST_TOK_CHAR, c); // tokenize the path part
						if(pTok_path < pFound - 1) // the splitter has some TCHARs after
						{
							size_t cntPath = c - (pTok_path - pBuf) - 1;
							strncpy_TCHAR(bufPath, pTok_path + 1, cntPath);
							bufPath[cntPath] = 0;
						}
					}
					break;
				}
			}
			else
			{
				b = c * sizeof(TCHAR); // \n does not show up even if we have exhaust the buffer, thus use the buffer's full length
			}

			bytesRead += b; // skip the trailing '\n'
			posRowBegin += b; // move cursor to the beginning of the next line
		}
	}
	if(!isFound)
	{
		r = -1; // not found
	}
	return r;
}


int playlist_remove(playlist_t* p, const TCHAR* key, /* out */ int* posRowBegin)
{
	int r = 0;
	// preserve
	size_t cntRow_old = p->cntRow;
	int posRowBegin_old = p->posRowBegin;
	// rewind
	p->posRowBegin = -1;
	// verify key
	size_t cntKey = strnlen_TCHAR(key, 256);
	// iterate
	int r2;
	while(1)
	{
		r2 = playlist_next(p, NULL, NULL, 0);
		if(r2 == 0)
		{
			// compare the key and the buffered line of playlist
			if(p->cntRow > cntKey)
			{
				int r_cmp = memcmp(p->buf, key, cntKey * sizeof(TCHAR));
				TCHAR splt = p->buf[cntKey]; // the char right after the key must be a splitter
				if((r_cmp == 0) && (splt == PLAYLIST_TOK_CHAR))
				{
					// matched
					// position the file to the beginning of the line
					f_lseek(p->fp, p->posRowBegin);
					*posRowBegin = p->posRowBegin;
					// overwrite the beginning of the line by a deleted mark
					TCHAR delMark = PLAYLIST_DELETED_CHAR; // alignment?
					size_t bwritten;
					f_write(p->fp, &delMark, sizeof(TCHAR), &bwritten);
					p->is_modified = 1;
					break;
				}
			}
		}
		else // playlist exhausted but no matched record found
		{
			r = -1;
			break;
		}
	}
	// restore
	p->cntRow = cntRow_old;
	p->posRowBegin = posRowBegin_old;

	return r;
}

size_t playlist_saveas(playlist_t* p, FIL* fp_new, size_t maxbytes)
{
	playlist_goto(p, -1, NULL, NULL, 0);
	f_rewind(fp_new);
	int r = 0;
	size_t bytesTotalWritten = 0;
	while (r == 0)
	{
		r = playlist_next(p, NULL, NULL, 0);
		if ((r == 0) && (p->cntRow > 0))
		{
			size_t bytesToWrite = (p->cntRow + 1) * sizeof(TCHAR); // add 1 for additional trailing \n
			if (bytesTotalWritten + bytesToWrite <= maxbytes)
			{
				size_t bytesWritten;
				p->buf[p->cntRow] = PLAYLIST_LINE_END_CHAR; // recover the line end mark, if somehow has been modified
				f_write(fp_new, p->buf, bytesToWrite, &bytesWritten);
				bytesTotalWritten += bytesWritten;
				p->is_modified = 0;
			}
			else // out of space
			{
				break;
			}
		}
	}
	return bytesTotalWritten;
}

#ifdef SIM
void playlist_test()
{
	static playlist_t pl;
	FIL f;
	f_open(&f, _T("test.pl"), FA_WRITE | FA_CREATE_ALWAYS);
	playlist_init(&pl, &f, _T("test.pl"));
#define N_KEYS 3
	const TCHAR* arrKeys[N_KEYS] = {_T("1.wav"), _T("2.wav"), _T("3.wav")};
	const TCHAR* arrPaths[N_KEYS] = {_T("/1.wav"), _T("2.wav"), _T("3.wav")};
	for (int i = 0; i < N_KEYS; ++i)
	{
		int pos;
		size_t bw;
		playlist_add(&pl, arrKeys[i], arrPaths[i], &pos, &bw);
	}
	TCHAR wcsFname[256] = { 0 };
	TCHAR wcsPath[256] = { 0 };
	playlist_next(&pl, wcsFname, wcsPath, 256);
	playlist_next(&pl, wcsFname, wcsPath, 256);
	playlist_prev(&pl, wcsFname, wcsPath, 256);
	playlist_prev(&pl, wcsFname, wcsPath, 256);
	playlist_prev(&pl, wcsFname, wcsPath, 256);
	playlist_next(&pl, wcsFname, wcsPath, 256);
	playlist_next(&pl, wcsFname, wcsPath, 256);
	playlist_next(&pl, wcsFname, wcsPath, 256);
	playlist_next(&pl, wcsFname, wcsPath, 256);
	playlist_prev(&pl, wcsFname, wcsPath, 256);
	playlist_prev(&pl, wcsFname, wcsPath, 256);
	playlist_prev(&pl, wcsFname, wcsPath, 256);
	//f_unlink(_T("test.pl"));
}
#endif
