/*
 * audio_player.c
 *
 *  Created on: Apr 4, 2024
 *      Author: cpholzn
 */


#include<string.h>
#include<stdbool.h>
#include <stdio.h>
#include <assert.h>
typedef float float32_t;
#include "cmsis_os2.h"
#include "audio_DSP.h"
#include "audio_buffers.h"
#include "audio_player.h"
#include "fatfs.h"

#include "arm_math.h"
#include "Config_user_define.h"
#include "utf_convert.h"

#include "decoder.h"


#include "LVGL_GUI.h"
#include "LVGL_toast.h"
#include "lv_simple_file_explorer_unicode.h"

#ifndef MIN
#define MIN(a, b)  (((a) < (b)) ? ((a)) : ((b)))
#endif

osMutexId_t mtxFSPlayer = NULL;
KFIFO_DMA fifo_from_FS;
// (f32) PCM data memory for fifo
float32_t bufFSFIFO[FS_FIFO_SIZE / sizeof(float)] = {0}; // 96KB ~ 110ms @ 96k float 2ch
uint32_t FIFO_length_ms = FS_FIFO_SIZE / (DEFAULT_SAMPLE_RATE_HZ / 1000 * MAX_CHANNELS * sizeof(float));

uint32_t fifoOverflowCounter_FS = 0;
uint32_t fifoUnderflowCounter_FS = 0;

osThreadId_t taskFSLoader = NULL;
float volumef_FS = 0.;


static audio_interpolator_t arr_audio_interpolator_48K_to_96K_FS[MAX_CHANNELS];
static float32_t  bufsInterpolatorStateFS[MAX_CHANNELS][COUNT_FIR_BUF]; // 1KB
static audio_sinc_resampler_t  audio_sinc_resampler_44K_to_48K_FS; // 1 object fit for all chs

#define MAX_PATH_LENGTH 255U


//osMutexId_t mtxGUIWidgetsHandle;



FS_play_mode_t modePlay = FS_PLAY_MODE_ONE_LOOP;

/* FILE */
TCHAR wcsFolder[_MAX_LFN+1]; size_t cntFolder;
TCHAR wcsFile[_MAX_LFN+1]; size_t cntFile;
TCHAR wcsPathToPlay[_MAX_LFN * 2 + 1]; size_t cntPathToPlay;

FIL __attribute__((section(".DMA_no_cache"))) __attribute__((aligned(32))) fileToPlay;
static DIR __attribute__((section(".DMA_no_cache"))) __attribute__((aligned(32)))  dirToPlay;
static FILINFO __attribute__((section(".DMA_no_cache"))) __attribute__((aligned(32))) finfo;
static bool isOpen = false;
bool isOpen_PL = false; // indicate if playlist file is opened
FIL filePL;
FIL filePL_new; // file for playlist
playlist_t __attribute__((section(".DMA_no_cache"))) pl;

typedef enum
{
    FILE_FORMAT_UNKNOWN = 0,
    FILE_FORMAT_WAV,
    FILE_FORMAT_FLAC,
    FILE_FORMAT_MP3,
} file_format_t;
file_format_t formatToPlay = FILE_FORMAT_UNKNOWN;
/* Bump-allocator arena for decoder_pool (dr_wav, dr_flac, dr_mp3).
 * In non-cacheable .DMA_no_cache; no cache maintenance needed at runtime.
 * MP3 requires at least 64KB, other decoders only use 4~8kb */
uint8_t __attribute__((aligned(32))) decoder_pool_arena[64U * 1024U];


/* PLAY CONTROL */
osMessageQueueId_t qFSLoader = NULL;
FS_player_state_t FS_player_state = FS_PLAYER_STOP;
// global variables for GUI
static size_t duration_ms = 0; // protected by mtxFSPlayer
static size_t totalSamples = 0; // protected by mtxFSPlayer
size_t samplesPlayed = 0; // no protection, dirty read is okay for GUI
size_t samplesPerMs = (DEFAULT_SAMPLE_RATE_HZ / 1000); // protected by mtxFSPlayer

// start to play autonomouslty after changing track, without the need for manual operations
// this flag will be set each time the player enters PLAY state, and be cleared when paused or stopped
static bool startPlayInstantly = false;
/* PLAY CONTROL */

