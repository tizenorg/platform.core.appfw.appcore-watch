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


#define _GNU_SOURCE

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <malloc.h>
#include <assert.h>

#include <Elementary.h>

#include <app_control.h>
#include <app_control_internal.h>

#include <aul.h>
#include <dlog.h>
#include <vconf.h>
#include <alarm.h>
#include <glib-object.h>


#include <widget_app.h>

#include "appcore-watch.h"
#include "appcore-watch-log.h"
#include "appcore-watch-signal.h"
#include "appcore-watch-internal.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif

#define LOG_TAG "WATCH_CORE"

#include <unicode/utypes.h>
#include <unicode/putil.h>
#include <unicode/uiter.h>
#include <unicode/udat.h>
#include <unicode/udatpg.h>
#include <unicode/ustring.h>

#define WATCH_ID      			"internal://WATCH_ID"

#define AUL_K_CALLER_APPID      "__AUL_CALLER_APPID__"
#define APPID_BUFFER_MAX		255

#define TIMEZONE_BUFFER_MAX		1024
#define LOCAL_TIME_PATH			"/opt/etc/localtime"

#define ONE_SECOND  1000
#define ONE_MINUTE  60
#define ONE_HOUR    60

static Ecore_Timer *watch_tick = NULL;

static alarm_id_t alarm_id = 0;

/**
 * Appcore internal system event
 */
enum sys_event {
	SE_UNKNOWN,
	SE_LOWMEM,
	SE_LOWBAT,
	SE_LANGCHG,
	SE_REGIONCHG,
	SE_TIMECHG,
	SE_MAX
};

/**
 * watch internal state
 */
enum watch_state {
	WS_NONE,
	WS_CREATED,
	WS_RUNNING,
	WS_PAUSED,
	WS_DYING,
};

enum watch_event {
	WE_UNKNOWN,
	WE_CREATE,
	WE_PAUSE,
	WE_RESUME,
	WE_APPCONTROL,
	WE_AMBIENT,
	WE_TERMINATE,
	WE_MAX
};


static enum watch_core_event to_ae[SE_MAX] = {
	WATCH_CORE_EVENT_UNKNOWN,	/* SE_UNKNOWN */
	WATCH_CORE_EVENT_LOW_MEMORY,	/* SE_LOWMEM */
	WATCH_CORE_EVENT_LOW_BATTERY,	/* SE_LOWBAT */
	WATCH_CORE_EVENT_LANG_CHANGE,
	WATCH_CORE_EVENT_REGION_CHANGE,
};

static int watch_core_event_initialized[SE_MAX] = {0};

enum cb_type {			/* callback */
	_CB_NONE,
	_CB_SYSNOTI,
	_CB_APPNOTI,
	_CB_VCONF,
};

struct evt_ops {
	enum cb_type type;
	union {
		enum watch_core_event sys;
		enum watch_event app;
		const char *vkey;
	} key;

	int (*cb_pre) (void *);
	int (*cb) (void *);
	int (*cb_post) (void *);

	int (*vcb_pre) (void *, void *);
	int (*vcb) (void *, void *);
	int (*vcb_post) (void *, void *);
};

struct watch_priv {
	const char *appid;
	const char *name;
	pid_t pid;
	enum watch_state state;
	int ambient_mode;
	int ambient_mode_skip_resume;

	struct watchcore_ops *ops;
};

static struct watch_priv priv;

struct watch_ops {
	void *data;
	void (*cb_app)(enum watch_event, void *, bundle *);
};

/**
 * Appcore system event operation
 */
struct sys_op {
	int (*func) (void *, void *);
	void *data;
};

struct watch_core {
	int init;

	const struct watch_ops *ops;
	struct sys_op sops[SE_MAX];
};

static struct watch_core core;

static int __sys_lowmem_post(void *data, void *evt);
static int __sys_lowmem(void *data, void *evt);
static int __sys_lowbatt(void *data, void *evt);
static int __sys_langchg_pre(void *data, void *evt);
static int __sys_langchg(void *data, void *evt);
static int __sys_regionchg_pre(void *data, void *evt);
static int __sys_regionchg(void *data, void *evt);


