/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

// Includes --------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <errno.h>
#include <bsd/sys/queue.h>

#include "pl.h"
#include "pl_button.h"
#include "pl_button_os_impl.h"
#include "pl_button_cam_impl.h"

#include "utility_log_module_id.h"
#include "utility_log.h"

// Macros ----------------------------------------------------------------------
#define EVENT_ID  0x9000
#define EVENT_ID_START 0x0000
#define EVENT_ID_MASK 0x00FF
#define LOG_ERR(event_id, format, ...) \
  WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, \
    "%s-%d:" format, __FILE__, __LINE__, ##__VA_ARGS__); \
  if (event_id >= 0) { \
    WRITE_ELOG_ERROR(MODULE_ID_SYSTEM, (EVENT_ID | \
                    (EVENT_ID_MASK & (EVENT_ID_START + event_id)))); \
  }

// Typedefs --------------------------------------------------------------------
typedef enum PlButtonEventId {
  // PlButtonGetInfo
  kPlButtonEventErrorId0x00 = 0x00,
  kPlButtonEventErrorId0x01,
  kPlButtonEventErrorId0x02,
  kPlButtonEventErrorId0x03,
  // PlButtonRegisterHandler
  kPlButtonEventErrorId0x04,
  kPlButtonEventErrorId0x05,
  kPlButtonEventErrorId0x06,
  kPlButtonEventErrorId0x07,
  kPlButtonEventErrorId0x08,
  kPlButtonEventErrorId0x09,
  kPlButtonEventErrorId0x0A,
  kPlButtonEventErrorId0x0B,  // Not use
  kPlButtonEventErrorId0x0C,  // Not use
  kPlButtonEventErrorId0x0D,
  // PlButtonUnregisterHandler
  kPlButtonEventErrorId0x0E,
  kPlButtonEventErrorId0x0F,
  kPlButtonEventErrorId0x10,
  kPlButtonEventErrorId0x11,
  kPlButtonEventErrorId0x12,
  kPlButtonEventErrorId0x13,
  kPlButtonEventErrorId0x14,
  kPlButtonEventErrorId0x15,  // Not use
  kPlButtonEventErrorId0x16,  // Not use
  kPlButtonEventErrorId0x17,
  kPlButtonEventErrorId0x18,
  // PlButtonInitialize
  kPlButtonEventErrorId0x19,
  kPlButtonEventErrorId0x1A,
  kPlButtonEventErrorId0x1B,
  kPlButtonEventErrorId0x1C,
  kPlButtonEventErrorId0x1D,
  kPlButtonEventErrorId0x1E,
  // PlButtonFinalize
  kPlButtonEventErrorId0x1F,
  kPlButtonEventErrorId0x20,
  kPlButtonEventErrorId0x21,
  kPlButtonEventErrorId0x22,
  kPlButtonEventErrorId0x23,  // Not use
  kPlButtonEventErrorId0x24,
  // local function
  kPlButtonEventErrorId0x25,
  kPlButtonEventErrorId0x26,
  kPlButtonEventErrorId0x27,
  kPlButtonEventErrorId0x28,
  kPlButtonEventErrorId0x29,
  kPlButtonEventErrorId0x2A,
  kPlButtonEventErrorId0x2B,
  kPlButtonEventErrorId0x2C,
  kPlButtonEventErrorId0x2D,
  kPlButtonEventErrorId0x2E,
  kPlButtonEventErrorId0x2F,  // Not use
  kPlButtonEventErrorId0x30,
  kPlButtonEventErrorId0x31,
  kPlButtonEventErrorId0x32,
  // Add later
  kPlButtonEventErrorId0x33,
  kPlButtonEventErrorId0x34,
  kPlButtonEventErrorId0x35,
  kPlButtonEventErrorId0x36,
  kPlButtonEventErrorId0x37,
  kPlButtonEventErrorId0x38,
} PlButtonEventId;

SLIST_HEAD(ButtonDataList, ButtonData);

