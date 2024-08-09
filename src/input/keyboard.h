//
// Created by arias on 8/5/24.
//

#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "../nwm_server.h"

struct keyboard
{
        struct wl_list       link;
        struct comp_server  *server;
        struct wlr_keyboard *wlr_keyboard;

        struct wl_listener modifiers;
        struct wl_listener key;
        struct wl_listener destroy;
};

void keyboard_focus_toplevel (struct toplevel *toplevel, struct wlr_surface *surface);
void server_new_keyboard (struct comp_server *server, struct wlr_input_device *device);

#endif // KEYBOARD_H
