#ifndef CG_SERVER_H
#define CG_SERVER_H

#include "config.h"

#include <wayland-server.h>
#include <wlr/backend.h>
#include <wlr/types/wlr_idle.h>
#include <wlr/types/wlr_idle_inhibit_v1.h>
#include <wlr/types/wlr_output_layout.h>
#if CAGE_HAS_XWAYLAND
#include <wlr/xwayland.h>
#endif

#include "output.h"
#include "seat.h"
#include "view.h"

struct cg_server {
	struct wl_display *wl_display;
	struct wlr_backend *backend;

	struct wl_listener new_xdg_shell_surface;
	struct wl_list views;

	struct cg_seat *seat;
	struct wlr_idle *idle;
	struct wlr_idle_inhibit_manager_v1 *idle_inhibit_v1;
	struct wl_listener new_idle_inhibitor_v1;
	struct wl_list inhibitors;

	struct wlr_output_layout *output_layout;
	struct cg_output *output;
	struct wl_listener new_output;

#if CAGE_HAS_XWAYLAND
	struct wl_listener new_xwayland_surface;
#endif

#ifdef DEBUG
	bool debug_damage_tracking;
#endif
};

void set_window_title(struct cg_server *server, struct cg_view *view);

#endif