struct ButtonData {
  uint32_t        button_id;
  uint32_t        pin;
  PlButtonHandler callback;
  void           *private_data;
  SLIST_ENTRY(ButtonData) next;
};

// External functions ----------------------------------------------------------

// Local functions -------------------------------------------------------------
static PlErrCode ButtonOpen(uint32_t button_id);
static PlErrCode ButtonClose(uint32_t button_id);
static void *ButtonCallbackThread(void *arg);
static bool IsOpen(uint32_t button_id);
static PlErrCode RegisterCallback(uint32_t button_id,
                                   PlButtonHandler callback,
                                   void *private_data);
static PlErrCode UnregisterCallback(uint32_t button_id);
static PlErrCode ExecCallback(uint32_t button_id,
                               PlButtonStatus button_status);
static PlErrCode GetPin(uint32_t button_id, uint32_t *pin);
static PlErrCode GetButtonId(uint32_t pin, uint32_t *button_id);
static PlErrCode GetRegisterInfo(uint32_t button_id,
                                 void **callback, void **private_data);
static PlErrCode DelListData(uint32_t button_id);
static void DelAllListData(void);

// Global Variables ------------------------------------------------------------
static struct ButtonDataList s_button_data_list = {0};
static pthread_mutex_t       s_button_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_t             s_button_thread = 0;
static bool                  s_is_initialized = false;
static bool                  s_thread_enable = false;

// Functions -------------------------------------------------------------------

// -----------------------------------------------------------------------------
//  PlButtonInitialize
// -----------------------------------------------------------------------------
PlErrCode PlButtonInitializeOsImpl(void) {
  PlErrCode err_code = kPlErrCodeOk;

  int ret = pthread_mutex_lock(&s_button_mutex);
  if (ret != 0) {
    LOG_ERR(kPlButtonEventErrorId0x19, "lock error(%d). errno=%d", ret, errno);
    err_code = kPlErrLock;
    goto err_end;
  }

  if (s_is_initialized) {
    LOG_ERR(kPlButtonEventErrorId0x1A, "Not initialize.");
    err_code = kPlErrInvalidState;
    goto release_unlock;
  }

  err_code = PlButtonInitializeCamImpl();
  if (err_code != 0) {
    LOG_ERR(kPlButtonEventErrorId0x1B,
            "PlButtonInitializeCamImpl err_code=%d", err_code);
    err_code = kPlErrInternal;
    goto release_unlock;
  }

  SLIST_INIT(&s_button_data_list);

  for (uint32_t btn_id = 0; btn_id < CONFIG_EXTERNAL_PL_BUTTON_NUM; btn_id++) {
    err_code = ButtonOpen(btn_id);
    if (err_code != kPlErrCodeOk) {
      LOG_ERR(kPlButtonEventErrorId0x1C,
              "ButtonOpen error. err_code=%d, btn_id=%d", err_code, btn_id);
      goto err_free_list;
    }
  }

  pthread_attr_t attr_button = {0};
  ret = pthread_attr_init(&attr_button);
  if (ret != 0) {
    LOG_ERR(kPlButtonEventErrorId0x33,
            "Button thread attr_init set error. err_code=%d errno=%d\n",
            ret, errno)
    err_code = kPlThreadError;
    goto err_free_list;
  }
  struct sched_param btn_param = {0};
  ret = pthread_attr_getschedparam(&attr_button, &btn_param);
  if (ret != 0) {
    LOG_ERR(kPlButtonEventErrorId0x34,
            "Button thread attr_getschedparam set error. "
            "err_code=%d errno=%d\n", ret, errno);
    err_code = kPlThreadError;
    goto err_free_list;
  }

  ret = pthread_attr_setschedpolicy(&attr_button, SCHED_RR);
  if (ret != 0) {
    LOG_ERR(kPlButtonEventErrorId0x35,
            "Button thread attr_setschedpolicy set error. "
            "err_code=%d errno=%d\n", ret, errno);
    err_code = kPlThreadError;
    goto err_free_list;
  }

  btn_param.sched_priority = CONFIG_EXTERNAL_PL_BUTTON_THREAD_PRIORITY;
  ret = pthread_attr_setschedparam(&attr_button, &btn_param);
  if (ret != 0) {
    LOG_ERR(kPlButtonEventErrorId0x36,
            "Button thread attr_setschedparam set error. "
            "err_code=%d errno=%d\n", ret, errno);
    err_code = kPlThreadError;
    goto err_free_list;
  }

  s_thread_enable = true;
  ret = pthread_create(&s_button_thread, &attr_button,
                       ButtonCallbackThread, NULL);
  if (ret != 0) {
    LOG_ERR(kPlButtonEventErrorId0x1D,
            "Button thread create error. err_code=%d errno=%d",
            err_code, errno);
    err_code = kPlThreadError;
    goto err_free_list;
  }

  s_is_initialized = true;
  goto release_unlock;

err_free_list:
  DelAllListData();

release_unlock:
  ret = pthread_mutex_unlock(&s_button_mutex);
  if (ret) {
    LOG_ERR(kPlButtonEventErrorId0x1E,
            "unlock error(%d). errno=%d", ret, errno);
  }

err_end:
  return err_code;
}