static struct evt_ops evtops[] = {
	{
		.type = _CB_VCONF,
		.key.vkey = VCONFKEY_SYSMAN_LOW_MEMORY,
		.vcb_post = __sys_lowmem_post,
		.vcb = __sys_lowmem,
	},
	{
		.type = _CB_VCONF,
		.key.vkey = VCONFKEY_SYSMAN_BATTERY_STATUS_LOW,
		.vcb = __sys_lowbatt,
	},
	{
		.type = _CB_VCONF,
		.key.vkey = VCONFKEY_LANGSET,
		.vcb_pre = __sys_langchg_pre,
		.vcb = __sys_langchg,
	},
	{
		.type = _CB_VCONF,
		.key.vkey = VCONFKEY_REGIONFORMAT,
		.vcb_pre = __sys_regionchg_pre,
		.vcb = __sys_regionchg,
	},
};

static void __watch_core_signal_init(void);
static int  __watch_core_widget_init(void);

static Eina_Bool __watch_core_time_tick(void *data);
static void __watch_core_time_tick_cancel(void);

static int __watch_core_ambient_tick(alarm_id_t id, void *data);

static int __widget_create(const char *id, const char *content, int w, int h, void *data);
static int __widget_resize(const char *id, int w, int h, void *data);
static int __widget_destroy(const char *id, widget_app_destroy_type_e reason, void *data);
static int __widget_pause(const char *id, void *data);
static int __widget_resume(const char *id, void *data);

extern int app_control_create_event(bundle *data, struct app_control_s **app_control);

static void __exit_loop(void *data)
{
	ecore_main_loop_quit();
}

static void __do_app(enum watch_event event, void *data, bundle * b)
{
	struct watch_priv *watch_data = data;
	app_control_h app_control = NULL;

	_ret_if(watch_data == NULL);

	if (event == WE_TERMINATE)
	{
		watch_data->state = WS_DYING;
		ecore_main_loop_thread_safe_call_sync((Ecore_Data_Cb)__exit_loop, NULL);
		return;
	}

	_ret_if(watch_data->ops == NULL);

	switch (event)
	{
		case WE_APPCONTROL:
			_D("appcontrol request");

			if (app_control_create_event(b, &app_control) != 0)
			{
				_E("failed to get the app_control handle");
				return;
			}

			if (watch_data->ops->app_control)
				watch_data->ops->app_control(app_control, watch_data->ops->data);

			app_control_destroy(app_control);

			break;

		case WE_PAUSE:
			_D("WE_PAUSE");

			if (watch_data->state == WS_CREATED)
			{
				_E("Invalid state");

				return;
			}

			// Handling the ambient mode
			if (watch_data->ambient_mode)
			{
				watch_data->ambient_mode_skip_resume = 1;
			}

			// Cancel the time_tick callback
			__watch_core_time_tick_cancel();

			if (watch_data->state == WS_RUNNING)
			{

				if (watch_data->ops->pause) {
					int r = priv.ops->pause(priv.ops->data);
					if (r < 0) {
						_E("pause() fails");
					}
				}
			}

			watch_data->state = WS_PAUSED;
			break;

		case WE_RESUME:
			_D("WE_RESUME");

			// Handling the ambient mode
			if (watch_data->ambient_mode)
			{
				watch_data->ambient_mode_skip_resume = 0;
				return;
			}

			if (watch_data->state == WS_PAUSED || watch_data->state == WS_CREATED)
			{
				if (watch_data->ops->resume) {
					int r = priv.ops->resume(priv.ops->data);
					if (r < 0) {
						_E("resume() fails");
					}
				}
			}

			watch_data->state = WS_RUNNING;

			// Set the time tick callback
			if (!watch_tick)
			{
				__watch_core_time_tick(NULL);
			}
			else
			{
				__watch_core_time_tick_cancel();
				__watch_core_time_tick(NULL);
			}

			break;

		case WE_AMBIENT:
			_D("WE_AMBIENT");

			if (priv.ops && priv.ops->ambient_changed) {
				priv.ops->ambient_changed(watch_data->ambient_mode, priv.ops->data);
			}

			break;

		default:
			break;
	}

}

