/*
 * lv_simple_file_explorer_unicode.c
 *
 *  Created on: May 7, 2024
 *      Author: cpholzn
 */


/**
 * @file lv_file_explorer.c
 *
 */
#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/

#include <string.h>
#include <wchar.h>
#include "lv_simple_file_explorer_unicode.h"
#include "utf_convert.h"


#include "lvgl/src/core/lv_global.h"
#ifndef LVGL_SIM
#include "ff.h"
#else
#include "sim.h"
#endif


/*********************
 *      DEFINES
 *********************/
#define MY_CLASS (&lv_simple_file_explorer_unicode_class)

#define FILE_EXPLORER_QUICK_ACCESS_AREA_WIDTH       (22)
#define FILE_EXPLORER_BROWSER_AREA_WIDTH            (100 - FILE_EXPLORER_QUICK_ACCESS_AREA_WIDTH)

#define quick_access_list_button_style (LV_GLOBAL_DEFAULT()->fe_list_button_style)

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void lv_simple_file_explorer_unicode_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void browser_file_event_handler(lv_event_t * e);
static void init_style(lv_obj_t * obj);
static void show_dir(lv_obj_t * obj, const TCHAR * wcsPath);
static void file_explorer_sort(lv_obj_t * obj);
static void sort_by_file_kind(lv_obj_t * tb, int16_t lo, int16_t hi);
static void exch_table_item(lv_obj_t * tb, int16_t i, int16_t j);
static bool is_end_with(const char * str1, const char * str2);

/**********************
 *  STATIC VARIABLES
 **********************/

const lv_obj_class_t lv_simple_file_explorer_unicode_class = {
    .constructor_cb = lv_simple_file_explorer_unicode_constructor,
    .width_def      = LV_SIZE_CONTENT,
    .height_def     = LV_SIZE_CONTENT,
    .instance_size  = sizeof(lv_simple_file_explorer_unicode_t),
    .base_class     = &lv_obj_class,
    .name = "simple-file-explorer-unicode",
};

/**********************
 *      MACROS
 **********************/


/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_obj_t * lv_simple_file_explorer_unicode_create(lv_obj_t * parent)
{
    LV_LOG_INFO("begin");
    lv_obj_t * obj = lv_obj_class_create_obj(MY_CLASS, parent);
    lv_obj_class_init_obj(obj);
    return obj;
}

/*=====================
 * Setter functions
 *====================*/

void lv_simple_file_explorer_unicode_set_sort(lv_obj_t * obj, lv_file_explorer_sort_t sort)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_simple_file_explorer_unicode_t * explorer = (lv_simple_file_explorer_unicode_t *)obj;

    explorer->sort = sort;

    file_explorer_sort(obj);
}

/*=====================
 * Getter functions
 *====================*/
const TCHAR * lv_simple_file_explorer_unicode_get_selected_file_name(const lv_obj_t * obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_simple_file_explorer_unicode_t * explorer = (lv_simple_file_explorer_unicode_t *)obj;

    return explorer->sel_filename;


}

const TCHAR * lv_simple_file_explorer_unicode_get_current_path(const lv_obj_t * obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_simple_file_explorer_unicode_t * explorer = (lv_simple_file_explorer_unicode_t *)obj;

    return explorer->current_path;
}

lv_obj_t * lv_simple_file_explorer_unicode_get_file_table(lv_obj_t * obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_simple_file_explorer_unicode_t * explorer = (lv_simple_file_explorer_unicode_t *)obj;

    return explorer->file_table;
}

lv_obj_t * lv_simple_file_explorer_unicode_get_header(lv_obj_t * obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_simple_file_explorer_unicode_t * explorer = (lv_simple_file_explorer_unicode_t *)obj;

    return explorer->head_area;
}

/*
lv_obj_t * lv_simple_file_explorer_unicode_get_quick_access_area(lv_obj_t * obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_simple_file_explorer_unicode_t * explorer = (lv_simple_file_explorer_unicode_t *)obj;

    return explorer->quick_access_area;
}*/

