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

#ifndef __APPCORE_WATCH_SIGNAL_H__
#define __APPCORE_WATCH_SIGNAL_H__

#include <dbus/dbus.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ALPM_VIEWER_PATH "/Org/Tizen/Coreapps/Alpmviewer/Clockdraw"
#define ALPM_VIEWER_INTERFACE "org.tizen.coreapps.alpmviewer.clockdraw"
#define ALPM_VIEWER_SIGNAL_DRAW_DONE "drawdone"

#define DEVICED_PATH "/Org/Tizen/System/DeviceD"
#define DEVICED_INTERFACE "org.tizen.system.deviced"
#define DEVICED_SIGNAL_HOME_SCREEN "HomeScreen"
#define CLOCK_START "clockbegin"
#define CLOCK_STOP "clockstop"

int _watch_core_listen_alpm_handler(int (*func) (int, void *), void *data);
int _watch_core_send_alpm_update_done(void);

#ifdef __cplusplus
}
#endif

#endif /* __APPCORE_WATCH_SIGNAL_H__ */
