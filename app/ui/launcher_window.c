#include <stdio.h>
#include <stdbool.h>
#include <memory.h>

#include "libgamestream/errors.h"

#include "backend/application_manager.h"
#include "launcher_window.h"

#define LINKEDLIST_TYPE PSERVER_LIST
#include "util/linked_list.h"

static PSERVER_LIST selected_server_node;

typedef enum pairing_state
{
    PS_NONE,
    PS_RUNNING,
    PS_FAIL
} pairing_state;

static struct
{
    pairing_state state;
    char pin[5];
    char *error;
} pairing_computer_state;

static struct nk_style_button cm_list_button_style;

static void pairing_window(struct nk_context *ctx);
static void pairing_error_dialog(struct nk_context *ctx);
static void cw_open_computer(PSERVER_LIST node);
static void cw_open_pair(int index, SERVER_DATA *item);

static bool cw_computer_dropdown(struct nk_context *ctx, PSERVER_LIST list, bool event_emitted);

void launcher_window_init(struct nk_context *ctx)
{
    selected_server_node = NULL;
    pairing_computer_state.state = PS_NONE;
    memcpy(&cm_list_button_style, &(ctx->style.button), sizeof(struct nk_style_button));
    cm_list_button_style.text_alignment = NK_TEXT_ALIGN_LEFT;
}

bool launcher_window(struct nk_context *ctx)
{
    /* GUI */
    nk_flags launcher_window_flags = NK_WINDOW_BORDER | NK_WINDOW_CLOSABLE | NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_TITLE;
    if (pairing_computer_state.state != PS_NONE)
    {
        launcher_window_flags |= NK_WINDOW_NO_INPUT;
    }
    int content_height_remaining;
    if (nk_begin(ctx, "Moonlight", nk_rect(50, 50, 300, 300), launcher_window_flags))
    {
        bool event_emitted = false;
        content_height_remaining = (int)nk_window_get_content_region_size(ctx).y;
        content_height_remaining -= ctx->style.window.padding.y * 2;
        content_height_remaining -= (int)ctx->style.window.border;
        nk_menubar_begin(ctx);
        nk_layout_row_begin(ctx, NK_STATIC, 25, 1);
        content_height_remaining -= nk_widget_bounds(ctx).h;

        nk_layout_row_push(ctx, 100);
        PSERVER_LIST computer_list = computer_manager_list();
        event_emitted |= cw_computer_dropdown(ctx, computer_list, event_emitted);

        nk_layout_row_end(ctx);

        nk_menubar_end(ctx);

        struct nk_list_view list_view;
        int computer_len = linkedlist_len(computer_list);
        nk_layout_row_dynamic(ctx, content_height_remaining, 1);
        if (selected_server_node != NULL)
        {
            event_emitted |= cw_application_list(ctx, selected_server_node, event_emitted);
        }
        else
        {
            nk_label(ctx, "Not selected", NK_TEXT_ALIGN_LEFT);
        }
    }
    nk_end(ctx);

    // Why Nuklear why, the button looks like "close" but it actually "hide"
    if (nk_window_is_hidden(ctx, "Moonlight"))
    {
        nk_window_close(ctx, "Moonlight");
        return false;
    }

    if (selected_server_node != NULL)
    {
    }
    else if (pairing_computer_state.state == PS_RUNNING)
    {
        pairing_window(ctx);
    }
    else if (pairing_computer_state.state == PS_FAIL)
    {
        pairing_error_dialog(ctx);
    }
    return true;
}

bool cw_computer_dropdown(struct nk_context *ctx, PSERVER_LIST list, bool event_emitted)
{
    if (nk_combo_begin_label(ctx, "Computer", nk_vec2(200, 200)))
    {
        nk_layout_row_dynamic(ctx, 25, 1);
        PSERVER_LIST cur = list;
        int i = 0;
        while (cur != NULL)
        {
            SERVER_DATA *server = (SERVER_DATA *)cur->server;
            if (nk_combo_item_label(ctx, server->serverInfo.address, NK_TEXT_LEFT))
            {
                if (!event_emitted)
                {
                    event_emitted = true;
                    if (server->paired)
                    {
                        cw_open_computer(cur);
                    }
                    else
                    {
                        cw_open_pair(i, server);
                    }
                }
            }
            cur = cur->next;
            i++;
        }
        // nk_combo_item_label(ctx, "Add manually", NK_TEXT_LEFT);
        nk_combo_end(ctx);
    }
    return event_emitted;
}

void cw_open_computer(PSERVER_LIST node)
{
    selected_server_node = node;
    pairing_computer_state.state = PS_NONE;
    if (node->apps == NULL)
    {
        application_manager_load(node);
    }
}

static void cw_pairing_callback(int result, const char *error)
{
    if (result == GS_OK)
    {
        // Close pairing window
        pairing_computer_state.state = PS_NONE;
    }
    else
    {
        // Show pairing error instead
        pairing_computer_state.state = PS_FAIL;
        pairing_computer_state.error = (char *)error;
    }
}

void cw_open_pair(int index, SERVER_DATA *item)
{
    selected_server_node = NULL;
    pairing_computer_state.state = PS_RUNNING;
    computer_manager_pair(item, &pairing_computer_state.pin[0], cw_pairing_callback);
}

void pairing_window(struct nk_context *ctx)
{
    if (nk_begin(ctx, "Pairing", nk_rect(350, 150, 300, 100),
                 NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_TITLE | NK_WINDOW_NO_SCROLLBAR))
    {
        nk_layout_row_dynamic(ctx, 50, 1);

        nk_labelf_wrap(ctx, "Please enter %s on your GameStream PC. This dialog will close when pairing is completed.",
                       pairing_computer_state.pin);
    }
    nk_end(ctx);
}

void pairing_error_dialog(struct nk_context *ctx)
{
    struct nk_vec2 win_region_size;
    int content_height_remaining;
    if (nk_begin(ctx, "Pairing Failed", nk_rect(350, 150, 300, 100),
                 NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_TITLE | NK_WINDOW_NO_SCROLLBAR))
    {
        win_region_size = nk_window_get_content_region_size(ctx);
        content_height_remaining = (int)win_region_size.y;
        content_height_remaining -= 8 * 2;
        /* remove bottom button height */
        content_height_remaining -= 30;
        nk_layout_row_dynamic(ctx, content_height_remaining, 1);

        if (pairing_computer_state.error != NULL)
        {
            nk_label_wrap(ctx, pairing_computer_state.error);
        }
        else
        {
            nk_label_wrap(ctx, "Pairing error.");
        }
        nk_layout_space_begin(ctx, NK_STATIC, 30, 1);
        nk_layout_space_push(ctx, nk_recti(win_region_size.x - 80, 0, 80, 30));
        if (nk_button_label(ctx, "OK"))
        {
            pairing_computer_state.state = PS_NONE;
        }
        nk_layout_space_end(ctx);
    }
    nk_end(ctx);
}