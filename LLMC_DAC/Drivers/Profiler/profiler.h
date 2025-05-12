/*
 * profiler.h
 *
 *  Created on: 2024年9月20日
 *      Author: cpholzn
 */

#ifndef PROFILER_PROFILER_H_
#define PROFILER_PROFILER_H_
#include <stdint.h>


void enable_DWT();

#define N_PROFILER_COUNTER_ENTRIES 128
typedef struct
{
	uint32_t cyc;
	uint8_t iwr;
	int arrCycDiffs[N_PROFILER_COUNTER_ENTRIES];
	int cycDiff_max, cycDiff_mean;
} profiler_counter_t;

void init_profiler_counter(profiler_counter_t* p);


void profiler_mark(profiler_counter_t* p);
void _profiler_mark(profiler_counter_t* p, uint32_t cycnow);


#endif /* PROFILER_PROFILER_H_ */
