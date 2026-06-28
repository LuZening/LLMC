#include "lvgl.h"
#include "LVGL_GUI.h"
#define SCREEN_WIDTH 480
#define SCREEN_HEIGHT 272

#ifndef LVGL_SIM
#include "main.h"
#include "audio_DSP.h"
#include "audio_buffers.h"
#include "audio_settings.h"
#include "ff.h"
#include "config.h"
#else
#define N_EQ_SEGMENTS 8
#define N_INPUTS 4
#define N_OUTPUTS 2
const char* arr_DSP_names[N_INPUTS] = { "麦克风1", "麦克风2", "USB回放", "USB录音" };

typedef struct
{
    float gains_db[N_EQ_SEGMENTS];
} audio_EQ_cfg_t;
audio_EQ_cfg_t cfgEQ_default;
typedef struct
{
    float attack_time_ms;
    float release_time_ms;
    float threshold_db;
    float makeup_gain_db;
    float ratio;
    float knee_width_db;
} audio_compress_cfg_t;
audio_compress_cfg_t cfgCompress_default;
typedef struct
{
    /* comb filter */
    float predelay_ms;
    float decay_ms;
    /* allpass filter */
    float feedback; // -0.9 < x < 0.9
} audio_reverb_cfg_t;
audio_reverb_cfg_t cfgReverb_default;
#define REVERB_PREDELAY_MS_MAX 20U
const float EQ_center_freqs[N_EQ_SEGMENTS] = { 30.f, 180.f, 330.f, 600.f, 1000.f, 4000.0f, 12000.0f, 16000.0f };
#endif
// Function to create the DSP UI
static bool arr_is_created[N_INPUTS] = { 0 };
// record how many windows are created, to avoid repeated creation
static lv_group_t *arr_pgroups[N_INPUTS] = { NULL };
static lv_timer_t* arr_timers[N_INPUTS] = {NULL};
static lv_obj_t* arr_progbars_compress[N_INPUTS] = {NULL};
static float arr_compress_gain_lin_peak[N_INPUTS];


static lv_obj_t* arr_EQ_pSliders[N_INPUTS][N_EQ_SEGMENTS] = {NULL};

#define N_COMPRESS_PARAMS 6
const char *arr_compress_varnames[N_COMPRESS_PARAMS] =
{ "Attack(ms)", "Release(ms)", "Threshold(db)", "Makeup(db)", "Ratio", "Knee(db)" };
const int arr_compress_range_max[N_COMPRESS_PARAMS] =
{ 20, 200, 0, 60, 60, 60 };
const int arr_compress_range_min[N_COMPRESS_PARAMS] =
{ 0, 0, -240, 0, 10, 0 };
const int arr_compress_multipliers[N_COMPRESS_PARAMS] =
{ 1, 1, 10, 10, 10, 10 }; // 1:1, 10:0.1


#define N_REVERB_PARAMS 3
const char *arr_reverb_varnames[N_REVERB_PARAMS] =
{ "predelay(ms)", "decay(ms)", "feedback", };
const int arr_reverb_range_max[N_REVERB_PARAMS] =
{ REVERB_PREDELAY_MS_MAX, 200, 9, };
const int arr_reverb_range_min[N_REVERB_PARAMS] =
{ 0, 0 , -9,};
const int arr_reverb_multipliers[N_REVERB_PARAMS] =
{ 1, 1 , 10,}; // 1:1, 10:0.1

static char sValue[8]; // at most 1 for sign and 2 for digits
// Event handler for the close button click event
static void close_btn_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t* btn = lv_event_get_target_obj(e);
    if (code == LV_EVENT_CLICKED)
    {
        // Add the operation to close the window here, e.g., close the current screen or perform other closing logic
        // note: the main container is the parent widget of the close button
        // note: the main container also carries the pointer to the preset_dropdown widget
        lv_obj_t *cont = (lv_obj_t*) lv_obj_get_parent(btn);
        int idx_DSP = (int)lv_event_get_user_data(e);
        dismiss_dsp_ui(cont, idx_DSP);
    }
}

// 预设文件代码设置的 dropdown 事件处理函数
static void preset_dropdown_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_VALUE_CHANGED)
    {
        // idx_DSP is carried with event user data
        int idx_DSP = ((int) lv_event_get_user_data(e)) & 0xff;
        lv_obj_t *dropdown = lv_event_get_target(e);
        int idxPreset = lv_dropdown_get_selected(dropdown);
#ifndef LVGL_SIM
        if(idxPreset > 0)
        {
            audio_DSP_t* pDSP = arr_DSPs[idx_DSP];
            if(pDSP)
            {
                // load preset from file system
                int r = load_and_apply_DSP_preset(pDSP, idxPreset);
                // load failed, move drop down value to 0 (which stands for no preset)
                if(r == 0)
                {
                    cfg.dsp_preset_ids[idx_DSP] = idxPreset;
                    isModified = true;
                }
                else
                {
//					lv_dropdown_set_selected(dropdown, cfg.dsp_preset_ids[idx_DSP]);
                    show_toast("加载DSP预设失败，将创建新预设");
                }
            }
        }
#endif
    }
}


static void master_switch_event_handler(lv_event_t* e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_VALUE_CHANGED)
    {
        // idx_DSP is carried with event user data
        int idx_DSP = ((int)lv_event_get_user_data(e)) & 0xff;
        lv_obj_t* sw = lv_event_get_target(e);
        bool now_enabled = lv_obj_get_state(sw) & LV_STATE_CHECKED;
#ifndef LVGL_SIM
        if(now_enabled)
        {
            // enable the DSP to all outputs
            cfg.outputs_dsp_usage[OUTPUT_IDX_DECODER] |= (1 << idx_DSP);
            cfg.outputs_dsp_usage[OUTPUT_IDX_USB_RECORDING] |= (1 << idx_DSP);
        }
        else
        {
            // disable the DSP to all outputs
            cfg.outputs_dsp_usage[OUTPUT_IDX_DECODER] &= ~(1 << idx_DSP);
            cfg.outputs_dsp_usage[OUTPUT_IDX_USB_RECORDING] &= ~(1 << idx_DSP);
        }
        isModified = true;
#endif
    }
}