/* DECODER */
// must be converted to float32 format before DSP
// decode output + SRC output buffer (dec_cap=3072, max SRC out=6688)
static float32_t bufCastToFloat[8192] = {0.}; // 32KB
// memory pool for file decoders
// 32KB each
float32_t __attribute((section(".DTCMRAM2_heap")))  __attribute__((aligned(32))) memFileDecoder1[SIZE_MEMPOOL_FILE_DECODER_1 / sizeof(float32_t)];
float32_t __attribute((section(".DTCMRAM2_heap")))  __attribute__((aligned(32))) memFileDecoder2[SIZE_MEMPOOL_FILE_DECODER_2 / sizeof(float32_t)];
static const decoder_t *active_decoder = NULL;
static void *decoder_ctx = NULL;
static uint32_t current_sample_rate = 0;
static uint8_t  current_bit_depth = 0;
uint8_t oversample = 1;
uint8_t oversample_frac = 0;
uint8_t nChannels = 2;
/* DECODER */


/* DSP */

/* DSP */


/* STATIC FUNCTIONS */
static void FS_player_reset(FS_play_mode_t mode);
static file_format_t judge_track_file_format(const TCHAR* wcsFn, size_t cntFn);
static int find_track_in_the_same_folder(int direction, /* out */ TCHAR* wcsFilename, size_t* pCntFilename);

static void task_FS_loader(void * argument);
static void player_do_seek(uint32_t target_frame);
/* STATIC FUNCTIONS */

// call before start task
void FS_player_init()
{
//	assert(FS_FIFO_SIZE % 4 == 0);
    memset((uint8_t*)bufFSFIFO, 0, FS_FIFO_SIZE);

    for(int i = 0; i < MAX_CHANNELS; ++i)
    {
        audio_interplolator_init(&(arr_audio_interpolator_48K_to_96K_FS[i]), 2, bufsInterpolatorStateFS[i], BLOCK_SIZE_FIR);
    }
    wcsFile[0] = 0;
    cntFile = 0;
    wcsFolder[0] = 0;
    cntFolder = 0;
    wcsPathToPlay[0] = 0;
    cntPathToPlay = 0;
    isOpen = false;
    FS_player_reset(FS_PLAY_MODE_ONE_SHOT);
}

// reset after each play
static void FS_player_reset(FS_play_mode_t mode)
{
    /* Close any active decoder */
    if (active_decoder && decoder_ctx) {
        active_decoder->close(&decoder_ctx);
        active_decoder = NULL;
    }

    samplesPlayed = 0;
    audio_sinc_resampler_init(&audio_sinc_resampler_44K_to_48K_FS, audio_sinc_coeffs_44K_to_48K, 44100U, 48000U, MAX_CHANNELS);
//	if(mode != FS_PLAY_MODE_ONE_LOOP)
//	{
        kfifo_DMA_static_init(&fifo_from_FS, (uint8_t*)bufFSFIFO, FS_FIFO_SIZE, 0);
        totalSamples = 0;
        duration_ms = 0;

//	}
}

// call after init
void FS_player_task_start()
{
    qFSLoader = osMessageQueueNew(4, sizeof(int32_t), NULL);
    vQueueAddToRegistry(qFSLoader, "qFSLoader");
    static const osThreadAttr_t taskFSLoader_attr= {
      .name = "fsloader",
      .stack_size = 18*1024U,
      .priority = (osPriority_t) osPriorityRealtime,
    };
    if(taskFSLoader == NULL)
    {
        taskFSLoader = osThreadNew(task_FS_loader, NULL, &taskFSLoader_attr);
    }

}

static file_format_t judge_track_file_format(const TCHAR* wcsFn, size_t cntFn)
{
    if(endswith_TCHAR(_T(".wav"), 4, wcsFn, cntFn) || endswith_TCHAR(_T(".WAV"), 4, wcsFn, cntFn))
    {
        return FILE_FORMAT_WAV;
    }
    else if(endswith_TCHAR(_T(".flac"), 5, wcsFn, cntFn) || endswith_TCHAR(_T(".FLAC"), 5, wcsFn, cntFn))
    {
        return FILE_FORMAT_FLAC;
    }
    else if(endswith_TCHAR(_T(".mp3"), 4, wcsFn, cntFn) || endswith_TCHAR(_T(".MP3"), 4, wcsFn, cntFn))
    {
        return FILE_FORMAT_MP3;
    }
    else
    {
        return FILE_FORMAT_UNKNOWN;
    }
}


