#pragma once

#include "usbd_audio.h"
#include "usbd_msc.h"

#include "usbd_def.h"


extern USBD_HandleTypeDef hUsbDeviceHS;

/* ****** Required Modifications on HAL library codes *************
 * Modify usbd_conf.h :　remove #define USBD_MAX_NUM_INTERFACES
 * Modify usbd_def.h : remove #define USBD_MAX_NUM_INTERFACES
 * Modify usbd_ctlreq.c : declare extern int USBD_MAX_NUM_INTERFACES;
 * */
void USB_my_init_no_cache_memory();

typedef enum
{
	USB_CFG_DESC_UAC1_0,
	USB_CFG_DESC_MASS_STORAGE,
} USBCfgDescSelector_t;


extern int USBD_MAX_NUM_INTERFACES_1;

void USB_switch_to_configuration(USBCfgDescSelector_t sel);

void * USBD_my_static_malloc(uint32_t size);