// -----------------------------------------------------------------------------
//  PlButtonFinalize
// -----------------------------------------------------------------------------
PlErrCode PlButtonFinalizeOsImpl(void) {
  PlErrCode err_code = kPlErrCodeOk;

  int ret = pthread_mutex_lock(&s_button_mutex);
  if (ret != 0) {
    LOG_ERR(kPlButtonEventErrorId0x1F, "lock error(%d). errno=%d", ret, errno);
    err_code = kPlErrLock;
    goto err_end;
  }
  if (!s_is_initialized) {
    LOG_ERR(kPlButtonEventErrorId0x20, "Not Initialize.");
    err_code = kPlErrInvalidState;
    goto unlock;
  }

  s_thread_enable = false;

  for (uint32_t btn_id = 0; btn_id < CONFIG_EXTERNAL_PL_BUTTON_NUM; btn_id++) {
    err_code = ButtonClose(btn_id);
    if (err_code != kPlErrCodeOk) {
      LOG_ERR(kPlButtonEventErrorId0x21,
              "Button Close error. btn_id=%d", btn_id);
      goto unlock;
    }
  }

  err_code = PlButtonFinalizeCamImpl();
  if (err_code != 0) {
    LOG_ERR(kPlButtonEventErrorId0x22,
            "PlButtonFinalizeCamImpl err_code=%d", err_code);
    err_code = kPlErrInternal;
    goto unlock;
  }

  pthread_join(s_button_thread, NULL);

  s_is_initialized = false;

unlock:
  ret = pthread_mutex_unlock(&s_button_mutex);
  if (ret) {
    LOG_ERR(kPlButtonEventErrorId0x24,
            "unlock error(%d). errno=%d", ret, errno);
  }

err_end:
  return err_code;
}

// -----------------------------------------------------------------------------
//  PlButtonGetInfo
// -----------------------------------------------------------------------------
PlErrCode PlButtonGetInfoOsImpl(PlButtonInfo *info) {
  PlErrCode err_code = kPlErrCodeOk;

  int ret = pthread_mutex_lock(&s_button_mutex);
  if (ret != 0) {
    LOG_ERR(kPlButtonEventErrorId0x00, "lock error(%d). errno=%d", ret, errno);
    err_code = kPlErrLock;
    goto err_end;
  }

  if (!s_is_initialized) {
    LOG_ERR(kPlButtonEventErrorId0x01, "Not initialize.");
    err_code = kPlErrInvalidState;
    goto unlock;
  }

  if (info == NULL) {
    LOG_ERR(kPlButtonEventErrorId0x02, "param info is NULL.");
    err_code = kPlErrInvalidParam;
    goto unlock;
  }

  info->button_total_num = CONFIG_EXTERNAL_PL_BUTTON_NUM;
  for (uint32_t btn_id = 0; btn_id < info->button_total_num; btn_id++) {
    info->button_ids[btn_id] = btn_id;
  }

unlock:
  ret = pthread_mutex_unlock(&s_button_mutex);
  if (ret) {
    LOG_ERR(kPlButtonEventErrorId0x03, "unlock error(%d). errno%d", ret, errno);
  }
err_end:
  return err_code;
}

