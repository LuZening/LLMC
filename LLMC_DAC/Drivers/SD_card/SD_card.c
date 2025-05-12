/*
 * SD_card.c
 *
 *  Created on: Apr 4, 2024
 *      Author: cpholzn
 */

#include "SD_card.h"

#include <stdio.h>
#include <string.h>

#include "main.h"

#define BLOCK_START_ADDR         0     /* Block start address      */
#define NUM_OF_BLOCKS            1     /* 扇区编号  */
#define BUFFER_WORDS_SIZE        ((BLOCKSIZE * NUM_OF_BLOCKS) >> 2) /* Total data size in bytes */

uint32_t i;




// 打印SD卡基本信息
void show_sdcard_info(SD_HandleTypeDef *phsd)
{
		printf("Micro SD Card Test...\r\n");
		/* 检测SD卡是否正常（处于数据传输模式的传输状态） */
		if(HAL_SD_GetCardState(phsd) == HAL_SD_CARD_TRANSFER)
		{
				printf("Initialize SD card successfully!\r\n");
				// 打印SD卡基本信息
				printf(" SD card information! \r\n");
				printf(" CardCapacity  				: %llu \r\n", (unsigned long long)phsd->SdCard.BlockSize * phsd->SdCard.BlockNbr);// 显示容量
				printf(" CardBlockSize 				: %u \r\n", phsd->SdCard.BlockSize);   // 块大小
				printf(" LogBlockNbr   				: %u \r\n", phsd->SdCard.LogBlockNbr);	// 逻辑块数量
				printf(" LogBlockSize  				: %u \r\n", phsd->SdCard.LogBlockSize);// 逻辑块大小
				printf(" RCA                 	: %u \r\n", phsd->SdCard.RelCardAdd);  // 卡相对地址
				printf(" CardType            	: %u \r\n", phsd->SdCard.CardType);    // 卡类型
				// 读取并打印SD卡的CID信息
				HAL_SD_CardCIDTypeDef sdcard_cid;
				HAL_SD_GetCardCID(phsd,&sdcard_cid);
				printf(" ManufacturerID: %d \r\n",sdcard_cid.ManufacturerID);
		}
		else
		{
				printf("SD card init fail!\r\n" );
		}
}