// listen wet record dry switch
static void drywet_switch_event_handler(lv_event_t* e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_VALUE_CHANGED)
    {
        // idx_DSP is carried with event user data
        int idx_DSP = ((int)lv_event_get_user_data(e)) & 0xff;
        lv_obj_t* sw = lv_event_get_target(e);
        bool now_enabled = lv_obj_get_state(sw) & LV_STATE_CHECKED;
#ifndef LVGL_SIM
        if(now_enabled)
        {
            // to enable the listen wet/record dry mode, the output should bypass the DSP
            if(idx_DSP < N_MICROPHONES) // all microphones share the same settings
            {
                // turn off DSP for both 2 mic channels
                cfg.outputs_dsp_usage[OUTPUT_IDX_USB_RECORDING] &= ~((1U << N_MICROPHONES) - 1U ); // 2mics : ~0b11
            }
            else
            {
                // turn of DSP for the designated channel
                cfg.outputs_dsp_usage[OUTPUT_IDX_USB_RECORDING] &= ~(1 << idx_DSP);
            }
        }
        else
        {
            // to disable the listen wet/record dry mode, the output engage the DSP
            if(idx_DSP < N_MICROPHONES)
            {
                cfg.outputs_dsp_usage[OUTPUT_IDX_USB_RECORDING] |= cfg.outputs_dsp_usage[OUTPUT_IDX_DECODER] & ((1U << N_MICROPHONES) - 1U); // 2mics : 0b11
            }
            else
            {
                cfg.outputs_dsp_usage[OUTPUT_IDX_USB_RECORDING] |= cfg.outputs_dsp_usage[OUTPUT_IDX_DECODER] & (1U << idx_DSP);
            }
        }
        isModified = true;
#endif
    }
}

// Event handler for the 8-band graphic equalizer's LED enable click event
static void eq_led_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    // idx_DSP is carried with event user data
    int idx_DSP = ((int) lv_event_get_user_data(e)) & 0xff;
    // the event is triggered by a led
    lv_obj_t *obj = lv_event_get_target(e);
    if (code == LV_EVENT_CLICKED)
    {
        bool enabled = lv_led_get_brightness(obj) > LV_LED_BRIGHT_MIN;
#ifndef LVGL_SIM
        audio_DSP_t *pDSP = arr_DSPs[idx_DSP];
        uint32_t funcbit = DSP_FUNCTION_BIT_EQ;
        if(pDSP)
            audio_DSP_set_function_bit(pDSP, funcbit, (!enabled)?1:0);
        else
            return;
#endif
        if (enabled)
        {
            // to turn off
            lv_led_set_brightness(obj, 0);
        }
        else
        {
            // to turn on
            lv_led_set_brightness(obj, 0xff);
        }
    }
}

static void eq_slider_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    // idx_DSP is carried with event user data
    int idx_DSP = ((int) lv_event_get_user_data(e)) & 0xff;
    int idx_band = ((int) lv_event_get_user_data(e)) >> 8;
    // the event is triggered by a slider
    lv_obj_t *slider = lv_event_get_target(e);
    // label_value is carried with the slider
    lv_obj_t *label_value = (lv_obj_t*) lv_obj_get_user_data(slider); // Get the value label
    if (code == LV_EVENT_VALUE_CHANGED)
    {
        int v = lv_slider_get_value(slider); // Update the predelay value
        number2text(sValue, v, 1, 0); // 10x
        lv_label_set_text(label_value, sValue);
#ifndef LVGL_SIM
        float gain = (float) v / 10.f;
        audio_EQ_modify_gain(&arr_DSPs[idx_DSP]->eq, idx_band, gain);
#endif   
    }
}

static void eq_reset_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    // idx_DSP is carried with event user data
    int idx_DSP = ((int) lv_event_get_user_data(e)) & 0xff;
    // label_value is carried with the slider
    if (code == LV_EVENT_CLICKED)
    {
        for(int idx_band = 0; idx_band < N_EQ_SEGMENTS; ++idx_band)
        {
            lv_obj_t *slider = arr_EQ_pSliders[idx_DSP][idx_band];
            if(slider)
            {
                // CHANGE slider value
                lv_slider_set_value(slider, 0, LV_ANIM_OFF);
                // change value label
                lv_obj_t *label_value = (lv_obj_t*) lv_obj_get_user_data(slider); // Get the value label
                number2text(sValue, 0, 1, 0); // to 0
                lv_label_set_text(label_value, sValue);
                // recalculate DSP coeffs
        #ifndef LVGL_SIM
                float gain = 0.;
                audio_EQ_modify_gain(&arr_DSPs[idx_DSP]->eq, idx_band, gain);
        #endif
            }
        }
    }
}

// Event handler for the compress effector's LED enable click event
static void compress_led_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    // idx_DSP is carried with event user data
    int idx_DSP = ((int) lv_event_get_user_data(e)) & 0xff;
    // the event is triggered by a led
    lv_obj_t *obj = lv_event_get_target(e);
    if (code == LV_EVENT_CLICKED)
    {
        bool enabled = lv_led_get_brightness(obj) > LV_LED_BRIGHT_MIN;
#ifndef LVGL_SIM
        audio_DSP_t *pDSP = arr_DSPs[idx_DSP];
        uint32_t funcbit = DSP_FUNCTION_BIT_COMPRESS;
        if(pDSP)
            audio_DSP_set_function_bit(pDSP, funcbit, (!enabled)?1:0);
        else
            return;
#endif
        if (enabled)
        {
            // to turn off
            lv_led_set_brightness(obj, 0);
        }
        else
        {
            // to turn on
            lv_led_set_brightness(obj, 0xff);
        }
    }
}

