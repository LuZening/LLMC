/*
 * audio_settings.c
 *
 *  Created on: 2024年4月16日
 *      Author: cpholzn
 */

#include "main.h"
#include "audio_settings.h"
#include "audio_buffers.h"
#include "PCM179x.h"
#include "TLV320ADCx120.h"
#include "Config.h"
#include "arm_math.h"

#define AUDIO_FCLK_HZ 24576000U

#define E 2.718281828459045f
#define MUTE_DB -60
float volume100_to_volumef(uint8_t volume100)
{
    float vf = 0.;
    // db ranges between -60db ~ 0db
    if(volume100 > 0)
    {
        float db =  ((float)volume100 - 100.f) *  (-MUTE_DB) / 100.f;
        vf = powf(10, db/20);
    }
    return vf;
}

uint8_t volume100_to_255(uint8_t volume100)
{
    int newvol = ((int)volume100 * 255) / 100;
    if(newvol > 255)
        newvol = 255;
    return newvol;
}

uint8_t volume100_linear_to_log(uint8_t volume100)
{
    // 0 -> 0
    // 10 -> 70
    // 20 -> 80
    // 50 -> 90
    // 100 -> 100
    int newvol;
    if(volume100 == 0)
        newvol = 0;
    else if(volume100 < 10)
        newvol = 7 * volume100;
    else if(volume100 < 20)
        newvol = 70 + (volume100 - 10);
    else if(volume100 < 50)
        newvol = 80 + (volume100 - 20) / 3;
    else
        newvol = 90 + (volume100 - 50) / 5;

    return newvol;
}
const uint32_t arr_valid_audio_sample_rates_Hz[N_VALID_AUDIO_SAMPLE_RATES] = {44100, 48000, 96000};
const uint8_t arr_valid_audio_bit_depths[N_VALID_AUDIO_BIT_DEPTHS] = {16, 24};

uint32_t audio_sample_rate_to_Hz(audio_sample_rate_t t)
{
    /*
     *
     * 	AUDIO_SAMPLE_RATE_44K1 = 0,
    AUDIO_SAMPLE_RATE_48K,
    AUDIO_SAMPLE_RATE_96K,
     */
    return arr_valid_audio_sample_rates_Hz[(int)t];
}

audio_sample_rate_t audio_sample_rate_from_Hz(uint32_t Hz)
{
    /*
     *
     * 	AUDIO_SAMPLE_RATE_44K1 = 0,
    AUDIO_SAMPLE_RATE_48K,
    AUDIO_SAMPLE_RATE_96K,
    AUDIO_SAMPLE_RATE_192K,
     */
    int i;
    for(i = 0; i < N_VALID_AUDIO_SAMPLE_RATES; ++i)
    {
        if(Hz == arr_valid_audio_sample_rates_Hz[i]) break;
    }
    return (audio_sample_rate_t)i;
}


//unsigned int audio_sample_rate_SAI; // for SAI init, unit Hz, e.g. 96000U
void set_audio_sample_rate_and_bit_depth(uint8_t idx, audio_sample_rate_t rate_new, uint8_t bitdepth_new)
{
    //TODO: how to set FS for decoder PCM1792, now decoder PCM1792 auto detects frequency
    if(idx == 0) // decoder - speaker
    {
        pcm179x_init(&pcm179x, AUDIO_FCLK_HZ);
        switch(rate_new)
        {
        case AUDIO_SAMPLE_RATE_96K:
            SAI_decoder_init(96000U);
            PCM179x_set_sample_rate(&pcm179x, 96000U);
            PCM179x_set_oversample_multiplier(&pcm179x, 128);
            break;
        case AUDIO_SAMPLE_RATE_48K:
            SAI_decoder_init(48000U);
            PCM179x_set_sample_rate(&pcm179x, 48000U);
            PCM179x_set_oversample_multiplier(&pcm179x, 128);
            break;
        default:
            break;
        };

        cfg.audio_sample_rate = rate_new;
        cfg.audio_sample_rate_Hz = audio_sample_rate_to_Hz(rate_new);
        cfg.audio_bit_depth = bitdepth_new;
        isModified = true;

    }
    else if(idx == 1) // encoder - microphone
    {
//		adcx120_init(&adcx120); // init in main
//		adcx120_test(&adcx120);
        switch(rate_new)
        {
        case AUDIO_SAMPLE_RATE_96K:
            SAI_encoder_init(96000U);
            adcx120_set_format(&adcx120, adcx120.format, bitdepth_new, ADCX120_FS_88K2_OR_96K);
            break;
        case AUDIO_SAMPLE_RATE_48K:
            SAI_encoder_init(48000U);
            adcx120_set_format(&adcx120, adcx120.format, bitdepth_new, ADCX120_FS_44K1_OR_48K);
            break;
        default:
            break;
        }


    }

}