static struct watch_ops w_ops = {
	.data = &priv,
	.cb_app = __do_app,
};

char* __get_domain_name(const char *appid)
{
	char *name_token = NULL;

	if (appid == NULL)
	{
		_E("appid is NULL");
		return NULL;
	}

	// com.vendor.name -> name
	name_token = strrchr(appid, '.');

	if (name_token == NULL)
	{
		_E("appid is invalid");
		return NULL;
	}

	name_token++;

	return strdup(name_token);
}

static int __set_data(struct watch_priv *watch, const char *appid, struct watchcore_ops *ops)
{
	if (ops == NULL) {
		errno = EINVAL;
		return -1;
	}

	watch->ops = ops;

	watch->appid = strdup(appid);
	watch->name = __get_domain_name(appid);
	watch->pid = getpid();

	return 0;
}

static int __watch_appcontrol(void *data, bundle * k)
{
	struct watch_core *wc = data;
	_retv_if(wc == NULL || wc->ops == NULL, -1);
	_retv_if(wc->ops->cb_app == NULL, 0);

	wc->ops->cb_app(WE_APPCONTROL, wc->ops->data, k);

	return 0;
}

static int __watch_terminate(void *data)
{
	struct watch_core *wc = data;

	_retv_if(wc == NULL || wc->ops == NULL, -1);
	_retv_if(wc->ops->cb_app == NULL, 0);

	wc->ops->cb_app(WE_TERMINATE, wc->ops->data, NULL);

	return 0;
}

static int __sys_do_default(struct watch_core *wc, enum sys_event event)
{
	int r;

	switch (event) {
	case SE_LOWBAT:
		/*r = __def_lowbatt(wc);*/
		r = 0;
		break;
	default:
		r = 0;
		break;
	};

	return r;
}

static int __sys_do(struct watch_core *wc, void *event_info, enum sys_event event)
{
	struct sys_op *op;

	_retv_if(wc == NULL || event >= SE_MAX, -1);

	op = &wc->sops[event];

	if (op->func == NULL)
		return __sys_do_default(wc, event);

	return op->func(event_info, op->data);
}

static int __sys_lowmem_post(void *data, void *evt)
{
	keynode_t *key = evt;
	int val;

	val = vconf_keynode_get_int(key);

	if (val == VCONFKEY_SYSMAN_LOW_MEMORY_SOFT_WARNING) {
#if defined(MEMORY_FLUSH_ACTIVATE)
		struct appcore *wc = data;
		wc->ops->cb_app(AE_LOWMEM_POST, wc->ops->data, NULL);
#else
		malloc_trim(0);
#endif
	}
	return 0;
}

static int __sys_lowmem(void *data, void *evt)
{
	keynode_t *key = evt;
	int val;

	val = vconf_keynode_get_int(key);

	if (val == VCONFKEY_SYSMAN_LOW_MEMORY_SOFT_WARNING)
		return __sys_do(data, (void *)&val, SE_LOWMEM);

	return 0;
}

static int __sys_lowbatt(void *data, void *evt)
{
	keynode_t *key = evt;
	int val;

	val = vconf_keynode_get_int(key);

	/* VCONFKEY_SYSMAN_BAT_CRITICAL_LOW or VCONFKEY_SYSMAN_POWER_OFF */
	if (val <= VCONFKEY_SYSMAN_BAT_CRITICAL_LOW)
		return __sys_do(data, (void *)&val, SE_LOWBAT);

	return 0;
}

static int __sys_langchg_pre(void *data, void *evt)
{
	_update_lang();
	return 0;
}