decoder_info_t dec_info;
const decoder_t *decoders[] = {&decoder_wav_dr, &decoder_flac, &decoder_mp3, NULL};
static int8_t FS_open_file(const TCHAR* path)
{

    int8_t r = -1;
    bool _isOpen = false;
    const char* sErrMsg = NULL;
    FRESULT fr = f_open(&fileToPlay, path, FA_READ);
    // open up a file
    if(fr == FR_OK)
    {
        _isOpen = true;
        if(active_decoder && decoder_ctx)
        {
            active_decoder->close(&decoder_ctx);
            active_decoder = NULL;
        }
        /* Try each decoder in order */
        for(int i = 0; decoders[i]; i++)
        {
            void *ctx = NULL;
            if(decoders[i]->open(&fileToPlay, &ctx, &dec_info) == DECODER_OK)
                {
                    active_decoder = decoders[i];
                    decoder_ctx = ctx;
                    formatToPlay = (i == 0) ? FILE_FORMAT_WAV : (i == 1) ? FILE_FORMAT_FLAC : FILE_FORMAT_MP3;
                    totalSamples = dec_info.total_samples;
                    current_sample_rate = dec_info.sample_rate;
                    current_bit_depth  = dec_info.bits_per_sample;
                    oversample = DEFAULT_SAMPLE_RATE_HZ / dec_info.sample_rate;
                    oversample_frac = DEFAULT_SAMPLE_RATE_HZ % dec_info.sample_rate;
                    nChannels = dec_info.num_channels;
                    duration_ms = dec_info.duration_ms;
                    samplesPerMs = dec_info.sample_rate / 1000;
                    r = 0;
                    break;
                }
            }
            if(r != 0)
            {
                sErrMsg = "Unsupported format";
                r = -1;
                goto FS_OPEN_FILE_ERROR;
            }

    }
    return r;
FS_OPEN_FILE_ERROR:
    if(_isOpen) f_close(&fileToPlay);
    // show err msg
    if(sErrMsg)
    {
        lv_lock();
        show_toast(sErrMsg);
        lv_unlock();
    }
    return r;
}


static int find_track_in_the_same_folder(int direction, /* out */ TCHAR* wcsFilename, size_t* pCntFilename)
{
    static TCHAR wcsBuf[_MAX_LFN+1];
    int r = -1;
    const char* sErrMsg = NULL;
    bool found = false;
    FRESULT fr;
    fr = f_opendir(&dirToPlay, wcsFolder);
    if(fr != 0)
    {
        r = -1;
        sErrMsg = "Unable to open the folder";
        goto FIND_ITEM_IN_THE_SAME_FOLDER_ERROR;
    }

    int fsm = 0; // 0: init 1: same file found
    int iBefore = 0; // how many qualified files were found before this file
    int iAfter = 0;
    TCHAR* wcsFn;
    size_t cntFn;

    TCHAR* wcsFn_prev = wcsBuf; // used to keep track of the name of previous file
    TCHAR* wcsFn_1st = wcsBuf;

    while(1)
    {
    	// call f_readdir repetitively to get file infos in sequence
        fr = f_readdir(&dirToPlay, &finfo);
        if(fr != FR_OK)
        {
    		r = -1;
    		sErrMsg = "Driver, file or directory does not exist";
    		goto FIND_ITEM_IN_THE_SAME_FOLDER_ERROR;
        }

        /* fn is empty, if no more files to read*/
        wcsFn = finfo.fname;
        cntFn = strnlen_TCHAR(wcsFn, _MAX_LFN);
        if(cntFn == 0U)
        {
        	// dir exhausted
        	break;
        }

        file_format_t format = judge_track_file_format(wcsFn, cntFn);
        /* FIRST: locate the file which has the same filename */
        if(fsm == 0)
        {
            size_t cntToCmp = MIN(cntFn, cntFile);
            if((cntToCmp > 0) && (memcmp(wcsFn, wcsFile, cntToCmp * sizeof(TCHAR)) == 0))
            {
                fsm = 1;
                // terminate cond: found the previous file
                if((direction < 0) && (iBefore > 0))
                {
                    found = true;
                    break;
                }
            }
            else if(format != FILE_FORMAT_UNKNOWN)
            {
                if(direction > 0)
                {
                    if(iBefore == 0)
                    {
                        // save the very first unmatched file,
                        // so when attempt to  find the next of the very last file
                        // we can wrap back by returning the first file in the dir
                        strncpy_TCHAR(wcsFn_1st, wcsFn, _MAX_LFN);
                    }
                }
                else
                {
                    // temporarily save the previous file's filename in wcsFile
                    strncpy_TCHAR(wcsFn_prev, wcsFn, _MAX_LFN);
                }
                iBefore++; // mark if a previous file has been visited
            }
        }
        else if((fsm == 1) && (format != FILE_FORMAT_UNKNOWN))
        {
        	// terminate cond: found the next file
        	if(direction > 0)
        	{
        		found = true;
        		break;
        	}
        	// terminate code: attempted to find the prev file of the very first file, which will be the wrapped back
        	else
        	{
        		// keep track of the filename, so the empty filename indicating the end of the dir will not overwrite
        		strncpy_TCHAR(wcsFn_prev, wcsFn, _MAX_LFN);
        		iAfter++;
        	}
        }
    }
    f_closedir(&dirToPlay);
    /* return the found result */
    if(direction > 0) // find next
    {
        if(found) // finfo.fname is the next filename we found
        {
            strncpy_TCHAR(wcsFilename, finfo.fname, _MAX_LFN);
            *pCntFilename = cntFn;
            r = 0;
        }
        else if(iBefore > 0) // wrap back, return the very first path
        {
            const TCHAR* pS = strncpy_TCHAR(wcsFilename, wcsFn_1st, _MAX_LFN);
            *pCntFilename = pS - wcsFilename;
            r = 0;
        }
    }
    else // find previous
    {
        // case 1: wcsFn_prev is the next filename we found
        // case 2: wrap back, return the very last path which is already stored in wcsFn_prev
        if(found || (iAfter > 0))
        {
            const TCHAR* pS = strncpy_TCHAR(wcsFilename, wcsFn_prev, _MAX_LFN);
            *pCntFilename = pS - wcsFilename;
            r = 0;
        }
    }

FIND_ITEM_IN_THE_SAME_FOLDER_ERROR:
    if(sErrMsg)
    {
        lv_lock();
        show_toast(sErrMsg);
        lv_unlock();
    }
    return r;
}


