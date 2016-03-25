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

#ifndef __APPCORE_WATCH_H__
#define __APPCORE_WATCH_H__

#include <libintl.h>
#include <bundle.h>

#include "app_control.h"

#ifdef __cplusplus
extern "C" {
#endif

struct watchcore_ops {
	void *data; /**< Callback data */
	int (*create) (int w, int h, void *); /**< This callback function is called at the start of the application. */
	int (*app_control) (app_control_h, void *); /**< This callback function is called when other application send the launch request to the application. */
	int (*pause) (void *); /**< Called when every window goes back */
	int (*resume) (void *); /**< Called when any window comes on top */
	int (*terminate) (void *); /**< This callback function is called once after the main loop of application exits. */
	void (*time_tick) (void *, void *);
	void (*ambient_tick) (void *, void *);
	void (*ambient_changed) (int, void *);
	void *reserved[6]; /**< Reserved */
};

enum watch_core_event {
	WATCH_CORE_EVENT_UNKNOWN, /**< Unknown event */
	WATCH_CORE_EVENT_LOW_MEMORY, /**< Low memory */
	WATCH_CORE_EVENT_LOW_BATTERY, /**< Low battery */
	WATCH_CORE_EVENT_LANG_CHANGE, /**< Language setting is changed */
	WATCH_CORE_EVENT_REGION_CHANGE, /**< Region setting is changed */
};

struct watch_time_s {
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

int watch_core_main(const char *appid, int argc, char **argv,
		struct watchcore_ops *ops);
int watch_core_terminate();
int watch_core_set_event_callback(enum watch_core_event event,
		int (*cb) (void *, void *), void *data);
const char *watch_core_get_appid();
void watch_core_get_timeinfo(struct watch_time_s *timeinfo);
bool watch_core_get_24h_mode(void);

#ifdef __cplusplus
}
#endif

#endif /* __APPCORE_WATCH_H__ */