// -----------------------------------------------------------------------------
//  PlButtonRegisterHandler
// -----------------------------------------------------------------------------
PlErrCode PlButtonRegisterHandlerOsImpl(uint32_t button_id,
                                    PlButtonHandler handler,
                                    void *private_data) {
  PlErrCode err_code = kPlErrCodeOk;

  int ret = pthread_mutex_lock(&s_button_mutex);
  if (ret != 0) {
    LOG_ERR(kPlButtonEventErrorId0x04, "lock error(%d). errno=%d", ret, errno);
    err_code = kPlErrLock;
    goto end;
  }

  if (!s_is_initialized) {
    LOG_ERR(kPlButtonEventErrorId0x05, "Not initialize.");
    err_code = kPlErrInvalidState;
    goto unlock;
  }

  if (handler == NULL) {
    LOG_ERR(kPlButtonEventErrorId0x06, "handler is NULL.");
    err_code = kPlErrInvalidParam;
    goto unlock;
  }

  if (!IsOpen(button_id)) {
    LOG_ERR(kPlButtonEventErrorId0x07,
            "Button open error. button_id=%d", button_id);
    err_code = kPlErrOpen;
    goto unlock;
  }

  err_code = RegisterCallback(button_id, handler, private_data);
  if (err_code != kPlErrCodeOk) {
    LOG_ERR(kPlButtonEventErrorId0x08,
            "Callback error. err_code=%d, button_id=%d", err_code, button_id);
    goto unlock;
  }

  uint32_t pin = 0;
  err_code = GetPin(button_id, &pin);
  if (err_code != kPlErrCodeOk) {
    LOG_ERR(kPlButtonEventErrorId0x09,
            "Failed get pin. button_id=%d", button_id);
    goto unregister;
  }

  PlButtonStatus btn_status = kPlButtonStatusMax;
  err_code = PlButtonRegisterHandlerCamImpl(pin, &btn_status);
  if (err_code != kPlErrCodeOk) {
    LOG_ERR(kPlButtonEventErrorId0x0A,
            "PlButtonRegisterHandlerCamImpl error=%d.", err_code);
    goto unregister;
  }

  handler(btn_status, private_data);

  goto unlock;

unregister:
  UnregisterCallback(button_id);

unlock:
  ret = pthread_mutex_unlock(&s_button_mutex);
  if (ret) {
    LOG_ERR(kPlButtonEventErrorId0x0D,
            "unlock error(%d). errno=%d", ret, errno);
  }

end:
  return err_code;
}

