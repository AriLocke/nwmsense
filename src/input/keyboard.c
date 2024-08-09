//
// Created by arias on 8/5/24.
//

#include "keyboard.h"
#include "../xdg_shell.h"

#include <stdlib.h>
#include <wlr/types/wlr_scene.h>

void keyboard_focus_toplevel (struct toplevel *toplevel, struct wlr_surface *surface) {
        /* Note: this function only deals with keyboard focus. */
        if (toplevel == NULL) {
                return;
        }
        struct comp_server *server       = toplevel->server;
        struct wlr_seat    *seat         = server->seat;
        struct wlr_surface *prev_surface = seat->keyboard_state.focused_surface;
        if (prev_surface == surface) {
                /* Don't re-focus an already focused surface. */
                return;
        }
        if (prev_surface) {
                /*
                 * Deactivate the previously focused surface. This lets the client know
                 * it no longer has focus and the client will repaint accordingly, e.g.
                 * stop displaying a caret.
                 */
                struct wlr_xdg_toplevel *prev_toplevel
                    = wlr_xdg_toplevel_try_from_wlr_surface (prev_surface);
                if (prev_toplevel != NULL) {
                        wlr_xdg_toplevel_set_activated (prev_toplevel, false);
                }
        }
        struct wlr_keyboard *keyboard = wlr_seat_get_keyboard (seat);
        /* Move the toplevel to the front */
        wlr_scene_node_raise_to_top (&toplevel->scene_tree->node);
        wl_list_remove (&toplevel->link);
        wl_list_insert (&server->toplevels, &toplevel->link);
        /* Activate the new surface */
        wlr_xdg_toplevel_set_activated (toplevel->xdg_toplevel, true);
        /*
         * Tell the seat to have the keyboard enter this surface. wlroots will keep
         * track of this and automatically send key events to the appropriate
         * clients without additional work on your part.
         */
        if (keyboard != NULL) {
                wlr_seat_keyboard_notify_enter (seat,
                                                toplevel->xdg_toplevel->base->surface,
                                                keyboard->keycodes,
                                                keyboard->num_keycodes,
                                                &keyboard->modifiers);
        }
}

static void keyboard_handle_modifiers (struct wl_listener *listener, void *data) {
        /* This event is raised when a modifier key, such as shift or alt, is
         * pressed. We simply communicate this to the client. */
        struct keyboard *keyboard = wl_container_of (listener, keyboard, modifiers);
        /*
         * A seat can only have one keyboard, but this is a limitation of the
         * Wayland protocol - not wlroots. We assign all connected keyboards to the
         * same seat. You can swap out the underlying wlr_keyboard like this and
         * wlr_seat handles this transparently.
         */
        wlr_seat_set_keyboard (keyboard->server->seat, keyboard->wlr_keyboard);
        /* Send modifiers to the client. */
        wlr_seat_keyboard_notify_modifiers (keyboard->server->seat,
                                            &keyboard->wlr_keyboard->modifiers);
}

static bool handle_keybinding (struct comp_server *server, xkb_keysym_t sym) {
        /*
         * Here we handle compositor keybindings. This is when the compositor is
         * processing keys, rather than passing them on to the client for its own
         * processing.
         *
         * This function assumes Alt is held down.
         */
        switch (sym) {
        case XKB_KEY_Escape:
                wl_display_terminate (server->wl_display);
                break;
        case XKB_KEY_F1:
                /* Cycle to the next toplevel */
                if (wl_list_length (&server->toplevels) < 2) {
                        break;
                }
                struct toplevel *next_toplevel
                    = wl_container_of (server->toplevels.prev, next_toplevel, link);
                keyboard_focus_toplevel (next_toplevel, next_toplevel->xdg_toplevel->base->surface);
                break;
        default:
                return false;
        }
        return true;
}

static void keyboard_handle_key (struct wl_listener *listener, void *data) {
        /* This event is raised when a key is pressed or released. */
        struct keyboard               *keyboard = wl_container_of (listener, keyboard, key);
        struct comp_server            *server   = keyboard->server;
        struct wlr_keyboard_key_event *event    = data;
        struct wlr_seat               *seat     = server->seat;

        /* Translate libinput keycode -> xkbcommon */
        uint32_t            keycode = event->keycode + 8;
        /* Get a list of keysyms based on the keymap for this keyboard */
        const xkb_keysym_t *syms;
        int nsyms = xkb_state_key_get_syms (keyboard->wlr_keyboard->xkb_state, keycode, &syms);

        bool     handled   = false;
        uint32_t modifiers = wlr_keyboard_get_modifiers (keyboard->wlr_keyboard);
        if ((modifiers & WLR_MODIFIER_ALT) && event->state == WL_KEYBOARD_KEY_STATE_PRESSED) {
                /* If alt is held down and this button was _pressed_, we attempt to
                 * process it as a compositor keybinding. */
                for (int i = 0; i < nsyms; i++) {
                        handled = handle_keybinding (server, syms[i]);
                }
        }

        if (!handled) {
                /* Otherwise, we pass it along to the client. */
                wlr_seat_set_keyboard (seat, keyboard->wlr_keyboard);
                wlr_seat_keyboard_notify_key (seat, event->time_msec, event->keycode, event->state);
        }
}

static void keyboard_handle_destroy (struct wl_listener *listener, void *data) {
        /* This event is raised by the keyboard base wlr_input_device to signal
         * the destruction of the wlr_keyboard. It will no longer receive events
         * and should be destroyed.
         */
        struct keyboard *keyboard = wl_container_of (listener, keyboard, destroy);
        wl_list_remove (&keyboard->modifiers.link);
        wl_list_remove (&keyboard->key.link);
        wl_list_remove (&keyboard->destroy.link);
        wl_list_remove (&keyboard->link);
        free (keyboard);
}

void server_new_keyboard (struct comp_server *server, struct wlr_input_device *device) {
        struct wlr_keyboard *wlr_keyboard = wlr_keyboard_from_input_device (device);

        struct keyboard *keyboard = calloc (1, sizeof (*keyboard));
        keyboard->server          = server;
        keyboard->wlr_keyboard    = wlr_keyboard;

        /* We need to prepare an XKB keymap and assign it to the keyboard. This
         * assumes the defaults (e.g. layout = "us"). */
        struct xkb_context *context = xkb_context_new (XKB_CONTEXT_NO_FLAGS);
        struct xkb_keymap  *keymap
            = xkb_keymap_new_from_names (context, NULL, XKB_KEYMAP_COMPILE_NO_FLAGS);

        wlr_keyboard_set_keymap (wlr_keyboard, keymap);
        xkb_keymap_unref (keymap);
        xkb_context_unref (context);
        wlr_keyboard_set_repeat_info (wlr_keyboard, 25, 600);

        /* Here we set up listeners for keyboard events. */
        keyboard->modifiers.notify = keyboard_handle_modifiers;
        wl_signal_add (&wlr_keyboard->events.modifiers, &keyboard->modifiers);
        keyboard->key.notify = keyboard_handle_key;
        wl_signal_add (&wlr_keyboard->events.key, &keyboard->key);
        keyboard->destroy.notify = keyboard_handle_destroy;
        wl_signal_add (&device->events.destroy, &keyboard->destroy);

        wlr_seat_set_keyboard (server->seat, keyboard->wlr_keyboard);

        /* And add the keyboard to our list of keyboards */
        wl_list_insert (&server->keyboards, &keyboard->link);
}