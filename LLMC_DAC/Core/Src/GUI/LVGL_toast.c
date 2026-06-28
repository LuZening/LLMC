#include "LVGL_toast.h"
#include "LVGL_GUI.h"

// 淡入动画执行回调
static void toast_anim_in_exec(void *obj, int32_t value)
{
    lv_obj_set_style_opa((lv_obj_t *)obj, value, 0);
}

// 淡出动画完成回调，用于删除对象
static void toast_anim_out_ready(lv_anim_t *anim)
{
    lv_obj_delete_async((lv_obj_t *)anim->var);
}

// 淡出动画执行回调
static void toast_anim_out_exec(void *obj, int32_t value)
{
    lv_obj_set_style_opa((lv_obj_t *)obj, value, 0);
}


static void toast_timer_callback(lv_timer_t *t)
{
    lv_obj_t *obj = (lv_obj_t *)t->user_data;
    // 淡出动画
    static lv_anim_t anim_out;
    lv_anim_init(&anim_out);
    lv_anim_set_var(&anim_out, obj);
    lv_anim_set_values(&anim_out, 255, 0);
    lv_anim_set_path_cb(&anim_out, lv_anim_path_ease_in_out);
    lv_anim_set_time(&anim_out, 500);
    lv_anim_set_exec_cb(&anim_out, toast_anim_out_exec);
    lv_anim_set_completed_cb(&anim_out, toast_anim_out_ready);
    lv_anim_start(&anim_out);
}


// 创建并显示toast
void show_toast(const char *message) {
    // 创建toast对象
    lv_obj_t* toast_obj = lv_obj_create(lv_scr_act());
    lv_obj_set_size(toast_obj, 240, 50);
    lv_obj_align(toast_obj, LV_ALIGN_TOP_MID, 0, 10);

    // 创建文本标签
    lv_obj_t *toast_label = lv_label_create(toast_obj);
    lv_label_set_text(toast_label, message);
    lv_label_set_long_mode(toast_label, LV_LABEL_LONG_WRAP);

    // 设置样式
    lv_obj_set_style_bg_color(toast_obj, COLOR_BG, 0);
    lv_obj_set_style_border_width(toast_obj, 1, 0);
    lv_obj_set_style_border_color(toast_obj, COLOR_BUTTON, 0);
    lv_obj_set_style_text_color(toast_label, COLOR_PLAIN_TEXT, 0);

    // 淡入动画
    lv_anim_t anim_in;
    lv_anim_init(&anim_in);
    lv_anim_set_var(&anim_in, toast_obj);
    lv_anim_set_values(&anim_in, 0, 255);
    lv_anim_set_path_cb(&anim_in, lv_anim_path_ease_in_out);
    lv_anim_set_time(&anim_in, 500);
    lv_anim_set_exec_cb(&anim_in, toast_anim_in_exec);
    lv_anim_start(&anim_in);

    // 延迟3秒后淡出并删除
    lv_timer_t *timer = lv_timer_create(toast_timer_callback, 3000, toast_obj);
    timer->repeat_count = 1;
}