// -----------------------------------------------------------------------------
//  PlButtonUnregisterHandler
// -----------------------------------------------------------------------------
PlErrCode PlButtonUnregisterHandlerOsImpl(uint32_t button_id) {
  PlErrCode err_code = kPlErrCodeOk;
  PlErrCode ret_restore = kPlErrCodeOk;

  int ret = pthread_mutex_lock(&s_button_mutex);
  if (ret != 0) {
    LOG_ERR(kPlButtonEventErrorId0x0E, "lock error(%d). errno=%d", ret, errno);
    err_code = kPlErrLock;
    goto err_end;
  }

  if (!s_is_initialized) {
    LOG_ERR(kPlButtonEventErrorId0x0F, "Not initialize.");
    err_code = kPlErrInvalidState;
    goto unlock;
  }

  if (!IsOpen(button_id)) {
    LOG_ERR(kPlButtonEventErrorId0x10,
            "Button open error. button_id=%d", button_id);
    err_code = kPlErrOpen;
    goto unlock;
  }

  void *callback = NULL, *private_data = NULL;
  // Save the data to be deleted for error recovery.
  err_code = GetRegisterInfo(button_id, &callback, &private_data);
  if (err_code != kPlErrCodeOk) {
    LOG_ERR(kPlButtonEventErrorId0x11,
            "No entry to unregister. err_code=%u, button_id=%u",
            err_code, button_id);
    goto unlock;
  }

  err_code = UnregisterCallback(button_id);
  if (err_code != kPlErrCodeOk) {
    LOG_ERR(kPlButtonEventErrorId0x12,
             "callback error. err_code=%d, button_id=%d",
             err_code, button_id);
    goto unlock;
  }

  uint32_t pin = 0;
  err_code = GetPin(button_id, &pin);
  if (err_code != kPlErrCodeOk) {
    LOG_ERR(kPlButtonEventErrorId0x13,
            "Failed get pin. button_id=%d", button_id);
    goto restore;
  }

  err_code = PlButtonUnregisterHandlerCamImpl(pin);
  if (err_code != kPlErrCodeOk) {
    LOG_ERR(kPlButtonEventErrorId0x14,
            "PlButtonUnregisterHandlerCamImpl error=%d.", err_code);
    goto restore;
  }

  goto unlock;

restore:
  // Restore callback hander and private data.
  ret_restore = RegisterCallback(button_id, callback, private_data);
  if (ret_restore != kPlErrCodeOk) {
    LOG_ERR(kPlButtonEventErrorId0x17,
            "Failed to RegisterCallback for restore. ret=%u", ret_restore);
  }

unlock:
  ret = pthread_mutex_unlock(&s_button_mutex);
  if (ret) {
    LOG_ERR(kPlButtonEventErrorId0x18,
            "unlock error(%d). errno=%d", ret, errno);
  }

err_end:
  return err_code;
}

// Local Functions -------------------------------------------------------------
// -----------------------------------------------------------------------------
//  GetPin()
// -----------------------------------------------------------------------------
static PlErrCode GetPin(uint32_t button_id, uint32_t *pin) {
  PlErrCode err_code = kPlErrCodeOk;

  switch (button_id) {
    case 0:
      *pin = CONFIG_EXTERNAL_PL_BUTTON0_GPIO;
      break;
    default:
      LOG_ERR(kPlButtonEventErrorId0x25, "button_id=%d", button_id);
      err_code = kPlErrInvalidParam;
      break;
  }

  return err_code;
}

// -----------------------------------------------------------------------------
//  ButtonOpen
// -----------------------------------------------------------------------------
static PlErrCode ButtonOpen(uint32_t button_id) {
  PlErrCode err_code = kPlErrCodeOk;

  if (IsOpen(button_id)) {
    LOG_ERR(kPlButtonEventErrorId0x26,
            "ButtonId is already opened. button_id=%d", button_id);
    err_code = kPlErrOpen;
    goto err_end;
  }

  struct ButtonData *button_data = malloc(sizeof(struct ButtonData));
  if (button_data == NULL) {
    LOG_ERR(kPlButtonEventErrorId0x27, "ButtonData malloc error.");
    err_code = kPlErrMemory;
    goto err_end;
  }

  uint32_t pin = 0;
  err_code = GetPin(button_id, &pin);
  if (err_code != kPlErrCodeOk) {
    LOG_ERR(kPlButtonEventErrorId0x28,
            "Failed get pin. button_id=%d", button_id);
    goto err_free;
  }

  err_code = PlButtonOpenCamImpl(pin);
  if (err_code != kPlErrCodeOk) {
    LOG_ERR(kPlButtonEventErrorId0x29,
            "PlButtonOpenCamImpl error=%d.", err_code);
    goto err_free;
  }

  button_data->button_id = button_id;
  button_data->pin = pin;
  button_data->callback = NULL;
  button_data->private_data = NULL;

  SLIST_INSERT_HEAD(&s_button_data_list, button_data, next);

err_free:
  if (err_code != kPlErrCodeOk) {
    free(button_data);
  }

err_end:
  return err_code;
}