bool FS_is_EOF = false;
static void task_FS_loader(void * argument)
{
    /* This task is going to use floating point operations. Therefore it calls
       portTASK_USES_FLOATING_POINT() once on task entry, before entering the loop
       that implements its functionality. From this point on the task has a floating
       point context.
       This task also conflicts with USB MSC mode, for the two applications will race on one SDIO device
       */
    portTASK_USES_FLOATING_POINT();
    osStatus_t r;
    FRESULT fr;

    while(1)
    {
        int32_t msgCtrl;
        const char * sErrMsg = NULL;
        /* STATE 1: STOP BEGIN */
        if(FS_player_state == FS_PLAYER_STOP)
        {
            if(isPlay_lvgl)
            {
                lv_lock();
                GUI_notify_play_pause(0);
                lv_unlock();
            }
            // close unclosed file
            if(isOpen) {f_close(&fileToPlay); isOpen = false;}
            // clear up buffers reset cursor
            FS_player_reset(modePlay);
            // wait for the next control message
            r = osMessageQueueGet(qFSLoader, &msgCtrl, 0, osWaitForever);
            /* Open file and parse format BEGIN */
            if(r == osOK)
            {
            	bool needToOpenNewFile = false;
            	/* protected: mtxFSPlayer BEGIN */
            	osMutexAcquire(mtxFSPlayer, osWaitForever);
            	if(msgCtrl & FS_LOADER_SIGNAL_START)
            	{
                    needToOpenNewFile = true;
                    startPlayInstantly = true;
            	}
            	else if(msgCtrl & FS_LOADER_SIGNAL_NEXT)
            	{
            		if(isOpen_PL)
            		{
            			// has playlist opened, then switch to the previous music on the playlist
            			int rFound = playlist_next(&pl, wcsFile, wcsPathToPlay, _MAX_LFN);
            			if(rFound < 0)
            			{
            				// try to wrap back
            				sErrMsg = "已是播放列表中的最后一首";
            				rFound = playlist_goto(&pl, 0, wcsFile, wcsPathToPlay, _MAX_LFN);
            			}
            			if(rFound >= 0) // ok
            			{
            				cntFile = strnlen_TCHAR(wcsFile, _MAX_LFN);
            				cntPathToPlay = strnlen_TCHAR(wcsPathToPlay, sizeof(wcsPathToPlay) / sizeof(TCHAR));
            				cntFolder = cntPathToPlay-cntFile;
                            needToOpenNewFile = true;
            			startPlayInstantly = true;
            			}
            			else
            			{
            				sErrMsg = "播放列表中没有更多歌曲了";
            			}
            		}
            		else
            		{
            			// no playlist opened, switch to the next music in the folder
            			int rFound = find_track_in_the_same_folder(1, wcsFile, &cntFile);
            			if(rFound >= 0) // ok
            			{
                            cntPathToPlay = concat_full_path(wcsFolder, wcsFile, wcsPathToPlay, sizeof(wcsPathToPlay) / sizeof(TCHAR));
                            needToOpenNewFile = true;
            			startPlayInstantly = true;
            			}
            			else
            			{
            				sErrMsg = "没有下一曲了";
            			}
            		}
            	}
            	else if(msgCtrl & FS_LOADER_SIGNAL_PREV)
            	{
            		if(isOpen_PL)
            		{
            			// has playlist opened, then switch to the next music on the playlist
            			int rFound = playlist_prev(&pl, wcsFile, wcsPathToPlay, _MAX_LFN);
            			if(rFound < 0)
                        {
                            // try to wrap back
                            sErrMsg = "已是播放列表中的第一首";
                            rFound = playlist_goto(&pl, INT32_MAX, wcsFile, wcsPathToPlay, _MAX_LFN);
                        }
            			if(rFound == 0) // ok
            			{
            				cntFile = strnlen_TCHAR(wcsFile, _MAX_LFN);
            				cntPathToPlay = strnlen_TCHAR(wcsPathToPlay, sizeof(wcsPathToPlay) / sizeof(TCHAR));
            				cntFolder = cntPathToPlay-cntFile;
                            needToOpenNewFile = true;
                            startPlayInstantly = true;
            			}
            			else
            			{
            				sErrMsg = "已是播放列表中的第一首";
            			}
            		}
            		else
            		{
            			// no playlist opened, switch to the previous music in the folder
            			int rFound = find_track_in_the_same_folder(-1, wcsFile, &cntFile);
            			if(rFound == 0) // ok
            			{
                            cntPathToPlay = concat_full_path(wcsFolder, wcsFile, wcsPathToPlay, sizeof(wcsPathToPlay) / sizeof(TCHAR));
                            needToOpenNewFile = true;
                            startPlayInstantly = true;
            			}
            			else
            			{
            				sErrMsg = "没有上一曲了";
            			}
            		}
            	}
            	/* after all next/prev/continue commands, finally we can open the file */
            	if(needToOpenNewFile)
            	{
                    if(FS_open_file(wcsPathToPlay) == 0)
                    {
                        isOpen = true;
                        if(startPlayInstantly) FS_player_state = FS_PLAYER_PLAY;
                    }
                    else
                    {
                        sErrMsg = "无法打开文件";
                        isOpen = false;
                    }
            	}
            	osMutexRelease(mtxFSPlayer);
            	/* protected: mtxFSPlayer END */
            	// update GUI,
            	// to avoid deadlock, do not lock GUI mutex within mtxFSplayer context to
            	if(needToOpenNewFile && isOpen)
            	{
                    lv_lock();

                    GUI_notify_change_track(&track_info, duration_ms, wcsFile, cntFile, wcsPathToPlay, cntPathToPlay);

                    if(FS_player_state == FS_PLAYER_PLAY) GUI_notify_play_pause(1);

                    // SRC mark
                    GUI_notify_sample_rate_and_bitdepth(current_sample_rate, current_bit_depth);
                    lv_unlock();
            	}
            }
            /* Open file and parse format END */
        }
        /* STATE 1: STOP END */
        /* STATE 2: PLAY BEGIN */
        else if(FS_player_state == FS_PLAYER_PLAY)
        {
            /* GUI: update state */
            if(!isPlay_lvgl)
            {
                lv_lock();
                GUI_notify_play_pause(1);
                lv_unlock();
            }
            /* TRANSITIONS:  check for signals */
            r = osMessageQueueGet(qFSLoader, &msgCtrl, 0, 0);
            /* SEEK: handle and continue decoding (no state change, no FIFO flush) */
            if((r == osOK) && (msgCtrl & FS_LOADER_SIGNAL_SEEK))
            {
                osMutexAcquire(mtxFSPlayer, osWaitForever);
                uint32_t target = seek_target_frame;
                osMutexRelease(mtxFSPlayer);
                player_do_seek(target);
            }
            /* STOP/PAUSE: transition away from PLAY */
            else if((r == osOK) && ((msgCtrl & FS_LOADER_SIGNAL_START) == 0))
            {
                if(msgCtrl & FS_LOADER_SIGNAL_STOP)
                {
                    startPlayInstantly = false;
                    FS_player_state = FS_PLAYER_STOP;
                }
                else if(msgCtrl & FS_LOADER_SIGNAL_PAUSE)
                {
                    startPlayInstantly = false;
                    FS_player_state = FS_PLAYER_PAUSE;
                }
            }
            /* State: NORMAL: keep playing BEGIN */
            else
            {
                // variables below will be reset after each task cycle
//				size_t size_free = kfifo_get_free_space(&fifo_from_FS);
                size_t samplesConsumed = 0;
                size_t sizePadded = 0;
                FS_is_EOF = false;
                int decodeStatus = 0;
                /* 1. Decode loop (callback-based I/O, no bufFileLoader needed) */
                while( // keep decoding, until
                    (kfifo_get_free_space(&fifo_from_FS) >= (fifo_from_FS.size >> 1)) // FIFO 3/4 full
                    && (active_decoder) // decoder invalid
                    && (!FS_is_EOF) // reached the end of file
                    && (decodeStatus >= 0) // decoder failed
                 ) // decode until EOF (exits via FS_is_EOF → break)
                {
                    /* 2. Decode dataflow for each different fileformat BEGIN */
                    /* **********************************************
                     * Modify: sizeConsumed, samplesConsumed (for duration), needToLoadFile, sizePadded
                     * *********************************************/
                    if(1)
                    {
                        /* Dynamic batch size: larger for 96kHz (no SRC).
                         * SRC chain limited to 8192 floats per memFileDecoder:
                         *  96kHz: no SRC → full bufCastToFloat (8192 fl = 4096 fr)
                         *  48kHz: ×2 interp → 4096 fl = 2048 fr
                         *  44.1kHz: sinc×48/44.1 + ×2 interp → 3584 fl = 1792 fr */
                        size_t dec_cap = sizeof(bufCastToFloat) / sizeof(float);
                        if (oversample_frac > 0)
                            dec_cap = 1792 * MAX_CHANNELS;
                        else if (oversample > 1)
                            dec_cap = 2048 * MAX_CHANNELS;
                        size_t out_frames = 0;
                        /* key code: call the decoder */
                        int dec_ret = active_decoder->decode(
                            decoder_ctx,
                            bufCastToFloat,
                            dec_cap,
                            &out_frames);
                        if(dec_ret >= 0)
                        {
                            sizePadded = out_frames * sizeof(float);
                            samplesConsumed = out_frames / nChannels;
                            decodeStatus = 0;
                            if(dec_ret == DECODER_ERR_EOF)
                                FS_is_EOF = true;
                        }
                        else
                        {
                            decodeStatus = -1;
                        }
                    }
                    /* process decoded data BEGIN */
                    /*
                     * @dependence:
                     * sizePadded
                     * bufCastToFloat
                     * @output
                     * countPadded
                     * sizePadded
                     * bufCastToFloat
                     * */
                    if(decodeStatus >= 0) // successfully decoded
                    {
                        if(sizePadded > 0)
                        {
                            size_t countPadded = sizePadded >> 2;
                            float32_t* pTempBuf_f32 = (float32_t*)memFileDecoder1;
                            float32_t* pTempBuf2_f32 = (float32_t*)memFileDecoder2;
                            /* interpolate and filtering BEGIN */
                            if((oversample > 1) || (oversample_frac > 0))
                            {
                                // deinterlace (temp <- in) : ~ 200us for 2048 cnts
                                audio_DSP_deinterlace_channels(pTempBuf_f32, bufCastToFloat, countPadded, nChannels);

                                float* bufTempIn = pTempBuf_f32;
                                float* bufTempOut = pTempBuf2_f32;
                                // 1: fractional sinc resampler 44.1k->48k
                                if(oversample_frac > 0)
                                {
                                    size_t countEachCh_out;
                                    size_t countConsumed_in;
                                    audio_sinc_resampler_process(&audio_sinc_resampler_44K_to_48K_FS,
                                            bufTempOut,
                                            bufTempIn, countPadded / nChannels,
                                            &countEachCh_out, &countConsumed_in);
                                    countPadded = countEachCh_out * nChannels;
                                    // exchange in and out buffers
                                    float *buf = bufTempIn;
                                    bufTempIn = bufTempOut;
                                    bufTempOut = buf;
                                }
                                // 2: integer intepolator 48k->96k
                                if(oversample > 1)
                                {
                                    // interpolating (temp2 <- temp ) : ~2ms for 2048 cnts
                                    countPadded = audio_interpolator_filter_apply(arr_audio_interpolator_48K_to_96K_FS, bufTempOut, bufTempIn, countPadded,
                                            nChannels);
//                                    countPadded = audio_interpolator_simple(oversample, bufTempOut, bufTempIn, countPadded,
//                                                                              nChannels);
                                    // exchange in and out buffers
                                    float *buf = bufTempIn;
                                    bufTempIn = bufTempOut;
                                    bufTempOut = buf;
                                }
                                // interlace( out <- temp2) : ~200us for 4096 cnts
                                audio_DSP_interlace_channels(bufCastToFloat, bufTempIn, countPadded, nChannels);
                                sizePadded = countPadded * sizeof(float);
                            }
                            /* interpolate and filtering END */
                            /* volume control BEGIN*/
                            if(volumef_FS < .99f)
                                arm_scale_f32(bufCastToFloat, volumef_FS, bufCastToFloat, countPadded);
                            /* volume control END*/
                            /* put into FIFO begin */
                            uint8_t nRetry = 16;
                            // wait until FIFO has enough space
                            while(1)
                            {
                                size_t sizeFree = kfifo_get_free_space(&fifo_from_FS);
                                if(sizeFree < sizePadded)
                                {
//                                    osThreadYield(); // yield to lower-priority tasks (LVGL, input)
                                    osDelay(pdMS_TO_TICKS(40));
                                }
                                else
                                    break;
                                if(nRetry-- == 0)
                                    break;
                            }
                            kfifo_put(&fifo_from_FS, (uint8_t*)bufCastToFloat, sizePadded);
                            /* put into FIFO end */
                            samplesPlayed += samplesConsumed; // for estimating play progess
                        }
                    }
                    else
                    {
                        printf("decode failed");
                        FS_is_EOF = true;
                    }
                    /* process decoded data END */
                    /* 2.Decode dataflow END */
                    /* 3. End of play, Transition to next music BEGIN */
                    if(FS_is_EOF)
                    {
                        // wait until buffer cleared, then STOP
                        uint32_t waitms = 0;
                        while((kfifo_get_free_space(&fifo_from_FS) > 0) && (waitms < 300))
                        {
                            osDelay(pdMS_TO_TICKS(100));
                            waitms += 100;
                        };

                        switch(modePlay)
                        {
                        case FS_PLAY_MODE_ONE_LOOP:
                            FS_player_state = FS_PLAYER_STOP;
                            FS_player_signal_replay();
                            break;
                        case FS_PLAY_MODE_SEQUENTIAL:
                            if(isOpen)
                            {
                                FS_player_state = FS_PLAYER_STOP;
                                FS_player_signal_next();
                            }
                            // TODO: get next track in sequential

                            break;
                        case FS_PLAY_MODE_RANDOM:
                            if(isOpen)
                            {
                                FS_player_state = FS_PLAYER_STOP;
                                FS_player_signal_prev();
                            }
                            // TODO: round robbin pick
                            break;
                        default: // FS_PLAY_MODE_ONE_SHOT and others
                            FS_player_state = FS_PLAYER_STOP;
                        }
                        break; // exit the decoder's while loop
                    }
                    /* 3. End of play, Transition to next music END */
                }
            }
            /* State: NORMAL: keep playing END */
        }
        /* STATE 2: PLAY END */
        /* STATE 3: PAUSE BEGIN */
        else if(FS_player_state == FS_PLAYER_PAUSE)
        {
            if(isPlay_lvgl)
            {
                lv_lock();
                GUI_notify_play_pause(0);
                lv_unlock();
            }
            // wait for the next start signal
            r = osMessageQueueGet(qFSLoader, &msgCtrl, 0, 0);
            if(r == osOK)
            {
                /* TRANSITION 1:  check if STOP  signal exists */
                if(msgCtrl & FS_LOADER_SIGNAL_STOP)
                {
                    FS_player_state = FS_PLAYER_STOP;
                }
                /* TRANSITION 2: check if PAUSE signal exists */
                else if(msgCtrl & FS_LOADER_SIGNAL_START)
                {
                    FS_player_state = FS_PLAYER_PLAY;
                }
            }

        }
        /* STATE 3: PAUSE END */
        if(FS_player_state == FS_PLAYER_PLAY)
        {
            size_t sizeInFIFO = kfifo_get_filled_size(&fifo_from_FS);
//			size_t estTime_ms = sizeInFIFO / (2 * 4 * DEFAULT_SAMPLE_RATE_HZ / 1000);
            if(sizeInFIFO > fifo_from_FS.half) osDelay(pdMS_TO_TICKS(20));
        }
        else
        {
            osDelay(pdMS_TO_TICKS(100));
        }
        continue;
        /* AFTER LOOP END: ERROR clean up*/
PLAY_ERROR:
        lv_lock();
        show_msgbox_warning(sErrMsg);
        lv_unlock();

        if(isOpen)
        {
            f_close(&fileToPlay);
            isOpen = false;
        }
    }
}