static int __sys_langchg(void *data, void *evt)
{
	keynode_t *key = evt;
	char *val;

	val = vconf_keynode_get_str(key);

	return __sys_do(data, (void *)val, SE_LANGCHG);
}

static int __sys_regionchg_pre(void *data, void *evt)
{
	_update_region();
	return 0;
}

static int __sys_regionchg(void *data, void *evt)
{
	keynode_t *key = evt;
	char *val;

	val = vconf_keynode_get_str(key);

	return __sys_do(data, (void *)val, SE_REGIONCHG);
}

static void __vconf_do(struct evt_ops *eo, keynode_t * key, void *data)
{
	_ret_if(eo == NULL);

	if (eo->vcb_pre)
		eo->vcb_pre(data, key);

	if (eo->vcb)
		eo->vcb(data, key);

	if (eo->vcb_post)
		eo->vcb_post(data, key);
}

static void __vconf_cb(keynode_t *key, void *data)
{
	int i;
	const char *name;

	name = vconf_keynode_get_name(key);
	_ret_if(name == NULL);

	_D("vconf changed: %s", name);

	// Check the time changed event
	if (!strcmp(name, VCONFKEY_SYSTEM_TIME_CHANGED))
	{
		struct watch_priv *watch_data = data;

		_D("ambient_mode: %d", watch_data->ambient_mode);

		if (watch_data->ambient_mode)
		{
			__watch_core_ambient_tick(0, NULL);
		}

		return;
	}

	for (i = 0; i < sizeof(evtops) / sizeof(evtops[0]); i++) {
		struct evt_ops *eo = &evtops[i];

		switch (eo->type) {
		case _CB_VCONF:
			if (!strcmp(name, eo->key.vkey))
				__vconf_do(eo, key, data);
			break;
		default:
			break;
		}
	}
}

static int __add_vconf(struct watch_core *wc, enum sys_event se)
{
	int r;

	switch (se) {
	case SE_LOWMEM:
		r = vconf_notify_key_changed(VCONFKEY_SYSMAN_LOW_MEMORY, __vconf_cb, wc);
		break;
	case SE_LOWBAT:
		r = vconf_notify_key_changed(VCONFKEY_SYSMAN_BATTERY_STATUS_LOW, __vconf_cb, wc);
		break;
	case SE_LANGCHG:
		r = vconf_notify_key_changed(VCONFKEY_LANGSET, __vconf_cb, wc);
		break;
	case SE_REGIONCHG:
		r = vconf_notify_key_changed(VCONFKEY_REGIONFORMAT, __vconf_cb, wc);
		break;
	case SE_TIMECHG:
		r = vconf_notify_key_changed(VCONFKEY_SYSTEM_TIME_CHANGED, __vconf_cb, &priv);
		break;
	default:
		r = -1;
		break;
	}

	return r;
}

static int __del_vconf(enum sys_event se)
{
	int r;

	switch (se) {
	case SE_LOWMEM:
		r = vconf_ignore_key_changed(VCONFKEY_SYSMAN_LOW_MEMORY, __vconf_cb);
		break;
	case SE_LOWBAT:
		r = vconf_ignore_key_changed(VCONFKEY_SYSMAN_BATTERY_STATUS_LOW, __vconf_cb);
		break;
	case SE_LANGCHG:
		r = vconf_ignore_key_changed(VCONFKEY_LANGSET, __vconf_cb);
		break;
	case SE_REGIONCHG:
		r = vconf_ignore_key_changed(VCONFKEY_REGIONFORMAT, __vconf_cb);
		break;
	case SE_TIMECHG:
		r = vconf_ignore_key_changed(VCONFKEY_SYSTEM_TIME_CHANGED, __vconf_cb);
		break;
	default:
		r = -1;
		break;
	}

	return r;
}

static int __del_vconf_list(void)
{
	int r;
	enum sys_event se;

	for (se = SE_LOWMEM; se < SE_MAX; se++) {
		if (watch_core_event_initialized[se]) {
			r = __del_vconf(se);
			if (r < 0)
				_E("Delete vconf callback failed");
			else
				watch_core_event_initialized[se] = 0;
		}
	}

	return 0;
}

