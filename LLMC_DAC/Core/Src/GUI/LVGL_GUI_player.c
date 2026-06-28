/*
 * LVGL_GUI_player.c
 *
 *  Created on: 2024年5月19日
 *      Author: cpholzn
 */

#ifdef __cplusplus
extern "C" {
#endif


#include "LVGL_GUI.h"
#include "utf_convert.h"
#include "lv_simple_file_explorer_unicode.h"
#include "LLMC_LVGL_assets.h"
#include "playlist.h"

#ifndef LVGL_SIM
#include "audio_player.h"
#include "cmsis_os2.h"
#include "ff.h"
#include "Config.h"
#else
#include "sim.h"
playlist_t pl = { 0 };
FIL filePL;
FIL filePL_new; // file for playlist
bool isOpen_PL = false; // indicate if playlist file is opened
FS_play_mode_t modePlay = 0;
static bool is_valid_wav_file(const TCHAR* fn, size_t len)
{
    return true;
}
#endif




/************************ FS Player BEGIN ************************/
bool isFSPlayerScreenCreated = false;
bool isFSPlayerScreenHidden = false;
lv_group_t * lvGroupFSPlayerScreen = NULL;
lv_obj_t* lvMainContFS = NULL;
static lv_timer_t* duration_update_timer = NULL; // count playing time

static uint32_t elapsed_time_ms = 0; // elapsed time(millisec) since playing
static lv_obj_t* duration_time_label = NULL; // the object for displaying total duration
static lv_obj_t* elapsed_time_label = NULL; // the object for displaying elapsed time
static lv_obj_t* sample_rate_label = NULL;
static lv_obj_t* slider_obj = NULL; // the object for showing progress played
static lv_obj_t* play_obj = NULL; // the object of play button

track_info_t track_info;
lv_obj_t *title_label = NULL, *artist_label = NULL, *genre_label = NULL;


static TCHAR wcsFullPath[_MAX_LFN * 2 + 1] = { 0 };
static TCHAR wcsFilename[_MAX_LFN + 1] = { 0 };

static void dismiss_FS_player_screen(void)
{
    /* save playlist if modified */
    if (isOpen_PL)
    {
        if (pl.is_modified)
        {
            int fr = f_open(&filePL_new, FS_FOLDER_PLAYLIST _T("/") FS_FNAME_PLAYLSIT_TEMP, FA_WRITE | FA_CREATE_ALWAYS);
            if (fr == 0)
            {
                playlist_saveas(&pl, &filePL_new, 1024U * 1024U); // at most 1MB
                f_close(&filePL_new);
                f_close(&filePL);
                f_unlink(pl.filepath); // delete the old playlist
                f_rename(FS_FOLDER_PLAYLIST _T("/") FS_FNAME_PLAYLSIT_TEMP, pl.filepath);
                // reopen the old fild
                fr = f_open(&filePL, pl.filepath, FA_OPEN_EXISTING | FA_READ | FA_WRITE);
                if (fr != 0)
                    isOpen_PL = false;
            }
        }
    }

    /* hide GUI widgets, instead of close the window */
    if(isFSPlayerScreenCreated)
    {
    	deregister_group_without_del(lvGroupFSPlayerScreen);
        lv_obj_add_flag(lvMainContFS, LV_OBJ_FLAG_HIDDEN);
        isFSPlayerScreenHidden = true;
    }

    /* delete GUI widgets */
//    if (lvGroupFSPlayerScreen)
//    {
//        deregister_group(lvGroupFSPlayerScreen);
//        lvGroupFSPlayerScreen = NULL;
//
//        lv_obj_del_async(lvMainContFS);
//        lvMainContFS = NULL;
//        isFSPlayerScreenCreated = false;
//        lv_timer_pause(duration_update_timer);
//        lv_timer_delete(duration_update_timer);
//    }

}

static void file_explorer_event_handler_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);
    const char* sFileType = lv_event_get_param(e);
    // TODO: check


    if(code == LV_EVENT_VALUE_CHANGED) {
        const TCHAR * cur_path =  lv_simple_file_explorer_unicode_get_current_path(obj);
        const TCHAR * sel_fn = lv_simple_file_explorer_unicode_get_selected_file_name(obj);
        size_t cntFolderPath = strnlen_TCHAR(cur_path, _MAX_LFN);
        size_t cntFilename = strnlen_TCHAR(sel_fn, _MAX_LFN);
        if (strcmp(sFileType, "file") == 0)
        {
            // concat folder and filename to full_path
            if((cntFolderPath > 0) && (cntFilename > 3))
            {
                #ifndef LVGL_SIM
                // change song
                FS_player_change_track(cur_path, sel_fn);
                #else
                // concat full path to the FS player
                memcpy(wcsFullPath, cur_path, cntFolderPath * sizeof(TCHAR));
                memcpy(wcsFullPath + cntFolderPath, sel_fn, cntFilename * sizeof(TCHAR));
                wcsFullPath[cntFolderPath + cntFilename] = 0U; // terminate with \0\0
                // copy file name to an independant memory
                memcpy(wcsFilename, sel_fn, cntFilename * sizeof(TCHAR));
                wcsFilename[cntFilename] = 0U;
                lv_lock();
                GUI_notify_change_track(&track_info, 60000, wcsFilename, cntFilename, wcsFullPath, cntFolderPath + cntFilename);
                lv_unlock();
                #endif

            }
        }
    }
}

