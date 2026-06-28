/*
 * MyUSB.c
 *
 *  Created on: Apr 12, 2024
 *      Author: cpholzn
 */

#include "MyUSB.h"
#include "Config.h"

#include "main.h"
#include "fatfs.h"
#include "audio_buffers.h"
#include "audio_player.h"
#include "usbd_core.h"
#include "usbd_audio_if.h"
#include "usbd_desc.h"
#include "usbd_storage_if.h"

extern uint8_t _siUSB_CD;
extern uint8_t _sUSB_CD;
extern uint8_t _eUSB_CD;
extern uint8_t _sUSB_CD_bss;
extern uint8_t _eUSB_CD_bss;
extern PCD_HandleTypeDef hpcd_USB_OTG_HS; // defined in usbd_conf.c
extern USBD_DescriptorsTypeDef HS_Desc;
extern bool isFilesystemOK; // defined in main.c

void USB_my_init_no_cache_memory()
{
    // load DMA_no_cache_CD section with data from FLASH
    memcpy((void*)&_sUSB_CD, (void*)&_siUSB_CD, &_eUSB_CD - &_sUSB_CD);
    memset((void*)&_sUSB_CD_bss, 0, &_eUSB_CD_bss - &_sUSB_CD_bss);
}
// if nothing has been done, by default enumerate as a mass storage device
//USBCfgDescSelector_t USBCfgDescSel = USB_CFG_DESC_MASS_STORAGE;
// if nothing has been done, by default enumerate as a UAC
//USBCfgDescSelector_t USBCfgDescSel = USB_CFG_DESC_UAC1_0;
int USBD_MAX_NUM_INTERFACES_1 = USBD_MAX_NUM_INTERFACES_UAC1_0;

void USB_switch_to_configuration(USBCfgDescSelector_t sel)
{
    if(sel == USB_CFG_DESC_UAC1_0)
    {
        cfg.usbCfgDescSelector = USB_CFG_DESC_UAC1_0;
        USBD_MAX_NUM_INTERFACES_1 = USBD_MAX_NUM_INTERFACES_UAC1_0;
    }
    else if(sel == USB_CFG_DESC_MASS_STORAGE)
    {
        cfg.usbCfgDescSelector = USB_CFG_DESC_MASS_STORAGE;
        USBD_MAX_NUM_INTERFACES_1 = USBD_MAX_NUM_INTERFACES_MSC;
    }
}

void * USBD_my_static_malloc(uint32_t size)
{
  static __attribute__((section(".USB_CD"))) uint32_t memUSBD[(sizeof(USBD_AUDIO_HandleTypeDef)/4 + 1)+(sizeof(USBD_MSC_BOT_HandleTypeDef) / 4 + 1)];/* On 32-bit boundary */
  return memUSBD;
}

void USB_switch_configuration_hot(USBCfgDescSelector_t new_sel)
{
    if(new_sel == cfg.usbCfgDescSelector)
        return;

    /* ---- Phase 1: Stop current activity ---- */
    if(cfg.usbCfgDescSelector == USB_CFG_DESC_MASS_STORAGE)
    {
        /* Leaving MSC mode: unmount FatFS.
         * Risk: if the host is writing to the SD card, a forced unmount
         * may lose or corrupt data. In practice, Windows flushes file
         * buffers before releasing the device, but this is not guaranteed.
         * A 100ms delay here reduces the chance of colliding with an
         * in-flight write. */
        isFilesystemOK = false;
        osDelay(pdMS_TO_TICKS(100));
        USBD_Stop(&hUsbDeviceHS);
        USBD_DeInit(&hUsbDeviceHS);
        osDelay(pdMS_TO_TICKS(100));
    }
    else if(cfg.usbCfgDescSelector == USB_CFG_DESC_UAC1_0)
    {
        /* Leaving UAC mode: stop audio streaming to the host */
        output_to_usb_stop();
        encocder_input_stop();
        /* Stop FS player if it is reading from SD card, then unmount FatFS
         * so the SD card is fully released before MSC takes over. */
        FS_player_signal_stop();
        osDelay(pdMS_TO_TICKS(200));
        isFilesystemOK = false;
        FRESULT fr = f_mount(NULL, _T(""), 0); // unmount FS
        USBD_Stop(&hUsbDeviceHS);
        USBD_DeInit(&hUsbDeviceHS);
    }

    /* ---- Phase 2: clear USB states ---- */
    /* Clear USB DMA buffers for a clean re-init */
    memset(&hpcd_USB_OTG_HS, 0, sizeof(PCD_HandleTypeDef));

    /* ---- Phase 3: Set new configuration ---- */
    cfg.usbCfgDescSelector = new_sel;
    USBD_MAX_NUM_INTERFACES_1 = (new_sel == USB_CFG_DESC_UAC1_0)
        ? USBD_MAX_NUM_INTERFACES_UAC1_0
        : USBD_MAX_NUM_INTERFACES_MSC;

    /* ---- Phase 4: Re-init USB ---- */
    USBD_Init(&hUsbDeviceHS, &HS_Desc, DEVICE_HS);

    if(new_sel == USB_CFG_DESC_UAC1_0)
    {
        USBD_RegisterClass(&hUsbDeviceHS, &USBD_AUDIO);
        USBD_AUDIO_RegisterInterface(&hUsbDeviceHS, &USBD_AUDIO_fops_HS);

        /* Re-mount FatFS — the host may have modified the SD card while
         * in MSC mode, so the cached FatFS state is stale. */
        MX_FATFS_Init();
        SD_Driver.disk_initialize(0);
        osDelay(pdMS_TO_TICKS(200));
        FRESULT fr = f_mount(&SDFatFS, (const TCHAR*)SDPath, 1);
        isFilesystemOK = (fr == FR_OK);

        output_to_usb_init();
    }
    else
    {
        USBD_RegisterClass(&hUsbDeviceHS, &USBD_MSC);
        USBD_MSC_RegisterStorage(&hUsbDeviceHS, &USBD_Storage_Interface_fops_HS);
    }

    USBD_Start(&hUsbDeviceHS);
    HAL_PWREx_EnableUSBVoltageDetector();
}

void USB_stop(void)
{
    /* Tear down USB so FatFS can access SD card (e.g. for config save) */
    USBD_Stop(&hUsbDeviceHS);
    USBD_DeInit(&hUsbDeviceHS);
    osDelay(pdMS_TO_TICKS(50));
}

