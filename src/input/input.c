//
// Created by arias on 8/4/24.
//

#include "input.h"
#include "keyboard.h"

#include <wlr/util/log.h>

static void server_new_pointer (struct comp_server *server, struct wlr_input_device *device) {
        /* We don't do anything special with pointers. All of our pointer handling
         * is proxied through wlr_cursor. On another compositor, you might take this
         * opportunity to do libinput configuration on the device to set
         * acceleration, etc. */
        wlr_cursor_attach_input_device (server->cursor, device);
}

void server_new_input (struct wl_listener *listener, void *data) {
        /* This event is raised by the backend when a new input device becomes
         * available. */
        struct comp_server      *server = wl_container_of (listener, server, new_input);
        struct wlr_input_device *device = data;
        switch (device->type) {
        case WLR_INPUT_DEVICE_KEYBOARD:
                server_new_keyboard (server, device);
                break;
        case WLR_INPUT_DEVICE_POINTER:
                server_new_pointer (server, device);
                break;
        default:
                break;
        }
        /* We need to let the wlr_seat know what our capabilities are, which is
         * communiciated to the client. In TinyWL we always have a cursor, even if
         * there are no pointer devices, so we always include that capability. */
        uint32_t caps = WL_SEAT_CAPABILITY_POINTER;
        if (!wl_list_empty (&server->keyboards)) {
                caps |= WL_SEAT_CAPABILITY_KEYBOARD;
        }
        wlr_seat_set_capabilities (server->seat, caps);
        wlr_log (WLR_INFO, "Input %s Added", device->name);
}