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

#ifndef __TIZEN_APPFW_WATCH_APP_H__
#define __TIZEN_APPFW_WATCH_APP_H__

#include <tizen.h>
#include <time.h>
#include <app_control.h>
#include <app_common.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup CAPI_WATCH_APP_MODULE
 * @{
 */

/**
 * @brief watch_time_h watch_time handle
 * @since_tizen 2.3.1
 */
typedef struct _watch_time_s* watch_time_h;

/**
 * @brief Called when the application starts.
 * @since_tizen 2.3.1
 *
 * @details The callback function is called before the main loop of the application starts.
 *          In this callback, you can initialize application resources like window creation, data structure, and so on.
 *          After this callback function returns @c true, the main loop starts up and watch_app_control_cb() is subsequently called.
 *          If this callback function returns @c false, the main loop doesn't start and  watch_app_terminate_cb() is subsequently called.
 *
 * @param[in] width The width of the window of idle screen that will show the watch UI
 * @param[in] height The height of the window of idle screen that will show the watch UI
 * @param[in] user_data The user data passed from the callback registration function
 * @return @c true on success,
 *         otherwise @c false
 * @see watch_app_main()
 * @see #watch_app_lifecycle_callback_s
 */
typedef bool (*watch_app_create_cb) (int width, int height, void *user_data);

/**
 * @brief Called when another application sends a launch request to the application.
 * @since_tizen 2.3.1
 *
 * @details When the application is launched, this callback function is called after the main loop of the application starts up.
 *          The passed app_control handle describes the launch request and contains the information about why the application is launched.
 *          If the launch request is sent to the application in the running or pause state,
 *          this callback function can be called again to notify that the application has been asked to launch.
 *
 *          The application is responsible for handling each launch request and responding appropriately.
 *          Using the App Control API, the application can get information about what is to be performed.
 *          The app_control handle may include only the default operation (#APP_CONTROL_OPERATION_DEFAULT) without any data.
 *          For more information, see The @ref CAPI_APP_CONTROL_MODULE API description.
 *
 * @param[in] app_control The handle to the app_control
 * @param[in] user_data The user data passed from the callback registration function
 * @see watch_app_main()
 * @see #watch_app_lifecycle_callback_s
 * @see @ref CAPI_APP_CONTROL_MODULE API
 */
typedef void (*watch_app_control_cb) (app_control_h app_control, void *user_data);

/**
 * @brief Called when the application is completely obscured by another application and becomes invisible.
 * @since_tizen 2.3.1
 *
 * @details The application is not terminated and still running in the paused state.
 *
 * @param[in] user_data The user data passed from the callback registration function
 * @see watch_app_main()
 * @see #watch_app_lifecycle_callback_s
 */
typedef void (*watch_app_pause_cb) (void *user_data);

/**
 * @brief Called when the application becomes visible.
 * @since_tizen 2.3.1
 *
 * @remarks This callback function is not called when the application moves from the created state to the running state.
 *
 * @param[in] user_data The user data passed from the callback registration function
 * @see watch_app_main()
 * @see #watch_app_lifecycle_callback_s
 */
typedef void (*watch_app_resume_cb) (void *user_data);

/**
 * @brief Called when the application's main loop exits.
 * @details You should release the application's resources in this function.
 * @since_tizen 2.3.1
 *
 * @param[in] user_data The user data passed from the callback registration function
 * @see watch_app_main()
 * @see #watch_app_lifecycle_callback_s
 */
typedef void (*watch_app_terminate_cb) (void *user_data);

/**
 * @brief Called at each second. This callback is not called while the app is paused or the device is in ambient mode.
 * @since_tizen 2.3.1
 *
 * @param[in] watch_time The watch time handle. watch_time will not be available after returning this callback. It will be freed by the framework.
 * @param[in] user_data The user data to be passed to the callback functions
 */
typedef void (*watch_app_time_tick_cb) (watch_time_h watch_time, void *user_data);

/**
 * @brief Called at each minute when the device in the ambient mode.
 * @since_tizen 2.3.1
 *
 * @remarks You should not do a job that takes long time in this callback. You should update the UI as fast as possible in this callback. The platform might make the device to sleep in short time after the ambient tick expires.
 *
 * @param[in] watch_time The watch time handle. watch_time will not be available after returning this callback. It will be freed by the framework.
 * @param[in] user_data The user data to be passed to the callback functions
 */
typedef void (*watch_app_ambient_tick_cb) (watch_time_h watch_time, void *user_data);

/**
 * @brief Called when the device enters or exits the ambient mode.
 * @since_tizen 2.3.1
 *
 * @remarks For low powered wearable device, Tizen watch application supports a special mode that is named 'ambient'. When the device enters ambient mode, Tizen watch application that is shown in the idle screen can show limited UI and receives only ambient tick event at each minute to reduce power consumption. The limitation of UI that can be drawn in the ambient mode depends on the device. Usually, you should draw black and white UI only, and you should use below 20% of the pixels of the screen. If you don't want to draw your own ambient mode UI, you can set the 'ambient-support' attribute of the application as 'false' in the tizen-manifest.xml. Then, the platform will show proper default ambient mode UI.
 *
 * @param[in] ambient_mode If @c true the device enters the ambient mode,
 *                          otherwise @c false
 * @param[in] user_data The user data to be passed to the callback functions
 */
typedef void (*watch_app_ambient_changed_cb) (bool ambient_mode, void *user_data);


/**
 * @brief The structure type containing the set of callback functions for handling application events.
 * @details It is one of the input parameters of the watch_app_main() function.
 * @since_tizen 2.3.1
 *
 * @see watch_app_main()
 * @see watch_app_create_cb()
 * @see watch_app_control_cb()
 * @see watch_app_pause_cb()
 * @see watch_app_resume_cb()
 * @see watch_app_time_tick_cb()
 * @see watch_app_ambient_tick_cb()
 * @see watch_app_ambient_changed_cb()
 * @see watch_app_terminate_cb()
 */
typedef struct {
	watch_app_create_cb create; /**< This callback function is called at the start of the application. */
	watch_app_control_cb app_control; /**< This callback function is called when another application sends the launch request to the application. */
	watch_app_pause_cb pause; /**< This callback function is called each time the application is completely obscured by another application and becomes invisible to the user. */
	watch_app_resume_cb resume; /**< This callback function is called each time the application becomes visible to the user. */
	watch_app_terminate_cb terminate; /**< This callback function is called before the application exits. */
	watch_app_time_tick_cb time_tick; /**< This callback function is called at each second. */
	watch_app_ambient_tick_cb ambient_tick; /**< This callback function is called at each minute in ambient mode. */
	watch_app_ambient_changed_cb ambient_changed; /**< This callback function is called when the device enters or exits ambient mode. */
} watch_app_lifecycle_callback_s;


/**
 * @brief Adds the system event handler
 * @since_tizen 2.3.1
 *
 * @param[out] handler The event handler
 * @param[in] event_type The system event type
 * @param[in] callback The callback function
 * @param[in] user_data The user data to be passed to the callback function
 *
 * @return 0 on success, otherwise a negative error value
 * @retval #APP_ERROR_NONE Successfull
 * @retval #APP_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #APP_ERROR_OUT_OF_MEMORY Out of memory
 *
 * @see app_event_type_e
 * @see app_event_cb
 * @see watch_app_remove_event_handler
 */
int watch_app_add_event_handler(app_event_handler_h *handler, app_event_type_e event_type, app_event_cb callback, void *user_data);


/**
 * @brief Removes registered event handler
 * @since_tizen 2.3.1
 *
 * @param[in] event_handler The event handler
 *
 * @return 0 on success, otherwise a negative error value
 * @retval #APP_ERROR_NONE Successfull
 * @retval #APP_ERROR_INVALID_PARAMETER Invalid parameter
 *
 * @see watch_app_add_event_handler
 */
int watch_app_remove_event_handler(app_event_handler_h event_handler);


/**
 * @brief Runs the main loop of the application until watch_app_exit() is called.
 * @since_tizen 2.3.1
 *
 * @remarks http://tizen.org/privilege/alarm.set privilege is needed to receive ambient ticks at each minute. If your app hasn't the privilege and you set the watch_app_ambient_tick_cb(), #APP_ERROR_PERMISSION_DENIED will be returned.
 *
 * @param[in] argc The argument count
 * @param[in] argv The argument vector
 * @param[in] callback The set of callback functions to handle application events
 * @param[in] user_data The user data to be passed to the callback functions
 *
 * @return @c 0 on success,
 *         otherwise a negative error value.
 * @retval #APP_ERROR_NONE Successful
 * @retval #APP_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #APP_ERROR_INVALID_CONTEXT The application is illegally launched, not launched by the launch system.
 * @retval #APP_ERROR_ALREADY_RUNNING The main loop has already started
 * @retval #APP_ERROR_PERMISSION_DENIED Permission denied
 *
 * @see watch_app_main()
 * @see watch_app_create_cb()
 * @see watch_app_control_cb()
 * @see watch_app_pause_cb()
 * @see watch_app_resume_cb()
 * @see watch_app_time_tick_cb()
 * @see watch_app_ambient_tick_cb()
 * @see watch_app_ambient_changed_cb()
 * @see watch_app_terminate_cb()
 * @see #watch_app_lifecycle_callback_s
 */
int watch_app_main(int argc, char **argv, watch_app_lifecycle_callback_s *callback, void *user_data);


/**
 * @brief Exits the main loop of the application.
 * @details The main loop of the application stops and watch_app_terminate_cb() is invoked.
 * @since_tizen 2.3.1
 *
 * @see watch_app_main()
 * @see watch_app_terminate_cb()
 */
void watch_app_exit(void);

/**
 * @brief Gets the current time.
 * @since_tizen 2.3.1
 *
 * @remarks You must release @a watch_time using watch_time_delete() after using it.
 *
 * @param[out] watch_time The watch_time handle to be newly created on successl
 *
 * @return 0 on success, otherwise a negative error value
 * @retval #APP_ERROR_INVALID_PARAMETER Invalid Parameter
 * @retval #APP_ERROR_OUT_OF_MEMORY Out of Memory
 * @retval #APP_ERROR_NONE Successful
 */
int watch_time_get_current_time(watch_time_h* watch_time);

/**
 * @brief Deletes the watch time handle and releases all its resources.
 * @since_tizen 2.3.1
 *
 * @param[in] watch_time The watch_time handle
 *
 * @return 0 on success, otherwise a negative error value
 * @retval #APP_ERROR_INVALID_PARAMETER Invalid Parameter
 * @retval #APP_ERROR_NONE Successful
 */
int watch_time_delete(watch_time_h watch_time);

/**
 * @brief Gets the year info.
 * @since_tizen 2.3.1
 *
 * @param[in] watch_time The watch_time handle
 * @param[out] year The year info
 *
 * @return 0 on success, otherwise a negative error value
 * @retval #APP_ERROR_INVALID_PARAMETER Invalid Parameter
 * @retval #APP_ERROR_NONE Successful
 */
int watch_time_get_year(watch_time_h watch_time, int *year);

/**
 * @brief Gets the month info.
 * @since_tizen 2.3.1
 *
 * @param[in] watch_time The watch_time handle
 * @param[out] month The month info
 *
 * @return 0 on success, otherwise a negative error value
 * @retval #APP_ERROR_INVALID_PARAMETER Invalid Parameter
 * @retval #APP_ERROR_NONE Successful
 */
int watch_time_get_month(watch_time_h watch_time, int *month);

/**
 * @brief Gets the day info.
 * @since_tizen 2.3.1
 *
 * @param[in] watch_time The watch_time handle
 * @param[out] day The day info
 *
 * @return 0 on success, otherwise a negative error value
 * @retval #APP_ERROR_INVALID_PARAMETER Invalid Parameter
 * @retval #APP_ERROR_NONE Successful
 */
int watch_time_get_day(watch_time_h watch_time, int *day);

/**
 * @brief Gets the day of week info.
 * @since_tizen 2.3.1
 *
 * @param[in] watch_time The watch_time handle
 * @param[out] day_of_week The day of week info. The value returns from 1 (Sunday) to 7 (Saturday).
 *
 * @return 0 on success, otherwise a negative error value
 * @retval #APP_ERROR_INVALID_PARAMETER Invalid Parameter
 * @retval #APP_ERROR_NONE Successful
 */
int watch_time_get_day_of_week(watch_time_h watch_time, int *day_of_week);

/**
 * @brief Gets the hour info.
 * @since_tizen 2.3.1
 *
 * @param[in] watch_time The watch_time handle
 * @param[out] hour The hour info
 *
 * @return 0 on success, otherwise a negative error value
 * @retval #APP_ERROR_INVALID_PARAMETER Invalid Parameter
 * @retval #APP_ERROR_NONE Successful
 */
int watch_time_get_hour(watch_time_h watch_time, int *hour);

/**
 * @brief Gets the hour info in 24-hour presentation.
 * @since_tizen 2.3.1
 *
 * @param[in] watch_time The watch_time handle
 * @param[out] hour24 The hour info
 *
 * @return 0 on success, otherwise a negative error value
 * @retval #APP_ERROR_INVALID_PARAMETER Invalid Parameter
 * @retval #APP_ERROR_NONE Successful
 */
int watch_time_get_hour24(watch_time_h watch_time, int *hour24);

/**
 * @brief Gets the minute info.
 * @since_tizen 2.3.1
 *
 * @param[in] watch_time The watch_time handle
 * @param[out] minute The minute info
 *
 * @return 0 on success, otherwise a negative error value
 * @retval #APP_ERROR_INVALID_PARAMETER Invalid Parameter
 * @retval #APP_ERROR_NONE Successful
 */
int watch_time_get_minute(watch_time_h watch_time, int *minute);

/**
 * @brief Gets the second info.
 * @since_tizen 2.3.1
 *
 * @param[in] watch_time The watch_time handle
 * @param[out] second The second info
 *
 * @return 0 on success, otherwise a negative error value
 * @retval #APP_ERROR_INVALID_PARAMETER Invalid Parameter
 * @retval #APP_ERROR_NONE Successful
 */
int watch_time_get_second(watch_time_h watch_time, int *second);

/**
 * @brief Gets the millisecond info.
 * @since_tizen 2.3.1
 *
 * @param[in] watch_time The watch_time handle
 * @param[out] millisecond The millisecond info
 *
 * @return 0 on success, otherwise a negative error value
 * @retval #APP_ERROR_INVALID_PARAMETER Invalid Parameter
 * @retval #APP_ERROR_NONE Successful
 */
int watch_time_get_millisecond(watch_time_h watch_time, int *millisecond);

/**
 * @brief Gets the UTC time.
 * @since_tizen 2.3.1
 *
 * @param[in] watch_time The watch_time handle
 * @param[out] utc_time The UTC time
 *
 * @return 0 on success, otherwise a negative error value
 * @retval #APP_ERROR_INVALID_PARAMETER Invalid Parameter
 * @retval #APP_ERROR_NONE Successful
 */
int watch_time_get_utc_time(watch_time_h watch_time, struct tm *utc_time);

/**
 * @brief Gets the UTC timestamp.
 * @since_tizen 2.3.1
 *
 * @param[in] watch_time The watch_time handle
 * @param[out] utc_timestamp The UTC timestamp
 *
 * @return 0 on success, otherwise a negative error value
 * @retval #APP_ERROR_INVALID_PARAMETER Invalid Parameter
 * @retval #APP_ERROR_NONE Successful
 */
int watch_time_get_utc_timestamp(watch_time_h watch_time, time_t *utc_timestamp);

/**
 * @brief Gets the ID of timezone for the @a watch_time handle.
 * @since_tizen 2.3.1
 *
 * @remarks You must release @a time_zone_id using free() after using it.
 *
 * @param[in] watch_time The watch_time handle
 * @param[out] time_zone_id The Timezone ID, such as "America/Los_Angeles"
 *
 * @return 0 on success, otherwise a negative error value
 * @retval #APP_ERROR_INVALID_PARAMETER Invalid Parameter
 * @retval #APP_ERROR_NONE Successful
 */
int watch_time_get_time_zone(watch_time_h watch_time, char **time_zone_id);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __TIZEN_APPFW_WATCH_APP_H__ */
