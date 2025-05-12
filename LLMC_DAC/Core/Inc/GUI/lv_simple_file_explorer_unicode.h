/*
 * lv_simple_file_explorer_unicode.h
 *
 *  Created on: May 7, 2024
 *      Author: cpholzn
 */

#ifndef INC_GUI_LV_SIMPLE_FILE_EXPLORER_UNICODE_H_
#define INC_GUI_LV_SIMPLE_FILE_EXPLORER_UNICODE_H_

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "lvgl/src/lvgl.h"


/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

#if LV_USE_FILE_EXPLORER == 0
typedef enum {
    LV_EXPLORER_SORT_NONE,
    LV_EXPLORER_SORT_KIND,
} lv_file_explorer_sort_t;
#endif

/*Data of canvas*/
typedef struct {
    lv_obj_t obj;
    lv_obj_t * cont;
    lv_obj_t * head_area;
    lv_obj_t * browser_area;
    lv_obj_t * file_table;
    lv_obj_t * path_label;

    TCHAR   sel_filename[LV_FILE_EXPLORER_PATH_MAX_LEN]; // filename, WIDE CHAR
    TCHAR   current_path[LV_FILE_EXPLORER_PATH_MAX_LEN]; // folder, WIDE CHAR
    lv_file_explorer_sort_t sort;
} lv_simple_file_explorer_unicode_t;

extern const lv_obj_class_t lv_simple_file_explorer_unicode_class;

/***********************
 * GLOBAL VARIABLES
 ***********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/
lv_obj_t * lv_simple_file_explorer_unicode_create(lv_obj_t * parent);
void strip_slash(TCHAR * dir);
size_t concat_full_path(const TCHAR* wcsFolder, const TCHAR* wcsFilename, /* out */ TCHAR* wcsOut, size_t cntMax);
/*=====================
 * Setter functions
 *====================*/



/**
 * Set file_explorer sort
 * @param obj   pointer to a file explorer object
 * @param sort  the sort from 'lv_file_explorer_sort_t' enum.
 */
void lv_simple_file_explorer_unicode_set_sort(lv_obj_t * obj, lv_file_explorer_sort_t sort);

/*=====================
 * Getter functions
 *====================*/

/**
 * Get file explorer Selected file (no folder info)
 * @param obj   pointer to a file explorer object
 * @return      pointer to the file explorer selected file name
 */
const TCHAR * lv_simple_file_explorer_unicode_get_selected_file_name(const lv_obj_t * obj);

/**
 * Get file explorer cur path (folder only, no filename)
 * @param obj   pointer to a file explorer object
 * @return      pointer to the file explorer cur path
 */
const TCHAR * lv_simple_file_explorer_unicode_get_current_path(const lv_obj_t * obj);

/**
 * Get file explorer head area obj
 * @param obj   pointer to a file explorer object
 * @return      pointer to the file explorer head area obj(lv_obj)
 */
lv_obj_t * lv_simple_file_explorer_unicode_get_header(lv_obj_t * obj);

/**
 * Get file explorer head area obj
 * @param obj   pointer to a file explorer object
 * @return      pointer to the file explorer quick access area obj(lv_obj)
 */
// lv_obj_t * lv_simple_file_explorer_unicode_get_quick_access_area(lv_obj_t * obj);

/**
 * Get file explorer path obj(label)
 * @param obj   pointer to a file explorer object
 * @return      pointer to the file explorer path obj(lv_label)
 */
lv_obj_t * lv_simple_file_explorer_unicode_get_path_label(lv_obj_t * obj);


/**
 * Get file explorer file list obj(lv_table)
 * @param obj   pointer to a file explorer object
 * @return      pointer to the file explorer file table obj(lv_table)
 */
lv_obj_t * lv_simple_file_explorer_unicode_get_file_table(lv_obj_t * obj);

/**
 * Set file_explorer sort
 * @param obj   pointer to a file explorer object
 * @return the current mode from 'lv_file_explorer_sort_t'
 */
lv_file_explorer_sort_t lv_simple_file_explorer_unicode_get_sort(const lv_obj_t * obj);

/*=====================
 * Other functions
 *====================*/

/**
 * Open a specified path
 * @param obj   pointer to a file explorer object
 * @param dir   pointer to the path
 */
void lv_simple_file_explorer_unicode_open_dir(lv_obj_t * obj, const TCHAR * dir);

/**********************
 *      MACROS
 **********************/


#ifdef __cplusplus
} /*extern "C"*/
#endif



#endif /* INC_GUI_LV_SIMPLE_FILE_EXPLORER_UNICODE_H_ */