static void loop_icon_change(lv_obj_t* icon, FS_play_mode_t mode)
{
    switch(mode)
    {
    case FS_PLAY_MODE_ONE_SHOT:
        lv_image_set_src(icon, LV_SYMBOL_MINUS);
        break;
    case FS_PLAY_MODE_ONE_LOOP:
    	lv_image_set_src(icon, LV_SYMBOL_LOOP);
    	break;
    case FS_PLAY_MODE_SEQUENTIAL:
    	lv_image_set_src(icon, LV_SYMBOL_LIST);
    	break;
    case FS_PLAY_MODE_RANDOM:
    	lv_image_set_src(icon, LV_SYMBOL_SHUFFLE);
    	break;
    }
}
static void loop_event_handler_cb(lv_event_t* e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t* obj = lv_event_get_target(e);
    if(code == LV_EVENT_CLICKED)
    {
    	FS_play_mode_t modeNew;
    	switch(modePlay)
    	{
        case FS_PLAY_MODE_ONE_SHOT:
    		modeNew = FS_PLAY_MODE_ONE_LOOP;
    		break;
        case FS_PLAY_MODE_ONE_LOOP:
        	modeNew = FS_PLAY_MODE_SEQUENTIAL;
        	break;
        case FS_PLAY_MODE_SEQUENTIAL:
        	modeNew = FS_PLAY_MODE_RANDOM;
        	break;
        case FS_PLAY_MODE_RANDOM:
        	modeNew = FS_PLAY_MODE_ONE_SHOT;
        	break;
        default:
        	modeNew = FS_PLAY_MODE_ONE_LOOP;
    	}
    	loop_icon_change(obj, modeNew);
    	modePlay = modeNew;
    }
}

static void prev_event_handler_cb(lv_event_t* e)
{
    if(lv_event_get_code(e) == LV_EVENT_CLICKED) {
#ifndef LVGL_SIM
        osMutexAcquire(mtxFSPlayer, osWaitForever);
        int32_t current_ms = (samplesPerMs > 0) ? (int32_t)(samplesPlayed / samplesPerMs) : 0;
        osMutexRelease(mtxFSPlayer);
        uint32_t new_ms = (current_ms > 10000) ? (uint32_t)(current_ms - 10000) : 0;
        FS_player_signal_seek(new_ms);
#endif
    }
}

bool isPlay_lvgl = false;
static void play_pause_event_handler_cb(lv_event_t* e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t* obj = lv_event_get_target(e);

    // TODO: play_pause_event_handler_cb
    // CASE: to pause
    if (isPlay_lvgl)
    {
#ifndef LVGL_SIM
        FS_player_signal_pause();
#endif
    }
    // CASE: to play
    else
    {
        // send resume signal
#ifndef LVGL_SIM
        FS_player_signal_start();
#endif
    }
}

static void next_event_handler_cb(lv_event_t* e)
{
    if(lv_event_get_code(e) == LV_EVENT_CLICKED) {
#ifndef LVGL_SIM
        osMutexAcquire(mtxFSPlayer, osWaitForever);
        int32_t current_ms = (samplesPerMs > 0) ? (int32_t)(samplesPlayed / samplesPerMs) : 0;
        osMutexRelease(mtxFSPlayer);
        FS_player_signal_seek((uint32_t)(current_ms + 10000));
#endif
    }
}


static void slider_seek_event_cb(lv_event_t *e)
{
    if(lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED) {
        int32_t ms = lv_slider_get_value(lv_event_get_target(e));
#ifndef LVGL_SIM
        FS_player_signal_seek((uint32_t)ms);
#endif
    }
}
static void del_counter_timer_event_cb(lv_event_t* e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t* obj = lv_event_get_target(e);
    // TODO: del_counter_timer_event_cb
}


