//
// Created by arias on 8/3/24.
//

#ifndef COMP_SERVER_H
#define COMP_SERVER_H

#include <wayland-server-core.h>
#include <wlr/types/wlr_output.h>

struct comp_server
{
        struct wl_display              *wl_display;    // accepts clients from unix socket
        struct wl_event_loop           *wl_event_loop; // wl_display_get_event_loop (wl_display)
        struct wlr_backend             *backend;       // abstracts hardware i/o
        struct wlr_renderer            *renderer;
        struct wlr_allocator           *allocator;
        struct wlr_output_layout       *output_layout;
        struct wlr_scene               *scene;
        struct wlr_scene_output_layout *scene_layout;

        struct wl_listener new_output;
        struct wl_list     outputs; // comp_output::link
};

#endif // COMP_SERVER_H
