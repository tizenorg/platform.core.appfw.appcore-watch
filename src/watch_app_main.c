/*
 * Copyright (c) 2015 Samsung Electronics Co., Ltd All Rights Reserved
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


#include <stdlib.h>

#include <bundle.h>
#include <aul.h>
#include <dlog.h>
#include <app_common.h>
#include <app_control.h>

#include <Eina.h>
#include <Evas.h>
#include <Elementary.h>

#include "appcore-watch.h"
#include "appcore-watch-log.h"
#include "watch_app_private.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif

#define LOG_TAG "CAPI_WATCH_APPLICATION"

#define WATCH_ID			"internal://WATCH_ID"

typedef enum {
	WATCH_APP_STATE_NOT_RUNNING, // The application has been launched or was running but was terminated
	WATCH_APP_STATE_CREATING, // The application is initializing the resources on watch_app_create_cb callback
	WATCH_APP_STATE_RUNNING, // The application is running in the foreground and background
} watch_app_state_e;

typedef struct {
	char *package;
	char *watch_app_name;
	watch_app_state_e state;
	watch_app_lifecycle_callback_s *callback;
	void *data;
} watch_app_context_s;

typedef watch_app_context_s *watch_app_context_h;

struct _watch_time_s
{
	int year;
	int month;
	int day_of_week;
	int day;
	int hour;
	int hour24;
	int minute;
	int second;
	int millisecond;
	time_t timestamp;
	char *timezone;
};


#define WATCH_APP_EVENT_MAX 5
static Eina_List *handler_list[WATCH_APP_EVENT_MAX] = {NULL, };
static int _initialized = 0;
static int watch_core_initialized = 0;

struct app_event_handler {
	app_event_type_e type;
	app_event_cb cb;
	void *data;
};

struct app_event_info {
	app_event_type_e type;
	void *value;
};

struct watch_app_context {
	char *appid;
	watch_app_state_e state;
	watch_app_lifecycle_callback_s *callback;
	void *data;
};

static void _free_handler_list(void)
{
	int i;
	app_event_handler_h handler;

	for (i = 0; i < WATCH_APP_EVENT_MAX; i++) {
		EINA_LIST_FREE(handler_list[i], handler)
			free(handler);
	}

	eina_shutdown();
}

static int _watch_core_low_memory(void *event_info, void *data)
{
	Eina_List *l;
	app_event_handler_h handler;
	struct app_event_info event;

	_I("watch_app_low_memory");

	event.type = APP_EVENT_LOW_MEMORY;
	event.value = event_info;

	EINA_LIST_FOREACH(handler_list[APP_EVENT_LOW_MEMORY], l, handler) {
		handler->cb(&event, handler->data);
	}

	return APP_ERROR_NONE;
}

static int _watch_core_low_battery(void *event_info, void *data)
{
	Eina_List *l;
	app_event_handler_h handler;
	struct app_event_info event;

	_I("watch_app_low_battery");

	event.type = APP_EVENT_LOW_BATTERY;
	event.value = event_info;

	EINA_LIST_FOREACH(handler_list[APP_EVENT_LOW_BATTERY], l, handler) {
		handler->cb(&event, handler->data);
	}

	return APP_ERROR_NONE;
}

static int _watch_core_lang_changed(void *event_info, void *data)
{
	Eina_List *l;
	app_event_handler_h handler;
	struct app_event_info event;

	_I("_watch_core_lang_changed");

	event.type = APP_EVENT_LANGUAGE_CHANGED;
	event.value = event_info;

	EINA_LIST_FOREACH(handler_list[APP_EVENT_LANGUAGE_CHANGED], l, handler) {
		handler->cb(&event, handler->data);
	}

	return APP_ERROR_NONE;
}

static int _watch_core_region_changed(void *event_info, void *data)
{
	Eina_List *l;
	app_event_handler_h handler;
	struct app_event_info event;

	_I("_ui_app_appcore_region_changed");

	event.type = APP_EVENT_REGION_FORMAT_CHANGED;
	event.value = event_info;

	EINA_LIST_FOREACH(handler_list[APP_EVENT_REGION_FORMAT_CHANGED], l, handler) {
		handler->cb(&event, handler->data);
	}

	return APP_ERROR_NONE;
}

static void _watch_core_set_appcore_event_cb(struct watch_app_context *app_context)
{
	watch_core_set_event_callback(WATCH_CORE_EVENT_LOW_MEMORY, _watch_core_low_memory, app_context);
	watch_core_set_event_callback(WATCH_CORE_EVENT_LANG_CHANGE, _watch_core_lang_changed, app_context);
	watch_core_set_event_callback(WATCH_CORE_EVENT_REGION_CHANGE, _watch_core_region_changed, app_context);

	if (eina_list_count(handler_list[APP_EVENT_LOW_BATTERY]) > 0)
		watch_core_set_event_callback(WATCH_CORE_EVENT_LOW_BATTERY, _watch_core_low_battery, app_context);
}

static void _watch_core_unset_appcore_event_cb(void)
{
	watch_core_set_event_callback(WATCH_CORE_EVENT_LOW_MEMORY, NULL, NULL);
	watch_core_set_event_callback(WATCH_CORE_EVENT_LANG_CHANGE, NULL, NULL);
	watch_core_set_event_callback(WATCH_CORE_EVENT_REGION_CHANGE, NULL, NULL);

	if (eina_list_count(handler_list[APP_EVENT_LOW_BATTERY]) > 0)
		watch_core_set_event_callback(WATCH_CORE_EVENT_LOW_BATTERY, NULL, NULL);
}

static int _watch_core_create(int w, int h, void *data)
{
	_W("_watch_core_create");
	struct watch_app_context *app_context = data;
	watch_app_create_cb create_cb;

	if (app_context == NULL) {
		return watch_app_error(APP_ERROR_INVALID_CONTEXT, __FUNCTION__, NULL);
	}

	watch_core_initialized = 1;
	_watch_core_set_appcore_event_cb(app_context);

	create_cb = app_context->callback->create;

	if (create_cb == NULL || create_cb(w, h, app_context->data) == false) {
		return watch_app_error(APP_ERROR_INVALID_CONTEXT, __FUNCTION__, "watch_app_create_cb() returns false");
	}

	app_context->state = WATCH_APP_STATE_RUNNING;

	return APP_ERROR_NONE;
}

static int _watch_core_control(app_control_h app_control, void *data)
{
	_W("_watch_core_control");
	struct watch_app_context *app_context = data;
	watch_app_control_cb app_control_cb;

	if (app_context == NULL)
		return watch_app_error(APP_ERROR_INVALID_CONTEXT, __FUNCTION__, NULL);

	app_control_cb = app_context->callback->app_control;

	if (app_control_cb != NULL)
		app_control_cb(app_control, app_context->data);

	return APP_ERROR_NONE;
}

static int _watch_core_pause(void *data)
{
	_W("_watch_core_pause");
	struct watch_app_context *app_context = data;
	watch_app_pause_cb pause_cb;

	if (app_context == NULL)
		return watch_app_error(APP_ERROR_INVALID_CONTEXT, __FUNCTION__, NULL);

	pause_cb = app_context->callback->pause;

	if (pause_cb != NULL)
		pause_cb(app_context->data);

	return APP_ERROR_NONE;
}

static int _watch_core_resume(void *data)
{
	_W("_watch_core_resume");
	struct watch_app_context *app_context = data;
	watch_app_resume_cb resume_cb;

	if (app_context == NULL)
		return watch_app_error(APP_ERROR_INVALID_CONTEXT, __FUNCTION__, NULL);

	resume_cb = app_context->callback->resume;

	if (resume_cb != NULL)
		resume_cb(app_context->data);

	return APP_ERROR_NONE;
}

static int _watch_core_terminate(void *data)
{
	_W("_watch_core_terminate");
	struct watch_app_context *app_context = data;
	watch_app_terminate_cb terminate_cb;

	if (app_context == NULL)
		return watch_app_error(APP_ERROR_INVALID_CONTEXT, __FUNCTION__, NULL);

	terminate_cb = app_context->callback->terminate;

	if (terminate_cb != NULL)
		terminate_cb(app_context->data);

	_watch_core_unset_appcore_event_cb();

	if (_initialized)
		_free_handler_list();

	return APP_ERROR_NONE;
}

static void _watch_core_time_tick(void *watchtime, void *data)
{
	_I("_watch_core_time_tick");
	struct watch_app_context *app_context = data;
	watch_app_time_tick_cb time_tick_cb;

	if (app_context == NULL)
		return;

	time_tick_cb = app_context->callback->time_tick;

	if (time_tick_cb != NULL)
		time_tick_cb((watch_time_h)watchtime, app_context->data);
}

static void _watch_core_ambient_tick(void *watchtime, void *data)
{
	_W("_watch_core_ambient_tick");
	struct watch_app_context *app_context = data;
	watch_app_ambient_tick_cb ambient_tick_cb;

	if (app_context == NULL)
		return;

	ambient_tick_cb = app_context->callback->ambient_tick;

	if (ambient_tick_cb != NULL)
		ambient_tick_cb((watch_time_h)watchtime, app_context->data);
}

static void _watch_core_ambient_changed(int ambient, void *data)
{
	_W("_watch_core_ambient_changed: %d", ambient);
	struct watch_app_context *app_context = data;
	watch_app_ambient_changed_cb ambient_changed_cb;

	if (app_context == NULL)
		return;

	ambient_changed_cb = app_context->callback->ambient_changed;

	if (ambient_changed_cb != NULL)
		ambient_changed_cb((bool)ambient, app_context->data);
}


EXPORT_API int watch_app_main(int argc, char **argv, watch_app_lifecycle_callback_s *callback, void *user_data)
{
	struct watch_app_context app_context = {
		.appid = NULL,
		.state = WATCH_APP_STATE_NOT_RUNNING,
		.callback = callback,
		.data = user_data
	};

	struct watchcore_ops watchcore_context = {
		.data = &app_context,
		.create = _watch_core_create,
		.app_control = _watch_core_control,
		.pause = _watch_core_pause,
		.resume = _watch_core_resume,
		.terminate = _watch_core_terminate,
		.time_tick = _watch_core_time_tick,
		.ambient_tick = _watch_core_ambient_tick,
		.ambient_changed = _watch_core_ambient_changed,
	};

	if (argc <= 0 || argv == NULL || callback == NULL)
		return watch_app_error(APP_ERROR_INVALID_PARAMETER, __FUNCTION__, NULL);

	if (callback->create == NULL)
		return watch_app_error(APP_ERROR_INVALID_PARAMETER, __FUNCTION__, "watch_app_create_cb() callback must be registered");

	if (app_context.state != WATCH_APP_STATE_NOT_RUNNING)
		return watch_app_error(APP_ERROR_ALREADY_RUNNING, __FUNCTION__, NULL);

	if (app_get_id(&(app_context.appid)) != APP_ERROR_NONE)
		return watch_app_error(APP_ERROR_INVALID_CONTEXT, __FUNCTION__, "failed to get the appid");

	app_context.state = WATCH_APP_STATE_CREATING;

	watch_core_main(app_context.appid, argc, argv, &watchcore_context);

	free(app_context.appid);

	return APP_ERROR_NONE;
}

EXPORT_API void watch_app_exit(void)
{
	_I("watch_app_exit");
	watch_core_terminate();
}

EXPORT_API int watch_app_add_event_handler(app_event_handler_h *event_handler, app_event_type_e event_type, app_event_cb callback, void *user_data)
{
	app_event_handler_h handler;
	Eina_List *l_itr;

	if (!_initialized) {
		eina_init();
		_initialized = 1;
	}

	if (event_handler == NULL || callback == NULL)
		return watch_app_error(APP_ERROR_INVALID_PARAMETER, __FUNCTION__, NULL);

	if (event_type < APP_EVENT_LOW_MEMORY || event_type > APP_EVENT_REGION_FORMAT_CHANGED)
		return watch_app_error(APP_ERROR_INVALID_PARAMETER, __FUNCTION__, NULL);

	EINA_LIST_FOREACH(handler_list[event_type], l_itr, handler) {
		if (handler->cb == callback)
			return watch_app_error(APP_ERROR_INVALID_PARAMETER, __FUNCTION__, NULL);
	}

	handler = calloc(1, sizeof(struct app_event_handler));
	if (!handler)
		return watch_app_error(APP_ERROR_OUT_OF_MEMORY, __FUNCTION__, NULL);

	handler->type = event_type;
	handler->cb = callback;
	handler->data = user_data;

	if (watch_core_initialized
			&& event_type == APP_EVENT_LOW_BATTERY
			&& eina_list_count(handler_list[event_type]) == 0)
		watch_core_set_event_callback(WATCH_CORE_EVENT_LOW_BATTERY, _watch_core_low_battery, NULL);

	handler_list[event_type] = eina_list_append(handler_list[event_type], handler);

	*event_handler = handler;

	return APP_ERROR_NONE;
}

EXPORT_API int watch_app_remove_event_handler(app_event_handler_h event_handler)
{
	app_event_handler_h handler;
	app_event_type_e type;
	Eina_List *l_itr;
	Eina_List *l_next;

	if (event_handler == NULL)
		return watch_app_error(APP_ERROR_INVALID_PARAMETER, __FUNCTION__, NULL);

	if (!_initialized) {
		_I("handler list is not initialized");
		return APP_ERROR_NONE;
	}

	type = event_handler->type;
	if (type < APP_EVENT_LOW_MEMORY || type > APP_EVENT_REGION_FORMAT_CHANGED)
		return watch_app_error(APP_ERROR_INVALID_PARAMETER, __FUNCTION__, NULL);

	EINA_LIST_FOREACH_SAFE(handler_list[type], l_itr, l_next, handler) {
		if (handler == event_handler) {
			free(handler);
			handler_list[type] = eina_list_remove_list(handler_list[type], l_itr);

			if (watch_core_initialized
					&& type == APP_EVENT_LOW_BATTERY
					&& eina_list_count(handler_list[type]) == 0)
				watch_core_set_event_callback(WATCH_CORE_EVENT_LOW_BATTERY, NULL, NULL);

			return APP_ERROR_NONE;
		}
	}

	return watch_app_error(APP_ERROR_INVALID_PARAMETER, __FUNCTION__, "cannot find such handler");
}

EXPORT_API int watch_time_get_current_time(watch_time_h* watch_time)
{
	if (watch_time == NULL)
		return watch_app_error(APP_ERROR_INVALID_PARAMETER, __FUNCTION__, NULL);

	struct watch_time_s *time_info;

	time_info = calloc(1, sizeof(struct watch_time_s));
	if (time_info == NULL)
		return watch_app_error(APP_ERROR_OUT_OF_MEMORY, __FUNCTION__, "failed to create a handle");

	watch_core_get_timeinfo(time_info);

	*watch_time = (struct _watch_time_s *)time_info;

	return APP_ERROR_NONE;
}

EXPORT_API int watch_time_delete(watch_time_h watch_time)
{
	if (watch_time == NULL)
		return watch_app_error(APP_ERROR_INVALID_PARAMETER, __FUNCTION__, NULL);

	if (watch_time->timezone)
		free(watch_time->timezone);

	free(watch_time);

	return APP_ERROR_NONE;
}

EXPORT_API int watch_time_get_year(watch_time_h watch_time, int *year)
{
	if (watch_time == NULL)
		return watch_app_error(APP_ERROR_INVALID_PARAMETER, __FUNCTION__, NULL);

	*year = watch_time->year;
	return APP_ERROR_NONE;
}

EXPORT_API int watch_time_get_month(watch_time_h watch_time, int *month)
{
	if (watch_time == NULL)
		return watch_app_error(APP_ERROR_INVALID_PARAMETER, __FUNCTION__, NULL);

	*month = watch_time->month;
	return APP_ERROR_NONE;
}

EXPORT_API int watch_time_get_day(watch_time_h watch_time, int *day)
{
	if (watch_time == NULL)
		return watch_app_error(APP_ERROR_INVALID_PARAMETER, __FUNCTION__, NULL);

	*day = watch_time->day;
	return APP_ERROR_NONE;
}

EXPORT_API int watch_time_get_day_of_week(watch_time_h watch_time, int *day_of_week)
{
	if (watch_time == NULL)
		return watch_app_error(APP_ERROR_INVALID_PARAMETER, __FUNCTION__, NULL);

	*day_of_week = watch_time->day_of_week;
	return APP_ERROR_NONE;
}

EXPORT_API int watch_time_get_hour(watch_time_h watch_time, int *hour)
{
	if (watch_time == NULL)
		return watch_app_error(APP_ERROR_INVALID_PARAMETER, __FUNCTION__, NULL);

	*hour = watch_time->hour;
	return APP_ERROR_NONE;
}

EXPORT_API int watch_time_get_hour24(watch_time_h watch_time, int *hour24)
{
	if (watch_time == NULL)
		return watch_app_error(APP_ERROR_INVALID_PARAMETER, __FUNCTION__, NULL);

	*hour24 = watch_time->hour24;
	return APP_ERROR_NONE;
}

EXPORT_API int watch_time_get_minute(watch_time_h watch_time, int *minute)
{
	if (watch_time == NULL)
		return watch_app_error(APP_ERROR_INVALID_PARAMETER, __FUNCTION__, NULL);

	*minute = watch_time->minute;
	return APP_ERROR_NONE;
}

EXPORT_API int watch_time_get_second(watch_time_h watch_time, int *second)
{
	if (watch_time == NULL)
		return watch_app_error(APP_ERROR_INVALID_PARAMETER, __FUNCTION__, NULL);

	*second = watch_time->second;
	return APP_ERROR_NONE;
}

EXPORT_API int watch_time_get_millisecond(watch_time_h watch_time, int *millisecond)
{
	if (watch_time == NULL)
		return watch_app_error(APP_ERROR_INVALID_PARAMETER, __FUNCTION__, NULL);

	*millisecond = watch_time->millisecond;
	return APP_ERROR_NONE;
}

EXPORT_API int watch_time_get_utc_time(watch_time_h watch_time, struct tm *utc_time)
{
	if (watch_time == NULL || utc_time == NULL)
		return watch_app_error(APP_ERROR_INVALID_PARAMETER, __FUNCTION__, NULL);

	time_t timestamp = watch_time->timestamp;

	gmtime_r(&timestamp, utc_time);

	return APP_ERROR_NONE;
}

EXPORT_API int watch_time_get_utc_timestamp(watch_time_h watch_time, time_t *utc_timestamp)
{
	if (watch_time == NULL)
		return watch_app_error(APP_ERROR_INVALID_PARAMETER, __FUNCTION__, NULL);

	*utc_timestamp = watch_time->timestamp;

	return APP_ERROR_NONE;
}

EXPORT_API int watch_time_get_time_zone(watch_time_h watch_time, char **time_zone_id)
{
	if (watch_time == NULL || watch_time->timezone == NULL || time_zone_id == NULL)
		return watch_app_error(APP_ERROR_INVALID_PARAMETER, __FUNCTION__, NULL);

	*time_zone_id = strdup(watch_time->timezone);

	return APP_ERROR_NONE;
}

EXPORT_API int watch_app_get_elm_win(Evas_Object **win)
{
	Evas_Object *ret_win;

	if (win == NULL)
		return watch_app_error(APP_ERROR_INVALID_PARAMETER, __FUNCTION__, NULL);

	ret_win = elm_win_add(NULL, NULL, ELM_WIN_BASIC);
	if (ret_win == NULL)
		return watch_app_error(APP_ERROR_OUT_OF_MEMORY, __FUNCTION__, NULL);

	*win = ret_win;
	return APP_ERROR_NONE;
}

