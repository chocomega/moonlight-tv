#pragma once

#include "lv_conf.h"
#include "lvgl.h"

lv_indev_t *lv_sdl_init_key_input();
void lv_sdl_deinit_key_input(lv_indev_t *indev);