/* Seek the decoder and reset SRC state for new position.
 * Does NOT clear FIFO — old data plays out naturally, then new data transitions. */
static void player_do_seek(uint32_t target_frame)
{
    if (!active_decoder || !decoder_ctx) return;
    /* target_frame is in ms, convert to PCM frame count */
    target_frame *= samplesPerMs;
    if (active_decoder->seek(decoder_ctx, target_frame) != DECODER_OK)
        return;

    /* Reset resampler states so stale history doesn't glitch */
    if (oversample_frac > 0) {
        audio_sinc_resampler_init(&audio_sinc_resampler_44K_to_48K_FS,
            audio_sinc_coeffs_44K_to_48K, 44100U, 48000U, MAX_CHANNELS);
    }
    if (oversample > 1) {
        for (int i = 0; i < MAX_CHANNELS; ++i) {
            audio_interplolator_init(&(arr_audio_interpolator_48K_to_96K_FS[i]),
                2, bufsInterpolatorStateFS[i], BLOCK_SIZE_FIR);
        }
    }

    samplesPlayed = target_frame;
    FS_is_EOF = false;
}

/* External Signal triggers */
int32_t FS_LOADER_SIGNAL_STOP  = (1 << FS_PLAYER_STOP);
int32_t  FS_LOADER_SIGNAL_START = (1 << FS_PLAYER_PLAY);
int32_t  FS_LOADER_SIGNAL_PAUSE = (1 << FS_PLAYER_PAUSE);
int32_t  FS_LOADER_SIGNAL_NEXT = (1 << FS_PLAYER_NEXT);
int32_t  FS_LOADER_SIGNAL_PREV = (1 << FS_PLAYER_PREV);
int32_t  FS_LOADER_SIGNAL_SEEK = (1 << 5);
uint32_t seek_target_frame = 0;