static lv_obj_t * create_player_cont(lv_obj_t * parent)
{
    /*A transparent container in which the player section will be scrolled */
#if 0
    lv_obj_t* main_cont = lv_obj_create(parent);
    lv_obj_remove_flag(main_cont, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(main_cont, LV_OBJ_FLAG_SCROLL_ELASTIC);
    lv_obj_remove_style_all(main_cont);                            /*Make it transparent*/
    lv_obj_set_size(main_cont, lv_pct(100), lv_pct(100));
    lv_obj_set_scroll_snap_y(main_cont, LV_SCROLL_SNAP_CENTER);    /*Snap the children to the center*/
#endif

    // back image
    lv_obj_t* back = lv_obj_create(parent);
    lv_obj_remove_flag(back, LV_OBJ_FLAG_SNAPPABLE);
    lv_obj_set_size(back, LV_PCT(50), LV_PCT(100));
    lv_obj_align(back, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_add_style(back, &lvStyleInvisibleCont, 0);
    lv_obj_set_style_bg_opa(back, LV_OPA_COVER, 0);
    lv_obj_t* img = lv_image_create(back);
    lv_image_set_src(img, &back_img_player_1);
    //lv_image_set_align(img, LV_IMAGE_ALIGN_CENTER);
    lv_obj_align(img, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
    /*Create a container for the player*/
    lv_obj_t * player = lv_obj_create(back);
    lv_obj_set_y(player, -20);
    #define LV_DEMO_MUSIC_HANDLE_SIZE 20
    
    lv_obj_add_style(player, &lvStyleInvisibleCont, 0);

    //lv_obj_set_style_bg_color(player, lv_color_hex(0xffffff), 0);
    //lv_obj_set_style_bg_opa(player, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(player, 0, 0);
    lv_obj_set_style_pad_all(player, 0, 0);
    //lv_obj_set_scroll_dir(player, LV_DIR_VER);



#if 0
    /* Transparent placeholders below the player container
     * It is used only to snap it to center.*/
    lv_obj_t * placeholder1 = lv_obj_create(main_cont);
    lv_obj_remove_style_all(placeholder1);
    lv_obj_remove_flag(placeholder1, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t * placeholder2 = lv_obj_create(main_cont);
    lv_obj_remove_style_all(placeholder2);
    lv_obj_remove_flag(placeholder2, LV_OBJ_FLAG_CLICKABLE);


    lv_obj_set_size(placeholder1, lv_pct(100), LV_VER_RES);
    lv_obj_set_y(placeholder1, 0);

    lv_obj_set_size(placeholder2, lv_pct(100),  LV_VER_RES - 2 * LV_DEMO_MUSIC_HANDLE_SIZE);
    lv_obj_set_y(placeholder2, LV_VER_RES + LV_DEMO_MUSIC_HANDLE_SIZE);


    lv_obj_update_layout(main_cont);
#endif
    return player;
}


static lv_obj_t * create_player_ctrl_box(lv_obj_t * parent)
{
    /*Create the control box*/
    lv_obj_t * cont = lv_obj_create(parent);
    lv_obj_remove_style_all(cont);
    lv_obj_set_size(cont, LV_PCT(90), LV_PCT(100));
    
    lv_obj_set_height(cont, LV_SIZE_CONTENT);

    lv_obj_set_style_pad_bottom(cont, 8, 0);
    lv_obj_set_style_pad_column(cont, 5, 0);
    lv_obj_set_style_pad_row(cont, 10, 0);
    static const int32_t grid_col[] = {LV_GRID_FR(2), LV_GRID_FR(3), LV_GRID_FR(5), LV_GRID_FR(3), LV_GRID_FR(2), LV_GRID_TEMPLATE_LAST};
    static const int32_t grid_row[] = {LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST};
    lv_obj_set_grid_dsc_array(cont, grid_col, grid_row);

    static lv_style_t style;
    lv_style_init(&style);
    lv_style_set_text_font(&style, &LV_FONT_AWESOME_LARGE);
    lv_style_set_text_color(&style, COLOR_BUTTON);

    lv_obj_t * icon;
    /* btn: nouse */
    icon = lv_image_create(cont);
    lv_obj_remove_flag(icon, LV_OBJ_FLAG_CLICK_FOCUSABLE);
    lv_group_add_obj(lvGroupFSPlayerScreen, icon);
    lv_obj_add_style(icon, &style, 0);
    lv_image_set_src(icon, LV_SYMBOL_LIST);
    lv_obj_set_grid_cell(icon, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_CENTER, 0, 1);
    lv_obj_add_flag(icon, LV_OBJ_FLAG_CLICKABLE);
    /* btn: prev */
    icon = lv_image_create(cont);
    lv_group_add_obj(lvGroupFSPlayerScreen, icon);
    lv_obj_add_style(icon, &style, 0);
    lv_image_set_src(icon, LV_SYMBOL_PREV);
    lv_obj_set_grid_cell(icon, LV_GRID_ALIGN_CENTER, 1, 1, LV_GRID_ALIGN_CENTER, 0, 1);
    lv_obj_add_event_cb(icon, prev_event_handler_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_flag(icon, LV_OBJ_FLAG_CLICKABLE);
    /* btn: play/pause bi-state checkable */
    play_obj = lv_image_create(cont);
    lv_group_add_obj(lvGroupFSPlayerScreen, play_obj);
    lv_obj_add_style(play_obj, &style, 0);
    if(isPlay_lvgl) // inversed display
        lv_image_set_src(play_obj, LV_SYMBOL_PAUSE); // when playing ,display pause symbol
    else
        lv_image_set_src(play_obj, LV_SYMBOL_PLAY); // when paused, display play symbol
    lv_obj_add_flag(play_obj, LV_OBJ_FLAG_CHECKABLE);
    lv_obj_add_flag(play_obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_grid_cell(play_obj, LV_GRID_ALIGN_CENTER, 2, 1, LV_GRID_ALIGN_CENTER, 0, 1);
    lv_obj_add_event_cb(play_obj, play_pause_event_handler_cb, LV_EVENT_CLICKED, NULL);


    /* btn: next */
    icon = lv_image_create(cont);
    lv_group_add_obj(lvGroupFSPlayerScreen, icon);
    lv_obj_add_style(icon, &style, 0);
    lv_image_set_src(icon, LV_SYMBOL_NEXT);
    lv_obj_set_grid_cell(icon, LV_GRID_ALIGN_CENTER, 3, 1, LV_GRID_ALIGN_CENTER, 0, 1);
    lv_obj_add_event_cb(icon, next_event_handler_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_flag(icon, LV_OBJ_FLAG_CLICKABLE);

    /* btn: loop */
    icon = lv_image_create(cont);
    lv_group_add_obj(lvGroupFSPlayerScreen, icon);
    lv_obj_add_style(icon, &style, 0);
    lv_obj_set_grid_cell(icon, LV_GRID_ALIGN_END, 4, 1, LV_GRID_ALIGN_CENTER, 0, 1);
    loop_icon_change(icon, modePlay); // set icon according to the current play mode
    lv_obj_add_event_cb(icon, loop_event_handler_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_flag(icon, LV_OBJ_FLAG_CLICKABLE);
    /* time slider */
    LV_IMAGE_DECLARE(img_slider_knob);
    slider_obj = lv_slider_create(cont);
//    lv_obj_set_style_anim_duration(slider_obj, 100, 0);
    lv_obj_set_height(slider_obj, 3);
    lv_obj_set_grid_cell(slider_obj, LV_GRID_ALIGN_STRETCH, 0, 4, LV_GRID_ALIGN_CENTER, 1, 1);
    lv_obj_set_style_bg_image_src(slider_obj, LV_SYMBOL_BELL, LV_PART_KNOB);
    //lv_obj_set_style_text_font(slider_obj, &LV_FONT_AWESOME, LV_PART_KNOB);
    lv_obj_set_style_bg_opa(slider_obj, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_set_style_pad_all(slider_obj, 20, LV_PART_KNOB);
    lv_obj_set_style_bg_grad_dir(slider_obj, LV_GRAD_DIR_HOR, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(slider_obj, lv_color_hex(0x569af8), LV_PART_INDICATOR);
    lv_obj_set_style_bg_grad_color(slider_obj, lv_color_hex(0xa666f1), LV_PART_INDICATOR);
    lv_obj_set_style_outline_width(slider_obj, 0, 0);
    lv_obj_add_event_cb(slider_obj, del_counter_timer_event_cb, LV_EVENT_DELETE, NULL);
    lv_obj_add_event_cb(slider_obj, slider_seek_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    /* time indicator */
    elapsed_time_label = lv_label_create(cont);
    lv_obj_set_style_text_font(elapsed_time_label, &FONT_SMALL, 0);
    lv_obj_set_style_text_color(elapsed_time_label, lv_color_hex(0x8a86b8), 0);
    lv_label_set_text(elapsed_time_label, "0:00");
    lv_obj_set_grid_cell(elapsed_time_label, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_START, 2, 1);  // below the slider, leftmost
//    lv_obj_add_event_cb(elapsed_time_label, del_counter_timer_event_cb, LV_EVENT_DELETE, NULL);

    duration_time_label = lv_label_create(cont);
    lv_obj_set_style_text_font(duration_time_label, &FONT_SMALL, 0);
    lv_obj_set_style_text_color(duration_time_label, lv_color_hex(0x8a86b8), 0);
    lv_label_set_text(duration_time_label, "0:00");
    lv_obj_set_grid_cell(duration_time_label, LV_GRID_ALIGN_START, 1, 1, LV_GRID_ALIGN_START, 2, 1); // alongside the elapsed
//    lv_obj_add_event_cb(duration_time_label, del_counter_timer_event_cb, LV_EVENT_DELETE, NULL);


    /* sample rate and bitdepth indicator */
    sample_rate_label = lv_label_create(cont);
    lv_obj_set_style_text_font(sample_rate_label, &FONT_SMALL, 0);
    lv_obj_set_style_text_color(sample_rate_label, lv_color_hex(0x8a86b8), 0);
    lv_label_set_text(sample_rate_label, "");
    lv_obj_set_grid_cell(sample_rate_label, LV_GRID_ALIGN_START, 0, 3, LV_GRID_ALIGN_START, 3, 1);  // below the slider, leftmost
//    lv_obj_add_event_cb(elapsed_time_label, del_counter_timer_event_cb, LV_EVENT_DELETE, NULL);

//    lv_obj_add_event_cb(duration_time_label, del_counter_timer_event_cb, LV_EVENT_DELETE, NULL);
    return cont;
}





static lv_obj_t * create_player_title_box(lv_obj_t * parent)
{

    /*Create the titles*/
    lv_obj_t * cont = lv_obj_create(parent);
    lv_obj_remove_style_all(cont);
    lv_obj_remove_flag(cont, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_pad_top(cont, 16, 0);
    lv_obj_set_height(cont, LV_SIZE_CONTENT);
    lv_obj_set_width(cont, LV_PCT(90));
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    title_label = lv_label_create(cont);
    lv_obj_set_style_text_font(title_label, &FONT_MED, 0);
    lv_obj_set_style_text_color(title_label, lv_color_hex(0x504d6d), 0);
    lv_obj_set_height(title_label, lv_font_get_line_height(&FONT_MED) * 3 / 2);
    lv_obj_set_width(title_label, LV_PCT(90));
    lv_obj_set_style_text_align(title_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_long_mode(title_label, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_label_set_text(title_label, "");

//    artist_label = lv_label_create(cont);
//    lv_obj_set_style_text_font(artist_label, &FONT_SMALL, 0);
//    lv_obj_set_style_text_color(artist_label, lv_color_hex(0x504d6d), 0);
//    lv_obj_set_width(artist_label, LV_PCT(90));
//    lv_obj_set_style_text_align(artist_label, LV_TEXT_ALIGN_CENTER, 0);
//    lv_label_set_long_mode(artist_label, LV_LABEL_LONG_SCROLL_CIRCULAR);


//    genre_label = lv_label_create(cont);
//    lv_obj_set_style_text_font(genre_label, &FONT_SMALL, 0);
//    lv_obj_set_style_text_color(genre_label, lv_color_hex(0x8a86b8), 0);
//    lv_obj_set_width(genre_label, LV_PCT(90));
//    lv_obj_set_style_text_align(genre_label, LV_TEXT_ALIGN_CENTER, 0);
//    lv_label_set_long_mode(genre_label, LV_LABEL_LONG_SCROLL_CIRCULAR);

    return cont;
}


lv_obj_t* playlist_indicator = NULL;
static void light_on_playlist_logo(uint8_t state)
{
    if(state == 0)
        lv_img_set_src(playlist_indicator, &school_logo_inactive);
    else
        lv_img_set_src(playlist_indicator, &school_logo);
}

/* responds to clicks on the playlist logo */
static void  add_playlist_event_handler_cb(lv_event_t* e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t* obj = lv_event_get_target(e);
    // TODO: implement the add playlist event handler
    if (code == LV_EVENT_CLICKED)
    {
        if (isOpen_PL)
        {
            if (track_info.is_in_playlist) // logo is on 
            {
                // remove from playlist
                int posRowBeginFound;
                int r = playlist_remove(&pl, wcsFilename, &posRowBeginFound);
                if (r == 0) // remove succeeded
                {
                    light_on_playlist_logo(0); // deactivate the logo
                }
            }
            else // logo is off
            {
                // add to the playlist
                int posRowBeginAdded;
                size_t bytesWritten;
                int r = playlist_add(&pl, wcsFilename, wcsFullPath, &posRowBeginAdded, &bytesWritten);
                if (r == 0) // remove succeeded
                {
                    light_on_playlist_logo(1); // activeate the logo
                }
            }
        }
    }
}

/* display a symbol, to indicate whether this music is in the playlist */

static lv_obj_t* create_player_playlist_indicator(lv_obj_t* parent)
{
    lv_obj_t* cont = lv_obj_create(parent);
    lv_obj_remove_style_all(cont);
    lv_obj_remove_flag(cont, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_height(cont, LV_SIZE_CONTENT);
    lv_obj_set_width(cont, LV_PCT(90));
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);


    playlist_indicator = lv_image_create(cont);
    lv_img_set_src(playlist_indicator, &school_logo_inactive);
    lv_obj_add_flag(playlist_indicator, LV_OBJ_FLAG_CHECKABLE);
    lv_obj_add_flag(playlist_indicator, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(playlist_indicator, add_playlist_event_handler_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_border_width(playlist_indicator, 2, LV_STATE_HOVERED);
    //lv_obj_t* caption = lv_label_create(cont);
    //lv_obj_align_to(caption, playlist_indicator, LV_ALIGN_OUT_RIGHT_MID, 100, 0);
    //lv_obj_set_style_text_font(caption, &FONT_SMALL, 0);
    //lv_obj_set_style_text_color(caption, lv_color_hex(0x8a86b8), 0);
    //lv_label_set_text(caption, "add to playlist");
    return cont;
}


static void duration_timer_cb(lv_timer_t* t)
{
    // update progress bar
    // estimate progress by calculating the portion of data decoded by the FS player
    // variables are from FS player
#ifndef LVGL_SIM
    osMutexAcquire(mtxFSPlayer, 100);
    if(samplesPerMs > 0)
    	elapsed_time_ms = samplesPlayed / samplesPerMs;
    else
    	elapsed_time_ms = 0;
    osMutexRelease(mtxFSPlayer);
    lv_label_set_text_fmt(elapsed_time_label, "%"LV_PRIu32":%02"LV_PRIu32, elapsed_time_ms / 60000, elapsed_time_ms % 60000 / 1000);
    lv_slider_set_value(slider_obj, elapsed_time_ms, LV_ANIM_OFF);
#endif
}


/* MAIN　CONTENT BEGIN */
static TCHAR wcsFolder[_MAX_LFN + 1];
void display_FS_player_screen()
{

    // just hidden, no need to rebuild the contents
    if(isFSPlayerScreenCreated)
    {
        if(isFSPlayerScreenHidden)
        {
            // remove hide flag, so we can reveal the existing content
            lv_obj_remove_flag(lvMainContFS, LV_OBJ_FLAG_HIDDEN);
            register_group(lvGroupFSPlayerScreen);
            lv_group_set_default(lvGroupFSPlayerScreen);
            isFSPlayerScreenHidden = false;
        }
        goto EXIT_FS_PLAYER_SCREEN;
    }


    // 创建外部输入group，并聚焦
    if(lvGroupFSPlayerScreen == NULL)
    {
        lvGroupFSPlayerScreen = lv_group_create();
        register_group(lvGroupFSPlayerScreen);
        lv_group_set_default(lvGroupFSPlayerScreen);
    }

    /* Create background container BEGIN */
    lv_obj_t* scr = lv_scr_act();
    lvMainContFS = lv_obj_create(scr);
    lv_obj_set_size(lvMainContFS, LV_PCT(100), LV_PCT(100));
    lv_obj_remove_flag(lvMainContFS, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(lvMainContFS, LV_OBJ_FLAG_CLICKABLE);
    //lv_obj_set_style_margin_all(lvMainContFS, 0, 0);
    //lv_obj_set_style_pad_all(lvMainContFS, 0, 0);
    lv_obj_add_style(lvMainContFS, &lvStyleInvisibleCont, 0);
    /* Create background container END */

    /* File Exp Part BEGIN */
    // LHS
//	lv_obj_set_layout(lvCont, LV_LAYOUT_GRID);
    const uint16_t COL_WIDTH = LV_HOR_RES_MAX / 2;
    const uint16_t ROW_HEIGHT =LV_VER_RES_MAX;
//	static int32_t column_dsc[] = {COL_WIDTH, COL_WIDTH, LV_GRID_TEMPLATE_LAST};   /*4 columns with 100 and 400 ps width*/
//	static int32_t row_dsc[] = {ROW_HEIGHT,LV_GRID_TEMPLATE_LAST}; /*3 100 px tall rows*/
//	lv_obj_set_grid_dsc_array(lvCont, column_dsc, row_dsc);

    lv_obj_t* lvContFileExp = lv_obj_create(lvMainContFS);
    lv_obj_set_size(lvContFileExp, COL_WIDTH, ROW_HEIGHT);
    lv_obj_align(lvContFileExp, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_add_style(lvMainContFS, &lvStyleInvisibleCont, 0);

    // NOTE: NO NEED TO ADD TO GROUP AGAIN, will be added to the default group automatically
    lv_obj_t* lvFileExp = lv_simple_file_explorer_unicode_create(lvContFileExp);   
    // cast obj to a more meaningful type
    lv_simple_file_explorer_unicode_t* explorer = (lv_simple_file_explorer_unicode_t*)lvFileExp;
    lv_obj_add_style(explorer->cont, &lvStyleInvisibleCont, 0);

    lv_obj_add_event_cb(lvFileExp, file_explorer_event_handler_cb,  LV_EVENT_VALUE_CHANGED, NULL);
    // set initially showed directory
    // make music dir if not exist
    FRESULT fr;
#ifndef LVGL_SIM
    DIR dir;
    fr = f_opendir(&dir, FS_PATH_MUSIC_FOLDER);
    if(fr == FR_OK)
        f_closedir(&dir);
    else
    {
        // make dir
        fr = f_mkdir(FS_PATH_MUSIC_FOLDER);
        if(fr != FR_OK)
            show_toast("Music folder not found!");
    }
#endif
    lv_simple_file_explorer_unicode_open_dir(lvFileExp, FS_PATH_MUSIC_FOLDER);

    /*File Exp Part END */


    /* Player Part BEGIN */ // RHS
    lv_obj_t* lvPlayer = create_player_cont(lvMainContFS);
    lv_obj_set_style_margin_all(lvPlayer, 0, 0);
    lv_obj_set_style_pad_all(lvPlayer, 0, 0);
    lv_obj_set_size(lvPlayer, COL_WIDTH, ROW_HEIGHT);
    lv_obj_align(lvPlayer, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_flex_flow(lvPlayer, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(lvPlayer, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    // play timer
    duration_update_timer = lv_timer_create(duration_timer_cb, 1000, NULL);
    lv_timer_pause(duration_update_timer);
    // 
    lv_obj_t* title_box = create_player_title_box(lvPlayer);
    lv_obj_t* ctrl_box = create_player_ctrl_box(lvPlayer);


    // playlsit indicator
    lv_obj_t* playlist_indicator = create_player_playlist_indicator(lvPlayer);

    /* open default playlist file */
    // protedted: mtxFSPlayer BEGIN
#ifndef LVGL_SIM
    osMutexAcquire(mtxFSPlayer, osWaitForever);
#endif
    if (!isOpen_PL)
    {
    	// check folder existance. Make a new one if necessary
#ifndef LVGL_SIM
    	fr = f_opendir(&dir, FS_FOLDER_PLAYLIST);
    	if(fr == FR_OK)
    		f_closedir(&dir);
    	else
    	{
    		// make dir
    		fr = f_mkdir(FS_FOLDER_PLAYLIST);
    		if(fr != FR_OK)
    			show_toast("Playlist folder not found!");
    	}
#endif
    	fr = f_open(&filePL, FS_FOLDER_PLAYLIST _T("/") FS_FNAME_PLAYLSIT_1, FA_READ | FA_WRITE | FA_OPEN_ALWAYS);
        if (fr == 0)
        {
            isOpen_PL = true;
            playlist_init(&pl, &filePL, FS_FOLDER_PLAYLIST _T("/") FS_FNAME_PLAYLSIT_1); // path of the default playlist
            int found = playlist_next(&pl, wcsFilename, wcsFullPath, _MAX_LFN);
            if (found >= 0)
            {
                size_t cntFullPath = strnlen_TCHAR(wcsFullPath, _MAX_LFN);
                size_t cntFname = strnlen_TCHAR(wcsFilename, _MAX_LFN);
                // extract folder from fullpath
                strncpy_TCHAR(wcsFolder, wcsFullPath, cntFullPath - cntFname);
                // switch to the first track in playlist
#ifndef LVGL_SIM
                FS_player_change_track(wcsFolder, wcsFilename);
#endif
            }
        }
    }
    // continue the last playlist item
    else if(playlist_getcur(&pl, wcsFilename, wcsFullPath, _MAX_LFN) == 0) 
    {
        size_t cntFullPath = strnlen_TCHAR(wcsFullPath, _MAX_LFN);
        size_t cntFname = strnlen_TCHAR(wcsFilename, _MAX_LFN);
        // extract folder from fullpath
        strncpy_TCHAR(wcsFolder, wcsFullPath, cntFullPath - cntFname);
        // switch to the first track in playlist
//        GUI_notify_change_track(&track_info, wcsFilename, cntFname, wcsFullPath, cntFullPath);
#ifndef LVGL_SIM
        FS_player_change_track(wcsFolder, wcsFilename);
#endif
    }
    // the previously closed
    else if((wcsFilename[0] != 0) && (wcsFullPath[0] != 0))
    {
        size_t cntFullPath = strnlen_TCHAR(wcsFullPath, _MAX_LFN);
        size_t cntFname = strnlen_TCHAR(wcsFilename, _MAX_LFN);
        // extract folder from fullpath
        strncpy_TCHAR(wcsFolder, wcsFullPath, cntFullPath - cntFname);
//        GUI_notify_change_track(&track_info, wcsFilename, cntFname, wcsFullPath, cntFullPath);
#ifndef LVGL_SIM
        FS_player_change_track(wcsFolder, wcsFilename);
#endif
    }
    else
    {
        // nothing opened
    }
#ifndef LVGL_SIM
    osMutexRelease(mtxFSPlayer);
#endif
    // protedted: mtxFSPlayer END
    /*Player Part END*/

    /* back button */
    lv_obj_t* lvBtnBack=create_back_button(lvMainContFS, lvGroupFSPlayerScreen, dismiss_FS_player_screen);

    isFSPlayerScreenCreated = true;
EXIT_FS_PLAYER_SCREEN:
    return;
}

/* MAIN　CONTENT END */
/************************ FS Player END ************************/


int GUI_notify_change_track(track_info_t* p, size_t duration_ms, const TCHAR* wcsFname, size_t cntFname, const TCHAR* wcsFullPath, size_t cntFullPath)
{
    // note: must be protected by mtxFSPlayer or called from FS Player task, for playlist will be modified
    // do not call in LVGL routines
    // can only be called from external tasks
    int r = 0;
    if(isFSPlayerScreenCreated)
    {
        // set GUI title displays
        UTF16ToUTF8(wcsFname, wcsFname + cntFname, (UTF8*)track_info.sTitle, (UTF8*)track_info.sTitle + sizeof(track_info.sTitle));
        lv_label_set_text_static(title_label, p->sTitle);
        p->duration_ms = duration_ms;
        lv_slider_set_range(slider_obj, 0, p->duration_ms);
        lv_slider_set_value(slider_obj, 0, LV_ANIM_OFF);
        lv_label_set_text_fmt(duration_time_label, "%"LV_PRIu32":%02"LV_PRIu32, p->duration_ms / 60000, p->duration_ms % 60000 / 1000);
        // light up the playlist indicator if the track is in
        if (isOpen_PL)
        {

            int rowid = playlist_isin(&pl, wcsFname);

            if (rowid >= 0)
            {
                p->is_in_playlist = true;
                // in playlist, light up the logo
                if (playlist_indicator != NULL)
                {
                    light_on_playlist_logo(1);
                }
            }
            else
            {
                p->is_in_playlist = false;
                // not in playlist,  deactivate the logo
                if (playlist_indicator != NULL)
                {
                    light_on_playlist_logo(0);
                }
            }
        }
    }
    return r;
}

void GUI_notify_play_pause(uint8_t state)
{
    // note: must be protected by lv_lock
    // need lvlock
    if(state) // now start to play ,switch button to the pause symbol
    {
        // to play
        if (isFSPlayerScreenCreated)
        {
            lv_image_set_src(play_obj, LV_SYMBOL_PAUSE);
            lv_timer_resume(duration_update_timer); // resume timer
        }
        isPlay_lvgl = true;
    }
    else // to pause ,switch button to the play symbol
    {
        // to pause
        if (isFSPlayerScreenCreated)
        {
            lv_image_set_src(play_obj, LV_SYMBOL_PLAY);
            lv_timer_pause(duration_update_timer); // pause timer
        }
        isPlay_lvgl = false;
    }
}

void GUI_notify_sample_rate_and_bitdepth(uint32_t fsHz, uint8_t bitdepth)
{
    // note: must be protected by lv_lock
    // need lvlock
    if(!isFSPlayerScreenCreated)
        return;
    char sValue[16];
    if(fsHz > 0 && fsHz < 999999)
    {
        uint8_t n = number2text(sValue, fsHz / 100, 1, 'k'); // 44100->44.1k
        sValue[n++] = 'H';
        sValue[n++] = 'z';
        sValue[n++] = '@';
        number2text(sValue+n, bitdepth, 0, 0);
        lv_label_set_text(sample_rate_label, sValue);
    }


}

#ifdef __cplusplus
}
#endif
