/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#include "power_manager.h"

#include <errno.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#ifdef CONFIG_EXTERNAL_POWER_MANAGER_WDT_DUMP_ENABLE
#include <nuttx/arch.h>
#endif

#if !defined(__NuttX__)
/* Wrapping up_puts() */
#include "internal/compatibility.h"
#endif  // !defined(__NuttX__)

#include "led_manager.h"
#include "main.h"
#include "parameter_storage_manager.h"
#include "pl_power_manager.h"
#include "pl_system_control.h"
#include "pl_wdt.h"
#include "power_manager/power_manager_internal.h"
#include "utility_msg.h"
#include "utility_timer.h"

#include "utility_log.h"
#include "utility_log_module_id.h"
// Macros ----------------------------------------------------------------------
#define EVENT_ID 0x8C00
#define EVENT_ID_START 0x0000
#define LOG_ERR(event_id, format, ...)                                    \
  WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:" format, __FILE__, __LINE__, \
                   ##__VA_ARGS__);                                        \
  WRITE_ELOG_ERROR(MODULE_ID_SYSTEM,                                      \
                   (EVENT_ID | (0x00FF & (EVENT_ID_START + event_id))));

#define LOG_WARN(event_id, format, ...)                                  \
  WRITE_DLOG_WARN(MODULE_ID_SYSTEM, "%s-%d:" format, __FILE__, __LINE__, \
                  ##__VA_ARGS__);                                        \
  WRITE_ELOG_WARN(MODULE_ID_SYSTEM,                                      \
                  (EVENT_ID | (0x00FF & (EVENT_ID_START + event_id))));

#define LOG_INFO(format, ...)                                            \
  WRITE_DLOG_INFO(MODULE_ID_SYSTEM, "%s-%d:" format, __FILE__, __LINE__, \
                  ##__VA_ARGS__);

#define LOG_DBG(format, ...)                                              \
  WRITE_DLOG_DEBUG(MODULE_ID_SYSTEM, "%s-%d:" format, __FILE__, __LINE__, \
                   ##__VA_ARGS__);

#define LOG_TRACE(format, ...)                                            \
  WRITE_DLOG_TRACE(MODULE_ID_SYSTEM, "%s-%d:" format, __FILE__, __LINE__, \
                   ##__VA_ARGS__);

#if (ESF_POWER_MANAGER_EXCEPTION_INFO_SIZE != PL_SYSTEMCTL_EXCEPTION_INFO_SIZE)
#error \
    "The values of ESF_POWER_MANAGER_EXCEPTION_INFO_SIZE and " \
        "PL_SYSTEMCTL_EXCEPTION_INFO_SIZE are different."
#endif

struct EsfPwrMgrExceptionInfo {
  struct PlSystemCtlExceptionInfo pl_info;
};

#ifndef CONFIG_EXTERNAL_POWER_MANAGER_DISABLE
// Local functions ------------------------------------------------------------
static EsfPwrMgrError CommonMutexLock(int32_t timeout_msec);
static EsfPwrMgrError CommonMutexUnlock(void);
static void HoursMeterCallBack(void *cb_params);
static EsfPwrMgrError InitHoursMeter(void);
static EsfPwrMgrError DeinitHoursMeter(void);
static EsfPwrMgrError SaveHoursMeter(const EsfPwrMgrHoursMeter *data);
static EsfPwrMgrError LoadHoursMeter(EsfPwrMgrHoursMeter *data);
static bool HoursMeterStructMaskEnabled(EsfParameterStorageManagerMask mask);
static EsfPwrMgrError InitStorage(void);
static EsfPwrMgrError DeinitStorage(void);
static void WdtCallBack(void *private_data);
static EsfPwrMgrError InitWdt(void);
static EsfPwrMgrError DeinitWdt(void);
static void NotifyReboot(char *str);
#ifdef CONFIG_EXTERNAL_POWER_MANAGER_USB_CURRENT_LIMIT_ENABLE
static EsfPwrMgrSupplyType ConvertGetSupplyType(PlPowerMgrSupplyType pl_type);
#endif
static EsfPwrMgrResetCause ConvertResetCause(PlSystemCtlResetCause pl_cause);

// Global Variables -----------------------------------------------------------
static const EsfParameterStorageManagerMemberInfo kPwrMgrMembersInfo[] = {{
    .id = kEsfParameterStorageManagerItemHoursMeter,
    .type = kEsfParameterStorageManagerItemTypeString,
    .offset = 0,
    .size = ESF_PARAMETER_STORAGE_MANAGER_STRING_SIZEOF(EsfPwrMgrHoursMeter,
                                                        hours_meter_str),
    .enabled = HoursMeterStructMaskEnabled,
    .custom = NULL,
}};

static const EsfParameterStorageManagerStructInfo kPwrMgrStructInfo = {
    .items_num = sizeof(kPwrMgrMembersInfo) / sizeof(kPwrMgrMembersInfo[0]),
    .items = kPwrMgrMembersInfo,
};

static EsfPwrMgrResource s_resource = {
    ESF_PARAMETER_STORAGE_MANAGER_INVALID_HANDLE, NULL, 0};

static EsfPwrMgrStatus s_status = kEsfPwrMgrStatusStop;
static pthread_mutex_t s_mutex = PTHREAD_MUTEX_INITIALIZER;

static struct EsfPwrMgrExceptionInfo *s_exception_info = NULL;

// Functions ------------------------------------------------------------------
static EsfPwrMgrError CommonMutexLock(int32_t timeout_msec) {
  LOG_TRACE("func start");
  EsfPwrMgrError esf_ret = kEsfPwrMgrOk;

  struct timespec timeout;
  clock_gettime(CLOCK_REALTIME, &timeout);

  // Convert timeout_ms from milliseconds to seconds and nanoseconds and add.
  timeout.tv_sec += timeout_msec / 1000;
  timeout.tv_nsec += (timeout_msec % 1000) * 1000000;

  // Nanoseconds exceeding 1000000000 nanoseconds(= 1 seconds).
  if (timeout.tv_nsec >= 1000000000) {
    uint32_t sec = timeout.tv_nsec / 1000000000;
    timeout.tv_sec += sec;
    timeout.tv_nsec -= (sec * 1000000000);
  }

  int ret = pthread_mutex_timedlock(&s_mutex, &timeout);
  if (ret == ETIMEDOUT) {
    LOG_ERR(kEsfPwrMgrElogErrorId0x02EsfPwrMgr, "mutex lock timed out.");
    esf_ret = kEsfPwrMgrErrorTimeout;
    goto fin;
  } else if (ret != 0) {
    LOG_ERR(kEsfPwrMgrElogErrorId0x03EsfPwrMgr,
            "pthread_mutex_timedlock ret=%d. %s", ret, strerror(ret));
    esf_ret = kEsfPwrMgrErrorInternal;
    goto fin;
  } else {
    // do nothing.
  }

fin:
  LOG_TRACE("func end");
  return esf_ret;
}

// ----------------------------------------------------------------------------
static EsfPwrMgrError CommonMutexUnlock(void) {
  LOG_TRACE("func start");
  EsfPwrMgrError esf_ret = kEsfPwrMgrOk;

  int ret = pthread_mutex_unlock(&s_mutex);
  if (ret != 0) {
    LOG_ERR(kEsfPwrMgrElogErrorId0x04EsfPwrMgr,
            "pthread_mutex_unlock ret=%d. %s", ret, strerror(ret));
    esf_ret = kEsfPwrMgrErrorInternal;
    goto fin;
  }

fin:
  LOG_TRACE("func end");
  return esf_ret;
}

// ----------------------------------------------------------------------------
static void HoursMeterCallBack(void *cb_params) {
  LOG_TRACE("func start");
  (void)cb_params;

  // Hours Meter Update
  if (s_resource.hours_meter < INT32_MAX) {
    ++s_resource.hours_meter;
  } else {
    s_resource.hours_meter = 0;
  }

  LOG_DBG("s_resource.hours_meter=[%d]", s_resource.hours_meter);

  EsfPwrMgrHoursMeter data = {};
  snprintf((char *)data.hours_meter_str, sizeof(data.hours_meter_str), "%d",
           s_resource.hours_meter);
  EsfPwrMgrError ret = SaveHoursMeter(&data);
  if (ret != kEsfPwrMgrOk) {
    LOG_WARN(kEsfPwrMgrElogWarningId0x80EsfPwrMgr,
             "SaveHoursMeter error. ret=%d", ret);
  }

  LOG_TRACE("func end");
}

// ----------------------------------------------------------------------------
static EsfPwrMgrError InitHoursMeter(void) {
  LOG_TRACE("func start");
  EsfPwrMgrError ret = kEsfPwrMgrOk;

  // Get the current value of HoursMeter using ParameterStorageManager.
  // Continue processing as a Warning.
  EsfPwrMgrHoursMeter data;
  ret = LoadHoursMeter(&data);
  if (ret != kEsfPwrMgrOk) {
    LOG_WARN(kEsfPwrMgrElogWarningId0x83EsfPwrMgr,
             "LoadHoursMeter failed. ret=%d. Continue process", ret);
    data.hours_meter_str[0] = '0';
    data.hours_meter_str[1] = '\0';
    ret = kEsfPwrMgrOk;
  }

  char *endp;
  int32_t hour = strtol(data.hours_meter_str, &endp, 10);
  if ((data.hours_meter_str[0] == '\0') || (*endp != '\0')) {
    LOG_WARN(kEsfPwrMgrElogWarningId0x81EsfPwrMgr, "strtol convert failure %s",
             data.hours_meter_str);
    s_resource.hours_meter = 0;
  } else {
    s_resource.hours_meter = hour;
  }
  LOG_DBG("s_resource.hours_meter=[%d]", s_resource.hours_meter);

  UtilityTimerErrCode utility_ret =
      UtilityTimerCreate(HoursMeterCallBack, NULL, &s_resource.timer_handle);
  if (utility_ret != kUtilityTimerOk) {
    LOG_ERR(kEsfPwrMgrElogErrorId0x20UtlTimer,
            "UtilityTimerCreate error. ret=%d", utility_ret);
    ret = kEsfPwrMgrErrorExternal;
    goto err;
  }

  struct timespec interval_ts;
  interval_ts.tv_sec = ESF_POWER_MANAGER_HOURS_METER_ADD_INTERVAL * 3600;  // 1h
  interval_ts.tv_nsec = 0;

  utility_ret = UtilityTimerStart(s_resource.timer_handle, &interval_ts,
                                  kUtilityTimerRepeat);
  if (utility_ret != kUtilityTimerOk) {
    LOG_ERR(kEsfPwrMgrElogErrorId0x21UtlTimer,
            "UtilityTimerStart error. ret=%d", utility_ret);
    ret = kEsfPwrMgrErrorExternal;
    goto err;
  }

  LOG_TRACE("func end");
  return ret;

err:
  if (DeinitHoursMeter() != kEsfPwrMgrOk) {
    LOG_ERR(kEsfPwrMgrElogErrorId0x06EsfPwrMgr, "DeinitHoursMeter error.");
    // Log output only.
  }
  LOG_TRACE("func end");
  return ret;
}

// ----------------------------------------------------------------------------
static EsfPwrMgrError DeinitHoursMeter(void) {
  LOG_TRACE("func start");
  EsfPwrMgrError ret = kEsfPwrMgrOk;

  UtilityTimerErrCode utility_ret = UtilityTimerStop(s_resource.timer_handle);
  if (utility_ret != kUtilityTimerOk) {
    LOG_ERR(kEsfPwrMgrElogErrorId0x22UtlTimer, "UtilityTimerStop error. ret=%d",
            utility_ret);
    ret = kEsfPwrMgrErrorExternal;
  }

  utility_ret = UtilityTimerDelete(s_resource.timer_handle);
  if (utility_ret != kUtilityTimerOk) {
    LOG_ERR(kEsfPwrMgrElogErrorId0x23UtlTimer,
            "UtilityTimerDelete error. ret=%d", utility_ret);
    ret = kEsfPwrMgrErrorExternal;
  }

  if (utility_ret == kUtilityTimerOk) {
    s_resource.timer_handle = NULL;
  }

  LOG_TRACE("func end");
  return ret;
}

// ----------------------------------------------------------------------------
static EsfPwrMgrError SaveHoursMeter(const EsfPwrMgrHoursMeter *data) {
  LOG_TRACE("func start");
  EsfPwrMgrError ret = kEsfPwrMgrOk;
  EsfPwrMgrHoursMeterMask mask = {1};

  EsfParameterStorageManagerStatus storage_ret =
      kEsfParameterStorageManagerStatusOk;

  storage_ret = EsfParameterStorageManagerSave(
      s_resource.storage_handle, (EsfParameterStorageManagerMask)&mask,
      (EsfParameterStorageManagerData)data, &kPwrMgrStructInfo, NULL);
  if (storage_ret != kEsfParameterStorageManagerStatusOk) {
    LOG_ERR(kEsfPwrMgrElogErrorId0x30EsfPSM,
            "EsfParameterStorageManagerSave error. ret=%d", storage_ret);
    ret = kEsfPwrMgrErrorExternal;
    goto fin;
  }

fin:
  LOG_TRACE("func end");
  return ret;
}

// ----------------------------------------------------------------------------
static EsfPwrMgrError LoadHoursMeter(EsfPwrMgrHoursMeter *data) {
  LOG_TRACE("func start");
  EsfPwrMgrError ret = kEsfPwrMgrOk;
  EsfPwrMgrHoursMeterMask mask = {1};

  EsfParameterStorageManagerStatus storage_ret =
      kEsfParameterStorageManagerStatusOk;
  storage_ret = EsfParameterStorageManagerLoad(
      s_resource.storage_handle, (EsfParameterStorageManagerMask)&mask,
      (EsfParameterStorageManagerData)data, &kPwrMgrStructInfo, NULL);
  if (storage_ret != kEsfParameterStorageManagerStatusOk) {
    LOG_ERR(kEsfPwrMgrElogErrorId0x31EsfPSM,
            "EsfParameterStorageManagerLoad error. ret=%d", storage_ret);
    ret = kEsfPwrMgrErrorExternal;
    goto fin;
  }

fin:
  LOG_TRACE("func end");
  return ret;
}

// ----------------------------------------------------------------------------
static bool HoursMeterStructMaskEnabled(EsfParameterStorageManagerMask mask) {
  // false : 0 == EsfPwrMgrHoursMeterMask.hours_meter_str
  // true  : other
  return ESF_PARAMETER_STORAGE_MANAGER_MASK_IS_ENABLED(EsfPwrMgrHoursMeterMask,
                                                       hours_meter, mask);
}

// ----------------------------------------------------------------------------
static EsfPwrMgrError InitStorage(void) {
  LOG_TRACE("func start");
  EsfPwrMgrError ret = kEsfPwrMgrOk;

  EsfParameterStorageManagerStatus storage_ret =
      kEsfParameterStorageManagerStatusOk;
  storage_ret = EsfParameterStorageManagerOpen(&s_resource.storage_handle);
  if (storage_ret != kEsfParameterStorageManagerStatusOk) {
    LOG_ERR(kEsfPwrMgrElogErrorId0x32EsfPSM,
            "EsfParameterStorageManagerOpen error. ret=%d", storage_ret);
    ret = kEsfPwrMgrErrorExternal;
    goto fin;
  }

fin:
  LOG_TRACE("func end");
  return ret;
}

// ----------------------------------------------------------------------------
static EsfPwrMgrError DeinitStorage(void) {
  LOG_TRACE("func start");
  EsfPwrMgrError ret = kEsfPwrMgrOk;

  EsfParameterStorageManagerStatus storage_ret =
      kEsfParameterStorageManagerStatusOk;
  storage_ret = EsfParameterStorageManagerClose(s_resource.storage_handle);
  if (storage_ret != kEsfParameterStorageManagerStatusOk) {
    LOG_ERR(kEsfPwrMgrElogErrorId0x33EsfPSM,
            "EsfParameterStorageManagerOpen error. ret=%d", storage_ret);
    ret = kEsfPwrMgrErrorExternal;
    goto fin;
  }

fin:
  LOG_TRACE("func end");
  return ret;
}

// ----------------------------------------------------------------------------
static void WdtCallBack(void *private_data) {
  (void)private_data;
  uint32_t buf_size = 128;
  char _buf[buf_size];
  snprintf(_buf, buf_size, "[power_manager]WdtCallback start\n");
  up_puts(_buf);

  PlSystemCtlSetExceptionInfo();

#ifdef CONFIG_EXTERNAL_POWER_MANAGER_WDT_DUMP_ENABLE
  PlSystemCtlDumpAllStack();
#endif  // CONFIG_EXTERNAL_POWER_MANAGER_WDT_DUMP_ENABLE

  snprintf(_buf, buf_size, "[power_manager]WdtCallback end.\n");
  up_puts(_buf);

  return;
}

// ----------------------------------------------------------------------------
static EsfPwrMgrError InitWdt(void) {
  LOG_TRACE("func start");
  EsfPwrMgrError ret = kEsfPwrMgrOk;
  PlErrCode pl_ret = kPlErrCodeOk;

  pl_ret = PlWdtInitialize();
  if (pl_ret != kPlErrCodeOk) {
    LOG_ERR(kEsfPwrMgrElogErrorId0x50PlWdt, "PlWdtInitialize error. ret=%d",
            pl_ret);
    ret = kEsfPwrMgrErrorExternal;
    goto fin;
  }

  pl_ret = PlWdtRegisterIrqHandler(WdtCallBack, NULL);
  if (pl_ret != kPlErrCodeOk) {
    LOG_ERR(kEsfPwrMgrElogErrorId0x51PlWdt,
            "PlWdtRegisterIrqHandler error. ret=%d", pl_ret);
    LOG_TRACE("func end");
    ret = kEsfPwrMgrErrorExternal;
    goto err;
  }

  pl_ret = PlWdtStart();
  if (pl_ret != kPlErrCodeOk) {
    LOG_ERR(kEsfPwrMgrElogErrorId0x52PlWdt, "PlWdtStart error. ret=%d", pl_ret);
    LOG_TRACE("func end");
    ret = kEsfPwrMgrErrorExternal;
    goto err;
  }

fin:
  LOG_TRACE("func end");
  return ret;

err:
  if (DeinitWdt() != kEsfPwrMgrOk) {
    LOG_ERR(kEsfPwrMgrElogErrorId0x07EsfPwrMgr, "DeinitWdt error.");
    // Log output only.
  }
  return ret;
}

// ----------------------------------------------------------------------------
static EsfPwrMgrError DeinitWdt(void) {
  LOG_TRACE("func start");
  EsfPwrMgrError ret = kEsfPwrMgrOk;
  PlErrCode pl_ret = kPlErrCodeOk;

  pl_ret = PlWdtStop();
  if (pl_ret != kPlErrCodeOk) {
    LOG_ERR(kEsfPwrMgrElogErrorId0x53PlWdt, "PlWdtStop error. ret=%d", pl_ret);
    ret = kEsfPwrMgrErrorExternal;
  }

  pl_ret = PlWdtUnregisterIrqHandler();
  if (pl_ret != kPlErrCodeOk) {
    LOG_ERR(kEsfPwrMgrElogErrorId0x54PlWdt,
            "PlWdtUnregisterIrqHandler error. ret=%d", pl_ret);
    ret = kEsfPwrMgrErrorExternal;
  }

  pl_ret = PlWdtFinalize();
  if (pl_ret != kPlErrCodeOk) {
    LOG_ERR(kEsfPwrMgrElogErrorId0x55PlWdt, "PlWdtFinalize error. ret=%d",
            pl_ret);
    ret = kEsfPwrMgrErrorExternal;
  }

  LOG_TRACE("func end");
  return ret;
}

// ----------------------------------------------------------------------------
static void NotifyReboot(char *str) {
  LOG_TRACE("func start");

  while (1) {
    LOG_WARN(kEsfPwrMgrElogWarningId0x82EsfPwrMgr,
             "Processing is stopped by %s. Please Reboot the device.", str);
    sleep(1);
  }
}

// ----------------------------------------------------------------------------
#ifdef CONFIG_EXTERNAL_POWER_MANAGER_USB_CURRENT_LIMIT_ENABLE
static EsfPwrMgrSupplyType ConvertGetSupplyType(PlPowerMgrSupplyType pl_type) {
  switch (pl_type) {
    case kPlPowerMgrSupplyTypePoE:
      return kEsfPwrMgrSupplyTypePoE;
    case kPlPowerMgrSupplyTypeBC12:
    case kPlPowerMgrSupplyTypeCC15A:
      return kEsfPwrMgrSupplyTypeUsb;
    default:
      return kEsfPwrMgrSupplyTypeMax;
  }
}
#endif
// ----------------------------------------------------------------------------
static EsfPwrMgrResetCause ConvertResetCause(PlSystemCtlResetCause pl_cause) {
  switch (pl_cause) {
    case kPlSystemCtlResetCauseSysChipPowerOnReset:
      return kEsfPwrMgrResetCauseSysChipPowerOnReset;
    case kPlSystemCtlResetCauseSysBrownOut:
      return kEsfPwrMgrResetCauseSysBrownOut;
    case kPlSystemCtlResetCauseCoreDeepSleep:
      return kEsfPwrMgrResetCauseCoreDeepSleep;
    case kPlSystemCtlResetCauseWDT:
      return kEsfPwrMgrResetCauseWDT;
    case kPlSystemCtlResetCauseCoreSoft:
      return kEsfPwrMgrResetCauseCoreSoft;
    default:
      return kEsfPwrMgrResetCauseUnknown;
  }
}
#endif  // !CONFIG_EXTERNAL_POWER_MANAGER_DISABLE

// ----------------------------------------------------------------------------
EsfPwrMgrError EsfPwrMgrStart(void) {
#ifdef CONFIG_EXTERNAL_POWER_MANAGER_DISABLE
  return kEsfPwrMgrOk;
#else
  LOG_TRACE("func start");
  EsfPwrMgrError ret = kEsfPwrMgrOk;

  ret = CommonMutexLock(CONFIG_EXTERNAL_POWER_MANAGER_LOCKTIME);
  if (ret != kEsfPwrMgrOk) {
    LOG_ERR(kEsfPwrMgrElogErrorId0x00EsfPwrMgrCommonMutexLock,
            "Mutex Lock error. ret=%d", ret);
    goto fin;
  }

  if (s_status == kEsfPwrMgrStatusStart) {
    LOG_INFO("PowerManager Aready Started.");
    // Log output only.
    goto unlock;
  } else if (s_status == kEsfPwrMgrStatusReboot) {
    LOG_INFO("PowerManager is Rebooting.");
    // Log output only.
    goto unlock;
  } else if (s_status == kEsfPwrMgrStatusShutdown) {
    LOG_INFO("PowerManager is Shuting down.");
    // Log output only.
    goto unlock;
  } else if (s_status == kEsfPwrMgrStatusWaitWDTIgnition) {
    LOG_ERR(kEsfPwrMgrElogErrorId0x08EsfPwrMgr,
            "Aready called EsfPwrMgrStopForReboot. Please wait WDT Ignition.");
    ret = kEsfPwrMgrErrorWaitReboot;
    goto unlock;
  }

  ret = InitWdt();
  if (ret != kEsfPwrMgrOk) {
    LOG_ERR(kEsfPwrMgrElogErrorId0x09EsfPwrMgr, "InitWdt error. ret=%d", ret);
    goto unlock;
  }

  PlErrCode pl_ret = PlPowerMgrInitialize();
  if (pl_ret != kPlErrCodeOk) {
    LOG_ERR(kEsfPwrMgrElogErrorId0x60PlPowerMgr,
            "PlPowerMgrInitialize error. ret=%d", pl_ret);
    ret = kEsfPwrMgrErrorInternal;
    goto pl_power_mgr_err;
  }

#ifdef CONFIG_EXTERNAL_POWER_MANAGER_USB_CURRENT_LIMIT_ENABLE
  PlPowerMgrSupplyType pl_supply_type;
  pl_ret = PlPowerMgrGetSupplyType(&pl_supply_type);
  if ((pl_ret != kPlErrCodeOk) ||
      (pl_supply_type == kPlPowerMgrSupplyTypeNotSupport)) {
    LOG_WARN(kEsfPwrMgrElogWarningId0x90PlPowerMgr,
             "PlPowerMgrGetSupplyType : pl_ret=%u", pl_ret);

    EsfLedManagerLedStatusInfo led_info = {
        .led = kEsfLedManagerTargetLedPower,
        .status = kEsfLedManagerLedStatusErrorLegacyUSB,
        .enabled = true};
    EsfLedManagerResult led_ret = EsfLedManagerSetStatus(&led_info);
    if (led_ret != kEsfLedManagerSuccess) {
      LOG_WARN(kEsfPwrMgrElogWarningId0xb0EsfLedMgr,
               "EsfLedManagerSetStatus : led_ret=%u", led_ret);
    }

    char err_log[] = "Not Supported Supply Type";
    // NotifyReboot() Does not return because it enters an infinite loop.
    NotifyReboot(err_log);
  }
  EsfPwrMgrSupplyType supply_type = ConvertGetSupplyType(pl_supply_type);
  LOG_DBG("EsfPwrMgrSupplyType : %d, PlPowerMgrSupplyType : %u", supply_type,
          pl_supply_type);

#ifdef CONFIG_EXTERNAL_POWER_MANAGER_USB_DEVICE_ENABLE
  pl_ret = PlPowerMgrSetupUsb();
  if (pl_ret != kPlErrCodeOk) {
    char err_log[] = "Setup USB Error";
    // NotifyReboot() Does not return because it enters an infinite loop.
    NotifyReboot(err_log);
  }
  LOG_DBG("USB SW_ON");
#endif  // CONFIG_EXTERNAL_POWER_MANAGER_USB_DEVICE_ENABLE
#endif  // CONFIG_EXTERNAL_POWER_MANAGER_USB_CURRENT_LIMIT_ENABLE
  ret = InitStorage();
  if (ret != kEsfPwrMgrOk) {
    LOG_ERR(kEsfPwrMgrElogErrorId0x0aEsfPwrMgr, "InitStorage error. ret=%d",
            ret);
    ret = kEsfPwrMgrErrorInternal;
    goto storage_err;
  }

  ret = InitHoursMeter();
  if (ret != kEsfPwrMgrOk) {
    LOG_ERR(kEsfPwrMgrElogErrorId0x0bEsfPwrMgr, "InitHoursMeter error. ret=%d",
            ret);
    ret = kEsfPwrMgrErrorInternal;
    goto hour_meter_err;
  }

  s_status = kEsfPwrMgrStatusStart;

  // Success case
  goto unlock;

hour_meter_err:
  DeinitStorage();

storage_err:
  PlPowerMgrFinalize();

pl_power_mgr_err:
  DeinitWdt();

unlock:
  if (CommonMutexUnlock() != kEsfPwrMgrOk) {
    LOG_ERR(kEsfPwrMgrElogErrorId0x01EsfPwrMgrCommonMutexUnlock,
            "Mutex Unlock error.");
  }

fin:
  LOG_TRACE("func end");
  return ret;
#endif  // CONFIG_EXTERNAL_POWER_MANAGER_DISABLE
}

// ----------------------------------------------------------------------------
EsfPwrMgrError EsfPwrMgrStop(void) {
#ifdef CONFIG_EXTERNAL_POWER_MANAGER_DISABLE
  return kEsfPwrMgrOk;
#else
  LOG_TRACE("func start");
  EsfPwrMgrError ret = kEsfPwrMgrOk;

  ret = CommonMutexLock(CONFIG_EXTERNAL_POWER_MANAGER_LOCKTIME);
  if (ret != kEsfPwrMgrOk) {
    LOG_ERR(kEsfPwrMgrElogErrorId0x00EsfPwrMgrCommonMutexLock,
            "Mutex Lock error. ret=%d", ret);
    goto fin;
  }

  if (s_status == kEsfPwrMgrStatusStop ||
      s_status == kEsfPwrMgrStatusWaitWDTIgnition) {
    LOG_INFO("PowerManager Aready Stoped.");
    ret = kEsfPwrMgrErrorStatus;
    goto unlock;
  }

  DeinitHoursMeter();

  DeinitStorage();

  PlPowerMgrFinalize();

  DeinitWdt();

  if (s_exception_info) {
    free(s_exception_info);
    s_exception_info = NULL;
  }

  s_status = kEsfPwrMgrStatusStop;

unlock:
  if (CommonMutexUnlock() != kEsfPwrMgrOk) {
    LOG_ERR(kEsfPwrMgrElogErrorId0x01EsfPwrMgrCommonMutexUnlock,
            "Mutex Unlock error.");
  }

fin:
  LOG_TRACE("func end");
  return ret;
#endif  // CONFIG_EXTERNAL_POWER_MANAGER_DISABLE
}

// ----------------------------------------------------------------------------
EsfPwrMgrError EsfPwrMgrStopForReboot(void) {
#ifdef CONFIG_EXTERNAL_POWER_MANAGER_DISABLE
  return kEsfPwrMgrOk;
#else
  LOG_TRACE("func start");
  EsfPwrMgrError ret = kEsfPwrMgrOk;

  ret = CommonMutexLock(CONFIG_EXTERNAL_POWER_MANAGER_LOCKTIME);
  if (ret != kEsfPwrMgrOk) {
    LOG_ERR(kEsfPwrMgrElogErrorId0x00EsfPwrMgrCommonMutexLock,
            "Mutex Lock error. ret=%u", ret);
    goto fin;
  }

  if (s_status == kEsfPwrMgrStatusStop ||
      s_status == kEsfPwrMgrStatusWaitWDTIgnition) {
    LOG_INFO("PowerManager Aready Stoped.");
    ret = kEsfPwrMgrErrorStatus;
    goto unlock;
  }

  PlErrCode pl_ret = PlWdtTerminate();
  if (pl_ret != kPlErrCodeOk) {
    LOG_ERR(kEsfPwrMgrElogErrorId0x56PlWdt, "PlWdtTerminate error. ret=%u",
            pl_ret);
    ret = kEsfPwrMgrErrorExternal;
    goto unlock;
  }

  // Deinit other than PL WDT.
  DeinitHoursMeter();

  DeinitStorage();

  PlPowerMgrFinalize();

  s_status = kEsfPwrMgrStatusWaitWDTIgnition;

unlock:
  if (CommonMutexUnlock() != kEsfPwrMgrOk) {
    LOG_ERR(kEsfPwrMgrElogErrorId0x01EsfPwrMgrCommonMutexUnlock,
            "Mutex Unlock error.");
  }

fin:
  LOG_TRACE("func end");
  return ret;
#endif  // CONFIG_EXTERNAL_POWER_MANAGER_DISABLE
}

// ----------------------------------------------------------------------------
EsfPwrMgrError EsfPwrMgrPrepareReboot(void) {
#ifdef CONFIG_EXTERNAL_POWER_MANAGER_DISABLE
  return kEsfPwrMgrOk;
#else
  LOG_TRACE("func start");
  EsfPwrMgrError ret = kEsfPwrMgrOk;

  ret = CommonMutexLock(CONFIG_EXTERNAL_POWER_MANAGER_LOCKTIME);
  if (ret != kEsfPwrMgrOk) {
    LOG_ERR(kEsfPwrMgrElogErrorId0x00EsfPwrMgrCommonMutexLock,
            "Mutex Lock error. ret=%d", ret);
    goto fin;
  }

  if (s_status == kEsfPwrMgrStatusStop ||
      s_status == kEsfPwrMgrStatusWaitWDTIgnition) {
    LOG_INFO("PowerManager Aready Stoped.");
    ret = kEsfPwrMgrErrorStatus;
    goto unlock;
  } else if (s_status == kEsfPwrMgrStatusReboot) {
    LOG_INFO("PowerManager is Rebooting.");
    ret = kEsfPwrMgrErrorAlreadyRunning;
    goto unlock;
  } else if (s_status == kEsfPwrMgrStatusShutdown) {
    LOG_INFO("PowerManager is Shuting down.");
    ret = kEsfPwrMgrErrorAlreadyRunning;
    goto unlock;
  }

  EsfMainError ret_main = EsfMainNotifyMsg(kEsfMainMsgTypeReboot);
  if (ret_main != kEsfMainOk) {
    LOG_ERR(kEsfPwrMgrElogErrorId0x40EsfMain, "EsfMainNotifyMsg error. ret=%d",
            ret_main);
    ret = kEsfPwrMgrErrorExternal;
    goto unlock;
  }
  s_status = kEsfPwrMgrStatusReboot;

unlock:
  if (CommonMutexUnlock() != kEsfPwrMgrOk) {
    LOG_ERR(kEsfPwrMgrElogErrorId0x01EsfPwrMgrCommonMutexUnlock,
            "Mutex Unlock error.");
  }

fin:
  LOG_TRACE("func end");
  return ret;
#endif  // CONFIG_EXTERNAL_POWER_MANAGER_DISABLE
}

// ----------------------------------------------------------------------------
void EsfPwrMgrExecuteRebootEx(EsfPwrMgrRebootType reboot_type) {
#ifdef CONFIG_EXTERNAL_POWER_MANAGER_DISABLE
  return;
#else
  LOG_TRACE("func start");

  PlSystemCtlOperation reboot_operation = kPlSystemCtlRebootCpu;
  if (reboot_type == EsfPwrMgrRebootTypeSW) {
    reboot_operation = kPlSystemCtlRebootCpu;
  } else if (reboot_type == EsfPwrMgrRebootTypeHW) {
    reboot_operation = kPlSystemCtlRebootEdgeDevice;
  } else {
    // Do nothing
  }

  PlErrCode ret_pl = PlSystemCtlExecOperation(reboot_operation);
  if (ret_pl != kPlErrCodeOk) {
    LOG_WARN(kEsfPwrMgrElogWarningId0xa0PlSystemCtl,
             "PlSystemCtlExecOperation error. ret=%d", ret_pl);

    char err_log[] = "Reboot Error";
    // NotifyReboot() Does not return because it enters an infinite loop.
    NotifyReboot(err_log);
  }

  LOG_TRACE("func end");
  return;
#endif  // CONFIG_EXTERNAL_POWER_MANAGER_DISABLE
}

// TODO:Scheduled for deletion
void EsfPwrMgrExecuteReboot(void) {
#ifdef CONFIG_EXTERNAL_POWER_MANAGER_DISABLE
  return;
#else
  LOG_TRACE("func start");

#ifdef CONFIG_EXTERNAL_TARGET_T3P
  PlSystemCtlOperation reboot_operation = kPlSystemCtlRebootCpu;
#else
  PlSystemCtlOperation reboot_operation = kPlSystemCtlRebootEdgeDevice;
#endif

  PlErrCode ret_pl = PlSystemCtlExecOperation(reboot_operation);
  if (ret_pl != kPlErrCodeOk) {
    LOG_WARN(kEsfPwrMgrElogWarningId0xa0PlSystemCtl,
             "PlSystemCtlExecOperation error. ret=%d", ret_pl);

    char err_log[] = "Reboot Error";
    // NotifyReboot() Does not return because it enters an infinite loop.
    NotifyReboot(err_log);
  }

  LOG_TRACE("func end");
  return;
#endif  // CONFIG_EXTERNAL_POWER_MANAGER_DISABLE
}

// ----------------------------------------------------------------------------
EsfPwrMgrError EsfPwrMgrPrepareShutdown(void) {
#ifdef CONFIG_EXTERNAL_POWER_MANAGER_DISABLE
  return kEsfPwrMgrOk;
#else
  LOG_TRACE("func start");
  EsfPwrMgrError ret = kEsfPwrMgrOk;

  ret = CommonMutexLock(CONFIG_EXTERNAL_POWER_MANAGER_LOCKTIME);
  if (ret != kEsfPwrMgrOk) {
    LOG_ERR(kEsfPwrMgrElogErrorId0x00EsfPwrMgrCommonMutexLock,
            "Mutex Lock error. ret=%d", ret);
    goto fin;
  }

  if (s_status == kEsfPwrMgrStatusStop ||
      s_status == kEsfPwrMgrStatusWaitWDTIgnition) {
    LOG_INFO("PowerManager Aready Stoped.");
    ret = kEsfPwrMgrErrorStatus;
    goto unlock;
  } else if (s_status == kEsfPwrMgrStatusReboot) {
    LOG_INFO("PowerManager is Rebooting.");
    ret = kEsfPwrMgrErrorAlreadyRunning;
    goto unlock;
  } else if (s_status == kEsfPwrMgrStatusShutdown) {
    LOG_INFO("PowerManager is Shuting down.");
    ret = kEsfPwrMgrErrorAlreadyRunning;
    goto unlock;
  }

  EsfMainError ret_main = EsfMainNotifyMsg(kEsfMainMsgTypeShutdown);
  if (ret_main != kEsfMainOk) {
    LOG_ERR(kEsfPwrMgrElogErrorId0x41EsfMain, "EsfMainNotifyMsg error. ret=%d",
            ret_main);
    ret = kEsfPwrMgrErrorExternal;
    goto unlock;
  }
  s_status = kEsfPwrMgrStatusShutdown;

unlock:
  if (CommonMutexUnlock() != kEsfPwrMgrOk) {
    LOG_ERR(kEsfPwrMgrElogErrorId0x01EsfPwrMgrCommonMutexUnlock,
            "Mutex Unlock error.");
  }

fin:
  LOG_TRACE("func end");
  return ret;
#endif  // CONFIG_EXTERNAL_POWER_MANAGER_DISABLE
}

// ----------------------------------------------------------------------------
void EsfPwrMgrExecuteShutdown(void) {
#ifdef CONFIG_EXTERNAL_POWER_MANAGER_DISABLE
  return;
#else
  LOG_TRACE("func start");

  PlErrCode ret_pl = PlSystemCtlExecOperation(kPlSystemCtlPowerOff);
  if (ret_pl != kPlErrCodeOk) {
    LOG_WARN(kEsfPwrMgrElogWarningId0xa1PlSystemCtl,
             "PlSystemCtlExecOperation error. ret=%d", ret_pl);

    char err_log[] = "Shutdown Error";
    // NotifyReboot() Does not return because it enters an infinite loop.
    NotifyReboot(err_log);
  }

  LOG_TRACE("func end");
  return;
#endif  // CONFIG_EXTERNAL_POWER_MANAGER_DISABLE
}

// ----------------------------------------------------------------------------
EsfPwrMgrError EsfPwrMgrGetVoltage(int32_t *voltage) {
#ifdef CONFIG_EXTERNAL_POWER_MANAGER_DISABLE
  return kEsfPwrMgrErrorUnsupportedApi;
#else
  LOG_TRACE("func start");
  EsfPwrMgrError ret = kEsfPwrMgrOk;

  (void)voltage;  // Avoid compiler warning
  LOG_ERR(kEsfPwrMgrElogErrorId0x0cEsfPwrMgr,
          "EsfPwrMgrGetVoltage not supported.");
  ret = kEsfPwrMgrErrorUnsupportedApi;

  LOG_TRACE("func end");
  return ret;
#endif  // CONFIG_EXTERNAL_POWER_MANAGER_DISABLE
}

// ----------------------------------------------------------------------------
EsfPwrMgrError EsfPwrMgrHoursMeterGetValue(int32_t *hours) {
#ifdef CONFIG_EXTERNAL_POWER_MANAGER_DISABLE
  *hours = -1;
  return kEsfPwrMgrOk;
#else
  LOG_TRACE("func start");
  EsfPwrMgrError ret = kEsfPwrMgrOk;

  ret = CommonMutexLock(CONFIG_EXTERNAL_POWER_MANAGER_LOCKTIME);
  if (ret != kEsfPwrMgrOk) {
    LOG_ERR(kEsfPwrMgrElogErrorId0x00EsfPwrMgrCommonMutexLock,
            "Mutex Lock error. ret=%d", ret);
    goto fin;
  }

  if (hours == NULL) {
    ret = kEsfPwrMgrErrorInvalidArgument;
    LOG_ERR(kEsfPwrMgrElogErrorId0x0dEsfPwrMgr, "hours is NULL. ret=%u", ret);
    goto unlock;
  }

  if (s_status == kEsfPwrMgrStatusStop ||
      s_status == kEsfPwrMgrStatusWaitWDTIgnition) {
    LOG_INFO("PowerManager Aready Stoped.");
    ret = kEsfPwrMgrErrorStatus;
    goto unlock;
  } else if (s_status == kEsfPwrMgrStatusReboot) {
    LOG_INFO("PowerManager is Rebooting.");
    // Log output only.
  } else if (s_status == kEsfPwrMgrStatusShutdown) {
    LOG_INFO("PowerManager is Shuting down.");
    // Log output only.
  }

  *hours = s_resource.hours_meter;

unlock:
  if (CommonMutexUnlock() != kEsfPwrMgrOk) {
    LOG_ERR(kEsfPwrMgrElogErrorId0x01EsfPwrMgrCommonMutexUnlock,
            "Mutex Unlock error.");
  }

fin:
  LOG_TRACE("func end");
  return ret;
#endif  // CONFIG_EXTERNAL_POWER_MANAGER_DISABLE
}

// ----------------------------------------------------------------------------
EsfPwrMgrError EsfPwrMgrWdtTerminate(void) {
#ifdef CONFIG_EXTERNAL_POWER_MANAGER_DISABLE
  return kEsfPwrMgrOk;
#else
  LOG_TRACE("func start");
  EsfPwrMgrError ret = kEsfPwrMgrOk;

  ret = CommonMutexLock(CONFIG_EXTERNAL_POWER_MANAGER_LOCKTIME);
  if (ret != kEsfPwrMgrOk) {
    LOG_ERR(kEsfPwrMgrElogErrorId0x00EsfPwrMgrCommonMutexLock,
            "Mutex Lock error. ret=%u", ret);
    goto fin;
  }

  if (s_status == kEsfPwrMgrStatusStop ||
      s_status == kEsfPwrMgrStatusWaitWDTIgnition) {
    LOG_INFO("PowerManager Aready Stoped.");
    ret = kEsfPwrMgrErrorStatus;
    goto unlock;
  } else if (s_status == kEsfPwrMgrStatusReboot) {
    LOG_INFO("PowerManager is Rebooting.");
    // Log output only.
  } else if (s_status == kEsfPwrMgrStatusShutdown) {
    LOG_INFO("PowerManager is Shuting down.");
    // Log output only.
  }

  PlErrCode pl_ret = PlWdtTerminate();
  if (pl_ret != kPlErrCodeOk) {
    LOG_ERR(kEsfPwrMgrElogErrorId0x57PlWdt, "PlWdtTerminate error. ret=%u",
            pl_ret);
    ret = kEsfPwrMgrErrorExternal;
    goto unlock;
  }
  LOG_INFO("WDT Terminate.");

unlock:
  if (CommonMutexUnlock() != kEsfPwrMgrOk) {
    LOG_ERR(kEsfPwrMgrElogErrorId0x01EsfPwrMgrCommonMutexUnlock,
            "Mutex Unlock error.");
  }

fin:
  LOG_TRACE("func end");
  return ret;
#endif  // CONFIG_EXTERNAL_POWER_MANAGER_DISABLE
}

// ----------------------------------------------------------------------------
EsfPwrMgrError EsfPwrMgrGetSupplyType(EsfPwrMgrSupplyType *supply_type) {
#ifdef CONFIG_EXTERNAL_POWER_MANAGER_DISABLE
  *supply_type = kEsfPwrMgrSupplyTypeUnknown;
  return kEsfPwrMgrOk;
#else
  LOG_TRACE("func start");
  EsfPwrMgrError ret = kEsfPwrMgrOk;

  ret = CommonMutexLock(CONFIG_EXTERNAL_POWER_MANAGER_LOCKTIME);
  if (ret != kEsfPwrMgrOk) {
    LOG_ERR(kEsfPwrMgrElogErrorId0x00EsfPwrMgrCommonMutexLock,
            "Mutex Lock error. ret=%u", ret);
    goto fin;
  }

  if (supply_type == NULL) {
    ret = kEsfPwrMgrErrorInvalidArgument;
    LOG_ERR(kEsfPwrMgrElogErrorId0x0eEsfPwrMgr, "supply_type is NULL. ret=%u",
            ret);
    goto unlock;
  }

  if (s_status == kEsfPwrMgrStatusStop ||
      s_status == kEsfPwrMgrStatusWaitWDTIgnition) {
    LOG_INFO("PowerManager Aready Stoped.");
    ret = kEsfPwrMgrErrorStatus;
    goto unlock;
  } else if (s_status == kEsfPwrMgrStatusReboot) {
    LOG_INFO("PowerManager is Rebooting.");
    // Log output only.
  } else if (s_status == kEsfPwrMgrStatusShutdown) {
    LOG_INFO("PowerManager is Shuting down.");
    // Log output only.
  }

  PlPowerMgrSupplyType pl_supply_type;
  PlErrCode pl_ret = PlPowerMgrGetSupplyType(&pl_supply_type);
  if (pl_ret != kPlErrCodeOk) {
    LOG_ERR(kEsfPwrMgrElogErrorId0x61PlPowerMgr,
            "PlPowerMgrGetSupplyType : ret=%u", pl_ret);
    *supply_type = kEsfPwrMgrSupplyTypeUnknown;
    ret = kEsfPwrMgrErrorExternal;
    goto unlock;
  }
  LOG_DBG("PlPowerMgrSupplyType : %d", pl_supply_type);

  // Currently, the devices expected to be powered by Poe and USB
  // (T5, T3RS3, T3P)
  if (pl_supply_type == kPlPowerMgrSupplyTypePoE) {
    *supply_type = kEsfPwrMgrSupplyTypePoE;
    goto unlock;
  }

  *supply_type = kEsfPwrMgrSupplyTypeUsb;

unlock:
  if (CommonMutexUnlock() != kEsfPwrMgrOk) {
    LOG_ERR(kEsfPwrMgrElogErrorId0x01EsfPwrMgrCommonMutexUnlock,
            "Mutex Unlock error.");
  }

fin:
  LOG_TRACE("func end");
  return ret;
#endif  // CONFIG_EXTERNAL_POWER_MANAGER_DISABLE
}
// ----------------------------------------------------------------------------
EsfPwrMgrError EsfPwrMgrWdtKeepAlive(void) {
#ifdef CONFIG_EXTERNAL_POWER_MANAGER_DISABLE
  return kEsfPwrMgrOk;
#else
  LOG_TRACE("func start");
  EsfPwrMgrError ret = kEsfPwrMgrOk;

  ret = CommonMutexLock(CONFIG_EXTERNAL_POWER_MANAGER_LOCKTIME);
  if (ret != kEsfPwrMgrOk) {
    LOG_ERR(kEsfPwrMgrElogErrorId0x00EsfPwrMgrCommonMutexLock,
            "Mutex Lock error. ret=%u", ret);
    goto fin;
  }

  PlErrCode pl_ret = PlWdtKeepAlive();
  if (pl_ret != kPlErrCodeOk) {
    LOG_ERR(kEsfPwrMgrElogErrorId0x58PlWdt, "PlWdtKeepAlive error. ret=%u",
            pl_ret);
    ret = kEsfPwrMgrErrorExternal;
    goto unlock;
  }
  LOG_INFO("WDT KeepAlive.");

unlock:
  if (CommonMutexUnlock() != kEsfPwrMgrOk) {
    LOG_ERR(kEsfPwrMgrElogErrorId0x01EsfPwrMgrCommonMutexUnlock,
            "Mutex Unlock error.");
  }

fin:
  LOG_TRACE("func end");
  return ret;
#endif  // CONFIG_EXTERNAL_POWER_MANAGER_DISABLE
}

// ----------------------------------------------------------------------------
EsfPwrMgrError EsfPwrMgrGetExceptionInfo(struct EsfPwrMgrExceptionInfo **info,
                                         uint32_t *info_size) {
#ifdef CONFIG_EXTERNAL_POWER_MANAGER_DISABLE
  return kEsfPwrMgrOk;
#else
  LOG_TRACE("func start");
  EsfPwrMgrError ret = kEsfPwrMgrOk;

  ret = CommonMutexLock(CONFIG_EXTERNAL_POWER_MANAGER_LOCKTIME);
  if (ret != kEsfPwrMgrOk) {
    LOG_ERR(kEsfPwrMgrElogErrorId0x00EsfPwrMgrCommonMutexLock,
            "Mutex Lock error. ret=%u", ret);
    goto fin;
  }

  if (info == NULL) {
    ret = kEsfPwrMgrErrorInvalidArgument;
    LOG_ERR(kEsfPwrMgrElogErrorId0x0fEsfPwrMgr, "info is NULL. ret=%u", ret);
    goto unlock;
  }
  if (info_size == NULL) {
    ret = kEsfPwrMgrErrorInvalidArgument;
    LOG_ERR(kEsfPwrMgrElogErrorId0x0fEsfPwrMgr, "info_size is NULL. ret=%u",
            ret);
    goto unlock;
  }

  if (s_status == kEsfPwrMgrStatusStop ||
      s_status == kEsfPwrMgrStatusWaitWDTIgnition) {
    LOG_INFO("PowerManager Stoped.");
    ret = kEsfPwrMgrErrorStatus;
    goto unlock;
  } else if (s_status == kEsfPwrMgrStatusReboot) {
    LOG_INFO("PowerManager is Rebooting.");
    // Log output only.
  } else if (s_status == kEsfPwrMgrStatusShutdown) {
    LOG_INFO("PowerManager is Shuting down.");
    // Log output only.
  }

  if (s_exception_info == NULL) {
    s_exception_info = malloc(sizeof(struct EsfPwrMgrExceptionInfo));
    if (s_exception_info == NULL) {
      ret = kEsfPwrMgrErrorInternal;
      LOG_ERR(kEsfPwrMgrElogErrorId0x10EsfPwrMgr, "Failed to malloc. errno=%d",
              errno);
      goto unlock;
    }
  }
  memset(s_exception_info, 0x0, sizeof(struct EsfPwrMgrExceptionInfo));

  PlErrCode pl_ret = PlSystemCtlGetExceptionInfo(&(s_exception_info->pl_info));
  if (pl_ret != kPlErrCodeOk) {
    LOG_ERR(kEsfPwrMgrElogErrorId0xc0PlSystemCtl,
            "PlSystemCtlGetExceptionInfo error. ret=%u", pl_ret);
    ret = kEsfPwrMgrErrorExternal;
    free(s_exception_info);
    s_exception_info = NULL;
    goto unlock;
  }

  *info = s_exception_info;
  *info_size = sizeof(struct EsfPwrMgrExceptionInfo);

unlock:
  if (CommonMutexUnlock() != kEsfPwrMgrOk) {
    LOG_ERR(kEsfPwrMgrElogErrorId0x01EsfPwrMgrCommonMutexUnlock,
            "Mutex Unlock error.");
  }

fin:
  LOG_TRACE("func end");
  return ret;
#endif  // CONFIG_EXTERNAL_POWER_MANAGER_DISABLE
}

// ----------------------------------------------------------------------------
EsfPwrMgrError EsfPwrMgrConvExceptionInfo(struct EsfPwrMgrExceptionInfo *info,
                                          char *dst, uint32_t dst_size) {
#ifdef CONFIG_EXTERNAL_POWER_MANAGER_DISABLE
  return kEsfPwrMgrOk;
#else
  LOG_TRACE("func start");
  EsfPwrMgrError ret = kEsfPwrMgrOk;

  ret = CommonMutexLock(CONFIG_EXTERNAL_POWER_MANAGER_LOCKTIME);
  if (ret != kEsfPwrMgrOk) {
    LOG_ERR(kEsfPwrMgrElogErrorId0x00EsfPwrMgrCommonMutexLock,
            "Mutex Lock error. ret=%u", ret);
    goto fin;
  }

  if (s_status == kEsfPwrMgrStatusStop ||
      s_status == kEsfPwrMgrStatusWaitWDTIgnition) {
    LOG_INFO("PowerManager Stoped.");
    ret = kEsfPwrMgrErrorStatus;
    goto unlock;
  } else if (s_status == kEsfPwrMgrStatusReboot) {
    LOG_INFO("PowerManager is Rebooting.");
    // Log output only.
  } else if (s_status == kEsfPwrMgrStatusShutdown) {
    LOG_INFO("PowerManager is Shuting down.");
    // Log output only.
  }

  if (info == NULL) {
    ret = kEsfPwrMgrErrorInvalidArgument;
    LOG_ERR(kEsfPwrMgrElogErrorId0x11EsfPwrMgr, "info is NULL. ret=%u", ret);
    goto unlock;
  }
  if (dst == NULL) {
    ret = kEsfPwrMgrErrorInvalidArgument;
    LOG_ERR(kEsfPwrMgrElogErrorId0x12EsfPwrMgr, "dst is NULL. ret=%u", ret);
    goto unlock;
  }
  if (dst_size == 0) {
    ret = kEsfPwrMgrErrorInvalidArgument;
    LOG_ERR(kEsfPwrMgrElogErrorId0x13EsfPwrMgr, "dst_size == 0. ret=%u", ret);
    goto unlock;
  }

  PlErrCode pl_ret = PlSystemCtlConvExceptionInfo(&(info->pl_info), dst,
                                                  dst_size);
  if (pl_ret != kPlErrCodeOk) {
    LOG_ERR(kEsfPwrMgrElogErrorId0xc1PlSystemCtl,
            "PlSystemCtlConvExceptionInfo error. ret=%u", pl_ret);
    ret = kEsfPwrMgrErrorExternal;
    goto unlock;
  }

unlock:
  if (CommonMutexUnlock() != kEsfPwrMgrOk) {
    LOG_ERR(kEsfPwrMgrElogErrorId0x01EsfPwrMgrCommonMutexUnlock,
            "Mutex Unlock error.");
  }

fin:
  LOG_TRACE("func end");
  return ret;
#endif  // CONFIG_EXTERNAL_POWER_MANAGER_DISABLE
}

// ----------------------------------------------------------------------------
EsfPwrMgrError EsfPwrMgrClearExceptionInfo(void) {
#ifdef CONFIG_EXTERNAL_POWER_MANAGER_DISABLE
  return kEsfPwrMgrOk;
#else
  LOG_TRACE("func start");
  EsfPwrMgrError ret = kEsfPwrMgrOk;

  ret = CommonMutexLock(CONFIG_EXTERNAL_POWER_MANAGER_LOCKTIME);
  if (ret != kEsfPwrMgrOk) {
    LOG_ERR(kEsfPwrMgrElogErrorId0x00EsfPwrMgrCommonMutexLock,
            "Mutex Lock error. ret=%u", ret);
    goto fin;
  }

  if (s_status == kEsfPwrMgrStatusStop ||
      s_status == kEsfPwrMgrStatusWaitWDTIgnition) {
    LOG_INFO("PowerManager Stoped.");
    ret = kEsfPwrMgrErrorStatus;
    goto unlock;
  } else if (s_status == kEsfPwrMgrStatusReboot) {
    LOG_INFO("PowerManager is Rebooting.");
    // Log output only.
  } else if (s_status == kEsfPwrMgrStatusShutdown) {
    LOG_INFO("PowerManager is Shuting down.");
    // Log output only.
  }

  if (s_exception_info == NULL) {
    goto unlock;
  }

  PlErrCode pl_ret = PlSystemCtlClearExceptionInfo();
  if (pl_ret != kPlErrCodeOk) {
    LOG_ERR(kEsfPwrMgrElogErrorId0xc3PlSystemCtl,
            "PlSystemCtlClearExceptionInfo error. ret=%u", pl_ret);
    ret = kEsfPwrMgrErrorExternal;
    goto unlock;
  }

  free(s_exception_info);
  s_exception_info = NULL;

unlock:
  if (CommonMutexUnlock() != kEsfPwrMgrOk) {
    LOG_ERR(kEsfPwrMgrElogErrorId0x01EsfPwrMgrCommonMutexUnlock,
            "Mutex Unlock error.");
  }

fin:
  LOG_TRACE("func end");
  return ret;
#endif  // CONFIG_EXTERNAL_POWER_MANAGER_DISABLE
}

// ----------------------------------------------------------------------------
EsfPwrMgrError EsfPwrMgrGetResetCause(EsfPwrMgrResetCause *reset_cause) {
#ifdef CONFIG_EXTERNAL_POWER_MANAGER_DISABLE
  return kEsfPwrMgrOk;
#else
  LOG_TRACE("func start");
  EsfPwrMgrError ret = kEsfPwrMgrOk;

  if (reset_cause == NULL) {
    ret = kEsfPwrMgrErrorInvalidArgument;
    LOG_ERR(kEsfPwrMgrElogErrorId0x14EsfPwrMgr, "reset_cause is NULL. ret=%u",
            ret);
    goto fin;
  }

  PlSystemCtlResetCause pl_reset_cause;
  PlErrCode pl_ret = PlSystemCtlGetResetCause(&pl_reset_cause);
  if (pl_ret != kPlErrCodeOk) {
    LOG_ERR(kEsfPwrMgrElogErrorId0xc2PlSystemCtl,
            "PlSystemCtlGetResetCause : ret=%u", pl_ret);
    *reset_cause = kEsfPwrMgrResetCauseUnknown;
    ret = kEsfPwrMgrErrorExternal;
    goto fin;
  }
  LOG_DBG("PlSystemCtlGetResetCause : %d", pl_reset_cause);

  *reset_cause = ConvertResetCause(pl_reset_cause);
  if (*reset_cause == kEsfPwrMgrResetCauseUnknown) {
    ret = kEsfPwrMgrErrorInternal;
    LOG_ERR(kEsfPwrMgrElogErrorId0x15EsfPwrMgr, "ConvertResetCause : ret=%u",
            ret);
    goto fin;
  }

fin:
  LOG_TRACE("func end");
  return ret;
#endif  // CONFIG_EXTERNAL_POWER_MANAGER_DISABLE
}

// ----------------------------------------------------------------------------