void FS_player_signal_start()
{

    if(taskFSLoader && qFSLoader)
    {
        osMessageQueuePut(qFSLoader, &FS_LOADER_SIGNAL_START, 0, 100);
    }
}


void FS_player_signal_stop()
{
    if(taskFSLoader && qFSLoader)
    {
        osMessageQueuePut(qFSLoader, &FS_LOADER_SIGNAL_STOP, 0, 100);
    }
}

void FS_player_signal_pause()
{
    if(taskFSLoader && qFSLoader)
    {
        osMessageQueuePut(qFSLoader, &FS_LOADER_SIGNAL_PAUSE, 0, 100);
    }
}


void FS_player_signal_replay()
{
    if(taskFSLoader && qFSLoader)
    {
        osMessageQueuePut(qFSLoader, &FS_LOADER_SIGNAL_STOP, 0, 100);
        osMessageQueuePut(qFSLoader, &FS_LOADER_SIGNAL_START, 0, 100);
    }
}


void FS_player_signal_next()
{
    if(taskFSLoader && qFSLoader)
    {
        osMessageQueuePut(qFSLoader, &FS_LOADER_SIGNAL_STOP, 0, 100);
        osMessageQueuePut(qFSLoader, &FS_LOADER_SIGNAL_NEXT, 0, 100);
    }
}


