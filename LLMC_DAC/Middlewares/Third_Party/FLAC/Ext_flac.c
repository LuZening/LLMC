/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: main.c 25844 2010-05-06 17:35:13Z kugel $
 *
 * Copyright (C) 2005 Dave Chapman
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <inttypes.h>
#include <string.h>
#include <stdbool.h>
#include "FLAC/decoder.h"

#ifndef DECODER_SIM
#include "Audio/audio_settings.h" // defined MAX_CHANNELS
#include "ff.h"
#else
#include "sim.h"
#endif


int8_t PCM_buffer0[4 * MAX_BLOCKSIZE ];   //放置送CODE的PCM数据流,同时也与temp_buffer组合充当了之前版本的fc.decoded0(临时PCM数据)
int8_t PCM_buffer1[4 * MAX_BLOCKSIZE ];	  //放置送CODE的PCM数据流,同时也与temp_buffer组合充当了之前版本的fc.decoded1(临时PCM数据)
int8_t temp_buffer[4 * MAX_BLOCKSIZE ];

void flac_dump_headers(FLACContext *s)
{
    printf("\n\r  Blocksize: %d .. %d\n\r", s->min_blocksize,
                   s->max_blocksize);
    printf("  Framesize: %d .. %d\n\r", s->min_framesize,
                   s->max_framesize);
    printf("  Samplerate: %d\n\r", s->samplerate);
    printf("  Channels: %d\n\r", s->channels);
    printf("  Bits per sample: %d\n\r", s->bps);
    printf("  Metadata length: %d\n\r", s->metadatalength);
    printf("  Total Samples: %lu\n\r",s->totalsamples);
    printf("  Duration: %d ms\n\r",s->length);
    printf("  Bitrate: %d kbps\n\r",s->bitrate);
}

int parse_flac(FIL *fp, FLACContext* fc, uint8_t* bufFileLoader, size_t sizeBuf)
{
    uint8_t* buf = bufFileLoader;
    bool found_streaminfo=false;
    int endofmetadata=0;
    int blocklength;
    uint32_t* p;
    uint32_t seekpoint_lo,seekpoint_hi;
   	uint32_t offset_lo,offset_hi;

    if (f_rewind(fp) != FR_OK)
    {
        return -1;
    }
    uint32_t lenRead;
    if ((f_read(fp, buf, 4, &lenRead) != FR_OK))
    {
        return -1;
    }
    if(lenRead < 4)
    	return -1;

    if (memcmp(buf, "fLaC", 4) != 0)
    {
        return -2;
    }

    fc->metadatalength = 4;

    while (!endofmetadata) 
	{
        if (f_read(fp, buf, 4, &lenRead) != FR_OK)
        {
            return -3;
        }
        if(lenRead < 4)
        	return -3;

        endofmetadata = (buf[0]&0x80);
        blocklength = (buf[1] << 16) | (buf[2] << 8) | buf[3];
        fc->metadatalength+=blocklength+4;

        if ((buf[0] & 0x7f) == 0)       /* 0 is the STREAMINFO block */
        {
            /* FIXME: Don't trust the value of blocklength */
            if (f_read(fp, buf, blocklength, &lenRead) != FR_OK)
            {
                return -4;
            }

//            f_stat(fp, &finfo);
            fc->filesize = f_size(fp);
            fc->min_blocksize = (buf[0] << 8) | buf[1];
            fc->max_blocksize = (buf[2] << 8) | buf[3];
            fc->min_framesize = (buf[4] << 16) | (buf[5] << 8) | buf[6];
            fc->max_framesize = (buf[7] << 16) | (buf[8] << 8) | buf[9];
            fc->samplerate = (buf[10] << 12) | (buf[11] << 4) 
                             | ((buf[12] & 0xf0) >> 4);
            fc->channels = ((buf[12]&0x0e)>>1) + 1;
            fc->bps = (((buf[12]&0x01) << 4) | ((buf[13]&0xf0)>>4) ) + 1;

            /* totalsamples is a 36-bit field, but we assume <= 32 bits are 
               used */
            fc->totalsamples = (buf[14] << 24) | (buf[15] << 16) 
                               | (buf[16] << 8) | buf[17];

            /* Calculate track length (in ms) and estimate the bitrate 
               (in kbit/s) */
            fc->length = (fc->totalsamples / fc->samplerate) * 1000;

            found_streaminfo=true;
        }
		else if ((buf[0] & 0x7f) == 3) 	/* 3 is the SEEKTABLE block */
		{
               // rt_kprintf("Seektable length = %d bytes\n",blocklength);
			while (blocklength >= 18)
			{
                f_read(fp,buf,18,&lenRead);
                if (lenRead < 18) return -5;
                blocklength-=lenRead;// check
                p=(uint32_t*)buf;
                seekpoint_hi=betoh32(*(p++));
                seekpoint_lo=betoh32(*(p++));
                offset_hi=betoh32(*(p++));
                offset_lo=betoh32(*(p++));
            
                if ((seekpoint_hi != 0xffffffff) && (seekpoint_lo != 0xffffffff))
				{
         	         //rt_kprintf("Seekpoint: %u, Offset=%u\n",seekpoint_lo,offset_lo);
                }
            }
			f_lseek(fp, f_tell(fp) + blocklength);
        }
		else
		  {
			/* Skip to next metadata block */
			if (f_lseek(fp, f_tell(fp) + blocklength) != FR_OK)
			{
				return -6;
			}
		  }
    }

   if (found_streaminfo)
   {
       fc->bitrate = ((fc->filesize-fc->metadatalength) * 8) / fc->length;
       return 0;
   } 
   else 
   {
       return -6;
   }
}

