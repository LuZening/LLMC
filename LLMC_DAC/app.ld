/*
******************************************************************************
**
**  File        : LinkerScript.ld
**
**  Author      : STM32CubeIDE
**
**  Abstract    : Linker script for STM32H7 series
**                128Kbytes FLASH and 1376Kbytes RAM
**
**                Set heap size, stack size and stack location according
**                to application requirements.
**
**                Set memory bank area and size if external memory is used.
**
**  Target      : STMicroelectronics STM32
**
**  Distribution: The file is distributed as is, without any warranty
**                of any kind.
**
*****************************************************************************
** @attention
**
** Copyright (c) 2022 STMicroelectronics.
** All rights reserved.
**
** This software is licensed under terms that can be found in the LICENSE file
** in the root directory of this software component.
** If no LICENSE file comes with this software, it is provided AS-IS.
**
****************************************************************************
*/

/* Entry Point */
ENTRY(Reset_Handler)

/* Highest address of the user mode stack */
_estack = ORIGIN(RAM) + LENGTH(RAM);    /* end of RAM */
/* Generate a link error if heap and stack don't fit into RAM */
_Min_Heap_Size = 0x2000 ;      /* required amount of heap  */
_Min_Stack_Size = 0x2000 ; /* required amount of stack */

/* Specify the memory areas RAM_CD should be 128K */
MEMORY
{
  ITCMRAM (xrw)  : ORIGIN = 0x00000000, LENGTH = 64K
  FLASH (rx)     : ORIGIN = 0x90000000, LENGTH = 16M
  DTCMRAM1 (xrw) : ORIGIN = 0x20000000, LENGTH = 64K
  DTCMRAM2 (xrw) : ORIGIN = 0x20010000, LENGTH = 64K
  RAM (xrw)      : ORIGIN = 0x24000000, LENGTH = 1024K
  RAM_CD (xrw)   : ORIGIN = 0x30000000, LENGTH = 128K 
  RAM_SRD (xrw)  : ORIGIN = 0x38000000, LENGTH = 32K
}