void FS_player_signal_seek(uint32_t frame_ms)
{
    if(taskFSLoader && qFSLoader)
    {
        osMutexAcquire(mtxFSPlayer, osWaitForever);
        seek_target_frame = frame_ms;
        osMutexRelease(mtxFSPlayer);
        osMessageQueuePut(qFSLoader, &FS_LOADER_SIGNAL_SEEK, 0, 100);
    }
}

void FS_player_signal_prev()
{
    if(taskFSLoader && qFSLoader)
    {
        osMessageQueuePut(qFSLoader, &FS_LOADER_SIGNAL_STOP, 0, 100);
        osMessageQueuePut(qFSLoader, &FS_LOADER_SIGNAL_PREV, 0, 100);
    }
}

void FS_player_change_track(const TCHAR *wcsFolderPath, const TCHAR *wcsFilename)
{
    // note: need to lv_lock() before call
    uint32_t cntFolderPath = strnlen_TCHAR(wcsFolderPath, _MAX_LFN);
    uint32_t cntFilename = strnlen_TCHAR(wcsFilename, _MAX_LFN);
    if(((cntFolderPath > 0) || (cntFilename > 0)))
    {
        // send stop flag
        FS_player_signal_stop();
        // record the path to the folder
        cntFolder = cntFolderPath;
        strncpy_TCHAR(wcsFolder, wcsFolderPath, _MAX_LFN);
        // record the filename
        cntFile = cntFilename;
        strncpy_TCHAR(wcsFile, wcsFilename, _MAX_LFN);
        // concat full path
        const TCHAR* wcsConcat = strncpy_TCHAR(wcsPathToPlay, wcsFolderPath, _MAX_LFN);
        cntPathToPlay = cntFolderPath + cntFilename;
        strncpy_TCHAR(wcsPathToPlay + (wcsConcat - wcsPathToPlay), wcsFilename, _MAX_LFN);
        // send start flag
        FS_player_signal_start();
    }
}


void FS_player_test()
{
    static int cnt = 0;

    if(cnt % 3 == 0)
    {
        const TCHAR *folder = _T("0:music/Aqours/【01】专辑/【00】Aqours/【4th】未体験HORIZON/");
        const TCHAR *filename = _T("001-未体験HORIZON.wav");
        lv_lock();
        FS_player_change_track(folder, filename);
        lv_unlock();
    }
    else if(cnt % 3 == 1)
    {
        FS_player_signal_next();
    }
    else if(cnt %3 == 2)
    {
        FS_player_signal_prev();
    }
    cnt++;
}
