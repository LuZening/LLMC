/*
 * LVGL_flush.c
 *
 *  Created on: Feb 28, 2024
 *      Author: cpholzn
 */


#include "lvgl.h"
#include "stm32h7b0xx.h"
#include "LVGL_GUI.h"

/* display buffer */
lv_display_t* display;

// ONE FULL FRAMEBUFFER FOR LTDC
uint16_t __attribute__((section(".framebuffer1"))) framebuf1[LV_HOR_RES_MAX * LV_VER_RES_MAX];
// TWO HALF FRAMEBUFFERS FOR LVGL, cached
uint16_t __attribute__((section(".framebuffer2"))) framebuf2A[LV_HOR_RES_MAX * LV_VER_RES_MAX];
//uint16_t __attribute__((section(".framebuffer2"))) framebuf2B[LV_HOR_RES_MAX * LV_VER_RES_MAX / 2];

volatile uint8_t g_gpu_state = 0; // LTDC selected buffer
static void DMA2D_MemCopy(uint32_t pixelFormat, void * pSrc, void * pDst, int xSize, int ySize, int lineoffset_src, int lineoffset_dst)
{
    /* DMA2D配置 */
    DMA2D->CR      = 0x00000000UL;
    DMA2D->FGMAR   = (uint32_t)pSrc;
    DMA2D->OMAR    = (uint32_t)pDst;
    DMA2D->FGOR    = lineoffset_src;
    DMA2D->OOR     = lineoffset_dst;
    DMA2D->FGPFCCR = pixelFormat;
    DMA2D->NLR     = (uint32_t)(xSize << 16) | (uint16_t)ySize;

    /* 打开中断 */
    DMA2D->CR |= DMA2D_CR_TCIE | DMA2D_CR_TEIE | DMA2D_CR_CEIE;

    /* 启动传输 */
    DMA2D->CR   |= DMA2D_CR_START;

    g_gpu_state = 1;
//    /* 等待DMA2D传输完成 */
//    while (DMA2D->CR & DMA2D_CR_START) {}
}


void DMA2D_XferCpltCallback(DMA2D_HandleTypeDef *hdma2d)
{
	 /* Inform the graphics library that you are ready with the flushing*/
	g_gpu_state = 0;
	lv_display_flush_ready(display);
}

void LTDC_flush_cb(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map)
{
	/* A: transfer each part*/
//	  const int bytesPerPx = 2;
//	  int w = area->x2 - area->x1 + 1;
//	  int h = area->y2 - area->y1 + 1;
//	  size_t offset = bytesPerPx*(LV_HOR_RES_MAX * area->y1 + area->x1);
//	  uint16_t lineoffset = LV_HOR_RES_MAX - w;
	if(lv_display_flush_is_last(disp))
	{

	/* B: only transfer the full screen by DMA2D at the last flush*/
	  int w = LV_HOR_RES_MAX;
	  int h = LV_VER_RES_MAX;
	  size_t offset = 0;
	  uint16_t lineoffset = 0;

	  size_t addrSrc = (size_t)px_map + offset;
	  size_t addrDst = (size_t)framebuf1 + offset;
	  uint16_t skipPxSrc = 0;

		// note: framebuffer2 is cached, must invalidate first before DMA2D
	  SCB_CleanDCache_by_Addr((uint8_t*)framebuf2A, sizeof(framebuf2A));
	  DMA2D_MemCopy(LTDC_PIXEL_FORMAT_RGB565, (void*)addrSrc, (void*)addrDst,
			w,h, lineoffset, lineoffset);

	  /* IMPORTANT!!! NO BLOCKING, do not call here
	   * Inform the graphics library that you are ready with the flushing*/
	  //  lv_display_flush_ready(disp);
	}
	else
	{
		lv_display_flush_ready(disp); // fake a flush ready signal when not the last one
	}
}
