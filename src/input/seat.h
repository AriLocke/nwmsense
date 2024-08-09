//
// Created by arias on 8/4/24.
//

#ifndef SEAT_H
#define SEAT_H

#include "../nwm_server.h"

void seat_request_cursor (struct wl_listener *listener, void *data);
void seat_request_set_selection (struct wl_listener *listener, void *data);

#endif // SEAT_H
