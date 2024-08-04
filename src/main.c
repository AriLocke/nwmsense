#define _GNU_SOURCE
#include "xdg-shell-protocol.h"

#include "comp_output.h"
#include "comp_server.h"

#include <getopt.h>

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <wayland-server-core.h>
#include <wlr/backend.h>
#include <wlr/render/allocator.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_subcompositor.h>
#include <wlr/util/log.h>

int main (int argc, char *argv[]) {
        wlr_log_init (WLR_DEBUG, NULL);

        struct comp_server server = { 0 };

        server.wl_display = wl_display_create();
        assert (server.wl_display);

        server.wl_event_loop = wl_display_get_event_loop (server.wl_display);
        assert (server.wl_event_loop);

        server.backend = wlr_backend_autocreate (server.wl_event_loop, NULL);
        assert (server.backend);

        server.renderer = wlr_renderer_autocreate (server.backend);
        assert (server.renderer);

        wlr_renderer_init_wl_display (server.renderer, server.wl_display);

        server.allocator = wlr_allocator_autocreate (server.backend, server.renderer);
        assert (server.allocator);

        server.output_layout = wlr_output_layout_create (server.wl_display);
        assert (server.output_layout);

        server.scene = wlr_scene_create();
        assert (server.scene);

        server.scene_layout = wlr_scene_attach_output_layout (server.scene, server.output_layout);
        // Listening for new backend outputs

        wl_list_init (&server.outputs);
        server.new_output.notify = new_output_notify;
        wl_signal_add (&server.backend->events.new_output, &server.new_output);

        const char *socket = wl_display_add_socket_auto (server.wl_display);
        assert (socket);

        if (!wlr_backend_start (server.backend)) {
                wlr_log (WLR_ERROR, "Failed to start backend\n");
                wl_display_destroy (server.wl_display);
                return 1;
        }

        printf ("Running compositor on wayland display '%s'\n", socket);
        setenv ("WAYLAND_DISPLAY", socket, true);

        wl_display_init_shm (server.wl_display);

        wl_display_run (server.wl_display);
        wl_display_destroy (server.wl_display);

        wlr_log (WLR_DEBUG, "Pass");
        return 0;
}