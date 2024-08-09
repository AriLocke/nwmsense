//
// Created by arias on 8/4/24.
//

#include "xdg_shell.h"

#include "input/cursor.h"
#include "input/keyboard.h"

#include <assert.h>
#include <stdlib.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/util/edges.h>
#include <wlr/util/log.h>

// Interactions
static void xdg_begin_interactive (struct toplevel *toplevel,
                                   enum cursor_mode mode,
                                   uint32_t         edges) {
        /* This function sets up an interactive move or resize operation, where the
         * compositor stops propegating pointer events to clients and instead
         * consumes them itself, to move or resize windows. */
        struct comp_server *server          = toplevel->server;
        struct wlr_surface *focused_surface = server->seat->pointer_state.focused_surface;

        wlr_log (WLR_INFO, "XDG BEGIN INTERACTIVE");

        /* Deny move/resize requests from unfocused clients. */
        if (toplevel->xdg_toplevel->base->surface != wlr_surface_get_root_surface (focused_surface))
                return;

        server->grabbed_toplevel = toplevel;
        server->cursor_mode      = mode;

        switch (mode) {
        case CURSOR_MOVE:
                server->grab_x = server->cursor->x - toplevel->scene_tree->node.x;
                server->grab_y = server->cursor->y - toplevel->scene_tree->node.y;
                break;
        case CURSOR_RESIZE:
                struct wlr_box geo_box;
                wlr_xdg_surface_get_geometry (toplevel->xdg_toplevel->base, &geo_box);

                const double border_x = toplevel->scene_tree->node.x + geo_box.x
                                        + (edges & WLR_EDGE_RIGHT ? geo_box.width : 0);
                const double border_y = toplevel->scene_tree->node.y + geo_box.y
                                        + (edges & WLR_EDGE_BOTTOM ? geo_box.height : 0);
                server->grab_x = server->cursor->x - border_x;
                server->grab_y = server->cursor->y - border_y;

                server->grab_geobox = geo_box;
                server->grab_geobox.x += toplevel->scene_tree->node.x;
                server->grab_geobox.y += toplevel->scene_tree->node.y;

                server->resize_edges = edges;
                break;
        }
}

static void xdg_toplevel_request_move (struct wl_listener *listener, void *data) {
        /* This event is raised when a client would like to begin an interactive
         * move, typically because the user clicked on their client-side
         * decorations. Note that a more sophisticated compositor should check the
         * provided serial against a list of button press serials sent to this
         * client, to prevent the client from requesting this whenever they want. */
        struct toplevel *toplevel = wl_container_of (listener, toplevel, request_move);
        xdg_begin_interactive (toplevel, CURSOR_MOVE, 0);
}

static void xdg_toplevel_request_resize (struct wl_listener *listener, void *data) {
        /* This event is raised when a client would like to begin an interactive
         * resize, typically because the user clicked on their client-side
         * decorations. Note that a more sophisticated compositor should check the
         * provided serial against a list of button press serials sent to this
         * client, to prevent the client from requesting this whenever they want. */
        struct wlr_xdg_toplevel_resize_event *event = data;
        struct toplevel *toplevel = wl_container_of (listener, toplevel, request_resize);
        xdg_begin_interactive (toplevel, CURSOR_RESIZE, event->edges);
}

static void xdg_toplevel_request_maximize (struct wl_listener *listener, void *data) {
        /* This event is raised when a client would like to maximize itself,
         * typically because the user clicked on the maximize button on client-side
         * decorations. tinywl doesn't support maximization, but to conform to
         * xdg-shell protocol we still must send a configure.
         * wlr_xdg_surface_schedule_configure() is used to send an empty reply.
         * However, if the request was sent before an initial commit, we don't do
         * anything and let the client finish the initial surface setup. */
        struct toplevel *toplevel = wl_container_of (listener, toplevel, request_maximize);
        if (toplevel->xdg_toplevel->base->initialized) {
                wlr_xdg_surface_schedule_configure (toplevel->xdg_toplevel->base);
        }
}

static void xdg_toplevel_request_fullscreen (struct wl_listener *listener, void *data) {
        /* Just as with request_maximize, we must send a configure here. */
        struct toplevel *toplevel = wl_container_of (listener, toplevel, request_fullscreen);
        if (toplevel->xdg_toplevel->base->initialized) {
                wlr_xdg_surface_schedule_configure (toplevel->xdg_toplevel->base);
        }
}