static int __aul_handler(aul_type type, bundle *b, void *data)
{
	int ret;

	switch (type) {
	case AUL_START:
		ret = __watch_appcontrol(data, b);
		break;
	case AUL_RESUME:
		break;
	case AUL_TERMINATE:
		ret = __watch_terminate(data);
		break;
	default:
		break;
	}

	return 0;
}

EXPORT_API int watch_core_set_event_callback(enum watch_core_event event,
					  int (*cb) (void *, void *), void *data)
{
	struct watch_core *wc = &core;
	struct sys_op *op;
	enum sys_event se;
	int r = 0;

	for (se = SE_UNKNOWN; se < SE_MAX; se++) {
		if (event == to_ae[se])
			break;
	}

	if (se == SE_UNKNOWN || se >= SE_MAX) {
		_E("Unregistered event");
		errno = EINVAL;
		return -1;
	}

	op = &wc->sops[se];

	op->func = cb;
	op->data = data;

	if (op->func && !watch_core_event_initialized[se]) {
		r = __add_vconf(wc, se);
		if (r < 0)
			_E("Add vconf callback failed");
		else
			watch_core_event_initialized[se] = 1;
	} else if (!op->func && watch_core_event_initialized[se]) {
		r = __del_vconf(se);
		if (r < 0)
			_E("Delete vconf callback failed");
		else
			watch_core_event_initialized[se] = 0;
	}

	return 0;
}

static char* __get_timezone(void)
{
	char buf[TIMEZONE_BUFFER_MAX] = {0,};

	ssize_t len = readlink(LOCAL_TIME_PATH, buf, sizeof(buf)-1);

	if (len != -1)
	{
		buf[len] = '\0';
	}
	else  // handle error condition
	{
		return vconf_get_str(VCONFKEY_SETAPPL_TIMEZONE_ID);
	}

	return strdup(buf+20);
}

static void __get_timeinfo(struct watch_time_s *timeinfo)
{
	UErrorCode status = U_ZERO_ERROR;

	// UTC time
	struct timeval current;
	gettimeofday(&current, NULL);
	timeinfo->timestamp = current.tv_sec;

	// Time zone
	char* timezone = __get_timezone();
	UChar utf16_timezone[64] = {0};
	u_uastrncpy(utf16_timezone, timezone, 64);

	// Local time
	UCalendar *cal = ucal_open(utf16_timezone, u_strlen(utf16_timezone), uloc_getDefault(), UCAL_TRADITIONAL, &status);
	if (cal == NULL)
		return;

	timeinfo->year = ucal_get(cal, UCAL_YEAR, &status);
	timeinfo->month = ucal_get(cal, UCAL_MONTH, &status) + 1;
	timeinfo->day = ucal_get(cal, UCAL_DATE, &status);
	timeinfo->day_of_week = ucal_get(cal, UCAL_DAY_OF_WEEK, &status);
	timeinfo->hour = ucal_get(cal, UCAL_HOUR_OF_DAY, &status);
	timeinfo->hour24 = ucal_get(cal, UCAL_HOUR_OF_DAY, &status);
	timeinfo->minute = ucal_get(cal, UCAL_MINUTE, &status);
	timeinfo->second = ucal_get(cal, UCAL_SECOND, &status);
	timeinfo->millisecond = ucal_get(cal, UCAL_MILLISECOND, &status);
	timeinfo->timezone = timezone;

	timeinfo->hour -= (timeinfo->hour > 12) ? 12 : 0;
	timeinfo->hour = (timeinfo->hour == 0) ? 12 : timeinfo->hour;

	ucal_clear(cal);
	ucal_close(cal);
}