#if 0
static int play_flac(char* path)
{
    FLACContext fc;
    int fd;
    int n;
    int bytesleft;
    int consumed;
	int8_t i;

/*  文件buffer 用rt_malloc放到外部RAM
 *	为什么那么痛苦不把PCM buffer也放到外部RAM呢?
 *	原因很简单:外部RAM慢! 导致会断流,
 *  这里把文件buffer放到外部RAM主要有两个原因
 *	1.只有64K的内部RAM 不得不这么做
 *	2.文件buffer的"吞吐量"少
 */
	unsigned char *filebuf ; 

	/* audio device */
	rt_device_t snd_device;

	extern void vol(uint16_t v) ;
	vol(50); 

	//本次没用内存池..主要因为这种情况下不知道如何使用 ^_^
	if (rt_sem_init(&flac_sem, "flac_sem", 2, RT_IPC_FLAG_FIFO) != RT_EOK)
		rt_kprintf("init flac_sem semaphore failed\n");

    fd=open(path,O_RDONLY,0);

    if (fd < 0) {
        rt_kprintf("Can not parse %s\n",path);
        return(1);
    }

	/* open audio device */
	snd_device = rt_device_find("snd");
	if (snd_device != RT_NULL)
	{
		/*  set tx complete call back function 
		 *  设置回调函数,当DMA传输完毕时,会执行flac_decoder_tx_done
		 */
		rt_device_set_tx_complete(snd_device, flac_decoder_tx_done);
		rt_device_open(snd_device, RT_DEVICE_OFLAG_WRONLY);
	}


    /* Read the metadata and position the file pointer at the start of the 
       first audio frame */
    parse_flac(fd, &fc);
    dump_headers(&fc);

	if((fc.min_blocksize != fc.max_blocksize) || (fc.max_blocksize > MAX_BLOCKSIZE ) || (fc.max_framesize > MAX_FRAMESIZE))
	{
	  rt_kprintf("\n\rOo Do not support this file!!\n\r"); 
	  rt_kprintf("You can choose another Converter.Such as foobar2000 ^_^\n\r"); 
	  return false;
	}


	//set CODEC's samplerate
	rt_device_control(snd_device, CODEC_CMD_SAMPLERATE, &(fc.samplerate));

	filebuf = (unsigned char *)rt_malloc(MAX_FRAMESIZE); /* The input buffer */
    bytesleft=read(fd,filebuf,MAX_FRAMESIZE);

      
       while (bytesleft) 
	   {
    
		rt_sem_take(&flac_sem, RT_WAITING_FOREVER);

		//轮流使用两PCM_buffer
	    if(i++ & 1)
	    {
	    	//decoded0,decoded1用来存放临时PCM数据
			fc.decoded0 = (int32_t *)PCM_buffer0;
			fc.decoded1 = (int32_t *)temp_buffer;

        	if(flac_decode_frame(&fc,filebuf,bytesleft,(int16_t *)PCM_buffer0) < 0) 
			{
             rt_kprintf("DECODE ERROR, ABORTING\n");
             break;
 	        }

			rt_device_write(snd_device, 0, (int16_t *)PCM_buffer0, ((fc.max_blocksize * 4) ));
	    }
	    else
	    {	
	     	//decoded0,decoded1用来存放临时PCM数据
	  		fc.decoded0 = (int32_t *)PCM_buffer1;
    		fc.decoded1 = (int32_t *)temp_buffer;

	  		if(flac_decode_frame(&fc,filebuf,bytesleft,(int16_t *)PCM_buffer1) < 0) 
			{
       	      rt_kprintf("DECODE ERROR, ABORTING\n");
       	      break;
       		}

			rt_device_write(snd_device, 0, (int16_t *)PCM_buffer1, ((fc.max_blocksize * 4) ));
	    }

        consumed=fc.gb.index/8;
        rt_memmove(filebuf,&filebuf[consumed],bytesleft-consumed);
        bytesleft-=consumed;

        n=read(fd,&filebuf[bytesleft],MAX_FRAMESIZE-bytesleft);
        if (n > 0) 
		{
            bytesleft+=n;
        }
		 
		
      }

	/* close device and file */
    rt_device_close(snd_device);
	rt_free(filebuf);
    close(fd);
    return(0);
}
#endif
#ifdef RT_USING_FINSH
#include <finsh.h>
FINSH_FUNCTION_EXPORT(flac, flac(char* path) );
#endif