lv_obj_t * lv_simple_file_explorer_unicode_get_path_label(lv_obj_t * obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_simple_file_explorer_unicode_t * explorer = (lv_simple_file_explorer_unicode_t *)obj;

    return explorer->path_label;
}



lv_file_explorer_sort_t lv_simple_file_explorer_unicode_get_sort(const lv_obj_t * obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_simple_file_explorer_unicode_t * explorer = (lv_simple_file_explorer_unicode_t *)obj;

    return explorer->sort;
}

/*=====================
 * Other functions
 *====================*/

// shares the same buffer, do not use simultaneously
#ifndef LVGL_SIM
static char __attribute__((section(".DTCMRAM1_heap"))) bufUTF8[LV_FILE_EXPLORER_PATH_MAX_LEN];
static TCHAR __attribute__((section(".DTCMRAM1_heap"))) bufUTF16[LV_FILE_EXPLORER_PATH_MAX_LEN];
#else
static char  bufUTF8[LV_FILE_EXPLORER_PATH_MAX_LEN];
static TCHAR  bufUTF16[LV_FILE_EXPLORER_PATH_MAX_LEN];
#endif
void lv_simple_file_explorer_unicode_open_dir(lv_obj_t * obj, const TCHAR * dir)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    // Note:  Call this function to come here after the file explorer is constructed
    // so the file table will be constructed
    show_dir(obj, dir);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/
static void lv_simple_file_explorer_unicode_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
    LV_UNUSED(class_p);
    LV_TRACE_OBJ_CREATE("begin");

    lv_simple_file_explorer_unicode_t * explorer = (lv_simple_file_explorer_unicode_t *)obj;


    explorer->sort = LV_EXPLORER_SORT_NONE;

    lv_memzero(explorer->current_path, sizeof(explorer->current_path));

    lv_obj_set_size(obj, LV_PCT(100), LV_PCT(100));
    lv_obj_set_flex_flow(obj, LV_FLEX_FLOW_COLUMN);

    explorer->cont = lv_obj_create(obj);
    lv_obj_set_width(explorer->cont, LV_PCT(100));
    lv_obj_set_flex_grow(explorer->cont, 1);



    /*File table area on the right*/
    explorer->browser_area = lv_obj_create(explorer->cont);

    lv_obj_set_size(explorer->browser_area, LV_PCT(100), LV_PCT(100));

    lv_obj_set_flex_flow(explorer->browser_area, LV_FLEX_FLOW_COLUMN);

    /*The area displayed above the file browse list(head)*/
    explorer->head_area = lv_obj_create(explorer->browser_area);
    lv_obj_set_size(explorer->head_area, LV_PCT(100), LV_PCT(10));
    lv_obj_remove_flag(explorer->head_area, LV_OBJ_FLAG_SCROLLABLE);


    /*Show current path*/
    explorer->path_label = lv_label_create(explorer->head_area);
    // NOTE: lvgl text only accepts utf8
    lv_label_set_text(explorer->path_label, ".");
    lv_obj_center(explorer->path_label);

    /*Table showing the contents of the table of contents*/
    explorer->file_table = lv_table_create(explorer->browser_area);
    lv_obj_set_size(explorer->file_table, LV_PCT(100), LV_PCT(90));
    lv_table_set_column_width(explorer->file_table, 0, LV_PCT(100));
    // add some room for easier click
    lv_obj_set_style_pad_ver(explorer->file_table, 8, LV_PART_ITEMS);
    lv_obj_set_scroll_dir(explorer->file_table, LV_DIR_VER);
    lv_table_set_column_count(explorer->file_table, 1);
    //lv_obj_add_event_cb(explorer->file_table, browser_file_event_handler, LV_EVENT_VALUE_CHANGED | LV_EVENT_CLICKED | LV_EVENT_SIZE_CHANGED, obj);
    lv_obj_add_event_cb(explorer->file_table, browser_file_event_handler, LV_EVENT_VALUE_CHANGED, obj);
    // add to default group and focus
    lv_group_add_obj(lv_group_get_default(), explorer->file_table);
    lv_group_focus_obj(explorer->file_table);
    /*only scroll up and down*/
    lv_obj_set_scroll_dir(explorer->file_table, LV_DIR_TOP | LV_DIR_BOTTOM);

    /*Initialize style*/
    init_style(obj);

    LV_TRACE_OBJ_CREATE("finished");
}