// Event handler for the compress effector's attack time slider event
static void compress_attack_slider_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    // idx_DSP is carried with event user data
    int idx_DSP = ((int) lv_event_get_user_data(e)) & 0xff;
    // the event is triggered by a slider
    lv_obj_t *slider = lv_event_get_target(e);
    // label_value is carried with the slider
    lv_obj_t *label_value = (lv_obj_t*) lv_obj_get_user_data(slider); // Get the value label
    if (code == LV_EVENT_VALUE_CHANGED)
    {
        int v = lv_slider_get_value(slider); // Update the predelay value
        number2text(sValue, v, 0, 0);
        lv_label_set_text(label_value, sValue);
#ifndef LVGL_SIM
        audio_compress_cfg_t cfgNew = arr_DSPs[idx_DSP]->compress.cfg;
        cfgNew.attack_time_ms = (float)v;
        audio_compress_modify_params(&arr_DSPs[idx_DSP]->compress, &cfgNew);
#endif   
    }
}

// Event handler for the compress effector's release time slider event
static void compress_release_slider_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    // idx_DSP is carried with event user data
    int idx_DSP = ((int) lv_event_get_user_data(e)) & 0xff;
    // the event is triggered by a slider
    lv_obj_t *slider = lv_event_get_target(e);
    // label_value is carried with the slider
    lv_obj_t *label_value = (lv_obj_t*) lv_obj_get_user_data(slider); // Get the value label
    if (code == LV_EVENT_VALUE_CHANGED)
    {
        int v = lv_slider_get_value(slider); // Update the predelay value
        number2text(sValue, v, 0, 0);
        lv_label_set_text(label_value, sValue);
#ifndef LVGL_SIM
        audio_compress_cfg_t cfgNew = arr_DSPs[idx_DSP]->compress.cfg;
        cfgNew.release_time_ms = (float)v;
        audio_compress_modify_params(&arr_DSPs[idx_DSP]->compress, &cfgNew);
#endif   
    }
}

// Event handler for the compress effector's threshold  slider event
static void compress_threshold_slider_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    // idx_DSP is carried with event user data
    int idx_DSP = ((int) lv_event_get_user_data(e)) & 0xff;
    // the event is triggered by a slider
    lv_obj_t *slider = lv_event_get_target(e);
    // label_value is carried with the slider
    lv_obj_t *label_value = (lv_obj_t*) lv_obj_get_user_data(slider); // Get the value label
    if (code == LV_EVENT_VALUE_CHANGED)
    {
        int v = lv_slider_get_value(slider); // Update the predelay value
        number2text(sValue, v, 1, 0); // 10x
        lv_label_set_text(label_value, sValue);
#ifndef LVGL_SIM
        audio_compress_cfg_t cfgNew = arr_DSPs[idx_DSP]->compress.cfg;
        cfgNew.threshold_db = (float) v / 10.f;
        audio_compress_modify_params(&arr_DSPs[idx_DSP]->compress, &cfgNew);
#endif   
    }
}

// Event handler for the compress effector's makeup slider event
static void compress_makeup_slider_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    // idx_DSP is carried with event user data
    int idx_DSP = ((int) lv_event_get_user_data(e)) & 0xff;
    // the event is triggered by a slider
    lv_obj_t *slider = lv_event_get_target(e);
    // label_value is carried with the slider
    lv_obj_t *label_value = (lv_obj_t*) lv_obj_get_user_data(slider); // Get the value label
    if (code == LV_EVENT_VALUE_CHANGED)
    {
        int v = lv_slider_get_value(slider); // Update the predelay value
        number2text(sValue, v, 1, 0);
        lv_label_set_text(label_value, sValue);
#ifndef LVGL_SIM
        audio_compress_cfg_t cfgNew = arr_DSPs[idx_DSP]->compress.cfg;
        cfgNew.makeup_gain_db = (float) v / 10.f;
        audio_compress_modify_params(&arr_DSPs[idx_DSP]->compress, &cfgNew);
#endif   
    }
}

// Event handler for the compress effector's ratio slider event
static void compress_ratio_slider_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    // idx_DSP is carried with event user data
    int idx_DSP = ((int) lv_event_get_user_data(e)) & 0xff;
    // the event is triggered by a slider
    lv_obj_t *slider = lv_event_get_target(e);
    // label_value is carried with the slider
    lv_obj_t *label_value = (lv_obj_t*) lv_obj_get_user_data(slider); // Get the value label
    if (code == LV_EVENT_VALUE_CHANGED)
    {
        int v = lv_slider_get_value(slider); // Update the predelay value
        number2text(sValue, v, 1, 0); // 10x
        lv_label_set_text(label_value, sValue);
#ifndef LVGL_SIM
        audio_compress_cfg_t cfgNew = arr_DSPs[idx_DSP]->compress.cfg;
        cfgNew.ratio = (float) v / 10.f;
        audio_compress_modify_params(&arr_DSPs[idx_DSP]->compress, &cfgNew);
#endif   
    }
}

// Event handler for the compress effector's knee slider event
static void compress_knee_slider_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    // idx_DSP is carried with event user data
    int idx_DSP = ((int) lv_event_get_user_data(e)) & 0xff;
    // the event is triggered by a slider
    lv_obj_t *slider = lv_event_get_target(e);
    // label_value is carried with the slider
    lv_obj_t *label_value = (lv_obj_t*) lv_obj_get_user_data(slider); // Get the value label
    if (code == LV_EVENT_VALUE_CHANGED)
    {
        int v = lv_slider_get_value(slider); // Update the predelay value
        number2text(sValue, v, 1, 0); // 10x
        lv_label_set_text(label_value, sValue);
#ifndef LVGL_SIM
        audio_compress_cfg_t cfgNew = arr_DSPs[idx_DSP]->compress.cfg;
        cfgNew.knee_width_db = (float) v / 10.f;
        audio_compress_modify_params(&arr_DSPs[idx_DSP]->compress, &cfgNew);
#endif
    }
}

lv_event_cb_t arr_compress_slider_event_cb[N_COMPRESS_PARAMS] =
{ compress_attack_slider_event_handler, compress_release_slider_event_handler,
        compress_threshold_slider_event_handler,
        compress_makeup_slider_event_handler,
        compress_ratio_slider_event_handler,
        compress_knee_slider_event_handler, };

