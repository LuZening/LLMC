/*
 * audio_player.h
 *
 *  Created on: Apr 8, 2024
 *      Author: cpholzn
 */
#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "cmsis_os2.h"

#include "ff.h"
#include "playlist.h"

extern osThreadId_t taskFSLoader;

extern uint8_t decoder_pool_arena[];
extern TCHAR wcsPathToPlay[_MAX_LFN * 2 + 1];

extern playlist_t pl;

extern size_t samplesPerMs; // protected by mtxFSPlayer
extern size_t samplesPlayed; // no protection, dirty read is okay for GUI

#define FS_FIFO_SIZE  128U * 1024U // 96KBytes

#define SIZE_MEMPOOL_FILE_DECODER_1 (32*1024U) // 32KBytes
#define SIZE_MEMPOOL_FILE_DECODER_2 (32*1024U) // 32KBytes

#define  FLAG_FIFO_HALF_EMPTY (1U << 2)
typedef float float32_t;
extern float32_t memFileDecoder1[SIZE_MEMPOOL_FILE_DECODER_1 / sizeof(float32_t)];
extern float32_t memFileDecoder2[SIZE_MEMPOOL_FILE_DECODER_2 / sizeof(float32_t)];
typedef enum
{
    FS_PLAY_MODE_ONE_SHOT = 0,
    FS_PLAY_MODE_ONE_LOOP,
    FS_PLAY_MODE_SEQUENTIAL,
    FS_PLAY_MODE_RANDOM,
} FS_play_mode_t;
extern FS_play_mode_t modePlay;


typedef enum
{
    FS_PLAYER_STOP = 0,
    FS_PLAYER_PLAY,
    FS_PLAYER_PAUSE,
    FS_PLAYER_NEXT,
    FS_PLAYER_PREV,
} FS_player_state_t;
extern FS_player_state_t FS_player_state;
extern bool FS_is_EOF;

extern int32_t FS_LOADER_SIGNAL_STOP;
extern int32_t  FS_LOADER_SIGNAL_START;
extern int32_t  FS_LOADER_SIGNAL_PAUSE;
extern int32_t  FS_LOADER_SIGNAL_NEXT;
extern int32_t  FS_LOADER_SIGNAL_PREV;
extern int32_t  FS_LOADER_SIGNAL_SEEK;
extern uint32_t seek_target_frame;
// Thread
extern osMessageQueueId_t qFSLoader;
extern osMutexId_t mtxFSPlayer;
void FS_player_init();
void FS_player_task_start();


// Control signals
void FS_player_signal_start();
void FS_player_signal_stop();
void FS_player_signal_pause();
void FS_player_signal_replay(); // replay the current selected track from the beginning
void FS_player_signal_prev();
void FS_player_signal_next();
void FS_player_signal_seek(uint32_t frame_ms); // seek to millisecond position

// external accesses
// note: need to lv_lock() before call
void FS_player_change_track(const TCHAR *wcsFolder, const TCHAR *wcsFilename);

bool is_valid_wav_file(const TCHAR* path, size_t len);

void FS_player_test();