static void init_style(lv_obj_t * obj)
{
	lv_simple_file_explorer_unicode_t * explorer = (lv_simple_file_explorer_unicode_t *)obj;

    /*lv_file_explorer obj style*/
    lv_obj_set_style_radius(obj, 0, 0);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xf2f1f6), 0);

    /*main container style*/
    lv_obj_set_style_radius(explorer->cont, 0, 0);
    lv_obj_set_style_bg_opa(explorer->cont, LV_OPA_0, 0);
    lv_obj_set_style_border_width(explorer->cont, 0, 0);
    lv_obj_set_style_outline_width(explorer->cont, 0, 0);
    lv_obj_set_style_pad_column(explorer->cont, 0, 0);
    lv_obj_set_style_pad_row(explorer->cont, 0, 0);
    lv_obj_set_style_flex_flow(explorer->cont, LV_FLEX_FLOW_ROW, 0);
    lv_obj_set_style_pad_all(explorer->cont, 0, 0);
    lv_obj_set_style_layout(explorer->cont, LV_LAYOUT_FLEX, 0);

    /*head cont style*/
    lv_obj_set_style_radius(explorer->head_area, 0, 0);
    lv_obj_set_style_border_width(explorer->head_area, 0, 0);
    lv_obj_set_style_pad_top(explorer->head_area, 0, 0);


    /*File browser container style*/
    lv_obj_set_style_pad_all(explorer->browser_area, 0, 0);
    lv_obj_set_style_pad_row(explorer->browser_area, 0, 0);
    lv_obj_set_style_radius(explorer->browser_area, 0, 0);
    lv_obj_set_style_border_width(explorer->browser_area, 0, 0);
    lv_obj_set_style_outline_width(explorer->browser_area, 0, 0);
    lv_obj_set_style_bg_color(explorer->browser_area, lv_color_hex(0xffffff), 0);

    /*Style of the table in the browser container*/
    lv_obj_set_style_bg_color(explorer->file_table, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_pad_all(explorer->file_table, 0, 0);
    lv_obj_set_style_radius(explorer->file_table, 0, 0);
    lv_obj_set_style_border_width(explorer->file_table, 0, 0);
    lv_obj_set_style_outline_width(explorer->file_table, 0, 0);


}


