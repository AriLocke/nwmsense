//
// Created by arias on 8/4/24.
//

#include "seat.h"

#include <wlr/types/wlr_data_device.h>
#include <wlr/types/wlr_seat.h>

void seat_request_cursor (struct wl_listener *listener, void *data) {
        struct comp_server *server = wl_container_of (listener, server, request_cursor);
        /* This event is raised by the seat when a client provides a cursor image */
        struct wlr_seat_pointer_request_set_cursor_event *event = data;
        struct wlr_seat_client *focused_client = server->seat->pointer_state.focused_client;
        /* This can be sent by any client, so we check to make sure this one is
         * actually has pointer focus first. */
        if (focused_client == event->seat_client) {
                /* Once we've vetted the client, we can tell the cursor to use the
                 * provided surface as the cursor image. It will set the hardware cursor
                 * on the output that it's currently on and continue to do so as the
                 * cursor moves between outputs. */
                wlr_cursor_set_surface (
                    server->cursor, event->surface, event->hotspot_x, event->hotspot_y);
        }
}

void seat_request_set_selection (struct wl_listener *listener, void *data) {
        /* This event is raised by the seat when a client wants to set the selection,
         * usually when the user copies something. wlroots allows compositors to
         * ignore such requests if they so choose, but in tinywl we always honor
         */
        struct comp_server *server = wl_container_of (listener, server, request_set_selection);
        struct wlr_seat_request_set_selection_event *event = data;
        wlr_seat_set_selection (server->seat, event->source, event->serial);
}