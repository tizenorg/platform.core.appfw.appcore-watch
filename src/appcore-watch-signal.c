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
#include <dbus/dbus.h>
#include <dbus/dbus-glib-lowlevel.h>

#include "appcore-watch-log.h"
#include "appcore-watch-signal.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif

#define LOG_TAG "WATCH_CORE"

#define MAX_BUFFER_SIZE		512

static DBusConnection *bus = NULL;

static int (*_deviced_signal_alpm_handler) (int ambient, void *data);
static void *_deviced_signal_alpm_data;

static DBusHandlerResult __dbus_signal_filter(DBusConnection *conn,
		DBusMessage *message, void *user_data)
{
	const char *sender;
	const char *interface;
	const char *value;

	DBusError error;
	dbus_error_init(&error);

	sender = dbus_message_get_sender(message);
	if (sender == NULL)
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

	interface = dbus_message_get_interface(message);
	if (interface == NULL) {
		_E("reject by security issue - no interface\n");
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	}

	if (dbus_message_is_signal(message, interface, DEVICED_SIGNAL_HOME_SCREEN)) {
		if (dbus_message_get_args(message, &error, DBUS_TYPE_STRING, &value, DBUS_TYPE_INVALID) == FALSE) {
			_E("Failed to get data: %s", error.message);
			dbus_error_free(&error);
		}

		if (_deviced_signal_alpm_handler) {
			if (strcmp(value, CLOCK_START) == 0)
				_deviced_signal_alpm_handler(1, _deviced_signal_alpm_data);
			else if (strcmp(value, CLOCK_STOP) == 0)
				_deviced_signal_alpm_handler(0, _deviced_signal_alpm_data);
		}
	}

	return DBUS_HANDLER_RESULT_HANDLED;
}

static int __dbus_init(void)
{
	if (bus)
		return 0;

	DBusError error;
	dbus_error_init(&error);
	bus = dbus_bus_get_private(DBUS_BUS_SYSTEM, &error);
	if (!bus) {
		_E("Failed to connect to the D-BUS daemon: %s", error.message);
		dbus_error_free(&error);
		return -1;
	}

	dbus_connection_setup_with_g_main(bus, NULL);

	return 0;
}

static int __dbus_signal_handler_init(const char* path, const char* interface)
{
	char rule[MAX_BUFFER_SIZE] = {0,};

	DBusError error;
	dbus_error_init(&error);

	snprintf(rule, MAX_BUFFER_SIZE,
			"path='%s',type='signal',interface='%s'", path, interface);

	dbus_bus_add_match(bus, rule, &error);
	if (dbus_error_is_set(&error)) {
		_E("Fail to rule set: %s", error.message);
		dbus_error_free(&error);
		return -1;
	}

	if (dbus_connection_add_filter(bus, __dbus_signal_filter, NULL, NULL) == FALSE) {
		_E("add filter fail");
		return -1;
	}

	return 0;
}

int _watch_core_listen_alpm_handler(int (*func) (int, void *), void *data)
{
	_D("watch_core_listen_deviced_alpm");

	__dbus_init();

	if (__dbus_signal_handler_init(DEVICED_PATH, DEVICED_INTERFACE) < 0) {
		_E("error app signal init");
		return -1;
	}

	_deviced_signal_alpm_handler = func;
	_deviced_signal_alpm_data = data;

	return 0;
}

int _watch_core_send_alpm_update_done(void)
{
	DBusMessage *message;

	__dbus_init();

	message = dbus_message_new_signal(ALPM_VIEWER_PATH,
			ALPM_VIEWER_INTERFACE, ALPM_VIEWER_SIGNAL_DRAW_DONE);

	if (dbus_message_append_args(message,
				DBUS_TYPE_INVALID) == FALSE) {
		_E("Failed to load data error");
		return -1;
	}

	if (dbus_connection_send(bus, message, NULL) == FALSE) {
		_E("dbus send error");
		return -1;
	}

	dbus_connection_flush(bus);
	dbus_message_unref(message);

	_I("send a alpm update done signal");

	return 0;
}
