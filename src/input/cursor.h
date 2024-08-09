//
// Created by arias on 8/4/24.
//

#ifndef CURSOR_H
#define CURSOR_H

#include "../nwm_server.h"

void reset_cursor_mode (struct comp_server *server);

void server_cursor_button (struct wl_listener *listener, void *data);
void server_cursor_axis (struct wl_listener *listener, void *data);
void server_cursor_motion (struct wl_listener *listener, void *data);
void server_cursor_motion_absolute (struct wl_listener *listener, void *data);
void server_cursor_frame (struct wl_listener *listener, void *data);

#endif // CURSOR_H
