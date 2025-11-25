/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#include "pl_led.h"
#include "pl_led_hw.h"
#include "hal_ioexp.h"
#include "utility_log.h"
#include "utility_log_module_id.h"

// Debug log -------------------------------------------------------------------
#define EVENT_ID  0x9400
#define EVENT_ID_START 0xA0
#define LOG_E(event_id, format, ...) \
  WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:" \
                   format, __FILE__, __LINE__, ##__VA_ARGS__); \
  WRITE_ELOG_ERROR(MODULE_ID_SYSTEM, (EVENT_ID | (EVENT_ID_START + event_id)));

#define LOG_W(event_id, format, ...) \
  WRITE_DLOG_WARN(MODULE_ID_SYSTEM, "%s-%d:" \
                  format, __FILE__, __LINE__, ##__VA_ARGS__); \
  WRITE_ELOG_WARN(MODULE_ID_SYSTEM, (EVENT_ID | (EVENT_ID_START + event_id)));

#define LOG_D(format, ...) \
  WRITE_DLOG_DEBUG(MODULE_ID_SYSTEM, "%s-%d:" \
                   format, __FILE__, __LINE__, ##__VA_ARGS__);
                   
#define LOG_T(format, ...) \
  WRITE_DLOG_TRACE(MODULE_ID_SYSTEM, "%s-%d:" \
                   format, __FILE__, __LINE__, ##__VA_ARGS__);

// typedef --------------------------------------------------------------------
typedef enum {
  kIoexpIdx0 = 0,
  kIoexpIdx1,
  kIoexpIdxMax
} IoexpIdx;

struct LedHwCtrlInfo {
  uint32_t        led_id;
  uint32_t        color_id;
  uint32_t        ioexp_id[kIoexpIdxMax];
  HalIoexpHandle  ioexp_handle[kIoexpIdxMax];
};

static struct {
  HalErrCode    hal_err_code;
  PlErrCode     pl_err_code;
} s_conv_err_code[kPlErrCodeMax + 1] = {
  {kHalErrCodeOk,           kPlErrCodeOk},
  {kHalErrCodeError,        kPlErrCodeError},
  {kHalErrInvalidParam,     kPlErrInvalidParam},
  {kHalErrInvalidState,     kPlErrInvalidState},
  {kHalErrInvalidOperation, kPlErrInvalidOperation},
  {kHalErrLock,             kPlErrLock},
  {kHalErrUnlock,           kPlErrUnlock},
  {kHalErrAlready,          kPlErrAlready},
  {kHalErrNotFound,         kPlErrNotFound},
  {kHalErrNoSupported,      kPlErrNoSupported},
  {kHalErrMemory,           kPlErrMemory},
  {kHalErrInternal,         kPlErrInternal},
  {kHalErrConfig,           kPlErrConfig},
  {kHalErrInvalidValue,     kPlErrInvalidValue},
  {kHalErrHandler,          kPlErrHandler},
  {kHalErrIrq,              kPlErrIrq},
  {kHalErrCallback,         kPlErrCallback},
  {kHalThreadError,         kPlThreadError},
  {kHalErrOpen,             kPlErrOpen},
  {kHalErrInputDirection,   kPlErrInputDirection},
  {-1,                      kPlErrClose},
  {-1,                      kPlErrDevice},
  {-1,                      kPlErrMagicCode},
  {-1,                      kPlErrBufferOverflow},
  {-1,                      kPlErrWrite},
};

// Macros ---------------------------------------------------------------------
#define INVALID_VALUE       (0xDEADDEAD)
#define IOEXP_HANDLE_NUM    (2)
#define IOEXP_VALUE_NUM     (IOEXP_HANDLE_NUM)

// Global Variables -----------------------------------------------------------

// Local Variables ------------------------------------------------------------
static const PlLedHwInfo s_led_config = {      // LED configuration
  .leds_num = CONFIG_PL_LED_LEDS_NUM,
  .leds = {
    {
      .led_id     = CONFIG_PL_LED_LED0_ID,
      .colors_num = CONFIG_PL_LED_COLORS_NUM,
      .colors = {
        {
          .color_id = CONFIG_PL_LED_COLOR0_ID  // Red
        },
        {
          .color_id = CONFIG_PL_LED_COLOR1_ID  // Green
        },
        {
          .color_id = CONFIG_PL_LED_COLOR2_ID  // Orange (Red + Green)
        }
      }
    },
    {
      .led_id = CONFIG_PL_LED_LED1_ID,
      .colors_num = CONFIG_PL_LED_COLORS_NUM,
      .colors = {
        {
          .color_id = CONFIG_PL_LED_COLOR0_ID
        },
        {
          .color_id = CONFIG_PL_LED_COLOR1_ID
        },
        {
          .color_id = CONFIG_PL_LED_COLOR2_ID
        }
      }
    },
    {
      .led_id = CONFIG_PL_LED_LED2_ID,
      .colors_num = CONFIG_PL_LED_COLORS_NUM,
      .colors = {
        {
          .color_id = CONFIG_PL_LED_COLOR0_ID
        },
        {
          .color_id = CONFIG_PL_LED_COLOR1_ID
        },
        {
          .color_id = CONFIG_PL_LED_COLOR2_ID
        }
      }
    }
  },
};

static bool                   s_initialized = false;
static struct LedHwCtrlInfo   s_led_ctrl[CONFIG_PL_LED_LEDS_NUM] = {0};
static pthread_mutex_t        s_mutex = PTHREAD_MUTEX_INITIALIZER;

// Local functions -------------------------------------------------------------
static uint32_t  FindIndexOfLedId(uint32_t led_id);
static PlErrCode HalErrToPlErr(HalErrCode ret_hal);

// Functions -------------------------------------------------------------------
PlErrCode PlLedHwOn(uint32_t led_id, uint32_t color_id) {
  LOG_T("[IN] led_id:%u, color_id:%u", led_id, color_id);
  uint32_t led_idx = FindIndexOfLedId(led_id);
  if ((led_idx == INVALID_VALUE) || (led_idx >= CONFIG_PL_LED_LEDS_NUM)) {
    LOG_E(0x00, "Invalid led_id:%u", led_id);
    return kPlErrInvalidParam;
  }

  struct LedHwCtrlInfo *ctrl_info = &s_led_ctrl[led_idx];
  HalErrCode ret_hal = kHalErrCodeOk;
  HalIoexpHandle handle_array[IOEXP_HANDLE_NUM] = {
    ctrl_info->ioexp_handle[kIoexpIdx0], ctrl_info->ioexp_handle[kIoexpIdx1] };
  HalIoexpValue value_array[IOEXP_VALUE_NUM];
  switch (color_id) {
    case CONFIG_PL_LED_COLOR0_ID:   // Red
      value_array[0] = kHalIoexpValueHigh;
      value_array[1] = kHalIoexpValueLow;
      ret_hal = HalIoexpWriteMulti(handle_array, IOEXP_HANDLE_NUM,
                                   value_array, IOEXP_VALUE_NUM);
      break;
    case CONFIG_PL_LED_COLOR1_ID:   // Green
      value_array[0] = kHalIoexpValueLow;
      value_array[1] = kHalIoexpValueHigh;
      ret_hal = HalIoexpWriteMulti(handle_array, IOEXP_HANDLE_NUM,
                                   value_array, IOEXP_VALUE_NUM);
      break;
    case CONFIG_PL_LED_COLOR2_ID:   // Orange (Red + Green)
      value_array[0] = kHalIoexpValueHigh;
      value_array[1] = kHalIoexpValueHigh;
      ret_hal = HalIoexpWriteMulti(handle_array, IOEXP_HANDLE_NUM,
                                   value_array, IOEXP_VALUE_NUM);
      break;
    case CONFIG_PL_LED_COLOR_OFF_ID:   // OFF
      value_array[0] = kHalIoexpValueLow;
      value_array[1] = kHalIoexpValueLow;
      ret_hal = HalIoexpWriteMulti(handle_array, IOEXP_HANDLE_NUM,
                                   value_array, IOEXP_VALUE_NUM);
      break;
    default:
      LOG_E(0x01, "color_id=%u is invalid.", color_id);
      ret_hal = kHalErrInvalidParam;
      break;
  }

  if (ret_hal == kHalErrCodeOk) {
    ctrl_info->color_id = color_id;
  }

  LOG_T("[OUT] ret=%u", HalErrToPlErr(ret_hal));
  return HalErrToPlErr(ret_hal);
}

// -----------------------------------------------------------------------------
PlErrCode PlLedHwOff(uint32_t led_id) {
  uint32_t led_idx = FindIndexOfLedId(led_id);
  if ((led_idx == INVALID_VALUE) || (led_idx >= CONFIG_PL_LED_LEDS_NUM)) {
    LOG_E(0x02, "Invalid led_id:%u", led_id);
    return kPlErrInvalidParam;
  }
  struct LedHwCtrlInfo *ctrl_info = &s_led_ctrl[led_idx];
  LOG_T("[IN] led_id:%u, led_idx:%u, color_id:%u",
        led_id, led_idx, ctrl_info->color_id);

  HalIoexpHandle handle_array[IOEXP_HANDLE_NUM] = {
    ctrl_info->ioexp_handle[kIoexpIdx0], ctrl_info->ioexp_handle[kIoexpIdx1] };
  HalIoexpValue value_array[IOEXP_VALUE_NUM] = {
      kHalIoexpValueLow, kHalIoexpValueLow };
  HalErrCode ret_hal = HalIoexpWriteMulti(handle_array, IOEXP_HANDLE_NUM,
                                          value_array, IOEXP_VALUE_NUM);

  LOG_T("[OUT] ret=%u", HalErrToPlErr(ret_hal));
  return HalErrToPlErr(ret_hal);
}

// -----------------------------------------------------------------------------
PlErrCode PlLedHwGetInfo(PlLedHwInfo *dev_info) {
  LOG_T("[IN]");
  int ret_os = pthread_mutex_lock(&s_mutex);
  if (ret_os) {
    LOG_E(0x03, "pthread_mutex_lock() fail:%d", ret_os);
    return kPlErrLock;
  }

  PlErrCode ret = kPlErrCodeOk;
  if (s_initialized == false) {
    LOG_E(0x04, "LED is not initialized.");
    ret = kPlErrInvalidState;
    goto exit_to_unlock_mutex;
  }
  if (dev_info == NULL) {
    LOG_E(0x05, "dev_info is NULL.");
    ret = kPlErrInvalidParam;
    goto exit_to_unlock_mutex;
  }

  memcpy(dev_info, &s_led_config, sizeof(PlLedHwInfo));

exit_to_unlock_mutex:
  ret_os = pthread_mutex_unlock(&s_mutex);
  if (ret_os) {
    LOG_E(0x06, "pthread_mutex_unlock() fail:%d", ret_os);
  }
  LOG_T("[OUT] ret=%u", ret);
  return ret;
}

// -----------------------------------------------------------------------------
PlErrCode PlLedHwInitialize(void) {
  LOG_T("[IN]");
  int ret_os = pthread_mutex_lock(&s_mutex);
  if (ret_os) {
    LOG_E(0x07, "pthread_mutex_lock() fail:%d", ret_os);
    return kPlErrLock;
  }
  PlErrCode ret = kPlErrCodeOk;
  if (s_initialized) {
    LOG_E(0x08, "LED is already initialized.");
    ret = kPlErrInvalidState;
    goto exit_to_unlock_mutex;
  }
  // Clear control info of 3 LEDs.
  memset(&s_led_ctrl[0], 0,
          sizeof(struct LedHwCtrlInfo) * CONFIG_PL_LED_LEDS_NUM);

  for (uint32_t i_leds = 0; i_leds < CONFIG_PL_LED_LEDS_NUM; i_leds++) {
    struct LedHwCtrlInfo *ctrl_info = &s_led_ctrl[i_leds];
    ctrl_info->led_id = s_led_config.leds[i_leds].led_id;
    // Configure IOEXP ID of LED Red and Green
    switch (i_leds) {
      case 0:
        ctrl_info->ioexp_id[kIoexpIdx0] = CONFIG_PL_LED_LED0_RED;
        ctrl_info->ioexp_id[kIoexpIdx1] = CONFIG_PL_LED_LED0_GREEN;
        break;
      case 1:
        ctrl_info->ioexp_id[kIoexpIdx0] = CONFIG_PL_LED_LED1_RED;
        ctrl_info->ioexp_id[kIoexpIdx1] = CONFIG_PL_LED_LED1_GREEN;
        break;
      case 2:
        ctrl_info->ioexp_id[kIoexpIdx0] = CONFIG_PL_LED_LED2_RED;
        ctrl_info->ioexp_id[kIoexpIdx1] = CONFIG_PL_LED_LED2_GREEN;
        break;
      // no need default.
    }
    struct HalIoexpConfig ioexp_config = {0};
    ioexp_config.direction = kHalIoexpDirectionOutput;
    for (uint32_t i_ioexp = 0; i_ioexp < kIoexpIdxMax; i_ioexp++) {
      HalErrCode ret_hal = kHalErrCodeOk;
      ret_hal = HalIoexpOpen(ctrl_info->ioexp_id[i_ioexp],
                     &ctrl_info->ioexp_handle[i_ioexp]);
      if (ret_hal != kHalErrCodeOk) {
        LOG_E(0x0A, "HalIoexpOpen() fail:%u", ret_hal);
        goto exit_to_unlock_mutex;
      }
      ret_hal = HalIoexpSetConfigure(
                     ctrl_info->ioexp_handle[i_ioexp],
                     &ioexp_config);
      if (ret_hal != kHalErrCodeOk) {
        LOG_E(0x0B, "HalIoexpSetConfigure() fail:%u", ret_hal);
        goto exit_to_unlock_mutex;
      }
    }
  }
  s_initialized = true;

exit_to_unlock_mutex:
  ret_os = pthread_mutex_unlock(&s_mutex);
  if (ret_os) {
    LOG_E(0x0C, "pthread_mutex_unlock() fail:%d", ret_os);
  }
  LOG_T("[OUT] ret=%u", ret);
  return ret;
}

// -----------------------------------------------------------------------------
PlErrCode PlLedHwFinalize(void) {
  LOG_T("[IN]");
  int ret_os = pthread_mutex_lock(&s_mutex);
  if (ret_os) {
    LOG_E(0x0D, "pthread_mutex_lock() fail:%d", ret_os);
    return kPlErrLock;
  }
  PlErrCode ret = kPlErrCodeOk;
  if (s_initialized == false) {
    LOG_E(0x0E, "LED is already finalized.");
    ret = kPlErrInvalidState;
    goto exit_to_unlock_mutex;
  }

  for (uint32_t i_leds = 0; i_leds < CONFIG_PL_LED_LEDS_NUM; i_leds++) {
    // Release IOEXP handles

    struct LedHwCtrlInfo *ctrl_info = &s_led_ctrl[i_leds];
    for (uint32_t i_ioexp = 0; i_ioexp < kIoexpIdxMax; i_ioexp++) {
      if (ctrl_info->ioexp_handle[i_ioexp]) {
        HalErrCode ret_hal = kHalErrCodeOk;
        ret_hal = HalIoexpClose(ctrl_info->ioexp_handle[i_ioexp]);
        if (ret_hal != kHalErrCodeOk) {
          LOG_E(0x0F, "HalIoexpClose() fail:%u", ret_hal);
          // Continue with the termination process.
        }
        ctrl_info->ioexp_handle[i_ioexp] = NULL;
      }
    }
    memset(ctrl_info, 0, sizeof(*ctrl_info));
  }
  s_initialized = false;

exit_to_unlock_mutex:
  ret_os = pthread_mutex_unlock(&s_mutex);
  if (ret_os) {
    LOG_E(0x10, "pthread_mutex_unlock() fail:%d", ret_os);
  }
  LOG_T("[OUT] ret=%u", ret);
  return ret;
}

// -----------------------------------------------------------------------------
// Local functions
// -----------------------------------------------------------------------------
static uint32_t FindIndexOfLedId(uint32_t led_id) {
  // led_id does not have to be a consecutive value from 0 to 2.
  // To do this, this function searches for a matching led_id
  // in s_led_info[] and returns the index.
  for (uint32_t i_leds = 0; i_leds < CONFIG_PL_LED_LEDS_NUM; i_leds++) {
    if (s_led_config.leds[i_leds].led_id == led_id) {
      return i_leds;
    }
  }
  LOG_E(0x11, "led_id(%u) is not registered in s_led_config.", led_id);
  return INVALID_VALUE;
}

// -----------------------------------------------------------------------------
static PlErrCode HalErrToPlErr(HalErrCode ret_hal) {
  for (int16_t idx_hal = 0; idx_hal < kPlErrCodeMax; idx_hal++) {
    if (ret_hal == s_conv_err_code[idx_hal].hal_err_code) {
      PlErrCode ret = s_conv_err_code[idx_hal].pl_err_code;
      if (ret < 0) {
        ret = kPlErrInternal;
      }
      return ret;
    }
  }
  return kPlErrInternal;
}