static void browser_file_event_handler(lv_event_t * e)
{
	static char full_path[LV_FILE_EXPLORER_PATH_MAX_LEN];
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_user_data(e);
    

    lv_simple_file_explorer_unicode_t * explorer = (lv_simple_file_explorer_unicode_t *)obj;

	/* when selected, will firstly update file_name by the newly selected cell */
    if((code & (LV_EVENT_VALUE_CHANGED)) != 0)
    {
        const char * str_fn = NULL;
        uint32_t row;
        uint32_t col;

        lv_memzero(full_path, sizeof(full_path));
        lv_table_get_selected_cell(explorer->file_table, &row, &col);
        str_fn = lv_table_get_cell_value(explorer->file_table, row, col);

        if((lv_strcmp(str_fn, ".") == 0))  return;

        size_t countPath;
        countPath = strnlen_TCHAR(explorer->current_path, LV_FILE_EXPLORER_PATH_MAX_LEN);
        /* if clicked "..",  back to the parent folder */
        if((lv_strcmp(str_fn, "返回上级") == 0))
        {
        	// only when the current path is not the root folder
			if(countPath > 3)
			{
				/*Remove the last '/' character*/
				if(explorer->current_path[countPath - 1] == '/')
					strip_slash(explorer->current_path);
				/* Remove the last segment */
				strip_slash(explorer->current_path);
				// recount the path length after stripping
				countPath = strnlen_TCHAR(explorer->current_path, LV_FILE_EXPLORER_PATH_MAX_LEN);
				// convert wchar current_path to UTF8
				show_dir(obj, explorer->current_path);
				lv_obj_send_event(explorer, LV_EVENT_VALUE_CHANGED, "folder");
			}
        }
        /* otherwise, treat the selected path as a FOLDER or a FILE */
        else
        {
#if _LFN_UNICODE
			// concat current PATH(folder) and the selected PATH(either folder or file)
			int sizeUTF8 = UTF16ToUTF8((UTF16*)explorer->current_path, (UTF16*)explorer->current_path + countPath,
					(UTF8*)bufUTF8, (UTF8*)(bufUTF8 + sizeof(bufUTF8)));
			char* current_path_UTF8 = bufUTF8;
#else
            int sizeUTF8 = strlen(explorer->current_path);
			char* current_path_UTF8 = explorer->current_path;
#endif
			//lv_snprintf((char *)file_name, sizeof(file_name), "%s%s", current_path_UTF8, str_fn);
			strncpy(full_path, current_path_UTF8, sizeof(full_path));
			if (sizeUTF8 < sizeof(full_path))
			{
				strncpy(full_path + sizeUTF8, str_fn, sizeof(full_path) - sizeUTF8);
			}
	        // convert file_name from utf8 to wide char
	        DIR dir;
	        size_t cntFullPath = UTF8ToUTF16((UTF8*)full_path,(UTF8*)full_path + strnlen(full_path,sizeof(full_path)), (UTF16*)bufUTF16, ((UTF16*)bufUTF16) + sizeof(bufUTF16) / sizeof(TCHAR));
	        TCHAR* wcs_full_path = bufUTF16;
			/* CASE: DIRECTORY */
	        if (f_opendir(&dir, wcs_full_path) == FR_OK)
	        {
	            f_closedir(&dir);
	            // remove trailling '/'
	            if((cntFullPath > 0) && wcs_full_path[cntFullPath - 1] == _T('/'))
	            	wcs_full_path[cntFullPath - 1] = 0;
	            show_dir(obj, wcs_full_path);
	            lv_obj_send_event(explorer, LV_EVENT_VALUE_CHANGED, "folder");
	        }
			/* CASE: FILE */
	        else
	        {
	            // only change the filename, skip the folder part by forwarding the ptr by countPath
	            strncpy_TCHAR(explorer->sel_filename, wcs_full_path + countPath,  _MAX_LFN);
	            lv_obj_send_event(explorer, LV_EVENT_VALUE_CHANGED, "file");
	        }

        }
    }

}

