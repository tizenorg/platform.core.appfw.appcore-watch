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

#ifndef __TIZEN_APPFW_WATCH_APP_EFL_H__
#define __TIZEN_APPFW_WATCH_APP_EFL_H__

#include <Evas.h>

#ifdef __cplusplus
extern "C" {
#endif


/**
 * @addtogroup CAPI_WATCH_APP_MODULE
 * @{
 */

/**
 * @brief Gets Evas_Object for a Elementary window of watch application. You must use this window to draw watch UI on the idle screen.
 * @since_tizen 2.3.1
 *
 * @param[out] win The pointer of Evas_Object for a Elementary window.
 *
 * @return 0 on success, otherwise a negative error value
 * @retval #APP_ERROR_INVALID_PARAMETER Invalid Parameter
 * @retval #APP_ERROR_OUT_OF_MEMORY Out of memory
 * @retval #APP_ERROR_INVALID_CONTEXT Watch app is not initialized properly
 * @retval #APP_ERROR_NONE Successful
 */
int watch_app_get_elm_win(Evas_Object **win);


/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif
