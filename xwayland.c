/*
 * Cage: A Wayland kiosk.
 *
 * Copyright (C) 2018-2019 Jente Hidskes
 *
 * See the LICENSE file accompanying this file.
 */

#include <stdbool.h>
#include <stdlib.h>
#include <wayland-server.h>
#include <wlr/types/wlr_box.h>
#include <wlr/xwayland.h>
#include <wlr/util/log.h>

#include "server.h"
#include "view.h"
#include "xwayland.h"

struct cg_xwayland_view *
xwayland_view_from_view(struct cg_view *view)
{
	return (struct cg_xwayland_view *) view;
}

static char *
get_title(struct cg_view *view)
{
	struct cg_xwayland_view *xwayland_view = xwayland_view_from_view(view);
	return xwayland_view->xwayland_surface->title;
}

static void
get_geometry(struct cg_view *view, int *width_out, int *height_out)
{
	struct cg_xwayland_view *xwayland_view = xwayland_view_from_view(view);
	*width_out = xwayland_view->xwayland_surface->surface->current.width;
	*height_out = xwayland_view->xwayland_surface->surface->current.height;
}

static bool
is_primary(struct cg_view *view)
{
	struct cg_xwayland_view *xwayland_view = xwayland_view_from_view(view);
	struct wlr_xwayland_surface *parent = xwayland_view->xwayland_surface->parent;
	return parent == NULL;
}

static bool
is_parent(struct cg_view *parent, struct cg_view *child)
{
	if (child->type != CAGE_XWAYLAND_VIEW) {
		return false;
	}
	struct cg_xwayland_view *_parent = xwayland_view_from_view(parent);
	struct cg_xwayland_view *_child = xwayland_view_from_view(child);
	return _child->xwayland_surface->parent == _parent->xwayland_surface;
}

static void
activate(struct cg_view *view, bool activate)
{
	struct cg_xwayland_view *xwayland_view = xwayland_view_from_view(view);
	wlr_xwayland_surface_activate(xwayland_view->xwayland_surface, activate);
}

static void
maximize(struct cg_view *view, int output_width, int output_height)
{
	struct cg_xwayland_view *xwayland_view = xwayland_view_from_view(view);
	wlr_xwayland_surface_configure(xwayland_view->xwayland_surface, 0, 0, output_width, output_height);
	wlr_xwayland_surface_set_maximized(xwayland_view->xwayland_surface, true);
}

static void
destroy(struct cg_view *view)
{
	struct cg_xwayland_view *xwayland_view = xwayland_view_from_view(view);
	free(xwayland_view);
}

static void
for_each_surface(struct cg_view *view, wlr_surface_iterator_func_t iterator, void *data)
{
	wlr_surface_for_each_surface(view->wlr_surface, iterator, data);
}

static struct wlr_surface *
wlr_surface_at(struct cg_view *view, double sx, double sy, double *sub_x, double *sub_y)
{
	return wlr_surface_surface_at(view->wlr_surface, sx, sy, sub_x, sub_y);
}

static void
handle_xwayland_surface_unmap(struct wl_listener *listener, void *data)
{
	struct cg_xwayland_view *xwayland_view = wl_container_of(listener, xwayland_view, unmap);
	struct cg_view *view = &xwayland_view->view;

	view_unmap(view);
}

static void
handle_xwayland_surface_map(struct wl_listener *listener, void *data)
{
	struct cg_xwayland_view *xwayland_view = wl_container_of(listener, xwayland_view, map);
	struct cg_view *view = &xwayland_view->view;

	xwayland_view->ever_been_mapped = true;
	view_map(view, xwayland_view->xwayland_surface->surface);
}

static void
handle_xwayland_surface_destroy(struct wl_listener *listener, void *data)
{
	struct cg_xwayland_view *xwayland_view = wl_container_of(listener, xwayland_view, destroy);
	struct cg_view *view = &xwayland_view->view;

	wl_list_remove(&xwayland_view->map.link);
	wl_list_remove(&xwayland_view->unmap.link);
	wl_list_remove(&xwayland_view->destroy.link);
	xwayland_view->xwayland_surface = NULL;

	view_destroy(view);
}

static const struct cg_view_impl xwayland_view_impl = {
	.get_title = get_title,
	.get_geometry = get_geometry,
	.is_primary = is_primary,
	.is_parent = is_parent,
	.activate = activate,
	.maximize = maximize,
	.destroy = destroy,
	.for_each_surface = for_each_surface,
	.wlr_surface_at = wlr_surface_at,
};

void
handle_xwayland_surface_new(struct wl_listener *listener, void *data)
{
	struct cg_server *server = wl_container_of(listener, server, new_xwayland_surface);
	struct wlr_xwayland_surface *xwayland_surface = data;

	struct cg_xwayland_view *xwayland_view = calloc(1, sizeof(struct cg_xwayland_view));
	if (!xwayland_view) {
		wlr_log(WLR_ERROR, "Failed to allocate XWayland view");
		return;
	}

	view_init(&xwayland_view->view, server, CAGE_XWAYLAND_VIEW, &xwayland_view_impl);
	xwayland_view->xwayland_surface = xwayland_surface;

	xwayland_view->map.notify = handle_xwayland_surface_map;
	wl_signal_add(&xwayland_surface->events.map, &xwayland_view->map);
	xwayland_view->unmap.notify = handle_xwayland_surface_unmap;
	wl_signal_add(&xwayland_surface->events.unmap, &xwayland_view->unmap);
	xwayland_view->destroy.notify = handle_xwayland_surface_destroy;
	wl_signal_add(&xwayland_surface->events.destroy, &xwayland_view->destroy);
}
