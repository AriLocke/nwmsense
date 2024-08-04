//
// Created by arias on 8/3/24.
//

#ifndef COMP_OUTPUT_H
#define COMP_OUTPUT_H

#include "comp_server.h"

struct comp_output
{
        struct wlr_output  *wlr_output;
        struct comp_server *server;
        struct timespec     last_frame;

        struct wl_listener frame;
        struct wl_listener destroy;
        struct wl_list     link;
};

void new_output_notify (struct wl_listener *listener, void *data);
static void output_frame_notify (struct wl_listener *listener, void *data);
static void output_destroy_notify (struct wl_listener *listener, void *data);

#endif // COMP_OUTPUT_H