int watch_core_init(const char *name, const struct watch_ops *ops,
			    	int argc, char **argv)
{
	int r;

	if (core.init != 0) {
		errno = EALREADY;
		return -1;
	}

	if (ops == NULL || ops->cb_app == NULL) {
		errno = EINVAL;
		return -1;
	}

	r = _set_i18n(name);
	_retv_if(r == -1, -1);

	r = __add_vconf(&core, SE_TIMECHG);
	if (r < 0) {
		_E("Add vconf callback failed");
		goto err;
	}

	r = aul_launch_init(__aul_handler, &core);
	if (r < 0) {
		_E("Fail to call the aul_launch_init");
		assert(0);
		goto err;
	}

	r = aul_launch_argv_handler(argc, argv);
	if (r < 0) {
		_E("Fail to call the aul_launch_argv_handler");
		goto err;
	}

	core.ops = ops;
	core.init = 1;

	return 0;

 err:
	return -1;
}

static void __watch_core_alarm_init(void)
{
	int r = 0;
	int pid = getpid();
	char appid[APPID_BUFFER_MAX] = {0,};
	r = aul_app_get_appid_bypid(pid, appid, APPID_BUFFER_MAX);
	if (r < 0) {
		_E("fail to get the appid from the pid : %d", pid);
		assert(0);
	}

	r = alarmmgr_init(appid);
	if (r < 0) {
		_E("fail to alarmmgr_init : error_code : %d",r);
		assert(0);
	}

}

static void __watch_core_time_tick_cancel(void)
{
	if (watch_tick)
	{
		ecore_timer_del(watch_tick);
		watch_tick = NULL;
	}
}

static Eina_Bool __watch_core_time_tick(void *data)
{
	_D("state: %d", priv.state);

	if (priv.ops && priv.ops->time_tick && priv.state != WS_PAUSED)
	{
		struct watch_time_s timeinfo;
		__get_timeinfo(&timeinfo);

		// Set a next timer
		double sec = (ONE_SECOND - timeinfo.millisecond) / 1000.0;
		watch_tick = ecore_timer_add(sec, __watch_core_time_tick, NULL);

		_D("next time tick: %f", sec);

		priv.ops->time_tick(&timeinfo, priv.ops->data);
	}

	return ECORE_CALLBACK_CANCEL;
}

static int __watch_core_ambient_tick(alarm_id_t id, void *data)
{
	_D("state: %d", priv.state);

	if (priv.ops && priv.ops->ambient_tick && priv.state != WS_RUNNING)
	{
		struct watch_time_s timeinfo;
		__get_timeinfo(&timeinfo);

		priv.ops->ambient_tick(&timeinfo, priv.ops->data);
	}

	return 0;
}

static int __widget_create(const char *id, const char *content, int w, int h, void *data)
{
	_D("widget_create");

	if (priv.ops && priv.ops->create)
	{
		int r = priv.ops->create(w, h, priv.ops->data);
		if (r < 0)
		{
			_E("Failed to initialize the application");
			__exit_loop(NULL);
		}
	}

	priv.state = WS_CREATED;

	_D("widget_create done");

	// Alarm init
	__watch_core_alarm_init();

	return WIDGET_ERROR_NONE;
}

static int __widget_resize(const char *id, int w, int h, void *data)
{

	return WIDGET_ERROR_NONE;
}

static int __widget_destroy(const char *id, widget_app_destroy_type_e reason, void *data)
{
	_D("widget_destory");

	__exit_loop(NULL);

	return WIDGET_ERROR_NONE;
}

static int __widget_pause(const char *id, void *data)
{
	_D("widget_pause");

	__do_app(WE_PAUSE, &priv, NULL);

	return WIDGET_ERROR_NONE;
}

static int __widget_resume(const char *id, void *data)
{
	_D("widget_resume");

	__do_app(WE_RESUME, &priv, NULL);

	return WIDGET_ERROR_NONE;
}