static void show_dir(lv_obj_t * obj, const TCHAR * wcsPath)
{
    // transform wchar to utf8,

    lv_simple_file_explorer_unicode_t * explorer = (lv_simple_file_explorer_unicode_t *)obj;

//    char fn[LV_FILE_EXPLORER_PATH_MAX_LEN];
    FILINFO finfo;
    uint16_t index = 0;

    // to comply with Unicode, use FatFS to open folder
    DIR dir;
    FRESULT fr;
    fr = f_opendir(&dir, wcsPath);

    if(fr != FR_OK) {
        LV_LOG_USER("Open dir error %d!", fr);
        return;
    }


    lv_table_set_cell_value_fmt(explorer->file_table, index++, 0,  "%s", "返回上级");


    while(1) {
    	// call f_readdir repetitively to get file infos in sequence
        fr = f_readdir(&dir, &finfo);
        if(fr != FR_OK) {
            LV_LOG_USER("Driver, file or directory does not exist %d!", fr);
            break;
        }

        /*fn is empty, if no more files to read*/
        TCHAR* wcsFn = finfo.fname;
        if(wcsFn[0] == 0U) {
            LV_LOG_USER("No more files to read!");
            break;
        }

        // convert wide char fn to utf8 string
#if _LFN_UNICODE
        size_t countFn;
        countFn = strnlen_TCHAR(wcsFn, _MAX_LFN);
        UTF16ToUTF8((UTF16*)wcsFn, (UTF16*)(wcsFn + countFn),
        		(UTF8*)bufUTF8, (UTF8*)(bufUTF8 + sizeof(bufUTF8)));
        char* fn = bufUTF8;
#else
        char* fn = wcsFn;
#endif

        if(is_end_with(fn, ".wav") || is_end_with(fn, ".WAV") || is_end_with(fn, ".mp3"))
        {
            lv_table_set_cell_value_fmt(explorer->file_table, index++, 0,  "%s", fn);
            //lv_table_set_cell_value(explorer->file_table, index, 1, "2");
        }
        else if((is_end_with(fn, ".png") == true)  || (is_end_with(fn, ".PNG") == true)  || \
           (is_end_with(fn, ".jpg") == true) || (is_end_with(fn, ".JPG") == true) || \
           (is_end_with(fn, ".bmp") == true) || (is_end_with(fn, ".BMP") == true) || \
           (is_end_with(fn, ".gif") == true) || (is_end_with(fn, ".GIF") == true))
        {
            lv_table_set_cell_value_fmt(explorer->file_table, index++, 0,  "%s", fn);
            //lv_table_set_cell_value(explorer->file_table, index, 1, "1");
        }
//        else if((is_end_with(fn, ".mp4") == true) || (is_end_with(fn, ".MP4") == true)) {
//            lv_table_set_cell_value_fmt(explorer->file_table, index, 0, LV_SYMBOL_VIDEO "  %s", fn);
//            lv_table_set_cell_value(explorer->file_table, index, 1, "3");
//        }
        else if((is_end_with(fn, ".")) || (is_end_with(fn, "..")))
        {
            /*is relative return, hide the row */
        	// do nothing
        }
        else if(fn[0] == _T('/'))
        {/*is dir*/
            lv_table_set_cell_value_fmt(explorer->file_table, index++, 0,  "%s", fn + 1);
            //lv_table_set_cell_value(explorer->file_table, index, 1, "0");
        }
        else
        {
            lv_table_set_cell_value_fmt(explorer->file_table, index++, 0, "%s", fn);
            //lv_table_set_cell_value(explorer->file_table, index, 1, "4");
        }
    }

    f_closedir(&dir);

    lv_table_set_row_count(explorer->file_table, index);
    file_explorer_sort(obj);
    lv_obj_send_event(obj, LV_EVENT_READY, NULL);

    /*Move the table to the top*/
    lv_obj_scroll_to_y(explorer->file_table, 0, LV_ANIM_OFF);


    // sometimes the function parameter is just explorer->current_path it self, then no need to copy
    if(wcsPath != explorer->current_path)
    {
		strncpy_TCHAR(explorer->current_path, wcsPath,
				sizeof(explorer->current_path) / sizeof(TCHAR));
    }

    // convert wchar wcsPath to utf8
        size_t countPath;
#if _LFN_UNICODE
        countPath = strnlen_TCHAR(wcsPath, _MAX_LFN);
        int sizeUTF8 = UTF16ToUTF8((UTF16*)wcsPath, (UTF16*)(wcsPath + countPath),
        		(UTF8*)bufUTF8, (UTF8*)(bufUTF8 + sizeof(bufUTF8)));
        char* path = bufUTF8;
#else
        countPath = strnlen(wcsPath, _MAX_LFN);
        char* path = wcsPath;
#endif
    lv_label_set_text_fmt(explorer->path_label, "%s", path);

    // add '/' to the end of the dirpath when missing
    if((*((explorer->current_path) + countPath-1) != _T('/')) && (countPath < LV_FILE_EXPLORER_PATH_MAX_LEN-1)) {
        *((explorer->current_path) + countPath) = _T('/');
        *((explorer->current_path) + countPath+1) = _T('\0');
    }
}


