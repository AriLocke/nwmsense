#define _GNU_SOURCE
#include "xdg-shell-protocol.h"

#include "comp_output.h"
#include "comp_server.h"
#include "comp_xdg_shell.h"

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

        /* This creates some hands-off wlroots interfaces. The compositor is
         * necessary for clients to allocate surfaces, the subcompositor allows to
         * assign the role of subsurfaces to surfaces and the data device manager
         * handles the clipboard. Each of these wlroots interfaces has room for you
         * to dig your fingers in and play with their behavior if you want. Note that
         * the clients cannot set the selection directly without compositor approval,
         * see the handling of the request_set_selection event below.*/
        wlr_compositor_create (server.wl_display, 5, server.renderer);
        wlr_subcompositor_create (server.wl_display);
        // wlr_data_device_manager_create (server.wl_display);

        server.output_layout = wlr_output_layout_create (server.wl_display);

        server.scene        = wlr_scene_create();
        server.scene_layout = wlr_scene_attach_output_layout (server.scene, server.output_layout);

        wl_list_init (&server.toplevels);
        server.xdg_shell               = wlr_xdg_shell_create (server.wl_display, 3);
        server.new_xdg_toplevel.notify = new_xdg_toplevel_notify;
        server.new_xdg_popup.notify    = new_xdg_popup_notify;
        wl_signal_add (&server.xdg_shell->events.new_toplevel, &server.new_xdg_toplevel);
        wl_signal_add (&server.xdg_shell->events.new_popup, &server.new_xdg_popup);

        // Listening for new backend outputs

        wl_list_init (&server.outputs);
        server.new_output.notify = new_output_notify;
        wl_signal_add (&server.backend->events.new_output, &server.new_output);

        const char *socket = wl_display_add_socket_auto (server.wl_display);
        if (!socket) {
                wlr_backend_destroy (server.backend);
                wlr_log (WLR_ERROR, "Failed to open socket\n");
                return 1;
        }

        if (!wlr_backend_start (server.backend)) {
                wlr_backend_destroy (server.backend);
                wl_display_destroy (server.wl_display);
                wlr_log (WLR_ERROR, "Failed to start backend\n");
                return 1;
        }

        printf ("Running compositor on wayland display '%s'\n", socket);
        setenv ("WAYLAND_DISPLAY", socket, true);

        wl_display_init_shm (server.wl_display);

        wl_display_run (server.wl_display);

        wl_display_destroy_clients (server.wl_display);
        wlr_scene_node_destroy (&server.scene->tree.node);
        // wlr_xcursor_manager_destroy (server.cursor_mgr);
        // wlr_cursor_destroy (server.cursor);
        wlr_allocator_destroy (server.allocator);
        wlr_renderer_destroy (server.renderer);
        wlr_backend_destroy (server.backend);
        wl_display_destroy (server.wl_display);
        wlr_log (WLR_DEBUG, "Pass");
        return 0;
}