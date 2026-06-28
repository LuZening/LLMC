/*
 * LVGL_GUI_signal_path.c
 *
 *  Created on: May 23, 2024
 *      Author: cpholzn
 */

#ifdef __cplusplus
extern "C" {
#endif


#include "LVGL_GUI.h"

#ifndef LVGL_SIM
#include "cmsis_os2.h"
#include "audio_buffers.h"
#include "audio_DSP.h"
#include "audio_player.h"
#include "audio_settings.h"
#include "Config.h"
#else
    #define N_INPUTS 4
    #define N_OUTPUTS 2
#endif


/************************ Audio Signal Path  screen BEGIN ************************/
bool isAudioSignalPathScreenCreated = false;
lv_group_t * lvGroupAudioPathScreen = NULL;
lv_obj_t * lvMainContAudioPathScreen = NULL;
static void switch_audio_connections_cb(lv_event_t* e)
{

    // MSB --Output idx(>=1) -- Input Mask(16bits) -- LSB
    uint32_t idxOutput_maskInput = *((uint32_t*)lv_event_get_user_data(e));
    uint16_t idxOutput  = idxOutput_maskInput >> 16U;
    uint16_t maskInput = idxOutput_maskInput & 0xffffU;
    lv_obj_t* obj = lv_event_get_target_obj(e);
    lv_state_t lvstate = lv_obj_get_state(obj);
#ifndef LVGL_SIM
    if(maskInput <= (1U << (N_INPUTS - 1)) && (idxOutput > 0) && (idxOutput <= N_OUTPUTS))
    {
        osMutexAcquire(mtxConfig, osWaitForever);
        // clear bit
        cfg.audio_connections[idxOutput - 1]  &= (~maskInput);
        // now checked, can enable the function bit
        if((lvstate & LV_STATE_CHECKED) > 0)
        {
            // set bit
            cfg.audio_connections[idxOutput - 1] |= (maskInput );
        }
        // prepare devices and etc
        audio_connections_apply(cfg.audio_connections);
        osMutexRelease(mtxConfig);
    }
#endif
}

// when clicked the column header buttons
static void input_btns_click_cb(lv_event_t* e)
{

    lv_obj_t* obj = lv_event_get_target_obj(e);
    //get idx
    int idx = (int)lv_obj_get_user_data(obj);
    lv_obj_t* contDSP = create_dsp_ui(idx);
    
}

void dismiss_audio_signal_path_screen()
{

    if(lvMainContAudioPathScreen)
    {
        deregister_group(lvGroupAudioPathScreen);
        lvGroupAudioPathScreen = NULL;
        lv_obj_del_async(lvMainContAudioPathScreen);
        lvMainContAudioPathScreen = NULL;
        isAudioSignalPathScreenCreated = false;
    }

}


void display_audio_signal_path_screen()
{
    if(isAudioSignalPathScreenCreated)
        goto EXIT_AUDIO_PATH_SCREEN;

    isAudioSignalPathScreenCreated = true;

    lv_obj_t* scr = lv_scr_act();

    //// 创建外部输入group，并聚焦
    lvGroupAudioPathScreen = lv_group_create();
    register_group(lvGroupAudioPathScreen);

    // grid layout背景
    lvMainContAudioPathScreen = lv_obj_create(scr);
    lv_obj_remove_flag(lvMainContAudioPathScreen, LV_OBJ_FLAG_SCROLLABLE);
//	lv_obj_remove_flag(lvMainContAudioPathScreen, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_size(lvMainContAudioPathScreen, LV_PCT(90), LV_PCT(90));
    lv_obj_add_style(lvMainContAudioPathScreen, &lvStyleSemiTranspFrame, 0);
    lv_obj_set_style_bg_opa(lvMainContAudioPathScreen, LV_OPA_COVER, 0);
    lv_obj_align(lvMainContAudioPathScreen, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_layout(lvMainContAudioPathScreen, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(lvMainContAudioPathScreen, LV_FLEX_FLOW_COLUMN);
    // back button
    lv_obj_t* btnBack = create_back_button(lvMainContAudioPathScreen, lvGroupAudioPathScreen, dismiss_audio_signal_path_screen);


    /* 主界面图片按钮组 BEGIN */
    lv_obj_t* lvContMatrix = lv_obj_create(lvMainContAudioPathScreen);
    lv_obj_remove_flag(lvContMatrix, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(lvContMatrix, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_style(lvContMatrix, &lvStyleInvisibleCont, 0);
    lv_obj_set_layout(lvContMatrix, LV_LAYOUT_GRID);
    lv_obj_set_style_margin_ver(lvContMatrix, 10, 0);
    lv_obj_set_style_margin_hor(lvContMatrix, 10, 0);
    lv_obj_set_size(lvContMatrix, LV_PCT(100), LV_PCT(100));
    // grid layout, rows: outputs, index: inputs
    int i;
    uint16_t x_begin, y_begin;
    // static const char* arrPathBtnImgs[] = {FS_FOLDER_PICTURES "icon_pc.bin", FS_FOLDER_PICTURES "icon_headphone.bin"};
    static const char* arrOutputLabels[] = { "输出至耳机", "输出至USB录音" };
    static const char* arrInputLabels[] = {"麦克风1","麦克风2", "USB播放", "文件播放"};

    // note: column_dsc and row_dsc must be static!!!!!
//	static int32_t column_dsc[] = { LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST};   /*4 columns with 100 and 400 ps width*/
    static int32_t column_dsc[] = { LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};   /*4 columns with 100 and 400 ps width*/
    static int32_t row_dsc[] = { LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST}; /*3 100 px tall rows*/
    lv_obj_set_grid_dsc_array(lvContMatrix, column_dsc, row_dsc);

    static uint32_t bufUserData[N_INPUTS * N_OUTPUTS];
    // TODO: put items in grid
    for(i = 0; i < (1 + N_INPUTS) * (1 + N_OUTPUTS); ++i)
    {
    	// columns are inputs, indices are outputs
        uint8_t col= i % (N_INPUTS+1);
        uint8_t row = i / (N_INPUTS+1);
        int iInput = col - 1;
        int iOutput = row - 1;
        // skip upper-left corner, leave it to be empty
        if(i == 0)
        	continue;
        // making column header for inputs, buttons that trigger settings menu
        if(row == 0)
        {
        	lv_obj_t *btnInput = lv_button_create(lvContMatrix);
        	lv_group_add_obj(lvGroupAudioPathScreen, btnInput);
        	/*Stretch the cell horizontally and vertically too
             *Set span to 1 to make the cell 1 column/row sized*/
            lv_obj_set_grid_cell(btnInput, LV_GRID_ALIGN_CENTER, col, 1,
                    LV_GRID_ALIGN_CENTER, row, 1);
        	lv_obj_t *lblCaption = lv_label_create(btnInput);
        	lv_label_set_text_static(lblCaption, arrInputLabels[iInput]);
        	lv_label_set_long_mode(lblCaption, LV_LABEL_LONG_WRAP);
        	lv_obj_set_style_text_align(lblCaption, LV_TEXT_ALIGN_CENTER, 0);
        	lv_obj_center(lblCaption);
            lv_obj_set_user_data(btnInput, (void*)iInput);
            lv_obj_add_event_cb(btnInput, input_btns_click_cb, LV_EVENT_CLICKED, NULL);

        }
        // making indices for outputs, buttons that trigger settings menu
        else if(col == 0)
        {
        	lv_obj_t *btnOutput = lv_button_create(lvContMatrix);
        	/*Stretch the cell horizontally and vertically too
             *Set span to 1 to make the cell 1 column/row sized*/
            lv_obj_set_grid_cell(btnOutput, LV_GRID_ALIGN_CENTER, col, 1,
                    LV_GRID_ALIGN_CENTER, row, 1);
        	lv_obj_t *lblCaption = lv_label_create(btnOutput);
        	lv_label_set_text_static(lblCaption, arrOutputLabels[iOutput]);
        	lv_label_set_long_mode(lblCaption, LV_LABEL_LONG_WRAP);
        	lv_obj_set_style_text_align(lblCaption, LV_TEXT_ALIGN_CENTER, 0);
        	lv_obj_center(lblCaption);
        	lv_group_add_obj(lvGroupAudioPathScreen, btnOutput);
        }
        // col > 0 and row > 0, making switches
        else
        {

        	lv_obj_t *sw = lv_switch_create(lvContMatrix);
        	/*Stretch the cell horizontally and vertically too
             *Set span to 1 to make the cell 1 column/row sized*/
            lv_obj_set_grid_cell(sw, LV_GRID_ALIGN_CENTER, col, 1,
                    LV_GRID_ALIGN_CENTER, row, 1);
#ifndef LVGL_SIM
            if((cfg.audio_connections[iOutput] & (1U << iInput)) != 0)
            {
                lv_obj_add_state(sw, LV_STATE_CHECKED);
            }
#endif
            uint16_t maskInput = 1U << iInput;
            uint16_t maskOutput = 1U << iOutput;
            uint32_t *userdata = &(bufUserData[iOutput * N_INPUTS + iInput]);

            *userdata = (maskOutput << 16U) | maskInput;
            lv_obj_add_event_cb(sw, switch_audio_connections_cb, LV_EVENT_VALUE_CHANGED, (void*)userdata);

            lv_group_add_obj(lvGroupAudioPathScreen, sw);

        }

    }
EXIT_AUDIO_PATH_SCREEN:
    return;
}






/************************ Audio Signal Path screen END ************************/

#ifdef __cplusplus
}
#endif
