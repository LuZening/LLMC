/*
 * MyUSB.c
 *
 *  Created on: Apr 12, 2024
 *      Author: cpholzn
 */


#include "MyUSB.h"
#include "Config.h"

extern uint8_t _siUSB_CD;
extern uint8_t _sUSB_CD;
extern uint8_t _eUSB_CD;
extern uint8_t _sUSB_CD_bss;
extern uint8_t _eUSB_CD_bss;
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