// Event handler for the reverb effector's LED enable click event
static void reverb_led_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    // idx_DSP is carried with event user data
    int idx_DSP = ((int) lv_event_get_user_data(e)) & 0xff;
    // the event is triggered by a led
    lv_obj_t *obj = lv_event_get_target(e);
    if (code == LV_EVENT_CLICKED)
    {
        bool enabled = lv_led_get_brightness(obj) > LV_LED_BRIGHT_MIN;
#ifndef LVGL_SIM
        audio_DSP_t *pDSP = arr_DSPs[idx_DSP];
        uint32_t funcbit = DSP_FUNCTION_BIT_REVERB;
        if(pDSP)
            audio_DSP_set_function_bit(pDSP, funcbit, (!enabled)?1:0);
        else
            return;
#endif
        if (enabled)
        {
            // to turn off
            lv_led_set_brightness(obj, 0);
        }
        else
        {
            // to turn on
            lv_led_set_brightness(obj, 0xff);
        }

    }
}

// Event handler for the reverb effector's predelay slider event
static void reverb_predelay_slider_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    // idx_DSP is carried with event user data
    int idx_DSP = ((int) lv_event_get_user_data(e)) & 0xff;
    // the event is triggered by a slider
    lv_obj_t *slider = lv_event_get_target(e);
    // label_value is carried with the slider
    lv_obj_t *label_value = (lv_obj_t*) lv_obj_get_user_data(slider); // Get the value label
    if (code == LV_EVENT_VALUE_CHANGED)
    {
        int v = lv_slider_get_value(slider); // Update the predelay value
        number2text(sValue, v, 0, 0);
        lv_label_set_text(label_value, sValue);
#ifndef LVGL_SIM
        audio_DSP_t* pDSP = arr_DSPs[idx_DSP];
        if(pDSP)
        {
            audio_reverb_cfg_t cfgNew  = pDSP->reverb.cfg;
            cfgNew.predelay_ms = v;
            audio_reverb_modify_params(&pDSP->reverb, &cfgNew);
        }
#endif   
    }
}

// Event handler for the reverb effector's predelay slider event
static void reverb_decay_slider_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    // idx_DSP is carried with event user data
    int idx_DSP = ((int) lv_event_get_user_data(e)) & 0xff;
    // the event is triggered by a slider
    lv_obj_t *slider = lv_event_get_target(e);
    // label_value is carried with the slider
    lv_obj_t *label_value = (lv_obj_t*) lv_obj_get_user_data(slider); // Get the value label
    if (code == LV_EVENT_VALUE_CHANGED)
    {
        int v = lv_slider_get_value(slider); // Update the predelay value
        number2text(sValue, v, 0, 0);
        lv_label_set_text(label_value, sValue);
#ifndef LVGL_SIM
        audio_DSP_t* pDSP = arr_DSPs[idx_DSP];
        if(pDSP)
        {
            audio_reverb_cfg_t cfgNew  = pDSP->reverb.cfg;
            cfgNew.decay_ms = v;
            audio_reverb_modify_params(&pDSP->reverb, &cfgNew);
        }
#endif   
    }
}

// Event handler for the reverb effector's predelay slider event
static void reverb_feedback_slider_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    // idx_DSP is carried with event user data
    int idx_DSP = ((int) lv_event_get_user_data(e)) & 0xff;
    // the event is triggered by a slider
    lv_obj_t *slider = lv_event_get_target(e);
    // label_value is carried with the slider
    lv_obj_t *label_value = (lv_obj_t*) lv_obj_get_user_data(slider); // Get the value label
    if (code == LV_EVENT_VALUE_CHANGED)
    {
        int v = lv_slider_get_value(slider); // Update the predelay value
        number2text(sValue, v, 1, 0); // 10x
        lv_label_set_text(label_value, sValue);
#ifndef LVGL_SIM
        audio_DSP_t* pDSP = arr_DSPs[idx_DSP];
        if(pDSP)
        {
            audio_reverb_cfg_t cfgNew  = pDSP->reverb.cfg;
            cfgNew.feedback = (float)v / 10.f;
            audio_reverb_modify_params(&pDSP->reverb, &cfgNew);
        }
#endif
    }
}

lv_event_cb_t arr_reverb_slider_event_cb[N_REVERB_PARAMS] =
{ reverb_predelay_slider_event_handler, reverb_decay_slider_event_handler, reverb_feedback_slider_event_handler };



static void value_update_timer_cb(lv_timer_t* timer)
{
    //  call each 100ms to check if the window can be dismissed
    static char sValue[16];
    int idx_DSP = (int)lv_timer_get_user_data(timer);

    if(!arr_is_created[idx_DSP])
        return;

    // update compression
    if(arr_progbars_compress[idx_DSP])
    {
        lv_obj_t* bar = arr_progbars_compress[idx_DSP];
        lv_obj_t* value_label = (lv_obj_t*)lv_obj_get_user_data(bar);
#ifndef LVGL_SIM
        audio_DSP_t* pDSP =  arr_DSPs[idx_DSP];
        if(pDSP)
        {
            float gain_lin = pDSP->compress.history_gain; // must < 1

            if(gain_lin < arr_compress_gain_lin_peak[idx_DSP])
            {
                arr_compress_gain_lin_peak[idx_DSP] = gain_lin;
            }
            else
            {
                const float alpha = 0.1f;
                arr_compress_gain_lin_peak[idx_DSP] = (1-alpha) * arr_compress_gain_lin_peak[idx_DSP] +  alpha * gain_lin;
            }
            int gain_db_abs_10x = (-200.0f * log10f(arr_compress_gain_lin_peak[idx_DSP])); // minus keeps value positive
            lv_bar_set_value(bar, gain_db_abs_10x, LV_ANIM_OFF);
            uint8_t lenWrite = number2text(sValue, gain_db_abs_10x, 1, 0);
            sValue[lenWrite++] = 'd'; sValue[lenWrite++] = 'B'; sValue[lenWrite] = 0;
            lv_label_set_text(value_label, sValue);
        }
#endif
    }
}