/* Define output sections */
SECTIONS
{
  /* The startup code goes first into FLASH */
  
  .isr_vector :
  {
    . = ALIGN(4);
    KEEP(*(.isr_vector)) /* Startup code */
    . = ALIGN(4);
  } >FLASH /* put vector ISR in DTCM */

  /* The program code and other data goes into FLASH */
  .text :
  {
    . = ALIGN(4);
    *(.text)           /* .text sections (code) */
    *(.text*)          /* .text* sections (code) */
    *(.glue_7)         /* glue arm to thumb code */
    *(.glue_7t)        /* glue thumb to arm code */
    *(.eh_frame)

    KEEP (*(.init))
    KEEP (*(.fini))

    . = ALIGN(4);
    _etext = .;        /* define a global symbols at end of code */
  } >FLASH

  /* Constant data goes into FLASH */
  .rodata :
  {
    . = ALIGN(4);
    *(.rodata)         /* .rodata sections (constants, strings, etc.) */
    *(.rodata*)        /* .rodata* sections (constants, strings, etc.) */
    . = ALIGN(4);
  } >FLASH


  /*user added for Fonts*/
  .font :
  {
    . = ALIGN(4);
    KEEP (*(.font*))
    . = ALIGN(4);
  } >FLASH
  
  .ARM.extab   : { *(.ARM.extab* .gnu.linkonce.armextab.*) } >FLASH
  .ARM : {
    __exidx_start = .;
    *(.ARM.exidx*)
    __exidx_end = .;
  } >FLASH

  .preinit_array     :
  {
    PROVIDE_HIDDEN (__preinit_array_start = .);
    KEEP (*(.preinit_array*))
    PROVIDE_HIDDEN (__preinit_array_end = .);
  } >FLASH

  .init_array :
  {
    PROVIDE_HIDDEN (__init_array_start = .);
    KEEP (*(SORT(.init_array.*)))
    KEEP (*(.init_array*))
    PROVIDE_HIDDEN (__init_array_end = .);
  } >FLASH

  .fini_array :
  {
    PROVIDE_HIDDEN (__fini_array_start = .);
    KEEP (*(SORT(.fini_array.*)))
    KEEP (*(.fini_array*))
    PROVIDE_HIDDEN (__fini_array_end = .);
  } >FLASH
  
/*User added for ITCM */
. = ALIGN(4);
 .ITCMRAM_isr_vector (NOLOAD): 
 {
 	
 	KEEP (*(.ITCMRAM_isr_vector))
 	. = ORIGIN(ITCMRAM) + 0x400;
 } >ITCMRAM
  
 _siITCMRAM_heap = LOADADDR(.ITCMRAM_heap);
 . = ALIGN(4);
 .ITCMRAM_heap : 
 {
 	_sITCMRAM_heap = .;
 	KEEP (*(.ITCMRAM_heap))
 	. = ALIGN(4);
    _eITCMRAM_heap = .;
 } >ITCMRAM AT>FLASH
/*User added for DTCMRAM1 */

  _siDTCMRAM1_heap = LOADADDR(.DTCMRAM1_heap);
  . = ALIGN(4);
  .DTCMRAM1_heap :
  {
    
    _sDTCMRAM1_heap = .;
    KEEP (*(.DTCMRAM1_heap*))
    . = ALIGN(4);
    _eDTCMRAM1_heap = .;
  } >DTCMRAM1 AT>FLASH
  
   _siDTCMRAM2_heap = LOADADDR(.DTCMRAM2_heap);
   . = ALIGN(4);
  .DTCMRAM2_heap :
  {
    _sDTCMRAM2_heap = .;
    
    KEEP (*(.DTCMRAM2_heap*))
    . = ALIGN(4);
    _eDTCMRAM2_heap = .;
  } >DTCMRAM2 AT>FLASH
  
/*User added for Framebuffer (no cache)*/
  .framebuffer1 (NOLOAD) :
  {
    . = ORIGIN(RAM);
    KEEP (*(.framebuffer1))
  } >RAM
  
  /*User added for audio cache*/
  . = ALIGN(4);
  .DMA_no_cache :
  {
  	
    *fatfs*.o(.bss* .data* COMMON)
    KEEP (*(.DMA_no_cache))
    . = ORIGIN(RAM) + 256K + 32K + 16K;
  } >RAM AT>FLASH
  
  . = ALIGN(4);
  .framebuffer2 (NOLOAD):
  {
    KEEP (*(.framebuffer2))
  } >RAM


  
 /*User added for USB cache*/ 
 /*
   	*usb*.o(.data*)
    *USB*.o(.data*)
 */
  _siUSB_CD = LOADADDR(.USB_CD);
  . = ALIGN(4);
  .USB_CD :
  {
  	
  	_sUSB_CD = .;

   	*usb*.o(.data*)
    *USB*.o(.data*)

    KEEP (*(.USB_CD))
    . = ALIGN(4);
    _eUSB_CD = .;
  } >RAM_CD AT>FLASH
  
/*
  	*usb*.o(.bss* COMMON)
    *USB*.o(.bss* COMMON)
*/
  . = ALIGN(4);
  .USB_CD_bss :
  {
  	
  	_sUSB_CD_bss = .;

  	*usb*.o(.bss* COMMON)
    *USB*.o(.bss* COMMON)
    KEEP (*(.USB_CD_bss))
    . = ALIGN(4);
    _eUSB_CD_bss = .;

  } >RAM_CD

  . = ALIGN(4);
  .RAM_CD_heap (NOLOAD):
  {
  	
    KEEP (*(.RAM_CD_heap))

  } >RAM_CD
  
  /* used by the startup to initialize data */
  _sidata = LOADADDR(.data);

  /* Initialized data sections goes into RAM, load LMA copy after code */
  .data :
  {
    . = ALIGN(4);
    _sdata = .;        /* create a global symbol at data start */
    *(EXCLUDE_FILE (fat*.o) .data*)
    *(.data)           /* .data sections */
    *(.data*)          /* .data* sections */
    *(.RamFunc)        /* .RamFunc sections */
    *(.RamFunc*)       /* .RamFunc* sections */

    . = ALIGN(4);
    _edata = .;        /* define a global symbol at data end */
  } >RAM AT> FLASH

  /* Uninitialized data section */
  . = ALIGN(4);
  .bss :
  {
    /* This is used by the startup in order to initialize the .bss section */
    _sbss = .;         /* define a global symbol at bss start */
    __bss_start__ = _sbss;

    *(.bss)
    *(.bss*)
    *(COMMON)

	
    . = ALIGN(4);
    _ebss = .;         /* define a global symbol at bss end */
    __bss_end__ = _ebss;
  } >RAM

  /* User_heap_stack section, used to check that there is enough RAM left */
  ._user_heap_stack :
  {
    . = ALIGN(8);
    PROVIDE ( end = . );
    PROVIDE ( _end = . );
    . = . + _Min_Heap_Size;
    . = . + _Min_Stack_Size;
    . = ALIGN(8);
  } >RAM

  /* Remove information from the standard libraries */
  /DISCARD/ :
  {
    libc.a ( * )
    libm.a ( * )
    libgcc.a ( * )
  }

  .ARM.attributes 0 : { *(.ARM.attributes) }
}
