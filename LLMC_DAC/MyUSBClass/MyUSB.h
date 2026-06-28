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

/* Hot-switch USB configuration without rebooting.
 * Safely tears down the current config and re-initializes as the new one.
 * Host will see a disconnect/reconnect (~1-2s re-enumeration). */
void USB_switch_configuration_hot(USBCfgDescSelector_t new_sel);

void * USBD_my_static_malloc(uint32_t size);
void USB_stop(void); /* tear down USB so FatFS can access SD card */
