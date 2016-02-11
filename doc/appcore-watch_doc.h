/*
 * app-core
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 *
 * @ingroup CAPI_APPLICATION_FRAMEWORK
 * @defgroup CAPI_WATCH_APP_MODULE Watch Application
 * @brief Watch application API
 *
 * @section CAPI_WATCH_APP_MODULE_HEADER Required Header
 *   \#include <watch_app.h>
 *   \#include <watch_app_efl.h>
 * @section CAPI_WATCH_APP_MODULE_OVERVIEW Overview
 * The @ref CAPI_WATCH_APP_MODULE API provides functions for handling Tizen watch application state changes or system events. Tizen watch application can be shown in the idle screen of the wearable device.
 * This API also provides time utility functions for developting Tizen watch applications. You can develop a watch application that shows exact time using these time utility functions.
 * For low powered wearable device, Tizen watch application supports a special mode that is named 'ambient'. When the device enters ambient mode, Tizen watch application that is shown in the idle screen can show limited UI and receives only ambient tick event at each minute to reduce power consumption. The limitation of UI that can be drawn in the ambient mode depends on the device. Usually, you should draw black and white UI only, and you should use below 20% of the pixels of the screen. If you don't want to draw your own ambient mode UI, you can set the 'ambient-support' attribute of the application as 'false' in the tizen-manifest.xml. Then, the platform will show proper default ambient mode UI.
 *
 * This API provides interfaces for the following categories:
 * - Starting or exiting the main event loop
 * - Registering callbacks for application state change events including timetick events
 * - Registering callbacks for basic system events
 * - Time related utility APIs for watch applications
 *
 * @subsection CAPI_WATCH_APP_MODULE_STATE_CHANGE_EVENT Registering Callbacks for Application State Change Events
 * The state change events for Tizen watch application is similar to the Tizen UI applications. See the @ref CAPI_APPLICATION_MODULE.
 * In Tizen watch application, an ambient changed event is added to support ambient mode. Time tick related events are also added to provide an exact time tick for the watch application.
 *
 * <p>
 * <table>
 * <tr>
 *   <th> Callback </th>
 *   <th> Description </th>
 * </tr>
 * <tr>
 *   <td>watch_app_create_cb()</td>
 *   <td>Hook to take necessary actions before the main event loop starts.
 *   Your UI generation code should be placed here so that you do not miss any events from your application UI.
 *   </td>
 * </tr>
 * <tr>
 *  <td>watch_app_control_cb()</td>
 *  <td> Hook to take necessary actions when your application called by another application.
 *   When the application gets launch request, this callback function is called.
 *   The application can get information about what is to be performed by using App Control API from app_control handle.
 *  </td>
 * </tr>
 * <tr>
 *  <td> watch_app_resume_cb() </td>
 *  <td> Hook to take necessary actions when an application becomes visible.
 *   If anything is relinquished in app_pause_cb() but is necessary to resume
 *   the application, it must be re-allocated here.
 *  </td>
 * </tr>
 * <tr>
 *  <td> watch_app_pause_cb() </td>
 *  <td> Hook to take necessary actions when an application becomes invisible.
 *    For example, you might wish to release memory resources so other applications can use these resources.
 *    It is important not to starve the application in front, which is interacting with the user.
 * </td>
 * </tr>
 * <tr>
 *  <td> watch_app_terminate_cb() </td>
 *  <td> Hook to take necessary actions when your application is terminating.
 *   Your application should release all resources, especially any
 *   allocations and shared resources must be freed here so other running applications can fully use these shared resources.
 *  </td>
 * </tr>
 * <tr>
 *  <td> watch_app_ambient_changed_cb() </td>
 *  <td> Hook to take necessary actions when the device enters ambient mode. Your application needs to adopt its UI to be compatibile with the ambient mode. Note that, you only can use very limited colors and pixels of the screen when the device is in the ambient mode. Usually, you should use only black and white to draw the ambient mode UI and use below 20% of the pixels of the screen. If you don't want to draw your own ambient mode UI, you can set the 'ambient-support' attribute of the application as 'false' in the tizen-manifest.xml. Then, the platform will show proper default ambient mode UI.
 * </tr>
 * <tr>
 *  <td> watch_app_time_tick_cb() </td>
 *  <td> This callback is called at each second when your application is visible. This callback is not called when your application is not visible or the device is in ambient mode. You can use this tick to update the time that is being displayed by your watch application.</td>
 * </tr>
 * <tr>
 *  <td> watch_app_ambient_tick_cb() </td>
 *  <td> This callback is called at each minute when the device is ambient mode. You can use this tick to update the time that is being displayed by your watch application while the device is in ambient mode. You should not do a job that takes long time in this callback. You should update the UI as fast as possible in this callback. The platform might make the device to sleep in short time after the ambient tick expires.</td>
 * </tr>
 * </table>
 * </p>
 *
 * Refer to the following state diagram to see the possible transitions and callbacks that are called while transition.
 * @image html watch_app_lifecycle.png "Watch Application States"
 *
 * It is almost same as the Tizen UI application.
 * Here are some remarks:
 * - When your application is in running state, if the device enters the ambient mode, watch_app_pause_cb() will be called before the watch_app_ambient_changed_cb() is called. It is because you need to draw new UI for the ambient mode and release unnecessary resources in the ambient mode.
 * - When the device returns from the ambient mode, the watch_app_resume_cb() will be called if your application is visible after the device returns from the ambient mode.
 * - watch_app_time_tick() is only called while your application is visible.
 * - watch_app_ambient_tick() is only called while the device is in the ambient mode.
 *
 * @subsection CAPI_WATCH_APP_MODULE_SYSTEM_EVENT Registering Callbacks for System Events
 * Tizen watch applications can receive system events with watch_app_add_event_handler() api. The type of system events that can be received are same as Tizen UI applications. See @ref CAPI_APPLICATION_MODULE.
 *
 *
 */
