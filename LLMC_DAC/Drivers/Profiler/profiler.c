/*
 * profiler.c
 *
 *  Created on: 2024年9月20日
 *      Author: cpholzn
 */

#include "profiler.h"
#include "main.h"
#include <string.h>

void enable_DWT()
{
	// DWT and ITM units enabled
	CoreDebug->DEMCR  |= CoreDebug_DEMCR_TRCENA_Msk;
	// Enable write access
	DWT->LAR = 0xC5ACCE55;
	// Clear CYCCNT
	DWT->CYCCNT = 0;
	// CYCCNT counter enable
	DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}


void init_profiler_counter(profiler_counter_t* p)
{
	memset(p, 0, sizeof(profiler_counter_t));
}

void profiler_mark(profiler_counter_t* p)
{
	uint32_t cycnow = DWT->CYCCNT;
	_profiler_mark(p, cycnow);
}

void _profiler_mark(profiler_counter_t* p, uint32_t cycnow)
{
	if(p->cyc > 0)
	{
		int cycdiff = cycnow - p->cyc;
		if(p->cycDiff_mean > 0)
			p->cycDiff_mean = (p->cycDiff_mean + cycdiff) >> 1;
		else
			p->cycDiff_mean = cycdiff;

		if(cycdiff > p->cycDiff_max)
			p->cycDiff_max = cycdiff;


		p->arrCycDiffs[p->iwr] = cycdiff;
		if((++p->iwr) >= N_PROFILER_COUNTER_ENTRIES)
			p->iwr = 0;
	}
	p->cyc = cycnow;
	return;

}