static int __signal_alpm_handler(int ambient, void *data)
{
	_D("_signal_alpm_handler: ambient: %d, state: %d", ambient, priv.state);

	if (priv.ambient_mode == ambient)
	{
		_E("invalid state");
		return 0;
	}

	// Enter the ambient mode
	if (ambient)
	{
		if (priv.state != WS_PAUSED)
		{
			__do_app(WE_PAUSE, &priv, NULL);
			priv.ambient_mode_skip_resume = 0;
		}
		else
		{
			priv.ambient_mode_skip_resume = 1;
		}

		priv.ambient_mode = 1;
		__do_app(WE_AMBIENT, &priv, NULL);

		if (priv.ops && priv.ops->ambient_tick) {
			struct watch_time_s timeinfo;
			__get_timeinfo(&timeinfo);

			int sec = ONE_MINUTE - timeinfo.second;

			_D("next time tick: %d", sec);

			// Set a next alarm
			int r = alarmmgr_add_alarm_withcb(ALARM_TYPE_VOLATILE, sec, 60,
					__watch_core_ambient_tick, data, &alarm_id);
			if (r < 0) {
				_E("fail to alarmmgr_add_alarm_withcb : error_code : %d",r);
			}

			priv.ops->ambient_tick(&timeinfo, priv.ops->data);
		}

		// Send a update done signal
		_watch_core_send_alpm_update_done();

	}
	// Exit the ambient mode
	else
	{
		priv.ambient_mode = 0;
		__do_app(WE_AMBIENT, &priv, NULL);

		_D("Resume check: %d", priv.ambient_mode_skip_resume);
		if (!priv.ambient_mode_skip_resume)
		{
			_D("Call the resume after ambient mode changed");
			__do_app(WE_RESUME, &priv, NULL);
		}

		// Disable alarm
		if (alarm_id)
		{
			alarmmgr_remove_alarm(alarm_id);
			alarm_id = 0;
		}
	}

	return 0;
}

static int  __watch_core_widget_init(void)
{
	_D("Initialize the widget");

	return 0;

}

static void __watch_core_signal_init(void)
{
	_watch_core_listen_alpm_handler(__signal_alpm_handler, NULL);
}

static int __before_loop(struct watch_priv *watch, int argc, char **argv)
{
	int r = 0;

	if (argc <= 0 || argv == NULL) {
		errno = EINVAL;
		_E("argument are invalid");
		return -1;
	}

	priv.ambient_mode = 0;
	priv.ambient_mode_skip_resume = 0;

#if !(GLIB_CHECK_VERSION(2, 36, 0))
	g_type_init();
#endif

	elm_init(argc, argv);

	r = watch_core_init(watch->name, &w_ops, argc, argv);
	_retv_if(r < 0, -1);

	r = __watch_core_widget_init();
	_retv_if(r < 0, r);

	__watch_core_signal_init();

	return 0;
}

static void __after_loop(struct watch_priv *watch)
{
	if (priv.state == WS_RUNNING && watch->ops && watch->ops->pause)
		watch->ops->pause(watch->ops->data);

	priv.state = WS_DYING;

	/* Cancel the time_tick callback */
	__watch_core_time_tick_cancel();

	if (watch->ops && watch->ops->terminate)
		watch->ops->terminate(watch->ops->data);

	elm_shutdown();

	alarmmgr_fini();
}


EXPORT_API int watch_core_main(const char *appid, int argc, char **argv,
				struct watchcore_ops *ops)
{
	int r;

	r = __set_data(&priv, appid, ops);
	_retv_if(r == -1, -1);

	r = __before_loop(&priv, argc, argv);
	if (r != 0) {
		__del_vconf_list();
		return r;
	}

	ecore_main_loop_begin();

	aul_status_update(STATUS_DYING);

	__after_loop(&priv);


	return 0;
}

EXPORT_API int watch_core_terminate()
{
	ecore_main_loop_thread_safe_call_sync((Ecore_Data_Cb)__exit_loop, NULL);
	return 0;
}


EXPORT_API void watch_core_get_timeinfo(struct watch_time_s *timeinfo)
{
	__get_timeinfo(timeinfo);
}

