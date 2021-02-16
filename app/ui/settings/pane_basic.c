#include "window.h"

#include <stddef.h>
#include <stdio.h>

struct _resolution_option
{
    int w, h;
    char name[8];
};

struct _fps_option
{
    unsigned short fps;
    char name[8];
};

static const struct _resolution_option _supported_resolutions[] = {
    {1280, 720, "720P\0"},
    {1920, 1080, "1080P\0"},
    {2560, 1440, "2K\0"},
    {3840, 2160, "4K\0"},
};
#define _supported_resolutions_len sizeof(_supported_resolutions) / sizeof(struct _resolution_option)

static const struct _fps_option _supported_fps[] = {
    {30, "30 FPS"},
    {60, "60 FPS"},
    {120, "120 FPS"},
};
#define _supported_fps_len sizeof(_supported_fps) / sizeof(struct _fps_option)

#define BITRATE_MIN 5000
#define BITRATE_MAX 120000
#define BITRATE_STEP 500

static char _res_label[8], _fps_label[8];

static void _set_fps(int fps);
static void _set_res(int w, int h);
static void _update_bitrate();

bool _settings_pane_basic(struct nk_context *ctx, bool *showing_combo)
{
    static struct nk_rect item_bounds = {0, 0, 0, 0};
    int item_index = 0;

    nk_layout_row_dynamic_s(ctx, 25, 1);
    nk_label(ctx, "Resolution and FPS", NK_TEXT_LEFT);
    static const float ratio_resolution_fps[] = {0.6, 0.4};
    nk_layout_row_s(ctx, NK_DYNAMIC, 25, 2, ratio_resolution_fps);
    settings_item_update_selected_bounds(ctx, item_index++, &item_bounds);
    if (nk_combo_begin_label(ctx, _res_label, nk_vec2(nk_widget_width(ctx), 200 * NK_UI_SCALE)))
    {
        *showing_combo = true;
        nk_layout_row_dynamic_s(ctx, 25, 1);
        for (int i = 0; i < _supported_resolutions_len; i++)
        {
            struct _resolution_option o = _supported_resolutions[i];
            if (nk_combo_item_label(ctx, o.name, NK_TEXT_LEFT))
            {
                _set_res(o.w, o.h);
                _update_bitrate();
            }
        }
        nk_combo_end(ctx);
    }
    settings_item_update_selected_bounds(ctx, item_index++, &item_bounds);
    if (nk_combo_begin_label(ctx, _fps_label, nk_vec2(nk_widget_width(ctx), 200 * NK_UI_SCALE)))
    {
        *showing_combo = true;
        nk_layout_row_dynamic_s(ctx, 25, 1);
        for (int i = 0; i < _supported_fps_len; i++)
        {
            struct _fps_option o = _supported_fps[i];
            if (nk_combo_item_label(ctx, o.name, NK_TEXT_LEFT))
            {
                _set_fps(o.fps);
                _update_bitrate();
            }
        }
        nk_combo_end(ctx);
    }
    nk_layout_row_dynamic_s(ctx, 25, 1);
    nk_label(ctx, "Video bitrate", NK_TEXT_LEFT);
    settings_item_update_selected_bounds(ctx, item_index++, &item_bounds);
    nk_property_int(ctx, "kbps:", BITRATE_MIN, &app_configuration->stream.bitrate, BITRATE_MAX, BITRATE_STEP, 50);
    if (app_configuration->stream.bitrate > 50000)
    {
        nk_layout_row_dynamic_s(ctx, 50, 1);
        nk_label_wrap(ctx, "[!] Too high resolution/fps/bitrate may cause blank screen or crash.");
    }
    else
    {
        nk_layout_row_dynamic_s(ctx, 4, 1);
    }
    return true;
}

int _settings_pane_basic_itemcount()
{
    return 3;
}

void _pane_basic_open()
{
    _set_fps(app_configuration->stream.fps);
    _set_res(app_configuration->stream.width, app_configuration->stream.height);
}

bool _settings_pane_basic_navkey(struct nk_context *ctx, NAVKEY navkey, NAVKEY_STATE state, uint32_t timestamp)
{
    switch (navkey)
    {
    case NAVKEY_LEFT:
        if (settings_showing_combo)
        {
            return true;
        }
        else if (settings_hovered_item == 1)
        {
            if (state == NAVKEY_STATE_UP)
                settings_pane_item_offset(-1);
            return true;
        }
        else if (settings_hovered_item == 2)
        {
            if (!navkey_intercept_repeat(state, timestamp))
                app_configuration->stream.bitrate = NK_CLAMP(BITRATE_MIN, app_configuration->stream.bitrate - BITRATE_STEP,
                                                             BITRATE_MAX);
            return true;
        }
        return false;
    case NAVKEY_RIGHT:
        if (settings_showing_combo)
        {
            return true;
        }
        else if (settings_hovered_item == 0)
        {
            if (state == NAVKEY_STATE_UP)
                settings_pane_item_offset(1);
            return true;
        }
        else if (settings_hovered_item == 2)
        {
            if (!navkey_intercept_repeat(state, timestamp))
                app_configuration->stream.bitrate = NK_CLAMP(BITRATE_MIN, app_configuration->stream.bitrate + BITRATE_STEP,
                                                             BITRATE_MAX);
            return true;
        }
        return true;
    case NAVKEY_UP:
        if (settings_showing_combo)
        {
            return true;
        }
        else if (settings_hovered_item >= 2)
        {
            if (state == NAVKEY_STATE_UP)
                settings_pane_item_offset(0 - settings_hovered_item);
        }
        return true;
    case NAVKEY_DOWN:
        if (settings_showing_combo)
        {
            return true;
        }
        else if (settings_hovered_item < 2)
        {
            if (state == NAVKEY_STATE_UP)
                settings_pane_item_offset(2 - settings_hovered_item);
        }
        return true;
    default:
        break;
    }
    return false;
}

void _set_fps(int fps)
{
    // It is not possible to have overflow since fps is capped to 999
    sprintf(_fps_label, "%d FPS", fps % 1000);
    app_configuration->stream.fps = fps;
}

void _set_res(int w, int h)
{
    switch (RES_MERGE(w, h))
    {
    case RES_720P:
        sprintf(_res_label, "720P");
        break;
    case RES_1080P:
        sprintf(_res_label, "1080P");
        break;
    case RES_1440P:
        sprintf(_res_label, "1440P");
        break;
    case RES_4K:
        sprintf(_res_label, "4K");
        break;
    default:
        sprintf(_res_label, "%3d*%3d", w, h);
        break;
    }
    app_configuration->stream.width = w;
    app_configuration->stream.height = h;
}

void _update_bitrate()
{
    app_configuration->stream.bitrate = settings_optimal_bitrate(app_configuration->stream.width, app_configuration->stream.height, app_configuration->stream.fps);
}