/*Remove the right-most slash suffix and content after it*/
void strip_slash(TCHAR * dir)
{
	size_t countDir = strnlen_TCHAR(dir, _MAX_LFN);
    TCHAR * end = dir + countDir;

    while(end >= dir && *end != _T('/')) {
        --end;
    }

    if(end > dir) {
        *end = 0;
    }
    else if(end == dir) {
        *(end + 1) = 0;
    }
}


size_t concat_full_path(const TCHAR* wcsFolder, const TCHAR* wcsFilename, /* out */ TCHAR* wcsOut, size_t cntMax)
{
	const TCHAR* s = strncpy_TCHAR(wcsOut, wcsFolder, cntMax);
	size_t cntFolder = s - wcsOut;
	s = strncpy_TCHAR(wcsOut + cntFolder, wcsFilename, cntMax - cntFolder);
	wcsOut[cntMax - 1] = 0; // terminate the strung
	return s - wcsOut;
}


static void exch_table_item(lv_obj_t * tb, int16_t i, int16_t j)
{
   strncpy(bufUTF8, lv_table_get_cell_value(tb, i, 0), sizeof(bufUTF8) - 1);
    //lv_table_set_cell_value(tb, 0, 2, tmp);
    lv_table_set_cell_value(tb, i, 0, lv_table_get_cell_value(tb, j, 0));
    lv_table_set_cell_value(tb, j, 0, bufUTF8);

    //tmp = lv_table_get_cell_value(tb, i, 1);
    //lv_table_set_cell_value(tb, 0, 2, tmp);
    //lv_table_set_cell_value(tb, i, 1, lv_table_get_cell_value(tb, j, 1));
    //lv_table_set_cell_value(tb, j, 1, lv_table_get_cell_value(tb, 0, 2));
}

static void file_explorer_sort(lv_obj_t * obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_simple_file_explorer_unicode_t * explorer = (lv_simple_file_explorer_unicode_t*)obj;

    uint16_t sum = lv_table_get_row_count(explorer->file_table);

    if(sum > 1) {
        switch(explorer->sort) {
            case LV_EXPLORER_SORT_NONE:
                break;
            case LV_EXPLORER_SORT_KIND:
                sort_by_file_kind(explorer->file_table, 0, (sum - 1));
                break;
            default:
                break;
        }
    }
}

/*Quick sort 3 way*/
static void sort_by_file_kind(lv_obj_t * tb, int16_t lo, int16_t hi)
{
    if(lo >= hi) return;

    int16_t lt = lo;
    int16_t i = lo + 1;
    int16_t gt = hi;
    const char * v = lv_table_get_cell_value(tb, lo, 1);
    while(i <= gt) {
        if(lv_strcmp(lv_table_get_cell_value(tb, i, 1), v) < 0)
            exch_table_item(tb, lt++, i++);
        else if(lv_strcmp(lv_table_get_cell_value(tb, i, 1), v) > 0)
            exch_table_item(tb, i, gt--);
        else
            i++;
    }

    sort_by_file_kind(tb, lo, lt - 1);
    sort_by_file_kind(tb, gt + 1, hi);
}

static bool is_end_with(const char * str1, const char * str2)
{
    if(str1 == NULL || str2 == NULL)
        return false;

    uint16_t len1 = strlen(str1);
    uint16_t len2 = strlen(str2);
    if((len1 < len2) || (len1 == 0 || len2 == 0))
        return false;

    while(len2 >= 1) {
        if(str2[len2 - 1] != str1[len1 - 1])
            return false;

        len2--;
        len1--;
    }

    return true;
}

#ifdef __cplusplus
} /*extern "C"*/
#endif

/*LV_USE_FILE_EXPLORER*/
