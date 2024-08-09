//
// Created by arias on 8/3/24.
//

#ifndef COMP_SERVER_H
#define COMP_SERVER_H

#include "xdg_shell.h"

#include <wayland-server-core.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_output.h>

enum cursor_mode
{
        CURSOR_PASSTHROUGH,
        CURSOR_MOVE,
        CURSOR_RESIZE,
};

struct comp_server
{
        char                           *name; // TEST
        struct wl_display              *wl_display;    // accepts clients from unix socket
        struct wl_event_loop           *wl_event_loop; // wl_display_get_event_loop (wl_display)
        struct wlr_backend             *backend;       // abstracts hardware i/o
        struct wlr_renderer            *renderer;
        struct wlr_allocator           *allocator;
        struct wlr_output_layout       *output_layout;
        struct wlr_scene               *scene;
        struct wlr_scene_output_layout *scene_layout;

        struct wlr_xdg_shell *xdg_shell;
        struct wl_listener    new_xdg_toplevel;
        struct wl_listener    new_xdg_popup;
        struct wl_list        toplevels;

        struct wlr_cursor          *cursor;
        struct wlr_xcursor_manager *cursor_mgr;
        struct wl_listener          cursor_motion;
        struct wl_listener          cursor_motion_absolute;
        struct wl_listener          cursor_button;
        struct wl_listener          cursor_axis;
        struct wl_listener          cursor_frame;

        struct wlr_seat   *seat;
        struct wl_listener new_input;
        struct wl_listener request_cursor;
        struct wl_listener request_set_selection;
        struct wl_list     keyboards;
        enum cursor_mode   cursor_mode;
        struct toplevel   *grabbed_toplevel;
        double             grab_x, grab_y;
        struct wlr_box     grab_geobox;
        uint32_t           resize_edges;

        struct wl_listener new_output;
        struct wl_list     outputs; // comp_output::link
};

#endif // COMP_SERVER_H
