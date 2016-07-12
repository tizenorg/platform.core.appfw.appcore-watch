/*
 * Copyright (c) 2015 - 2016 Samsung Electronics Co., Ltd All Rights Reserved
 *
 * Licensed under the Apache License, Version 2.0 (the License);
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define _GNU_SOURCE

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <malloc.h>

#include <dlog.h>
#include <glib.h>
#include <gio/gio.h>

#include "appcore-watch-log.h"
#include "appcore-watch-signal.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif

#define LOG_TAG "WATCH_CORE"

#define MAX_BUFFER_SIZE		512

static GDBusConnection *conn;
static guint s_id;
static int (*_deviced_signal_alpm_handler)(int ambient, void *data);
static void *_deviced_signal_alpm_data;

/* LCOV_EXCL_START */
static void __dbus_signal_filter(GDBusConnection *connection,
		const gchar *sender_name, const gchar *object_name,
		const gchar *interface_name, const gchar *signal_name,
		GVariant *parameters, gpointer user_data)
{
	gchar *value = NULL;

	if (g_strcmp0(signal_name, DEVICED_SIGNAL_HOME_SCREEN) == 0) {
		if (_deviced_signal_alpm_handler) {
			g_variant_get(parameters, "(&s)", &value);
			if (g_strcmp0(value, CLOCK_START) == 0) {
				_deviced_signal_alpm_handler(1,
						_deviced_signal_alpm_data);
			} else if (g_strcmp0(value, CLOCK_STOP) == 0) {
				_deviced_signal_alpm_handler(0,
						_deviced_signal_alpm_data);
			}
		}
	}
}
/* LCOV_EXCL_STOP */

static int __dbus_init(void)
{
	GError *err = NULL;

	if (conn == NULL) {
		conn = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &err);
		if (conn == NULL) {
			_E("g_bus_get_sync() is failed. %s", err->message); /* LCOV_EXCL_LINE */
			g_error_free(err); /* LCOV_EXCL_LINE */
			return -1; /* LCOV_EXCL_LINE */
		}
	}

	g_clear_error(&err);

	return 0;
}

static int __dbus_signal_handler_init(const char *path, const char *interface)
{
	s_id = g_dbus_connection_signal_subscribe(conn,
					NULL,
					interface,
					NULL,
					path,
					NULL,
					G_DBUS_SIGNAL_FLAGS_NONE,
					__dbus_signal_filter,
					NULL,
					NULL);
	if (s_id == 0) {
		_E("g_dbus_connection_signal_subscribe() is failed."); /* LCOV_EXCL_LINE */
		return -1; /* LCOV_EXCL_LINE */
	}

	return 0;
}

int _watch_core_listen_alpm_handler(int (*func) (int, void *), void *data)
{
	_D("watch_core_listen_deviced_alpm");

	if (__dbus_init() < 0)
		return -1;

	if (__dbus_signal_handler_init(DEVICED_PATH, DEVICED_INTERFACE) < 0) {
		_E("error app signal init"); /* LCOV_EXCL_LINE */
		return -1; /* LCOV_EXCL_LINE */
	}

	_deviced_signal_alpm_handler = func;
	_deviced_signal_alpm_data = data;

	return 0;
}

/* LCOV_EXCL_START */
int _watch_core_send_alpm_update_done(void)
{
	GError *err = NULL;

	if (__dbus_init() < 0)
		return -1;

	if (g_dbus_connection_emit_signal(conn,
					NULL,
					ALPM_VIEWER_PATH,
					ALPM_VIEWER_INTERFACE,
					ALPM_VIEWER_SIGNAL_DRAW_DONE,
					NULL,
					&err) == FALSE) {
		_E("g_dbus_connection_emit_signal() is failed. %s",
				err->message);
		return -1;
	}

	if (g_dbus_connection_flush_sync(conn, NULL, &err) == FALSE) {
		_E("g_dbus_connection_flush_sync() is failed. %s",
				err->message);
		return -1;
	}

	g_clear_error(&err);

	_I("send a alpm update done signal");

	return 0;
}
/* LCOV_EXCL_STOP */