void dismiss_dsp_ui(lv_obj_t *cont, int idx_DSP)
{
    // idx DSP is carried by the main container
    lv_obj_t* preset_dropdown = (lv_obj_t*)lv_obj_get_user_data(cont);
    int idxPreset = lv_dropdown_get_selected(preset_dropdown);
    /* save preset */
#ifndef LVGL_SIM
    if(idx_DSP >= 0)
    {
        audio_DSP_t* pDSP = arr_DSPs[idx_DSP];
        if(pDSP)
        {
            if(save_DSP_preset(pDSP, idxPreset) != 0) // failed
            {
                show_toast("DSP预设保存失败！");
            }
        }
    }
#endif
    /* delete widgets */
    if (arr_is_created[idx_DSP])
    {
        lv_obj_delete_async(cont);
        arr_is_created[idx_DSP] = false;
        deregister_group(arr_pgroups[idx_DSP]);
        arr_pgroups[idx_DSP] = NULL;
        lv_timer_delete(arr_timers[idx_DSP]);
        arr_timers[idx_DSP] = NULL;
    }
}

lv_obj_t* create_dsp_ui(int idx_DSP)
{
    if (arr_is_created[idx_DSP])
        return NULL;
#ifndef LVGL_SIM
    if (arr_DSPs[idx_DSP] == NULL)
        return NULL;
#endif
    lv_group_t *pgroup = lv_group_create();
    register_group(pgroup);
    lv_group_set_default(pgroup);
    arr_pgroups[idx_DSP] = pgroup;

#ifndef LVGL_SIM
    audio_DSP_t *pDSP = arr_DSPs[idx_DSP];
#endif

    // Create the screen
    lv_obj_t *scr = lv_scr_act();
    lv_obj_t *cont = lv_obj_create(scr);
    lv_obj_set_size(cont, LV_PCT(96), LV_PCT(90));
    lv_obj_add_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(cont, LV_DIR_VER);
    lv_obj_set_style_bg_opa(cont, LV_OPA_COVER, 0);
    lv_obj_center(cont);
    // Create the close button
    lv_obj_t *close_btn = lv_btn_create(cont);
    lv_group_add_obj(pgroup, close_btn);
    lv_obj_set_size(close_btn, 50, 30);
    lv_obj_set_pos(close_btn, 10, 10);
    // user_data： send the idx_DSP as a parameter to the event callback, so the prest of the target DSP could be saved in the callback
    lv_obj_add_event_cb(close_btn, close_btn_event_handler, LV_EVENT_CLICKED, (void*)idx_DSP);
    lv_obj_t *close_label = lv_label_create(close_btn);
    lv_label_set_text(close_label, "返回");

    // create DSP index indication
    lv_obj_t* label_DSP = lv_label_create(cont);
    lv_obj_align_to(label_DSP, close_btn, LV_ALIGN_OUT_RIGHT_MID, 0, 0);
    lv_obj_add_style(label_DSP, &lvStyleTextLarge,0);
    if(arr_DSP_names[idx_DSP])
        lv_label_set_text(label_DSP, arr_DSP_names[idx_DSP]);

    // create DSP master ON/OFF switch
    lv_obj_t* label_master = lv_label_create(cont);
    lv_obj_add_style(label_master, &lvStyleTextLarge, 0);
    lv_obj_align_to(label_master, label_DSP, LV_ALIGN_OUT_RIGHT_MID, 5, 0);
    lv_label_set_text(label_master, "DSP开关");
    lv_obj_t* sw_master = lv_switch_create(cont);
    lv_obj_align_to(sw_master, label_master, LV_ALIGN_OUT_RIGHT_MID, 0, 0);
    lv_obj_add_event_cb(sw_master, master_switch_event_handler, LV_EVENT_VALUE_CHANGED, (void*)idx_DSP);
#ifndef LVGL_SIM
    if ((cfg.outputs_dsp_usage[OUTPUT_IDX_DECODER] & (1U << idx_DSP)) != 0)// 1 indicates that the DSP is on
    {
        lv_obj_add_state(sw_master, LV_STATE_CHECKED);
    }
#endif
    // Create the preset file ID setting
    lv_obj_t *preset_dropdown = lv_dropdown_create(cont);
    // store preset_dropdown the container, so the preset can be saved when dismiss
    lv_obj_set_user_data(cont, (void*) (preset_dropdown));
    lv_group_add_obj(pgroup, preset_dropdown);
    lv_dropdown_set_options(preset_dropdown, "无\n1\n2\n3\n4\n5\n6\n7\n8\n9");
    lv_obj_set_size(preset_dropdown, 80, 30);
    lv_obj_align_to(preset_dropdown, sw_master, LV_ALIGN_OUT_RIGHT_MID, 35, 0);
#ifndef LVGL_SIM
    if (cfg.dsp_preset_ids[idx_DSP] > 0)
    {
        lv_dropdown_set_selected(preset_dropdown, cfg.dsp_preset_ids[idx_DSP]); // no need to load and apply the preset here, coz it has already been loaded in the default task
    }
    else
    {
        lv_dropdown_set_selected(preset_dropdown, 0); // no preset
    }
#endif
    lv_obj_add_event_cb(preset_dropdown, preset_dropdown_event_handler,
        LV_EVENT_VALUE_CHANGED , (void*)idx_DSP);
    lv_obj_t *preset_label = lv_label_create(cont);
    lv_label_set_text(preset_label, "预设");
    lv_obj_align_to(preset_label, preset_dropdown, LV_ALIGN_OUT_LEFT_MID, 0,
            0);
    // create listen wet / record dry option
    lv_obj_t* label_drywet = lv_label_create(cont);
    lv_obj_add_style(label_drywet, &lvStyleTextLarge, 0);
    lv_obj_align_to(label_drywet, preset_dropdown, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
    lv_label_set_text(label_drywet, "听湿录干");
    lv_obj_t* sw_drywet = lv_switch_create(cont);
    lv_obj_align_to(sw_drywet, label_drywet, LV_ALIGN_OUT_RIGHT_MID, 5, 0);
    lv_obj_add_event_cb(sw_drywet, drywet_switch_event_handler, LV_EVENT_VALUE_CHANGED, (void*)idx_DSP);
    // sync drywet switch state
#ifndef LVGL_SIM
    if ((cfg.outputs_dsp_usage[OUTPUT_IDX_USB_RECORDING] & (1U << idx_DSP)) == 0)// 0 indicates that the output bypasses the DSP
    {
        lv_obj_add_state(sw_drywet, LV_STATE_CHECKED);
    }
#endif
    // Create the 8-band graphic equalizer area
    lv_obj_t *eq_area = lv_obj_create(cont);
    lv_obj_remove_flag(eq_area, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_style(eq_area, &lvStyleInvisibleCont, 0);
    lv_obj_set_size(eq_area, LV_PCT(90), LV_PCT(90));
    lv_obj_align(eq_area, LV_ALIGN_TOP_MID, 0, 60);

    // Create the 8-band graphic equalizer's LED enable control
    lv_obj_t *eq_led = lv_led_create(eq_area);
    lv_group_add_obj(pgroup, eq_led);
    lv_obj_set_size(eq_led, 20, 20);
    lv_obj_align(eq_led, LV_ALIGN_TOP_MID, -100, 5);
#ifndef LVGL_SIM
    // sync LED state
    if(pDSP && ((pDSP->function_bits & DSP_FUNCTION_BIT_EQ) != 0))
    {
        lv_led_set_brightness(eq_led, 255);
    }
    else
        lv_led_set_brightness(eq_led, 0);
#endif
    // user_data:  send the idx_DSP as a parameter to the event callback, so its value could be updated
    lv_obj_add_event_cb(eq_led, eq_led_event_handler, LV_EVENT_CLICKED,
            (void*) idx_DSP);
    lv_obj_t *eq_led_label = lv_label_create(eq_area);
    lv_label_set_text(eq_led_label, "均衡器");
    lv_obj_align_to(eq_led_label, eq_led, LV_ALIGN_OUT_RIGHT_MID, 20, 0);
    // reset EQ button
    lv_obj_t *eq_reset_btn = lv_button_create(eq_area);
    lv_obj_t *eq_reset_caption = lv_label_create(eq_reset_btn);
    lv_obj_set_size(eq_reset_btn, LV_SIZE_CONTENT, 30);
    lv_label_set_text(eq_reset_caption, "重置参数");
    lv_obj_align_to(eq_reset_btn, eq_led_label, LV_ALIGN_OUT_RIGHT_MID, 20, 0);
    lv_obj_add_event_cb(eq_reset_btn, eq_reset_event_handler, LV_EVENT_CLICKED, (void*)idx_DSP);
    // Create the 8-band graphic equalizer's gain adjustment sliders
#ifndef LVGL_SIM
    audio_EQ_cfg_t *peq_cfg = &pDSP->eq.cfg;
#else
    audio_EQ_cfg_t *peq_cfg = &cfgEQ_default;
#endif
    for (int i = 0; i < N_EQ_SEGMENTS; i++)
    {
        lv_obj_t *slider = lv_slider_create(eq_area);
        arr_EQ_pSliders[idx_DSP][i] = slider;
        lv_group_add_obj(pgroup, slider);
        lv_obj_set_size(slider, 10, 120);
        lv_obj_set_pos(slider, 50 + i * 38, 40);
        // Set the initial position of the slider based on the value of eq_cfg->gains_db[i]
        // Set the range of the slider， unit: 0.1db
        // -12db ~ +12db
        lv_slider_set_range(slider, -120, 120);
        lv_slider_set_value(slider, (int) (peq_cfg->gains_db[i] * 10),
                LV_ANIM_OFF);
        // show center freq of each band
        lv_obj_t *label_freq = lv_label_create(eq_area);
        lv_obj_set_style_text_font(label_freq, &FONT_SMALL, 0);
        lv_obj_set_style_text_color(label_freq, COLOR_BUTTON, 0);
        lv_obj_set_style_text_align(label_freq, LV_TEXT_ALIGN_CENTER, 0);
        int freq = (int) (EQ_center_freqs[i]);
        if (freq < 1000)
            number2text(sValue, freq, 0, 0);
        else
            number2text(sValue, freq / 1000, 0, 'k');
        lv_label_set_text(label_freq, sValue);
        lv_obj_align_to(label_freq, slider, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);
        // show the current gain values in the label
        lv_obj_t *label_value = lv_label_create(eq_area);
        // user_data:　add label_value  as user data for slider, for the callback to update its display text
        lv_obj_set_user_data(slider, (void*) label_value);
        lv_obj_set_style_text_font(label_value, &FONT_SMALL, 0);
        lv_obj_set_style_text_color(label_value, COLOR_BUTTON, 0);
        lv_obj_set_style_text_align(label_value, LV_TEXT_ALIGN_CENTER, 0);
        number2text(sValue, (int) (peq_cfg->gains_db[i] * 10), 1, 0);
        lv_label_set_text(label_value, sValue);
        lv_obj_align_to(label_value, label_freq, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);
        // user_data： send the idx_DSP as a parameter to the event callback, so its value could be updated
        // use lower 8bits as idx_DSP, and higher 8bits as EQ segment ordering
        lv_obj_add_event_cb(slider, eq_slider_event_handler,
                LV_EVENT_VALUE_CHANGED, (void*) (idx_DSP | (i << 8)));
    }

    /* COMPRESS BEGIN */
#define COMPRESS_SLIDER_WIDTH 160
#define COMPRESS_SLIDER_HEIGHT 10
    // Create the compress effector area
    // grid layout: 3 columns, 3 rows
    lv_obj_t *compress_area = lv_obj_create(cont);
    //lv_obj_add_style(compress_area, &lvStyleInvisibleCont, 0);
    lv_obj_remove_flag(compress_area, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_align_to(compress_area, eq_area, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);
    lv_obj_set_layout(compress_area, LV_LAYOUT_GRID);
    lv_obj_set_size(compress_area, LV_PCT(90), LV_SIZE_CONTENT);
    /*2 columns */
    static const int32_t compress_column_dsc[] =
    { LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };
    /*1+3+1 rows*/
    static const int32_t compress_row_dsc[] =
    { LV_SIZE_CONTENT, LV_SIZE_CONTENT, LV_SIZE_CONTENT,LV_SIZE_CONTENT,LV_SIZE_CONTENT,
            LV_GRID_TEMPLATE_LAST };
    lv_obj_set_grid_dsc_array(compress_area, compress_column_dsc,
            compress_row_dsc);
    uint8_t row = 0, col = 0;
    /* row [0]: enabled/disabled LED + effector name */
    // Create the compress effector's LED enable control
    lv_obj_t *compress_led = lv_led_create(compress_area);
    lv_group_add_obj(pgroup, compress_led);
    // Set span to 1 to make the cell 1 column/row sized
    lv_obj_set_grid_cell(compress_led, LV_GRID_ALIGN_CENTER, 0, 1,
            LV_GRID_ALIGN_CENTER, 0, 1);
    lv_obj_set_size(compress_led, 20, 20);
    lv_obj_align(compress_led, LV_ALIGN_LEFT_MID, 0, 0);
    // user_data： send the idx_DSP as a parameter to the event callback, so its value could be updated
    lv_obj_add_event_cb(compress_led, compress_led_event_handler,
            LV_EVENT_CLICKED, (void*) idx_DSP);
#ifndef LVGL_SIM
    // sync LED state
    if(pDSP && ((pDSP->function_bits & DSP_FUNCTION_BIT_COMPRESS) != 0))
    {
        lv_led_set_brightness(compress_led, 255);
    }
    else
        lv_led_set_brightness(compress_led, 0);
#endif

    lv_obj_t *compress_led_label = lv_label_create(compress_area);
    lv_obj_set_grid_cell(compress_led_label, LV_GRID_ALIGN_CENTER, 1, 1,
            LV_GRID_ALIGN_CENTER, 0, 1);
    lv_label_set_text(compress_led_label, "压缩器");
    lv_obj_align(compress_led_label, LV_ALIGN_CENTER, 0, 0);

    // build for config params
#ifndef LVGL_SIM
    audio_compress_cfg_t *pcompress_cfg = &pDSP->compress.cfg;
#else
    audio_compress_cfg_t *pcompress_cfg = &cfgCompress_default;
#endif
    float *arr_compress_params_ptr[N_COMPRESS_PARAMS] =
    { &pcompress_cfg->attack_time_ms, &pcompress_cfg->release_time_ms,
            &pcompress_cfg->threshold_db, &pcompress_cfg->makeup_gain_db,
            &pcompress_cfg->ratio,&pcompress_cfg->knee_width_db };

    /* row [1,3]: slider + value display*/
    int idx_item;
    idx_item = 0;
    for (row = 1; (row < 4) && (idx_item < N_COMPRESS_PARAMS); ++row)
    {
        for (col = 0; (col < 2) && (idx_item < N_COMPRESS_PARAMS); ++col)
        {
            // cont: slider + value display
            lv_obj_t *cont = lv_obj_create(compress_area);
            lv_obj_set_size(cont, LV_PCT(50), LV_SIZE_CONTENT);
            lv_obj_remove_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_grid_cell(cont, LV_GRID_ALIGN_CENTER, col, 1,
                    LV_GRID_ALIGN_CENTER, row, 1);
            lv_obj_t *slider = lv_slider_create(cont);
            lv_group_add_obj(pgroup, slider);
            lv_obj_set_size(slider, COMPRESS_SLIDER_WIDTH,
                    COMPRESS_SLIDER_HEIGHT);
            lv_obj_align(slider, LV_ALIGN_TOP_LEFT, 15, 10);
            // Set the range of the slider, multiply if decimal precision is insufficient
            lv_slider_set_range(slider, arr_compress_range_min[idx_item],
                    arr_compress_range_max[idx_item]);
            // Set the initial position of the slider based on the value of compress_cfg->ratio_time_ms
            lv_slider_set_value(slider,
                    (int) ((*arr_compress_params_ptr[idx_item])
                            * arr_compress_multipliers[idx_item]), LV_ANIM_OFF);
            lv_obj_t *caption = lv_label_create(cont);
            lv_label_set_text(caption, arr_compress_varnames[idx_item]);
            lv_obj_align_to(caption, slider, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);
            lv_obj_t *label_value = lv_label_create(cont);
            // user_data:　add label_value  as user data for slider, for the callback to update its display text
            lv_obj_set_user_data(slider, (void*) label_value);
            number2text(sValue,
                    (int) ((*arr_compress_params_ptr[idx_item])
                            * arr_compress_multipliers[idx_item]),
                    ((arr_compress_multipliers[idx_item] == 1) ? (0) : (1)), // 1->0, 10->1, >10 not supported
                    0);
            lv_label_set_text(label_value, sValue);
            lv_obj_align_to(label_value, slider, LV_ALIGN_OUT_BOTTOM_RIGHT, 0,
                    5);
            // user_data: send the idx_DSP as a parameter to the event callback, so its value could be updated
            if (arr_compress_slider_event_cb[idx_item] != NULL)
                lv_obj_add_event_cb(slider,
                        arr_compress_slider_event_cb[idx_item],
                        LV_EVENT_VALUE_CHANGED, (void*) idx_DSP);
            ++idx_item;
        }

    }
    /* row [4]: compression monitor slider*/
    if(1)
    {
        col = 0;
        row = 4;
        // cont: slider + value display
        lv_obj_t *cont = lv_obj_create(compress_area);
        lv_obj_set_size(cont, LV_PCT(50), LV_SIZE_CONTENT);
        lv_obj_remove_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_grid_cell(cont, LV_GRID_ALIGN_CENTER, col, 1,
                LV_GRID_ALIGN_CENTER, row, 1);
        // progress bar
        lv_obj_t *bar = lv_bar_create(cont);
        arr_progbars_compress[idx_DSP] = bar;
        lv_obj_set_size(bar, COMPRESS_SLIDER_WIDTH,
                COMPRESS_SLIDER_HEIGHT);
        lv_bar_set_range(bar, 0, 120); //compression: 10x 0.0db~12.0db
        lv_bar_set_start_value(bar, 0, LV_ANIM_OFF);
#ifdef LVGL_SIM
        lv_bar_set_value(bar, 30, LV_ANIM_OFF);
#endif
        lv_obj_align(bar, LV_ALIGN_TOP_LEFT, 15, 10);
        // value label
        lv_obj_t *label_value = lv_label_create(cont);
        // user_data:　add label_value  as user data for slider, for the callback to update its display text
        lv_obj_set_user_data(bar, (void*) label_value);
        lv_label_set_text(label_value, "0dB");
        arr_compress_gain_lin_peak[idx_DSP] = 1.f;
        lv_obj_align_to(label_value, bar, LV_ALIGN_OUT_BOTTOM_RIGHT, 0,
                5);
    }
    /* COMPRESS END */
    /* Reverb BEGIN */
#define REVERB_SLIDER_WIDTH LV_PCT(90)
#define REVERB_SLIDER_HEIGHT 10
    // Create the reverb effector area
    lv_obj_t *reverb_area = lv_obj_create(cont);
    //lv_obj_add_style(reverb_area, &lvStyleInvisibleCont, 0);
    lv_obj_remove_flag(reverb_area, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_align_to(reverb_area, compress_area, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);
    lv_obj_set_layout(reverb_area, LV_LAYOUT_GRID);
    lv_obj_set_size(reverb_area, LV_PCT(90), LV_SIZE_CONTENT);

    /*2  cols*/
    static const int32_t reverb_column_dsc[] =
    { LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST }; /*2 columns */
    /*4  rows*/
    static const int32_t reverb_row_dsc[] =
    { LV_SIZE_CONTENT, LV_SIZE_CONTENT, LV_SIZE_CONTENT,LV_SIZE_CONTENT, LV_GRID_TEMPLATE_LAST };
    lv_obj_set_grid_dsc_array(reverb_area, reverb_column_dsc, reverb_row_dsc);

    // Create the reverb effector's LED enable control
    lv_obj_t *reverb_led = lv_led_create(reverb_area);
    lv_group_add_obj(pgroup, reverb_led);
    lv_obj_set_grid_cell(reverb_led, LV_GRID_ALIGN_CENTER, 0, 1,
            LV_GRID_ALIGN_CENTER, 0, 1);
    lv_obj_add_flag(reverb_led, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_size(reverb_led, 20, 20);
    lv_obj_set_pos(reverb_led, 10, 10);
    // user_data： send the idx_DSP as a parameter to the event callback, so its value could be updated
    lv_obj_add_event_cb(reverb_led, reverb_led_event_handler, LV_EVENT_CLICKED,
            (void*) idx_DSP);
#ifndef LVGL_SIM
    // sync LED state
    if(pDSP && ((pDSP->function_bits & DSP_FUNCTION_BIT_REVERB) != 0))
    {
        lv_led_set_brightness(reverb_led, 255);
    }
    else
        lv_led_set_brightness(reverb_led, 0);
#endif

    lv_obj_t *reverb_led_label = lv_label_create(reverb_area);
    lv_obj_set_grid_cell(reverb_led_label, LV_GRID_ALIGN_CENTER, 1, 1,
            LV_GRID_ALIGN_CENTER, 0, 1);
    lv_label_set_text(reverb_led_label, "混响器");
    // Create the reverb effector's parameter adjustment controls
#ifndef LVGL_SIM
    audio_reverb_cfg_t *preverb_cfg = &pDSP->reverb.cfg; // Allocate memory for the reverb configuration
#else
    audio_reverb_cfg_t *preverb_cfg = &cfgReverb_default;
#endif
    float *arr_reverb_params_ptr[N_REVERB_PARAMS] =
    { &preverb_cfg->predelay_ms, &preverb_cfg->decay_ms, &preverb_cfg->feedback};
    idx_item = 0;

    for (row = 1; ((row < N_REVERB_PARAMS+1) && (idx_item < N_REVERB_PARAMS)); ++row)
    {
        lv_obj_t *cont = lv_obj_create(reverb_area);
        lv_obj_remove_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_size(cont, LV_PCT(90), LV_SIZE_CONTENT);
        lv_obj_set_grid_cell(cont, LV_GRID_ALIGN_CENTER, 0, 2,
                LV_GRID_ALIGN_CENTER, row, 1);
        lv_obj_t *slider = lv_slider_create(cont);
        lv_group_add_obj(pgroup, slider);
        lv_obj_set_size(slider, REVERB_SLIDER_WIDTH, REVERB_SLIDER_HEIGHT);
        lv_obj_align(slider, LV_ALIGN_TOP_LEFT, 15, 10);
        // Set the range of the slider, multiply if decimal precision is insufficient
        lv_slider_set_range(slider, arr_reverb_range_min[idx_item],
                arr_reverb_range_max[idx_item]);
        lv_slider_set_value(slider,
                (int) ((*arr_reverb_params_ptr[idx_item])
                        * arr_reverb_multipliers[idx_item]), LV_ANIM_OFF);
        // Set the initial position of the slider based on the value of compress_cfg->ratio_time_ms
        lv_obj_t *caption = lv_label_create(cont);
        lv_label_set_text(caption, arr_reverb_varnames[idx_item]);
        lv_obj_align_to(caption, slider, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);
        lv_obj_t *label_value = lv_label_create(cont);
        // user_data:　add label_value  as user data for slider, for the callback to update its display text
        lv_obj_set_user_data(slider, (void*) label_value);
        number2text(sValue,
                (int) ((*arr_reverb_params_ptr[idx_item])
                        * arr_reverb_multipliers[idx_item]),
                ((arr_reverb_multipliers[idx_item] == 1) ? (0) : (1)), // 1->0, 10->1, >10 not supported
                0);
        lv_label_set_text(label_value, sValue);
        lv_obj_align_to(label_value, slider, LV_ALIGN_OUT_BOTTOM_RIGHT, 0, 5);
        //　user_data： send the idx_DSP as a parameter to the event callback, so its value could be updated
        if (arr_reverb_slider_event_cb[idx_item] != NULL)
            lv_obj_add_event_cb(slider, arr_reverb_slider_event_cb[idx_item],
                    LV_EVENT_VALUE_CHANGED, (void*) idx_DSP);
        ++idx_item;
    }

    /* Reverb END */

    // create a update timer
    lv_timer_t* timer = lv_timer_create(value_update_timer_cb, 100, (void*)idx_DSP);
    lv_timer_set_repeat_count(timer, -1);
    arr_timers[idx_DSP] = timer;

    arr_is_created[idx_DSP] = true;
    return cont;
}