// -----------------------------------------------------------------------------
//  ButtonClose
// -----------------------------------------------------------------------------
static PlErrCode ButtonClose(uint32_t button_id) {
  PlErrCode err_code = kPlErrCodeOk;

  if (!IsOpen(button_id)) {
    LOG_ERR(kPlButtonEventErrorId0x2A,
            "Button open error. button_id=%d", button_id);
    // Already closed, it's OK.
    return kPlErrCodeOk;
  }

  uint32_t pin = 0;
  err_code = GetPin(button_id, &pin);
  if (err_code != kPlErrCodeOk) {
    LOG_ERR(kPlButtonEventErrorId0x2B,
            "Failed get pin. button_id=%d", button_id);
  }
  err_code = PlButtonCloseCamImpl(pin);
  if (err_code != kPlErrCodeOk) {
    LOG_ERR(kPlButtonEventErrorId0x2C,
            "PlButtonCloseCamImpl error. err_code=%d, button_id=%d",
            err_code, button_id);
  }

  err_code = DelListData(button_id);
  if (err_code != kPlErrCodeOk) {
    LOG_ERR(kPlButtonEventErrorId0x37,
            "Button delete error. err_code=%d, button_id=%d",
            err_code, button_id);
  }
  return err_code;
}

// -----------------------------------------------------------------------------
//  ButtonCallbackThread
// -----------------------------------------------------------------------------
static void *ButtonCallbackThread(void *arg) {
  (void)arg;  // Avoid compiler warning

  while (s_thread_enable) {
    PlErrCode err_code = PlButtonWaitEventCamImpl();
    if (err_code == kPlErrTimeout) {
      continue;
    }

    if (s_thread_enable == false) {
      break;
    }

    uint32_t pin = 0;
    PlButtonStatus button_status = kPlButtonStatusMax;
    err_code = PlButtonGetValCamImpl(&pin, &button_status);
    if (err_code != kPlErrCodeOk) {
      LOG_ERR(kPlButtonEventErrorId0x38,
              "PlButtonGetVal failed:%u\n", err_code);
      continue;
    }

    uint32_t button_id = 0;
    err_code = GetButtonId(pin, &button_id);
    if (err_code != kPlErrCodeOk) {
      LOG_ERR(kPlButtonEventErrorId0x2D,
              "Get ButtonId error. err_code=%u, pin=%u", err_code, pin);
      continue;
    }

    err_code = ExecCallback(button_id, button_status);
    if (err_code != kPlErrCodeOk) {
      LOG_ERR(kPlButtonEventErrorId0x2E,
              "Callback is none. err_code=%u, button_id=%u",
              err_code, button_id);
    }
  }

  pthread_exit(NULL);
  return NULL;
}

// -----------------------------------------------------------------------------
//  IsOpen
// -----------------------------------------------------------------------------
static bool IsOpen(uint32_t button_id) {
  struct ButtonData *entry_data = NULL, *temp_entry_data = NULL;
  SLIST_FOREACH_SAFE(entry_data, &s_button_data_list, next, temp_entry_data) {
    if (entry_data->button_id == button_id) {
      return true;
    }
  }

  return false;
}

// -----------------------------------------------------------------------------
//  RegisterCallback
// -----------------------------------------------------------------------------
static PlErrCode RegisterCallback(uint32_t button_id,
                                   PlButtonHandler callback,
                                   void *private_data) {
  struct ButtonData *entry_data = NULL, *temp_entry_data = NULL;
  SLIST_FOREACH_SAFE(entry_data, &s_button_data_list, next, temp_entry_data) {
    if (entry_data->button_id == button_id) {
      if (entry_data->callback == NULL) {
        entry_data->callback = callback;
        entry_data->private_data = private_data;
        return kPlErrCodeOk;
      } else {
        LOG_ERR(kPlButtonEventErrorId0x30,
                "Callback is already setting. button_id=%d", button_id);
      }
    }
  }

  return kPlErrInvalidParam;
}