struct toplevel *desktop_toplevel_at (struct comp_server  *server,
                                      double               lx,
                                      double               ly,
                                      struct wlr_surface **surface,
                                      double              *sx,
                                      double              *sy) {
        /* This returns the topmost node in the scene at the given layout coords.
         * We only care about surface nodes as we are specifically looking for a
         * surface in the surface tree of a tinywl_toplevel. */
        struct wlr_scene_node *node = wlr_scene_node_at (&server->scene->tree.node, lx, ly, sx, sy);
        if (node == NULL || node->type != WLR_SCENE_NODE_BUFFER) {
                return NULL;
        }
        struct wlr_scene_buffer  *scene_buffer  = wlr_scene_buffer_from_node (node);
        struct wlr_scene_surface *scene_surface = wlr_scene_surface_try_from_buffer (scene_buffer);
        if (!scene_surface) {
                return NULL;
        }

        *surface = scene_surface->surface;
        /* Find the node corresponding to the tinywl_toplevel at the root of this
         * surface tree, it is the only one for which we set the data field. */
        struct wlr_scene_tree *tree = node->parent;
        while (tree != NULL && tree->node.data == NULL) {
                tree = tree->node.parent;
        }
        return tree->node.data;
}

// Toplevel
void new_xdg_toplevel_notify (struct wl_listener *listener, void *data) {
        /* This event is raised when a client creates a new toplevel (application window). */
        struct comp_server      *server = wl_container_of (listener, server, new_xdg_toplevel);
        struct wlr_xdg_toplevel *xdg_toplevel = data;

        /* Allocate a tinywl_toplevel for this surface */
        struct toplevel *toplevel = calloc (1, sizeof (*toplevel));
        toplevel->server          = server;
        toplevel->xdg_toplevel    = xdg_toplevel;
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
        toplevel->request_move.notify       = xdg_toplevel_request_move;
        toplevel->request_resize.notify     = xdg_toplevel_request_resize;
        toplevel->request_maximize.notify   = xdg_toplevel_request_maximize;
        toplevel->request_fullscreen.notify = xdg_toplevel_request_fullscreen;

        wl_signal_add (&xdg_toplevel->events.request_resize, &toplevel->request_resize);
        wl_signal_add (&xdg_toplevel->events.request_move, &toplevel->request_move);
        wl_signal_add (&xdg_toplevel->events.request_maximize, &toplevel->request_maximize);
        wl_signal_add (&xdg_toplevel->events.request_fullscreen, &toplevel->request_fullscreen);
}

void xdg_toplevel_map_notify (struct wl_listener *listener, void *data) {
        /* Called when the surface is mapped, or ready to display on-screen. */
        struct toplevel *toplevel = wl_container_of (listener, toplevel, map);

        wl_list_insert (&toplevel->server->toplevels, &toplevel->link);

        keyboard_focus_toplevel (toplevel, toplevel->xdg_toplevel->base->surface);
}

void xdg_toplevel_unmap_notify (struct wl_listener *listener, void *data) {
        /* Called when the surface is unmapped, and should no longer be shown. */
        struct toplevel *toplevel = wl_container_of (listener, toplevel, unmap);

        /* Reset the cursor mode if the grabbed toplevel was unmapped. */
        if (toplevel == toplevel->server->grabbed_toplevel) {
                reset_cursor_mode (toplevel->server);
        }

        wl_list_remove (&toplevel->link);
}

void xdg_toplevel_commit_notify (struct wl_listener *listener, void *data) {
        /* Called when a new surface state is committed. */
        struct toplevel *toplevel = wl_container_of (listener, toplevel, commit);

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
        struct toplevel *toplevel = wl_container_of (listener, toplevel, destroy);

        wlr_log (WLR_INFO, "Toplevel %s Destroyed", toplevel->xdg_toplevel->app_id);

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

        struct popup *popup = calloc (1, sizeof (*popup));
        popup->xdg_popup    = xdg_popup;

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
        struct popup *popup = wl_container_of (listener, popup, commit);

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
        struct popup *popup = wl_container_of (listener, popup, destroy);

        wl_list_remove (&popup->commit.link);
        wl_list_remove (&popup->destroy.link);

        free (popup);
}