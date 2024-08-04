//
// Created by arias on 8/4/24.
//

#include "comp_xdg_shell.h"

#include <assert.h>
#include <stdlib.h>
#include <wlr/types/wlr_scene.h>

/// Toplevel
void new_xdg_toplevel_notify (struct wl_listener *listener, void *data) {
        /* This event is raised when a client creates a new toplevel (application window). */
        struct comp_server      *server = wl_container_of (listener, server, new_xdg_toplevel);
        struct wlr_xdg_toplevel *xdg_toplevel = data;

        /* Allocate a tinywl_toplevel for this surface */
        struct comp_toplevel *toplevel = calloc (1, sizeof (*toplevel));
        toplevel->server               = server;
        toplevel->xdg_toplevel         = xdg_toplevel;
        toplevel->scene_tree
            = wlr_scene_xdg_surface_create (&toplevel->server->scene->tree, xdg_toplevel->base);
        toplevel->scene_tree->node.data = toplevel;
        xdg_toplevel->base->data        = toplevel->scene_tree;

        /* Listen to the various events it can emit */
        toplevel->map.notify     = xdg_toplevel_map_notify;
        toplevel->unmap.notify   = xdg_toplevel_unmap_notify;
        toplevel->commit.notify  = xdg_toplevel_commit_notify;
        toplevel->destroy.notify = xdg_toplevel_destroy_notify;

        wl_signal_add (&xdg_toplevel->base->surface->events.map, &toplevel->map);
        wl_signal_add (&xdg_toplevel->base->surface->events.unmap, &toplevel->unmap);
        wl_signal_add (&xdg_toplevel->base->surface->events.commit, &toplevel->commit);
        wl_signal_add (&xdg_toplevel->events.destroy, &toplevel->destroy);

        /* cotd */
        // toplevel->request_move.notify = xdg_toplevel_request_move;
        // wl_signal_add (&xdg_toplevel->events.request_move, &toplevel->request_move);
        // toplevel->request_resize.notify = xdg_toplevel_request_resize;
        // wl_signal_add (&xdg_toplevel->events.request_resize, &toplevel->request_resize);
        // toplevel->request_maximize.notify = xdg_toplevel_request_maximize;
        // wl_signal_add (&xdg_toplevel->events.request_maximize, &toplevel->request_maximize);
        // toplevel->request_fullscreen.notify = xdg_toplevel_request_fullscreen;
        // wl_signal_add (&xdg_toplevel->events.request_fullscreen, &toplevel->request_fullscreen);
}

void xdg_toplevel_map_notify (struct wl_listener *listener, void *data) {
        /* Called when the surface is mapped, or ready to display on-screen. */
        struct comp_toplevel *toplevel = wl_container_of (listener, toplevel, map);

        wl_list_insert (&toplevel->server->toplevels, &toplevel->link);

        // focus_toplevel (toplevel, toplevel->xdg_toplevel->base->surface);
}

void xdg_toplevel_unmap_notify (struct wl_listener *listener, void *data) {
        /* Called when the surface is unmapped, and should no longer be shown. */
        struct comp_toplevel *toplevel = wl_container_of (listener, toplevel, unmap);

        /* Reset the cursor mode if the grabbed toplevel was unmapped. */
        // if (toplevel == toplevel->server->grabbed_toplevel) {
        //         reset_cursor_mode (toplevel->server);
        // }

        wl_list_remove (&toplevel->link);
}

void xdg_toplevel_commit_notify (struct wl_listener *listener, void *data) {
        /* Called when a new surface state is committed. */
        struct comp_toplevel *toplevel = wl_container_of (listener, toplevel, commit);

        if (toplevel->xdg_toplevel->base->initial_commit) {
                /* When an xdg_surface performs an initial commit, the compositor must
                 * reply with a configure so the client can map the surface. tinywl
                 * configures the xdg_toplevel with 0,0 size to let the client pick the
                 * dimensions itself. */
                wlr_xdg_toplevel_set_size (toplevel->xdg_toplevel, 0, 0);
        }
}

void xdg_toplevel_destroy_notify (struct wl_listener *listener, void *data) {
        /* Called when the xdg_toplevel is destroyed. */
        struct comp_toplevel *toplevel = wl_container_of (listener, toplevel, destroy);

        wl_list_remove (&toplevel->map.link);
        wl_list_remove (&toplevel->unmap.link);
        wl_list_remove (&toplevel->commit.link);
        wl_list_remove (&toplevel->destroy.link);
        wl_list_remove (&toplevel->request_move.link);
        wl_list_remove (&toplevel->request_resize.link);
        wl_list_remove (&toplevel->request_maximize.link);
        wl_list_remove (&toplevel->request_fullscreen.link);

        free (toplevel);
}

/// Popups
void new_xdg_popup_notify (struct wl_listener *listener, void *data) {
        /* This event is raised when a client creates a new popup. */
        struct wlr_xdg_popup *xdg_popup = data;

        struct comp_popup *popup = calloc (1, sizeof (*popup));
        popup->xdg_popup         = xdg_popup;

        /* We must add xdg popups to the scene graph so they get rendered. The
         * wlroots scene graph provides a helper for this, but to use it we must
         * provide the proper parent scene node of the xdg popup. To enable this,
         * we always set the user data field of xdg_surfaces to the corresponding
         * scene node. */
        struct wlr_xdg_surface *parent = wlr_xdg_surface_try_from_wlr_surface (xdg_popup->parent);
        assert (parent != NULL);
        struct wlr_scene_tree *parent_tree = parent->data;
        xdg_popup->base->data = wlr_scene_xdg_surface_create (parent_tree, xdg_popup->base);

        popup->commit.notify = xdg_popup_commit_notify;
        wl_signal_add (&xdg_popup->base->surface->events.commit, &popup->commit);

        popup->destroy.notify = xdg_popup_destroy_notify;
        wl_signal_add (&xdg_popup->events.destroy, &popup->destroy);
}

void xdg_popup_commit_notify (struct wl_listener *listener, void *data) {
        /* Called when a new surface state is committed. */
        struct comp_popup *popup = wl_container_of (listener, popup, commit);

        if (popup->xdg_popup->base->initial_commit) {
                /* When an xdg_surface performs an initial commit, the compositor must
                 * reply with a configure so the client can map the surface.
                 * tinywl sends an empty configure. A more sophisticated compositor
                 * might change an xdg_popup's geometry to ensure it's not positioned
                 * off-screen, for example. */
                wlr_xdg_surface_schedule_configure (popup->xdg_popup->base);
        }
}

void xdg_popup_destroy_notify (struct wl_listener *listener, void *data) {
        /* Called when the xdg_popup is destroyed. */
        struct comp_popup *popup = wl_container_of (listener, popup, destroy);

        wl_list_remove (&popup->commit.link);
        wl_list_remove (&popup->destroy.link);

        free (popup);
}