// -----------------------------------------------------------------------------
//  UnregisterCallback
// -----------------------------------------------------------------------------
static PlErrCode UnregisterCallback(uint32_t button_id) {
  struct ButtonData *entry_data = NULL, *temp_entry_data = NULL;
  SLIST_FOREACH_SAFE(entry_data, &s_button_data_list, next, temp_entry_data) {
    if (entry_data->button_id == button_id) {
      if (entry_data->callback != NULL) {
        entry_data->callback = NULL;
        entry_data->private_data = NULL;
        return kPlErrCodeOk;
      } else {
        LOG_ERR(kPlButtonEventErrorId0x31,
                "Callback is already NULL. button_id=%u", button_id);
      }
    }
  }

  return kPlErrInvalidParam;
}

// -----------------------------------------------------------------------------
//  ExecCallback
// -----------------------------------------------------------------------------
static PlErrCode ExecCallback(uint32_t button_id,
                              PlButtonStatus button_status) {
  struct ButtonData *entry_data = NULL, *temp_entry_data = NULL;
  SLIST_FOREACH_SAFE(entry_data, &s_button_data_list, next, temp_entry_data) {
    if (entry_data->button_id == button_id) {
      if (entry_data->callback != NULL) {
        entry_data->callback(button_status, entry_data->private_data);
        return kPlErrCodeOk;
      }
    }
  }

  return kPlErrInvalidParam;
}

// -----------------------------------------------------------------------------
//  GetButtonId()
// -----------------------------------------------------------------------------
static PlErrCode GetButtonId(uint32_t pin, uint32_t *button_id) {
  struct ButtonData *entry_data = NULL, *temp_entry_data = NULL;
  SLIST_FOREACH_SAFE(entry_data, &s_button_data_list, next, temp_entry_data) {
    if (entry_data->pin == pin) {
      *button_id = entry_data->button_id;
      return kPlErrCodeOk;
    }
  }

  return kPlErrInvalidParam;
}

// -----------------------------------------------------------------------------
//  GetRegisterInfo()
// -----------------------------------------------------------------------------
static PlErrCode GetRegisterInfo(uint32_t button_id,
                                 void **callback, void **private_data) {
  if ((callback == NULL) || (private_data == NULL)) {
    return kPlErrInvalidParam;
  }

  struct ButtonData *entry_data = NULL, *temp_entry_data = NULL;
  SLIST_FOREACH_SAFE(entry_data, &s_button_data_list, next, temp_entry_data) {
    if (entry_data->button_id == button_id) {
      if (entry_data->callback != NULL) {
        *callback     = (void *)entry_data->callback;
        *private_data = entry_data->private_data;
        return kPlErrCodeOk;
      } else {
        LOG_ERR(kPlButtonEventErrorId0x32,
                "Callback is NULL. button_id=%u", button_id);
      }
    }
  }
  return kPlErrNotFound;
}

// -----------------------------------------------------------------------------
//  DelListData()
// -----------------------------------------------------------------------------
static PlErrCode DelListData(uint32_t button_id) {
  struct ButtonData *entry_data = NULL, *temp_entry_data = NULL;
  SLIST_FOREACH_SAFE(entry_data, &s_button_data_list, next, temp_entry_data) {
    if (entry_data->button_id == button_id) {
      SLIST_REMOVE(&s_button_data_list, entry_data, ButtonData, next);
      free(entry_data);
      return kPlErrCodeOk;
    }
  }

  return kPlErrInvalidParam;
}

// -----------------------------------------------------------------------------
//  DelAllListData()
// -----------------------------------------------------------------------------
static void DelAllListData(void) {
  struct ButtonData *entry_data = NULL, *temp_entry_data = NULL;
  SLIST_FOREACH_SAFE(entry_data, &s_button_data_list, next, temp_entry_data) {
    SLIST_REMOVE(&s_button_data_list, entry_data, ButtonData, next);
    free(entry_data);
  }
}