void set_mic_LEDs(uint8_t i, uint8_t on)
{
    switch(i)
    {
    case 0:
        HAL_GPIO_WritePin(LED_MIC1_GPIO_Port, LED_MIC1_Pin, 1-on);
        break;
    case 1:
        HAL_GPIO_WritePin(LED_MIC2_GPIO_Port, LED_MIC2_Pin, 1-on);
        break;
    }
}

void audio_connections_apply(uint16_t *audio_connections)
{
    // audio_connections has N_OUTPUTS entrys
    uint16_t outflag_decoder =  audio_connections[0];
    uint16_t outflag_USB_record = audio_connections[1];
    // update global flags
    channels_connected_to_decoder = outflag_decoder;
    channels_connected_to_USB_record = outflag_USB_record;

    int i;

    bool isEncoderSel[N_MICROPHONES];
    for(i = 0; i < N_MICROPHONES; ++i)
        isEncoderSel[i] = false;
    bool isDecoderSel = false;
    uint8_t DecoderChEn = 0;
    isDecoderSel = (outflag_decoder != 0);
    for(i = 0; i < N_OUTPUTS; ++i)
    {
        // each decoder ch is selected
        DecoderChEn |= (audio_connections[i] & 0b11U);
        // any of the encoder is selected
        if((audio_connections[i] & MUXMIXER_SELECT_ENCODER_CH_1) != 0)
        {
            isEncoderSel[0] = true;

        }
        if((audio_connections[i] & MUXMIXER_SELECT_ENCODER_CH_2) != 0)
        {
            isEncoderSel[1] = true;

        }
    }


    /* turn on NP power if decoder or encoder is on */
    if(isEncoderSel[0] ||isEncoderSel[1] || isDecoderSel)
        NP_power_enable(1);
//	else
//		NP_power_enable(0);

    /* init decoder */
    if(isDecoderSel)
    {
        // will init if necessary
        output_to_decoder_start();
        if((DecoderChEn & 0b01) > 0)
        {
            encoder_preamp_power_enable(1, 1);
        }
        if((DecoderChEn & 0b10) > 0)
        {
            encoder_preamp_power_enable(2, 1);
        }
    }
    else
    {
        output_to_decoder_stop();
        encoder_preamp_power_enable(1, 0);
        encoder_preamp_power_enable(2,0);

    }


    /* init encoder */
    if(isEncoderSel[0] || isEncoderSel[1])
    {
        // will init if necessary
        encocder_input_start();
        for(int j = 0; j < N_MICROPHONES; ++j)
        {
            if(isEncoderSel[j])
            {
                // turn on MIC1 LED
                set_mic_LEDs(j, 1);
            }
            else
            {
                // turn off MIC1 LED
                set_mic_LEDs(j, 0);
            }
        }

    }
    else
    {
        encocder_input_stop();
    }

    if(outflag_USB_record > 0)
    {

        output_to_usb_start();
    }
    else
    {
        output_to_usb_stop();
    }


}

void audio_outpus_dsp_connection_apply(const uint16_t *output_dsp_cnnection)
{
    memcpy(outputs_dsp_usage, output_dsp_cnnection, N_OUTPUTS * sizeof(uint16_t));
}



