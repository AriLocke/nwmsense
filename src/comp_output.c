//
// Created by arias on 8/3/24.
//
#define _GNU_SOURCE

#include "comp_output.h"

#include <stdlib.h>
#include <wlr/util/log.h>

#include <assert.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <wayland-server-core.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_scene.h>

/** Raised by backend when a new display becomes available */
void new_output_notify (struct wl_listener *listener, void *data) {
        struct comp_server *server     = wl_container_of (listener, server, new_output);
        struct wlr_output  *wlr_output = data;

        wlr_output_init_render (wlr_output, server->allocator, server->renderer);

        /* The output may be disabled, switch it on. */
        struct wlr_output_state state;
        wlr_output_state_init (&state);
        wlr_output_state_set_enabled (&state, true);

        /* Some backends dont have modes. Those that do need them set. */
        struct wlr_output_mode *mode = wlr_output_preferred_mode (wlr_output);
        if (mode != NULL) {
                wlr_output_state_set_mode (&state, mode);
        }

        /* Atomically applies the new output state. */
        wlr_output_commit_state (wlr_output, &state);
        wlr_output_state_finish (&state);

        // ReSharper disable once CppDFAMemoryLeak
        struct comp_output *output = calloc (1, sizeof (struct comp_output));

        clock_gettime (CLOCK_MONOTONIC, &output->last_frame);
        output->server     = server;
        output->wlr_output = wlr_output;
        wl_list_insert (&server->outputs, &output->link);

        output->destroy.notify = output_destroy_notify;
        wl_signal_add (&wlr_output->events.destroy, &output->destroy);

        output->frame.notify = output_frame_notify;
        wl_signal_add (&wlr_output->events.frame, &output->frame);

        /* Adds this to the output layout. The add_auto function arranges outputs
         * from left-to-right in the order they appear. A more sophisticated
         * compositor would let the user configure the arrangement of outputs in the
         * layout.
         *
         * The output layout utility automatically adds a wl_output global to the
         * display, which Wayland clients can see to find out information about the
         * output (such as DPI, scale factor, manufacturer, etc).
         */
        struct wlr_output_layout_output *l_output
            = wlr_output_layout_add_auto (server->output_layout, wlr_output);
        struct wlr_scene_output *scene_output = wlr_scene_output_create (server->scene, wlr_output);
        wlr_scene_output_layout_add_output (server->scene_layout, l_output, scene_output);

        // wlr_output_create_global (wlr_output);
        wlr_log (WLR_INFO, "New Output Added");
}

static void output_frame_notify (struct wl_listener *listener, void *data) {
        struct comp_output *output = wl_container_of (listener, output, frame);
        struct wlr_scene   *scene  = output->server->scene;

        struct wlr_scene_output *scene_output
            = wlr_scene_get_scene_output (scene, output->wlr_output);

        /* Render the scene if needed and commit the output */
        if (scene_output != NULL) // May be unnecessary but may be necessary but may be unnecessary
                wlr_scene_output_commit (scene_output, NULL);

        struct timespec now;
        clock_gettime (CLOCK_MONOTONIC, &now);
        wlr_scene_output_send_frame_done (scene_output, &now);
}

static void output_destroy_notify (struct wl_listener *listener, void *data) {
        struct comp_output *output = wl_container_of (listener, output, destroy);
        wl_list_remove (&output->link);
        wl_list_remove (&output->destroy.link);
        wl_list_remove (&output->frame.link);
        free (output);
        wlr_log (WLR_INFO, "Output Destroyed");
}