//
// Created by arias on 8/4/24.
//

#ifndef COMP_XDG_SHELL_H
#define COMP_XDG_SHELL_H

#include "comp_server.h"
#include <wlr/types/wlr_xdg_shell.h>

struct comp_toplevel
{
        struct wl_list           link;
        struct comp_server      *server;
        struct wlr_xdg_toplevel *xdg_toplevel;
        struct wlr_scene_tree   *scene_tree;
        struct wl_listener       map;
        struct wl_listener       unmap;
        struct wl_listener       commit;
        struct wl_listener       destroy;
        struct wl_listener       request_move;
        struct wl_listener       request_resize;
        struct wl_listener       request_maximize;
        struct wl_listener       request_fullscreen;
};

struct comp_popup
{
        struct wlr_xdg_popup *xdg_popup;
        struct wl_listener    commit;
        struct wl_listener    destroy;
};

void new_xdg_toplevel_notify (struct wl_listener *listener, void *data);
void xdg_toplevel_map_notify (struct wl_listener *listener, void *data);
void xdg_toplevel_unmap_notify (struct wl_listener *listener, void *data);
void xdg_toplevel_commit_notify (struct wl_listener *listener, void *data);
void xdg_toplevel_destroy_notify (struct wl_listener *listener, void *data);

void new_xdg_popup_notify (struct wl_listener *listener, void *data);
void xdg_popup_commit_notify (struct wl_listener *listener, void *data);
void xdg_popup_destroy_notify (struct wl_listener *listener, void *data);

#endif // COMP_XDG_SHELL_H
