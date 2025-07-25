/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <errno.h>
#ifdef __NuttX__
#include <nuttx/config.h>
#else
#define FAR
#endif  // __NuttX__
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "clock_manager.h"
#include "firmware_manager.h"
#include "hal.h"
#include "hal_driver.h"
#include "hal_i2c.h"
#include "hal_ioexp.h"
#include "led_manager.h"
#include "log_manager.h"
#include "main.h"
#include "main_internal.h"
#include "main_log.h"
#include "memory_manager.h"
#include "network_manager.h"
#include "parameter_storage_manager.h"
#include "pl_main.h"
#include "power_manager.h"
#include "utility_log.h"
#include "utility_msg.h"
#include "utility_timer.h"
#ifdef CONFIG_EXTERNAL_WASM_BINDING_INIT
#include "wasm_binding_init.h"
#endif  // CONFIG_EXTERNAL_WASM_BINDING_INIT
#if defined(CONFIG_EXTERNAL_SENSOR_MAIN) || \
    defined(CONFIG_EXTERNAL_MAIN_ENABLE_SENSOR_MAIN_STUB)
#include "sensor_main.h"
#endif  // CONFIG_EXTERNAL_SENSOR_MAIN ||
        // CONFIG_EXTERNAL_MAIN_ENABLE_SENSOR_MAIN_STUB
#include "system_manager.h"

// define
#if defined(CONFIG_EXTERNAL_SYSTEMAPP) || \
    defined(CONFIG_EXTERNAL_MAIN_SYSTEMAPP_STUB)
#ifdef CONFIG_EXTERNAL_SYSTEMAPP
#define ESF_MAIN_SYSTEMAPP_PRIORITY CONFIG_EXTERNAL_SYSTEMAPP_PRIORITY
#define ESF_MAIN_SYSTEMAPP_STACKSIZE CONFIG_EXTERNAL_SYSTEMAPP_STACKSIZE
#else
#define ESF_MAIN_SYSTEMAPP_PRIORITY CONFIG_EXTERNAL_MAIN_SYSTEMAPP_STUB_PRIORITY
#define ESF_MAIN_SYSTEMAPP_STACKSIZE \
  CONFIG_EXTERNAL_MAIN_SYSTEMAPP_STUB_STACKSIZE
#endif  // CONFIG_EXTERNAL_SYSTEMAPP
#else   // CONFIG_EXTERNAL_SYSTEMAPP || CONFIG_EXTERNAL_MAIN_SYSTEMAPP_STUB
#define ESF_MAIN_SYSTEMAPP_PRIORITY (0)
#define ESF_MAIN_SYSTEMAPP_STACKSIZE (0)
#endif  // CONFIG_EXTERNAL_SYSTEMAPP || CONFIG_EXTERNAL_MAIN_SYSTEMAPP_STUB

// Global variables to hold resources.
static EsfMainInfo resource = {PTHREAD_MUTEX_INITIALIZER, false, 0, NULL, 4L};
// Global variable for exit notification.
static volatile sig_atomic_t esf_main_finish_flag;
// Gloval valiable for LedManager initialized flag.
static bool is_led_manager_initialized = false;
#if defined(CONFIG_EXTERNAL_SYSTEMAPP) || \
    defined(CONFIG_EXTERNAL_MAIN_SYSTEMAPP_STUB)
#ifndef ERROR
#define ERROR (-1)
#endif  // ERROR
static pid_t system_app_pid = ERROR;
#endif  // CONFIG_EXTERNAL_SYSTEMAPP || CONFIG_EXTERNAL_MAIN_SYSTEMAPP_STUB

#if defined(CONFIG_EXTERNAL_MAIN_FIRMWARE_MANAGER_STUB)
#define EsfFwMgrInit MAIN_EsfFwMgrInit
#define EsfFwMgrDeinit MAIN_EsfFwMgrDeinit
#define EsfFwMgrSwitchProcessorFirmwareSlot \
  MAIN_EsfFwMgrSwitchProcessorFirmwareSlot
#endif  // CONFIG_EXTERNAL_MAIN_FIRMWARE_MANAGER_STUB

// function
// """Convert EsfMainMsgType to PlMainFeatureType.

// Convert EsfMainMsgType to PlMainFeatureType.
// If an error occurs, PlMainFeatureMax will be returned.

// Args:
//     type (EsfMainMsgType): The value to be converted.

// Returns:
//     PlMainFeatureType: Converted value.

// Note:
//     This is an internal API and cannot be used externally.

// """
static PlMainFeatureType EsfMainCovertMsgTypeToFeatureType(
    EsfMainMsgType type) {
  if (type < 0 || kEsfMainMsgTypeFactoryResetForDowngrade < type) {
    return PlMainFeatureMax;
  }
  static const PlMainFeatureType kFeatures[] = {
      PlMainFeatureReboot,
      PlMainFeatureShutdown,
      PlMainFeatureFactoryReset,
      PlMainFeatureDowngrade,
  };
  return kFeatures[type];
}

// """Call the KeepAlive function.

// Call the KeepAlive function.
// This is used as a callback function to be called when formatting eMMC or
// Flash in PL Main.

// Args:
//     user_data (void *): user data.(unused)

// Returns:
//     Nothing.

// Note:
//     This is an internal API and cannot be used externally.

// """
static void EsfMainInvokeKeepAlive(void *user_data) {
  (void)user_data;
  EsfPwrMgrError pwr_ret = EsfPwrMgrWdtKeepAlive();
  if (pwr_ret != kEsfPwrMgrOk) {
    ESF_MAIN_LOG_INFO("EsfPwrMgrWdtKeepAlive error:%u", pwr_ret);
  }
}

// """Exit entry function from the OS.

// Exit entry function from the OS.
// This function supplements SIGTERM and SIGINT from the OS and
// notifies the main thread of the termination.

// Args:
//     sig (int): Signal Name. (unused.)

// Returns:
//     nothing.

// Note:
//     This is an internal API and cannot be used externally.

// """
static void EsfMainSigHandler(int sig) {
  (void)sig;
  esf_main_finish_flag = 1;
}

// """Get expiration time.

// Get expiration time to designate as mutex locked.

// Args:
//     add_msec (uint32_t): Time to expiration.
//     absolute_timeout (struct timespec *): expiration time.

// Returns:
//     One of the values of EsfMainError is returned
//     depending on the execution result.

// Yields:
//     kEsfMainOk: Success.
//     kEsfMainErrorInvalidArgument: Invalid argument specified.
//     kEsfMainErrorExternal: External API error occurred.

// Note:
//     This is an internal API and cannot be used externally.

// """
static EsfMainError EsfMainGetExpirationTime(
    uint32_t add_msec, struct timespec *absolute_timeout) {
  if (resource.is_initialized) {
    ESF_MAIN_TRACE("func start");
    ESF_MAIN_DBG("add_msec=%u", add_msec);
  } else {
    ESF_MAIN_LOG_TRACE("func start");
    ESF_MAIN_LOG_DBG("add_msec=%u", add_msec);
  }

  if (NULL == absolute_timeout) {
    if (resource.is_initialized) {
      ESF_MAIN_ERR("absolute_timeout is NULL.");
      ESF_MAIN_TRACE("func end");
    } else {
      ESF_MAIN_LOG_ERR("absolute_timeout is NULL.");
      ESF_MAIN_LOG_TRACE("func end");
    }
    return kEsfMainErrorInvalidArgument;
  }

  // Calculate absolute timeout time
  if (clock_gettime(CLOCK_REALTIME, absolute_timeout) == -1) {
    int errsv = errno;
    (void)errsv;  // "unused variable" To suppress warnings.
    if (resource.is_initialized) {
      ESF_MAIN_ERR("clock_gettime error. errno=%d, %s", errsv, strerror(errsv));
      ESF_MAIN_TRACE("func end");
    } else {
      ESF_MAIN_LOG_ERR("clock_gettime error. errno=%d, %s", errsv,
                       strerror(errsv));
      ESF_MAIN_LOG_TRACE("func end");
    }
    return kEsfMainErrorExternal;
  }

  if (resource.is_initialized) {
    ESF_MAIN_DBG("current time. %.f.%ld",
                 difftime(absolute_timeout->tv_sec, (time_t)0),
                 absolute_timeout->tv_nsec);
  } else {
    ESF_MAIN_LOG_DBG("current time. %.f.%ld",
                     difftime(absolute_timeout->tv_sec, (time_t)0),
                     absolute_timeout->tv_nsec);
  }

  // Calculate how many seconds are included in "add_msec".
  absolute_timeout->tv_sec += add_msec / 1000;
  // Calculate how many nanoseconds are included in "add_msec".
  absolute_timeout->tv_nsec += (add_msec % 1000) * 1000000;
  // If tv_nsec exceeds 1000000000 nanoseconds, add 1 to tv_sec and
  // decrease 1000000000 from tv_nsec.
  if (absolute_timeout->tv_nsec >= 1000000000) {
    absolute_timeout->tv_sec += 1;
    absolute_timeout->tv_nsec -= 1000000000;
  }

  if (resource.is_initialized) {
    ESF_MAIN_DBG("timeout time. %.f.%ld",
                 difftime(absolute_timeout->tv_sec, (time_t)0),
                 absolute_timeout->tv_nsec);
    ESF_MAIN_TRACE("func end");
  } else {
    ESF_MAIN_LOG_DBG("timeout time. %.f.%ld",
                     difftime(absolute_timeout->tv_sec, (time_t)0),
                     absolute_timeout->tv_nsec);
    ESF_MAIN_LOG_TRACE("func end");
  }
  return kEsfMainOk;
}

// """Initialize EsfSensor.

// Performs the eMMC mount and file setup process required by EsfSensor.

// Args:
//     nothing.

// Returns:
//     One of the values of EsfMainError is returned
//     depending on the execution result.

// Yields:
//     kEsfMainOk: Success.
//     kEsfMainErrorExternal: External API error occurred.

// Note:
//     This is an internal API and cannot be used externally.
//     If there is an error in the internal processing, processing continues but
//     an error response is given.

// """
#if (defined(CONFIG_EXTERNAL_SENSOR_MAIN) || \
     defined(CONFIG_EXTERNAL_MAIN_ENABLE_SENSOR_MAIN_STUB))
static EsfMainError EsfMainEsfSensorInitialize(void) {
  EsfMainError ret = kEsfMainOk;
  EsfSensorErrCode sensor_ret = kEsfSensorFail;
  EsfSystemManagerResult system_manager_ret =
      kEsfSystemManagerResultInternalError;

  EsfSystemManagerInitialSettingFlag data =
      kEsfSystemManagerInitialSettingNotCompleted;
  system_manager_ret = EsfSystemManagerGetInitialSettingFlag(&data);
  if (system_manager_ret != kEsfSystemManagerResultOk) {
    ESF_MAIN_ERR("EsfSystemManagerGetInitialSettingFlag ret=%d.",
                 system_manager_ret);
    ESF_MAIN_ELOG_ERR(ESF_MAIN_ELOG_EMMC_INIT_FAILURE);
    // Continue processing.
  }
  ESF_MAIN_INFO("EsfSystemManagerGetInitialSettingFlag data=%d.", data);

  do {
    PlErrCode pl_ret = PlMainFlashMount();
    if (pl_ret != kPlErrCodeOk) {
      ESF_MAIN_DBG("EsfMainFaData3Mount error.");
      ESF_MAIN_ELOG_ERR(ESF_MAIN_ELOG_EMMC_INIT_FAILURE);
    }
    if (data == kEsfSystemManagerInitialSettingCompleted) {
      ESF_MAIN_INFO(
          "EsfSystemManager InitialSettingFlag is currently Complated");
      pl_ret = PlMainEmmcMount();
      if (pl_ret == kPlErrCodeOk) {
        ESF_MAIN_DBG("PlMainEmmcMount success.");
        break;
      }
    }
    pl_ret = PlMainEmmcFormat(NULL, NULL);
    if (pl_ret == kPlErrCodeOk) {
      pl_ret = PlMainEmmcMount();
      if (pl_ret != kPlErrCodeOk) {
        ESF_MAIN_ERR("PlMainEmmcMount error.");
        ESF_MAIN_ELOG_ERR(ESF_MAIN_ELOG_EMMC_INIT_FAILURE);
        ret = kEsfMainErrorExternal;
      }
    } else {
      ESF_MAIN_ERR("PlMainEmmcFormat error.");
      ESF_MAIN_ELOG_ERR(ESF_MAIN_ELOG_EMMC_INIT_FAILURE);
      ret = kEsfMainErrorExternal;
    }
    ESF_MAIN_DBG("EsfMainEmmcInitialize finish.");

    ESF_MAIN_DBG("EsfSensorUtilitySetupFiles start.");
    sensor_ret = EsfSensorUtilitySetupFiles();
    if (sensor_ret != kEsfSensorOk) {
      ESF_MAIN_ERR("EsfSensorUtilitySetupFiles ret=%d", sensor_ret);
      ESF_MAIN_ELOG_ERR(ESF_MAIN_ELOG_EMMC_INIT_FAILURE);
      ret = kEsfMainErrorExternal;
      // Continue processing.
    }
    ESF_MAIN_DBG("EsfSensorUtilitySetupFiles finish.");

    system_manager_ret = EsfSystemManagerSetInitialSettingFlag(
        kEsfSystemManagerInitialSettingCompleted);
    if (system_manager_ret != kEsfSystemManagerResultOk) {
      ESF_MAIN_ERR("EsfSystemManagerSetInitialSettingFlag ret=%d.",
                   system_manager_ret);
      ESF_MAIN_ELOG_ERR(ESF_MAIN_ELOG_EMMC_INIT_FAILURE);
      ret = kEsfMainErrorExternal;
      // Continue processing.
    } else {
      ESF_MAIN_INFO("Set kEsfSystemManagerInitialSettingCompleted");
    }
  } while (0);

  ESF_MAIN_DBG("EsfSensorUtilityVerifyFiles start.");
  sensor_ret = EsfSensorUtilityVerifyFiles();
  if (sensor_ret != kEsfSensorOk) {
    ESF_MAIN_INFO("EsfSensorUtilityVerifyFiles ret=%d", sensor_ret);
    ret = kEsfMainErrorExternal;
    // Continue processing.
  }
  ESF_MAIN_DBG("EsfSensorUtilityVerifyFiles finish.");
  return ret;
}

// """Finalize EsfSensor.

// Performs the eMMC unmount process required by EsfSensor.

// Args:
//     nothing.

// Returns:
//     nothing.

// Note:
//     This is an internal API and cannot be used externally.

// """
static void EsfMainEsfSensorFinalize(void) {
  ESF_MAIN_LOG_DBG("EsfMainEmmcDeinitialize start.");
  if (PlMainEmmcUnmount() != kPlErrCodeOk) {
    ESF_MAIN_LOG_ERR("EsfMainEmmcDeinitialize error.");
  }
  ESF_MAIN_LOG_INFO("EsfMainEmmcDeinitialize finish.");
  if (PlMainFlashUnmount() != kPlErrCodeOk) {
    ESF_MAIN_LOG_ERR("EsfMainFaData3Unmount error.");
  }
  ESF_MAIN_LOG_INFO("EsfMainFaData3Unmount finish.");
  return;
}
#endif  // (CONFIG_EXTERNAL_SENSOR_MAIN ||
        // CONFIG_EXTERNAL_MAIN_ENABLE_SENSOR_MAIN_STUB)

// """Initialization process.

// Initialization process.
// Executes the process described by each module.

// Args:
//     nothing.

// Returns:
//     One of the values of EsfMainError is returned
//     depending on the execution result.

// Yields:
//     kEsfMainOk: Success.
//     kEsfMainErrorTimeout: Mutex lock timed out.
//     kEsfMainErrorResourceExhausted: Memory allocation failure.
//     kEsfMainErrorUninitialize: Error occurs when ESFMain is uninitialized.
//     kEsfMainErrorExternal: External API error occurred.
//     kEsfMainErrorInternal: Internal API error occurred.

// Note:
//     This is an internal API and cannot be used externally.

// """
static EsfMainError EsfMainBoot(void) {
  ESF_MAIN_LOG_TRACE("func start");

  int32_t ret = 0;
  is_led_manager_initialized = false;

  struct timespec absolute_timeout;
  ret = EsfMainGetExpirationTime(CONFIG_EXTERNAL_MAIN_LOCKTIME_MS,
                                 &absolute_timeout);
  if (ret != kEsfMainOk) {
    ESF_MAIN_LOG_ERR("EsfMainGetExpirationTime ret=%d", ret);
    ESF_MAIN_LOG_TRACE("func end");
    return kEsfMainErrorExternal;
  }

  // Try to acquire the mutex with timeout
  ret = pthread_mutex_timedlock(&resource.state_mutex, &absolute_timeout);
  if (ret == ETIMEDOUT) {
    ESF_MAIN_LOG_INFO("Mutex acquisition timed out.");
    ESF_MAIN_LOG_TRACE("func end");
    return kEsfMainErrorTimeout;
  } else if (ret != 0) {
    ESF_MAIN_LOG_ERR("pthread_mutex_timedlock ret=%d", ret);
    ESF_MAIN_LOG_TRACE("func end");
    return kEsfMainErrorExternal;
  } else {
    // do nothing.
  }
  ESF_MAIN_LOG_DBG("mutex lock.");

  if (resource.is_initialized == true) {
    ESF_MAIN_LOG_INFO("resource.is_initialized=true");
    pthread_mutex_unlock(&resource.state_mutex);
    ESF_MAIN_LOG_DBG("mutex unlock.");
    ESF_MAIN_LOG_TRACE("func end");
    return kEsfMainErrorInternal;
  }

  bool is_init_error = true;
  do {
    {
      ESF_MAIN_LOG_DBG("UtilityMsgInitialize Init start.");
      UtilityMsgErrCode utility_ret = UtilityMsgInitialize();
      if (kUtilityMsgOk != utility_ret) {
        ESF_MAIN_LOG_ERR("UtilityMsgInitialize ret=%u", utility_ret);
        break;
      }
      ESF_MAIN_LOG_INFO("UtilityMsgInitialize Init finish.");
    }
    {
      ESF_MAIN_LOG_DBG("UtilityTimerInitialize Init start.");
      UtilityTimerErrCode ext_ret = UtilityTimerInitialize();
      if (kUtilityTimerOk != ext_ret) {
        ESF_MAIN_LOG_ERR("UtilityTimerInitialize ret=%u", ext_ret);
        break;
      }
      ESF_MAIN_LOG_INFO("UtilityTimerInitialize Init finish.");
    }
    {
      ESF_MAIN_LOG_DBG("EsfLogManagerInit Init start.");
      EsfLogManagerStatus ext_ret = EsfLogManagerInit();
      if (kEsfLogManagerStatusOk != ext_ret) {
        ESF_MAIN_LOG_ERR("EsfLogManagerInit ret=%u", ext_ret);
        break;
      }
      ESF_MAIN_LOG_INFO("EsfLogManagerInit Init finish.");
    }
    {
      ESF_MAIN_LOG_DBG("UtilityLogInit Init start.");
      UtilityLogStatus ext_ret = UtilityLogInit();
      if (kUtilityLogStatusOk != ext_ret) {
        ESF_MAIN_LOG_ERR("UtilityLogInit ret=%u", ext_ret);
        break;
      }
      ESF_MAIN_INFO("UtilityLogInit Init finish.");
    }
#ifdef CONFIG_EXTERNAL_HAL_I2C
    {
      ESF_MAIN_DBG("HalI2cInitialize Init start.");
      HalErrCode ext_ret = HalI2cInitialize();
      if (kHalErrCodeOk != ext_ret) {
        ESF_MAIN_ERR("HalI2cInitialize ret=%d", ext_ret);
        break;
      }
      ESF_MAIN_INFO("HalI2cInitialize Init finish.");
    }
#endif  // CONFIG_EXTERNAL_HAL_I2C
#ifdef CONFIG_EXTERNAL_HAL_DRIVER
    {
      ESF_MAIN_DBG("HalDriverInitialize Init start.");
      HalErrCode ext_ret = HalDriverInitialize();
      if (kHalErrCodeOk != ext_ret) {
        ESF_MAIN_ERR("HalDriverInitialize ret=%d", ext_ret);
        break;
      }
      ESF_MAIN_INFO("HalDriverInitialize Init finish.");
    }
#endif  // CONFIG_EXTERNAL_HAL_DRIVER
#ifdef CONFIG_EXTERNAL_HAL_IOEXP
    {
      ESF_MAIN_DBG("HalIoexpInitialize Init start.");
      HalErrCode ext_ret = HalIoexpInitialize();
      if (kHalErrCodeOk != ext_ret) {
        ESF_MAIN_ERR("HalIoexpInitialize ret=%d", ext_ret);
        break;
      }
      ESF_MAIN_INFO("HalIoexpInitialize Init finish.");
    }
#endif  // CONFIG_EXTERNAL_HAL_IOEXP

    //
    // Execute processing for each module.
    //
    {
      ESF_MAIN_DBG("EsfMemoryManagerInitialize Init start.");
      int32_t app_mem_div_num = CONFIG_EXTERNAL_MAIN_APP_MEM_DIV_NUM;
      EsfMemoryManagerResult ext_ret =
          EsfMemoryManagerInitialize(app_mem_div_num);
      if (kEsfMemoryManagerResultSuccess != ext_ret) {
        ESF_MAIN_ERR("EsfMemoryManagerInitialize ret=%d", ext_ret);
        break;
      }
      ESF_MAIN_INFO("EsfMemoryManagerInitialize Init finish.");
    }
    {
      ESF_MAIN_DBG("EsfParameterStorageManagerInit Init start.");
      // Initialize ParameterStorageManager
      EsfParameterStorageManagerStatus result =
          EsfParameterStorageManagerInit();
      if (result != kEsfParameterStorageManagerStatusOk) {
        ESF_MAIN_ERR("EsfParameterStorageManagerInit return=%d", result);
        break;
      }
      ESF_MAIN_INFO("EsfParameterStorageManagerInit Init finish.");
    }
    {
      EsfLogManagerStatus ext_ret = EsfLogManagerStart();
      if (kEsfLogManagerStatusOk != ext_ret) {
        ESF_MAIN_ERR("EsfLogManagerStart ret=%d", ext_ret);
        break;
      }
    }
    {
      ESF_MAIN_DBG("EsfLedManagerInit Init start.");
      EsfLedManagerResult ext_ret = EsfLedManagerInit();
      if (kEsfLedManagerSuccess != ext_ret) {
        ESF_MAIN_ERR("EsfLedManagerInit ret=%d", ext_ret);
        ESF_MAIN_ELOG_ERR(ESF_MAIN_ELOG_INIT_FAILURE);
        break;
      }
      is_led_manager_initialized = true;

      ESF_MAIN_INFO("EsfLedManagerInit Init finish.");
      EsfLedManagerLedStatusInfo status;
      status.led = kEsfLedManagerTargetLedPower;
      status.status = kEsfLedManagerLedStatusUnableToAcceptInputs;
      status.enabled = true;
      ext_ret = EsfLedManagerSetStatus(&status);
      if (kEsfLedManagerSuccess != ext_ret) {
        ESF_MAIN_ERR("EsfLedManagerSetStatus ret=%d", ext_ret);
      }
    }
    {
      ESF_MAIN_DBG("EsfPwrMgrStart Init start.");
      EsfPwrMgrError ext_ret = EsfPwrMgrStart();
      if (kEsfPwrMgrOk != ext_ret) {
        ESF_MAIN_ERR("EsfPwrMgrStart ret=%d", ext_ret);
        ESF_MAIN_ELOG_ERR(ESF_MAIN_ELOG_INIT_FAILURE);
        break;
      }
      ESF_MAIN_INFO("EsfPwrMgrStart Init finish.");
    }
    {
      ESF_MAIN_DBG("EsfNetworkManagerInit Init start.");
      EsfNetworkManagerResult ext_ret = EsfNetworkManagerInit();
      if (kEsfNetworkManagerResultSuccess != ext_ret) {
        ESF_MAIN_ERR("EsfNetworkManagerInit ret=%d", ext_ret);
        ESF_MAIN_ELOG_ERR(ESF_MAIN_ELOG_INIT_FAILURE);
        break;
      }
      ESF_MAIN_INFO("EsfNetworkManagerInit Init finish.");
    }
    {
      ESF_MAIN_DBG("EsfClockManagerInit Init start.");
      EsfClockManagerReturnValue ext_ret = EsfClockManagerInit();
      if (kClockManagerSuccess != ext_ret) {
        ESF_MAIN_ERR("EsfClockManagerInit ret=%d", ext_ret);
        ESF_MAIN_ELOG_ERR(ESF_MAIN_ELOG_INIT_FAILURE);
        break;
      }
      ESF_MAIN_INFO("EsfClockManagerInit Init finish.");
    }
    {
      ESF_MAIN_DBG("EsfFwMgrInit Init start.");
      EsfFwMgrResult ext_ret = EsfFwMgrInit();
      if (kEsfFwMgrResultOk != ext_ret) {
        ESF_MAIN_ERR("EsfFwMgrInit ret=%d", ext_ret);
        ESF_MAIN_ELOG_ERR(ESF_MAIN_ELOG_INIT_FAILURE);
        break;
      }
      ESF_MAIN_INFO("EsfFwMgrInit Init finish.");
    }
#ifdef CONFIG_EXTERNAL_WASM_BINDING_INIT
    {
      ESF_MAIN_DBG("WasmBindingInit Init start.");
      if (!WasmBindingInit()) {
        ESF_MAIN_ERR("WasmBindingInit Init Failed.");
        ESF_MAIN_ELOG_ERR(ESF_MAIN_ELOG_INIT_FAILURE);
        break;
      }
      ESF_MAIN_INFO("WasmBindingInit Init finish.");
    }
#endif  // CONFIG_EXTERNAL_WASM_BINDING_INIT
    {
#if (defined(CONFIG_EXTERNAL_SENSOR_MAIN) || \
     defined(CONFIG_EXTERNAL_MAIN_ENABLE_SENSOR_MAIN_STUB))
      ESF_MAIN_DBG("EsfMainEsfSensorInitialize start.");
      EsfMainError ext_ret = EsfMainEsfSensorInitialize();
      if (kEsfMainOk != ext_ret) {
        ESF_MAIN_ERR("EsfMainEsfSensorInitialize Failed. ret=%d", ext_ret);
        ESF_MAIN_ELOG_ERR(ESF_MAIN_ELOG_INIT_FAILURE);
        // Continue processing.
      } else {
        ESF_MAIN_INFO("EsfMainEsfSensorInitialize finish.");
      }
#endif  // (CONFIG_EXTERNAL_SENSOR_MAIN ||
        // CONFIG_EXTERNAL_MAIN_ENABLE_SENSOR_MAIN_STUB)
    }
    {
#if defined(CONFIG_EXTERNAL_SYSTEMAPP) || \
    defined(CONFIG_EXTERNAL_MAIN_SYSTEMAPP_STUB)
      ESF_MAIN_DBG("SystemApp boot start.");
#ifndef __NuttX__
      extern int startup_system_app();

      ret = startup_system_app();
      if (ret) {
        ESF_MAIN_ERR("Failed to start SystemApp thread");
        ESF_MAIN_ELOG_ERR(ESF_MAIN_ELOG_INIT_FAILURE);
        break;
      }
#else // __NuttX__
      extern int SystemApp_main(int argc, char *argv[]);

      system_app_pid = task_create("SystemApp", ESF_MAIN_SYSTEMAPP_PRIORITY,
                                   ESF_MAIN_SYSTEMAPP_STACKSIZE, SystemApp_main,
                                   NULL);
      if (system_app_pid < 0) {
        ESF_MAIN_ERR("Failed to create task");
        ESF_MAIN_ELOG_ERR(ESF_MAIN_ELOG_INIT_FAILURE);
        break;
      }
#endif // __NuttX__
      ESF_MAIN_INFO("SystemApp boot finish.");
#endif  // CONFIG_EXTERNAL_SYSTEMAPP || CONFIG_EXTERNAL_MAIN_SYSTEMAPP_STUB
    }
    is_init_error = false;
  } while (0);

  if (is_init_error) {
    pthread_mutex_unlock(&resource.state_mutex);
    ESF_MAIN_LOG_DBG("mutex unlock.");
    ESF_MAIN_LOG_TRACE("func end");
    return kEsfMainErrorExternal;
  }

  // Process requiring main function
  resource.max_msg_size = 32;
  UtilityMsgErrCode utility_ret = UtilityMsgOpen(&resource.utility_msg_handle,
                                                 32, resource.max_msg_size);
  if (kUtilityMsgOk != utility_ret) {
    ESF_MAIN_ERR("UtilityMsgOpen ret=%d", utility_ret);
    ESF_MAIN_ELOG_ERR(ESF_MAIN_ELOG_INIT_FAILURE);
    pthread_mutex_unlock(&resource.state_mutex);
    ESF_MAIN_DBG("mutex unlock.");
    ESF_MAIN_TRACE("func end");
    return kEsfMainErrorExternal;
  }

  resource.recv_buf = malloc(resource.max_msg_size);
  if (resource.recv_buf == NULL) {
    ESF_MAIN_ERR("malloc() fail. errno=%d", errno);
    pthread_mutex_unlock(&resource.state_mutex);
    ESF_MAIN_DBG("mutex unlock.");
    ESF_MAIN_ELOG_ERR(ESF_MAIN_ELOG_SYSTEM_ERROR);
    ESF_MAIN_TRACE("func end");
    return kEsfMainErrorResourceExhausted;
  }
  resource.is_initialized = true;
  ESF_MAIN_INFO("state change. UNINIT -> INIT");

  pthread_mutex_unlock(&resource.state_mutex);
  ESF_MAIN_DBG("mutex unlock.");

  ESF_MAIN_TRACE("func end");
  return kEsfMainOk;
}

// """Termination process.

// Termination process.
// Executes the process described by each module.

// Args:
//     finish_led (bool): Finish LedManager flag.

// Returns:
//     One of the values of EsfMainError is returned
//     depending on the execution result.

// Yields:
//     kEsfMainOk: Success.
//     kEsfMainErrorTimeout: Mutex lock timed out.
//     kEsfMainErrorExternal: External API error occurred.
//     kEsfMainErrorInternal: Internal API error occurred.

// Note:
//     This is an internal API and cannot be used externally.

// """
static EsfMainError EsfMainFinish(bool finish_led) {
  ESF_MAIN_TRACE("func start %u", finish_led);

  int32_t ret = 0;

  struct timespec absolute_timeout;
  ret = EsfMainGetExpirationTime(CONFIG_EXTERNAL_MAIN_LOCKTIME_MS,
                                 &absolute_timeout);
  if (ret != kEsfMainOk) {
    ESF_MAIN_ERR("EsfMainGetExpirationTime ret=%d", ret);
    ESF_MAIN_ELOG_ERR(ESF_MAIN_ELOG_SYSTEM_ERROR);
    ESF_MAIN_TRACE("func end");
    return kEsfMainErrorExternal;
  }

  // Try to acquire the mutex with timeout
  ret = pthread_mutex_timedlock(&resource.state_mutex, &absolute_timeout);
  if (ret == ETIMEDOUT) {
    ESF_MAIN_INFO("Mutex acquisition timed out.");
    ESF_MAIN_ELOG_ERR(ESF_MAIN_ELOG_SYSTEM_ERROR);
    ESF_MAIN_TRACE("func end");
    return kEsfMainErrorTimeout;
  } else if (ret != 0) {
    ESF_MAIN_ERR("pthread_mutex_timedlock ret=%d", ret);
    ESF_MAIN_ELOG_ERR(ESF_MAIN_ELOG_SYSTEM_ERROR);
    ESF_MAIN_TRACE("func end");
    return kEsfMainErrorExternal;
  } else {
    // do nothing.
  }
  ESF_MAIN_DBG("mutex lock.");

  if (resource.is_initialized == false) {
    ESF_MAIN_INFO("resource.is_initialized=false");
    pthread_mutex_unlock(&resource.state_mutex);
    ESF_MAIN_DBG("mutex unlock.");
    ESF_MAIN_TRACE("func end");
    return kEsfMainErrorInternal;
  }

  // Process requiring main function
  UtilityMsgErrCode utility_ret = UtilityMsgClose(resource.utility_msg_handle);
  if (kUtilityMsgOk != utility_ret) {
    // Continue processing even if an error occurs.
    ESF_MAIN_ERR("UtilityMsgClose ret=%d", utility_ret);
    ESF_MAIN_ELOG_WARN(ESF_MAIN_ELOG_TERM_FAILURE);
  }

  //
  // Execute processing for each module.
  //
  {
#if defined(CONFIG_EXTERNAL_SYSTEMAPP) || \
    defined(CONFIG_EXTERNAL_MAIN_SYSTEMAPP_STUB)
    ESF_MAIN_DBG("SystemApp stub delete start.");
    if (system_app_pid >= 0) {
      int err = 0;
#if defined(CONFIG_EXTERNAL_MAIN_SYSTEMAPP_STUB)
#ifndef __NuttX__
      // OS[Linux]
      // waitpid is not timeoutable. Wait for SystemApp to exit with sleep
      // because it may keep waiting.
      int usec = CONFIG_EXTERNAL_MAIN_WAIT_SYSTEM_APP_TERM_TIME;
      err = usleep(usec);
      if (err != 0) {
        ESF_MAIN_ERR("usleep error. ret=%d", err);
        ESF_MAIN_ELOG_WARN(ESF_MAIN_ELOG_TERM_FAILURE);
      }
#else   // __NuttX__
      // OS[nuttx]
      kill(system_app_pid, SIGTERM);
      int status = 0;
      err = waitpid(system_app_pid, &status, 0);
      if (err != 0) {
        ESF_MAIN_ERR("SystemApp ret=%d", status);
        ESF_MAIN_ELOG_WARN(ESF_MAIN_ELOG_TERM_FAILURE);
      }
#endif  // __NuttX__
#else   // CONFIG_EXTERNAL_MAIN_SYSTEMAPP_STUB
      // waitpid is not timeoutable. Wait for SystemApp to exit with sleep
      // because it may keep waiting.
      int usec = CONFIG_EXTERNAL_MAIN_WAIT_SYSTEM_APP_TERM_TIME;
      err = usleep(usec);
      if (err != 0) {
        ESF_MAIN_ERR("usleep error. ret=%d", err);
        ESF_MAIN_ELOG_WARN(ESF_MAIN_ELOG_TERM_FAILURE);
      }
#endif  // CONFIG_EXTERNAL_MAIN_SYSTEMAPP_STUB
      system_app_pid = ERROR;
    }
    ESF_MAIN_INFO("SystemApp stub delete finish.");
#endif  // CONFIG_EXTERNAL_SYSTEMAPP || CONFIG_EXTERNAL_MAIN_SYSTEMAPP_STUB
  }
  {
    ESF_MAIN_DBG("EsfFwMgrDeinit start.");
    EsfFwMgrResult ext_ret = EsfFwMgrDeinit();
    if (kEsfFwMgrResultOk != ext_ret) {
      ESF_MAIN_ERR("EsfFwMgrDeinit ret=%d", ext_ret);
      ESF_MAIN_ELOG_WARN(ESF_MAIN_ELOG_TERM_FAILURE);
    }
    ESF_MAIN_INFO("EsfFwMgrDeinit finish.");
  }
  {
    ESF_MAIN_DBG("EsfClockManagerDeinit start.");
    EsfClockManagerReturnValue ext_ret = EsfClockManagerDeinit();
    if (kClockManagerSuccess != ext_ret) {
      ESF_MAIN_ERR("EsfClockManagerDeinit ret=%d", ext_ret);
      ESF_MAIN_ELOG_WARN(ESF_MAIN_ELOG_TERM_FAILURE);
    }
    ESF_MAIN_INFO("EsfClockManagerDeinit finish.");
  }
  {
    ESF_MAIN_DBG("EsfNetworkManagerDeinit start.");
    EsfNetworkManagerResult ext_ret = EsfNetworkManagerDeinit();
    if (kEsfNetworkManagerResultSuccess != ext_ret) {
      ESF_MAIN_ERR("EsfNetworkManagerDeinit ret=%d", ext_ret);
      ESF_MAIN_ELOG_WARN(ESF_MAIN_ELOG_TERM_FAILURE);
    }
    ESF_MAIN_INFO("EsfNetworkManagerDeinit finish.");
  }
  {
    ESF_MAIN_DBG("EsfPwrMgrStopForReboot start.");
    EsfPwrMgrError ext_ret = EsfPwrMgrStopForReboot();
    if (kEsfPwrMgrOk != ext_ret) {
      ESF_MAIN_ERR("EsfPwrMgrStopForReboot ret=%d", ext_ret);
      ESF_MAIN_ELOG_WARN(ESF_MAIN_ELOG_TERM_FAILURE);
    }
    ESF_MAIN_INFO("EsfPwrMgrStopForReboot finish.");
  }
  if (finish_led) {
    ESF_MAIN_DBG("EsfLedManagerDeinit start.");
    EsfLedManagerResult ext_ret = EsfLedManagerDeinit();
    if (kEsfLedManagerSuccess != ext_ret) {
      ESF_MAIN_ERR("EsfLedManagerDeinit ret=%d", ext_ret);
      ESF_MAIN_ELOG_WARN(ESF_MAIN_ELOG_TERM_FAILURE);
    }
    ESF_MAIN_INFO("EsfLedManagerDeinit finish.");
  } else {
    ESF_MAIN_INFO("EsfLedManagerDeinit finish skip.");
  }
  {
    ESF_MAIN_DBG("EsfParameterStorageManagerDeinit start.");
    // Deinitialize ParameterStorageManager
    EsfParameterStorageManagerStatus result =
        EsfParameterStorageManagerDeinit();
    if (result != kEsfParameterStorageManagerStatusOk) {
      ESF_MAIN_ERR("EsfParameterStorageManagerDeinit return=%d", result);
      ESF_MAIN_ELOG_WARN(ESF_MAIN_ELOG_TERM_FAILURE);
    }
    ESF_MAIN_INFO("EsfParameterStorageManagerDeinit finish.");
  }
  {
    ESF_MAIN_DBG("EsfMemoryManagerFinalize start.");
    EsfMemoryManagerResult ext_ret = EsfMemoryManagerFinalize();
    if (kEsfMemoryManagerResultSuccess != ext_ret) {
      ESF_MAIN_ERR("EsfMemoryManagerFinalize ret=%d", ext_ret);
      ESF_MAIN_ELOG_WARN(ESF_MAIN_ELOG_TERM_FAILURE);
    }
    ESF_MAIN_INFO("EsfMemoryManagerFinalize finish.");
  }

  if (finish_led) {
#ifdef CONFIG_EXTERNAL_HAL_IOEXP
    ESF_MAIN_DBG("HalIoexpFinalize start.");
    HalErrCode ext_ret = HalIoexpFinalize();
    if (kHalErrCodeOk != ext_ret) {
      ESF_MAIN_ERR("HalIoexpFinalize ret=%d", ext_ret);
      ESF_MAIN_ELOG_WARN(ESF_MAIN_ELOG_TERM_FAILURE);
    }
    ESF_MAIN_INFO("HalIoexpFinalize finish.");
#endif  // CONFIG_EXTERNAL_HAL_IOEXP
  } else {
    ESF_MAIN_INFO("HalIoexpFinalize finish skip.");
  }
  if (finish_led) {
#ifdef CONFIG_EXTERNAL_HAL_DRIVER
    ESF_MAIN_DBG("HalDriverFinalize start.");
    HalErrCode ext_ret = HalDriverFinalize();
    if (kHalErrCodeOk != ext_ret) {
      ESF_MAIN_ERR("HalDriverFinalize ret=%d", ext_ret);
      ESF_MAIN_ELOG_WARN(ESF_MAIN_ELOG_TERM_FAILURE);
    }
    ESF_MAIN_INFO("HalDriverFinalize finish.");
#endif  // CONFIG_EXTERNAL_HAL_DRIVER
  } else {
    ESF_MAIN_INFO("HalDriverFinalize finish skip.");
  }
  if (finish_led) {
#ifdef CONFIG_EXTERNAL_HAL_I2C
    ESF_MAIN_DBG("HalI2cFinalize start.");
    HalErrCode ext_ret = HalI2cFinalize();
    if (kHalErrCodeOk != ext_ret) {
      ESF_MAIN_ERR("HalI2cFinalize ret=%d", ext_ret);
      ESF_MAIN_ELOG_WARN(ESF_MAIN_ELOG_TERM_FAILURE);
    }
    ESF_MAIN_INFO("HalI2cFinalize finish.");
#endif  // CONFIG_EXTERNAL_HAL_I2C
  } else {
    ESF_MAIN_INFO("HalI2cFinalize finish skip.");
  }
  if (finish_led) {
    ESF_MAIN_DBG("UtilityLogDeinit start.");
    UtilityLogStatus ext_ret = UtilityLogDeinit();
    if (kUtilityLogStatusOk != ext_ret) {
      ESF_MAIN_LOG_ERR("UtilityLogDeinit ret=%u", ext_ret);
    }
    ESF_MAIN_LOG_INFO("UtilityLogDeinit finish.");
  } else {
    ESF_MAIN_INFO("UtilityLogDeinit finish skip.");
  }
  {
    ESF_MAIN_LOG_DBG("EsfLogManagerDeinit start.");
    EsfLogManagerStatus ext_ret = EsfLogManagerDeinit();
    if (kEsfLogManagerStatusOk != ext_ret) {
      ESF_MAIN_LOG_ERR("EsfLogManagerDeinit ret=%u", ext_ret);
    }
    ESF_MAIN_LOG_INFO("EsfLogManagerDeinit finish.");
  }
  if (finish_led) {
    ESF_MAIN_LOG_DBG("UtilityTimerFinalize start.");
    UtilityTimerErrCode ext_ret = UtilityTimerFinalize();
    if (kUtilityTimerOk != ext_ret) {
      ESF_MAIN_LOG_ERR("UtilityTimerFinalize ret=%u", ext_ret);
    }
    ESF_MAIN_LOG_INFO("UtilityTimerFinalize finish.");
  } else {
    ESF_MAIN_LOG_INFO("UtilityTimerFinalize finish skip.");
  }
  if (finish_led) {
    ESF_MAIN_LOG_DBG("UtilityMsgFinalize start.");
    utility_ret = UtilityMsgFinalize();
    if (kUtilityMsgOk != utility_ret) {
      // Continue processing even if an error occurs.
      ESF_MAIN_LOG_ERR("UtilityMsgFinalize ret=%u", utility_ret);
    }
    ESF_MAIN_LOG_INFO("UtilityMsgFinalize finish.");
  } else {
    ESF_MAIN_LOG_INFO("UtilityMsgFinalize finish skip.");
  }
  {
#if (defined(CONFIG_EXTERNAL_SENSOR_MAIN) || \
     defined(CONFIG_EXTERNAL_MAIN_ENABLE_SENSOR_MAIN_STUB))
    ESF_MAIN_LOG_DBG("EsfMainEsfSensorFinalize start.");
    EsfMainEsfSensorFinalize();
    ESF_MAIN_LOG_INFO("EsfMainEsfSensorFinalize finish.");
#endif  // (CONFIG_EXTERNAL_SENSOR_MAIN ||
        // CONFIG_EXTERNAL_MAIN_ENABLE_SENSOR_MAIN_STUB)
  }

  if (resource.recv_buf != NULL) {
    free(resource.recv_buf);
    resource.recv_buf = NULL;
  }

  resource.is_initialized = false;
  ESF_MAIN_LOG_INFO("state change. INIT -> UNINIT");

  pthread_mutex_unlock(&resource.state_mutex);
  ESF_MAIN_LOG_DBG("mutex unlock.");

  ESF_MAIN_LOG_TRACE("func end");
  return kEsfMainOk;
}

// """Termination LedManager process.

// Termination LedManager process.

// Args:
//     Nothing

// Returns:
//     One of the values of EsfMainError is returned
//     depending on the execution result.

// Yields:
//     kEsfMainOk: Success.
//     kEsfMainErrorTimeout: Mutex lock timed out.
//     kEsfMainErrorExternal: External API error occurred.
//     kEsfMainErrorInternal: Internal API error occurred.

// Note:
//     This is an internal API and cannot be used externally.

// """
static EsfMainError EsfMainFinishLed(void) {
  ESF_MAIN_LOG_TRACE("func start");

  int32_t ret = 0;

  struct timespec absolute_timeout;
  ret = EsfMainGetExpirationTime(CONFIG_EXTERNAL_MAIN_LOCKTIME_MS,
                                 &absolute_timeout);
  if (ret != kEsfMainOk) {
    ESF_MAIN_LOG_ERR("EsfMainGetExpirationTime ret=%d", ret);
    ESF_MAIN_LOG_TRACE("func end");
    return kEsfMainErrorExternal;
  }

  // Try to acquire the mutex with timeout
  ret = pthread_mutex_timedlock(&resource.state_mutex, &absolute_timeout);
  if (ret == ETIMEDOUT) {
    ESF_MAIN_LOG_INFO("Mutex acquisition timed out.");
    ESF_MAIN_LOG_TRACE("func end");
    return kEsfMainErrorTimeout;
  } else if (ret != 0) {
    ESF_MAIN_LOG_ERR("pthread_mutex_timedlock ret=%d", ret);
    ESF_MAIN_LOG_TRACE("func end");
    return kEsfMainErrorExternal;
  } else {
    // do nothing.
  }
  ESF_MAIN_LOG_DBG("mutex lock.");

  {
    ESF_MAIN_LOG_DBG("EsfLedManagerDeinit start.");
    EsfLedManagerResult ext_ret = EsfLedManagerDeinit();
    if (kEsfLedManagerSuccess != ext_ret) {
      ESF_MAIN_LOG_ERR("EsfLedManagerDeinit ret=%u", ext_ret);
    }
    ESF_MAIN_LOG_INFO("EsfLedManagerDeinit finish.");
  }
#ifdef CONFIG_EXTERNAL_HAL_IOEXP
  {
    ESF_MAIN_LOG_DBG("HalIoexpFinalize start.");
    HalErrCode ext_ret = HalIoexpFinalize();
    if (kHalErrCodeOk != ext_ret) {
      ESF_MAIN_LOG_ERR("HalIoexpFinalize ret=%u", ext_ret);
    }
    ESF_MAIN_LOG_INFO("HalIoexpFinalize finish.");
  }
#endif  // CONFIG_EXTERNAL_HAL_IOEXP
#ifdef CONFIG_EXTERNAL_HAL_DRIVER
  {
    ESF_MAIN_LOG_DBG("HalDriverFinalize start.");
    HalErrCode ext_ret = HalDriverFinalize();
    if (kHalErrCodeOk != ext_ret) {
      ESF_MAIN_LOG_ERR("HalDriverFinalize ret=%u", ext_ret);
    }
    ESF_MAIN_LOG_INFO("HalDriverFinalize finish.");
  }
#endif  // CONFIG_EXTERNAL_HAL_DRIVER
#ifdef CONFIG_EXTERNAL_HAL_I2C
  {
    ESF_MAIN_LOG_DBG("HalI2cFinalize start.");
    HalErrCode ext_ret = HalI2cFinalize();
    if (kHalErrCodeOk != ext_ret) {
      ESF_MAIN_LOG_ERR("HalI2cFinalize ret=%u", ext_ret);
    }
    ESF_MAIN_LOG_INFO("HalI2cFinalize finish.");
  }
#endif  // CONFIG_EXTERNAL_HAL_I2C
  {
    ESF_MAIN_LOG_DBG("UtilityLogDeinit start.");
    UtilityLogStatus ext_ret = UtilityLogDeinit();
    if (kUtilityLogStatusOk != ext_ret) {
      ESF_MAIN_LOG_ERR("UtilityLogDeinit ret=%u", ext_ret);
    }
    ESF_MAIN_LOG_INFO("UtilityLogDeinit finish.");
  }
  {
    ESF_MAIN_LOG_DBG("UtilityTimerFinalize start.");
    UtilityTimerErrCode ext_ret = UtilityTimerFinalize();
    if (kUtilityTimerOk != ext_ret) {
      ESF_MAIN_LOG_ERR("UtilityTimerFinalize ret=%u", ext_ret);
    }
    ESF_MAIN_LOG_INFO("UtilityTimerFinalize finish.");
  }
  {
    ESF_MAIN_LOG_DBG("UtilityMsgFinalize start.");
    UtilityMsgErrCode utility_ret = UtilityMsgFinalize();
    if (kUtilityMsgOk != utility_ret) {
      // Continue processing even if an error occurs.
      ESF_MAIN_LOG_ERR("UtilityMsgFinalize ret=%u", utility_ret);
    }
    ESF_MAIN_LOG_INFO("UtilityMsgFinalize finish.");
  }
  pthread_mutex_unlock(&resource.state_mutex);
  ESF_MAIN_LOG_DBG("mutex unlock.");

  ESF_MAIN_LOG_TRACE("func end");
  return kEsfMainOk;
}

// """Reboot process.

// Reboot process.
// Each module executes the process described.

// Args:
//     nothing.

// Returns:
//     One of the values of EsfMainError is returned
//     depending on the execution result.

// Yields:
//     kEsfMainOk: Success.

// Note:
//     This is an internal API and cannot be used externally.

// """
static EsfMainError EsfMainProcessReboot(void) {
  ESF_MAIN_TRACE("func start");
  ESF_MAIN_INFO("EsfMainProcessReboot called.");

  int32_t ret = 0;

  //
  // Execute processing for each module.
  //

  ret = EsfMainFinish(true);
  if (ret != kEsfMainOk) {
    // Continue processing even if an error occurs.
    ESF_MAIN_LOG_ERR("EsfMainFinish ret=%d", ret);
  }

  // TODO Future changes api.
  EsfPwrMgrExecuteRebootEx(EsfPwrMgrRebootTypeSW);

  ESF_MAIN_LOG_TRACE("func end");
  return kEsfMainOk;
}

// """Shutdown process.

// Shutdown process.
// Each module executes the process described.

// Args:
//     nothing.

// Returns:
//     One of the values of EsfMainError is returned
//     depending on the execution result.

// Yields:
//     kEsfMainOk: Success.

// Note:
//     This is an internal API and cannot be used externally.

// """
static EsfMainError EsfMainProcessShutdown(void) {
  ESF_MAIN_TRACE("func start");

  int32_t ret = 0;

  //
  // Execute processing for each module.
  //

  ret = EsfMainFinish(true);
  if (ret != kEsfMainOk) {
    // Continue processing even if an error occurs.
    ESF_MAIN_LOG_ERR("EsfMainFinish ret=%d", ret);
  }

  EsfPwrMgrExecuteShutdown();

  ESF_MAIN_LOG_TRACE("func end");
  return kEsfMainOk;
}

// """Factory reset process.

// Factory reset process.
// Each module executes the process described.

// Args:
//     is_downgrade (bool): Downgrade behavior identification.

// Returns:
//     One of the values of EsfMainError is returned
//     depending on the execution result.

// Yields:
//     kEsfMainOk: Success.

// Note:
//     This is an internal API and cannot be used externally.

// """
static EsfMainError EsfMainProcessFactoryReset(bool is_downgrade) {
  ESF_MAIN_TRACE("func start");
  ESF_MAIN_INFO("EsfMainProcessFactoryReset called. is_downgrade:%u",
                is_downgrade);

  int32_t ret = 0;
  bool is_failed = false;
 
  //
  // Execute processing for each module.
  //
  {
#if (defined(CONFIG_EXTERNAL_SENSOR_MAIN) || \
     defined(CONFIG_EXTERNAL_MAIN_ENABLE_SENSOR_MAIN_STUB))
    ESF_MAIN_DBG("EsfSensorUtilityResetFiles start.");
    EsfSystemManagerResult system_manager_ret = kEsfSystemManagerResultOk;
    EsfSensorErrCode sensor_ret = EsfSensorUtilityResetFiles();
    if (sensor_ret != kEsfSensorOk) {
      ESF_MAIN_ERR("EsfSensorUtilityResetFiles ret=%d", sensor_ret);
      system_manager_ret = EsfSystemManagerSetInitialSettingFlag(
          kEsfSystemManagerInitialSettingNotCompleted);
      is_failed = true;
      if (system_manager_ret != kEsfSystemManagerResultOk) {
        ESF_MAIN_ERR("EsfSystemManagerSetInitialSettingFlag ret=%d.",
                     system_manager_ret);
        is_failed = true;
      } else {
        ESF_MAIN_INFO("Set kEsfSystemManagerInitialSettingNotCompleted");
      }
    }
    ESF_MAIN_DBG("EsfSensorUtilityResetFiles finish.");
#endif  // (CONFIG_EXTERNAL_SENSOR_MAIN ||
        // CONFIG_EXTERNAL_MAIN_ENABLE_SENSOR_MAIN_STUB)
  }
  {
    // Factory Reset ParameterStorageManager
    EsfParameterStorageManagerStatus result =
        EsfParameterStorageManagerInvokeFactoryReset();
    if (result != kEsfParameterStorageManagerStatusOk) {
      ESF_MAIN_ERR("EsfParameterStorageManagerFactoryReset return=%d", result);
      is_failed = true;
    } else {
      ESF_MAIN_INFO("EsfParameterStorageManagerFactoryReset Success");
    }

    if (is_downgrade) {
      // Downgrade ParameterStorageManager
      result = EsfParameterStorageManagerDowngrade();
      if (result != kEsfParameterStorageManagerStatusOk) {
        ESF_MAIN_ERR("EsfParameterStorageManagerDowngrade return=%d", result);
        is_failed = true;
      } else {
        ESF_MAIN_INFO("EsfParameterStorageManagerDowngrade Success");
      }
    }
  }

  {
#if (defined(CONFIG_EXTERNAL_SENSOR_MAIN) || \
     defined(CONFIG_EXTERNAL_MAIN_ENABLE_SENSOR_MAIN_STUB))
    if (is_downgrade) {
      EsfSensorErrCode sensor_ret = EsfSensorUtilityDowngrade();
      if (sensor_ret != kEsfSensorOk) {
        ESF_MAIN_ERR("EsfSensorUtilityDowngrade ret=%d", sensor_ret);
        is_failed = true;
      } else {
        ESF_MAIN_INFO("EsfSensorUtilityDowngrade Success");
      }
    }
#endif  // (CONFIG_EXTERNAL_SENSOR_MAIN ||
        // CONFIG_EXTERNAL_MAIN_ENABLE_SENSOR_MAIN_STUB)
  }

  if (is_failed) {
    ESF_MAIN_ELOG_ERR(ESF_MAIN_ELOG_FACTORY_RESET_FAILURE);
  }

  ret = EsfMainFinish(false);
  if (ret != kEsfMainOk) {
    // Continue processing even if an error occurs.
    ESF_MAIN_LOG_ERR("EsfMainFinish ret=%d", ret);
  }

  PlErrCode pl_ret = PlMainFlashFormat(EsfMainInvokeKeepAlive, NULL);
  if (pl_ret != kPlErrCodeOk) {
    ESF_MAIN_LOG_ERR("PlMainFlashFormat ret=%d", (int)pl_ret);
  }

  pl_ret = PlMainEmmcFormat(EsfMainInvokeKeepAlive, NULL);
  if (pl_ret != kPlErrCodeOk) {
    ESF_MAIN_LOG_ERR("PlMainEmmcFormat ret=%d", (int)pl_ret);
  }

  if (is_downgrade && !is_failed) {
    EsfFwMgrResult fw_result = EsfFwMgrSwitchProcessorFirmwareSlot();
    if (fw_result != kEsfFwMgrResultOk) {
      ESF_MAIN_LOG_ERR("EsfFwMgrSwitchProcessorFirmwareSlot return=%u",
                       fw_result);
    } else {
      ESF_MAIN_LOG_INFO("EsfFwMgrSwitchProcessorFirmwareSlot Success");
    }
  }

  ret = EsfMainFinishLed();
  if (ret != kEsfMainOk) {
    // Continue processing even if an error occurs.
    ESF_MAIN_LOG_ERR("EsfMainFinishLed ret=%d", ret);
  }

  // Future changes api.
  EsfPwrMgrExecuteRebootEx(EsfPwrMgrRebootTypeSW);

  ESF_MAIN_LOG_TRACE("func end");
  return kEsfMainOk;
}

// """System stop notification message reception processing.

// It waits for a system stop notification message and an end notification,
// and then completes either the processing corresponding to the received
// message or the system stop notification message reception processing.

// Args:
//     nothing.

// Returns:
//     One of the values of EsfMainError is returned
//     depending on the execution result.

// Yields:
//     kEsfMainOk: Success.
//     kEsfMainErrorExternal: External API error occurred.
//     kEsfMainErrorInternal: Internal API error occurred.

// Note:
//     This is an internal API and cannot be used externally.

// """
static EsfMainError EsfMainProcessMsg(void) {
  ESF_MAIN_TRACE("func start");
  ESF_MAIN_DBG("*buf=%p, size=%d", resource.recv_buf, resource.max_msg_size);

  while (!esf_main_finish_flag) {
    int32_t recv_size = 0;
    UtilityMsgErrCode utility_ret = UtilityMsgRecv(
        resource.utility_msg_handle, resource.recv_buf, resource.max_msg_size,
        CONFIG_EXTERNAL_MAIN_OSAL_MSG_WAITTIME_MS, &recv_size);
    if (kUtilityMsgErrTimedout == utility_ret) {
      continue;
    } else if (kUtilityMsgOk != utility_ret) {
      ESF_MAIN_ERR("UtilityMsgRecv ret=%d", utility_ret);
      ESF_MAIN_ELOG_ERR(ESF_MAIN_ELOG_SYSTEM_ERROR);
      ESF_MAIN_TRACE("func end");
      return kEsfMainErrorExternal;
    } else {
      // do nothing.
    }

    if ((size_t)recv_size != sizeof(EsfMainMsgType)) {
      ESF_MAIN_ERR("recv_size=%zu, sizeof(EsfMainMsgType)=%zu",
                   (size_t)recv_size, sizeof(EsfMainMsgType));
      ESF_MAIN_ELOG_ERR(ESF_MAIN_ELOG_INVALID_PARAM);
      ESF_MAIN_TRACE("func end");
      return kEsfMainErrorExternal;
    }

    ESF_MAIN_INFO("recv EsfMainMsgType=%d",
                  *(EsfMainMsgType *)(resource.recv_buf));

    EsfMainError ret = kEsfMainOk;
    switch (*(EsfMainMsgType *)(resource.recv_buf)) {
      case kEsfMainMsgTypeReboot:
        ret = EsfMainProcessReboot();
        break;
      case kEsfMainMsgTypeShutdown:
        ret = EsfMainProcessShutdown();
        break;
      case kEsfMainMsgTypeFactoryReset:
        ret = EsfMainProcessFactoryReset(false);
        break;
      case kEsfMainMsgTypeFactoryResetForDowngrade:
        ret = EsfMainProcessFactoryReset(true);
        break;

      default:
        ESF_MAIN_DBG("Undefine MsgType=%d",
                     *(EsfMainMsgType *)(resource.recv_buf));
        ESF_MAIN_ELOG_ERR(ESF_MAIN_ELOG_INVALID_PARAM);
        continue;
        break;
    }

    if (ret != kEsfMainOk) {
      ESF_MAIN_ERR("ret=%d, MsgType=%d", ret,
                   *(EsfMainMsgType *)(resource.recv_buf));
      ESF_MAIN_TRACE("func end");
      return ret;
    }
  }

  ESF_MAIN_TRACE("func end");
  return kEsfMainOk;
}

static int InitSigaction(void) {
  int ret = 0;
  sigset_t sig_set;
  ret = sigfillset(&sig_set);
  if (ret != 0) {
    ESF_MAIN_LOG_ERR("sigfillset error:%d", ret);
    return -1;
  }
  ret = sigprocmask(SIG_BLOCK, &sig_set, NULL);
  if (ret != 0) {
    ESF_MAIN_LOG_ERR("sigprocmask error:%d errno=%d", ret, errno);
    return -1;
  }

  ret = sigemptyset(&sig_set);
  if (ret != 0) {
    ESF_MAIN_LOG_ERR("sigemptyset error:%d errno=%d", ret, errno);
    return -1;
  }
  const int allowed_signals[] = {SIGBUS,  SIGFPE,  SIGILL,
                                 SIGSEGV, SIGTERM, SIGINT};
  const int allowed_signals_count = sizeof(allowed_signals) /
                                    sizeof(allowed_signals[0]);
  for (int i = 0; i < allowed_signals_count; i++) {
    ret = sigaddset(&sig_set, allowed_signals[i]);
    if (ret != 0) {
      ESF_MAIN_LOG_ERR("sigaddset(%d) error:%d errno=%d", allowed_signals[i],
                       ret, errno);
      return -1;
    }
  }
  ret = sigprocmask(SIG_UNBLOCK, &sig_set, NULL);
  if (ret != 0) {
    ESF_MAIN_LOG_ERR("sigprocmask error:%d errno=%d", ret, errno);
    return -1;
  }

  struct sigaction sa;
  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = EsfMainSigHandler;
  ret = sigaction(SIGTERM, &sa, NULL);
  if (ret != 0) {
    ESF_MAIN_LOG_ERR("sigaction err:%d errno=%d", ret, errno);
    return -1;
  }
  ret = sigaction(SIGINT, &sa, NULL);
  if (ret != 0) {
    ESF_MAIN_LOG_ERR("sigaction err:%d errno=%d", ret, errno);
    return -1;
  }
  return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/
int main(int argc, FAR char *argv[]) {
  (void)argc;
  (void)argv;
  ESF_MAIN_LOG_TRACE("func start");

  EsfMainError ret = kEsfMainOk;

  // EsfMain Finish flag initialize.
  esf_main_finish_flag = 0;
  if (InitSigaction() != 0) {
    // No Elog output before initialization of LogManager
    return EXIT_FAILURE;
  }

  ret = EsfMainBoot();
  if (ret != kEsfMainOk) {
    ESF_MAIN_ERR("EsfMainBoot ret=%u", ret);
    if (is_led_manager_initialized) {
      // If an error occurs after starting the LED Manager, set the LED to the
      // Error state and keep it lit.
      EsfLedManagerLedStatusInfo status;
      status.led = kEsfLedManagerTargetLedPower;
      status.status =
          kEsfLedManagerLedStatusErrorPeripheralDriversInitializationFailed;
      status.enabled = true;
      EsfLedManagerResult ext_ret = EsfLedManagerSetStatus(&status);
      if (kEsfLedManagerSuccess != ext_ret) {
        ESF_MAIN_ERR("EsfLedManagerSetStatus ret=%d", ext_ret);
      }
      ESF_MAIN_ERR("Failed to boot, waiting for external restart operation.");
      while (1) {
        sleep(1);
      }
    }
    ESF_MAIN_LOG_TRACE("func end");
    return EXIT_FAILURE;
  }

  ret = EsfMainProcessMsg();
  if (ret != kEsfMainOk) {
    ESF_MAIN_ERR("EsfMainProcessMsg ret=%d", ret);
  }

  ret = EsfMainFinish(true);
  if (ret != kEsfMainOk) {
    ESF_MAIN_LOG_ERR("EsfMainFinish ret=%u", ret);
  }

  ESF_MAIN_LOG_TRACE("func end");
  return (ret == kEsfMainOk) ? EXIT_SUCCESS : EXIT_FAILURE;
}

EsfMainError EsfMainNotifyMsg(EsfMainMsgType type) {
  ESF_MAIN_TRACE("func start");
  ESF_MAIN_INFO(
      "notify type=%d (Reboot:0 Shutdown:1 FactoryReset:2 Downgrade:3)", type);

  int32_t ret = 0;
  PlErrCode pl_ret =
      PlMainIsFeatureSupported(EsfMainCovertMsgTypeToFeatureType(type));
  if (pl_ret == kPlErrInvalidParam) {
    ESF_MAIN_ERR("type=%d", type);
    ESF_MAIN_ELOG_ERR(ESF_MAIN_ELOG_INVALID_PARAM);
    ESF_MAIN_TRACE("func end");
    return kEsfMainErrorInvalidArgument;
  }
  if (pl_ret == kPlErrNoSupported) {
    ESF_MAIN_ERR("type=%d", type);
    ESF_MAIN_ELOG_ERR(ESF_MAIN_ELOG_INVALID_PARAM);
    ESF_MAIN_TRACE("func end");
    return kEsfMainErrorNotSupport;
  }
  if (pl_ret != kPlErrCodeOk) {
    ESF_MAIN_ERR("type=%d", type);
    ESF_MAIN_ELOG_ERR(ESF_MAIN_ELOG_INVALID_PARAM);
    ESF_MAIN_TRACE("func end");
    return kEsfMainErrorInternal;
  }
  struct timespec absolute_timeout;
  ret = EsfMainGetExpirationTime(CONFIG_EXTERNAL_MAIN_LOCKTIME_MS,
                                 &absolute_timeout);
  if (ret != kEsfMainOk) {
    ESF_MAIN_ERR("EsfMainGetExpirationTime ret=%d", ret);
    ESF_MAIN_ELOG_ERR(ESF_MAIN_ELOG_SYSTEM_ERROR);
    ESF_MAIN_TRACE("func end");
    return kEsfMainErrorExternal;
  }

  // Try to acquire the mutex with timeout
  ret = pthread_mutex_timedlock(&resource.state_mutex, &absolute_timeout);
  if (ret == ETIMEDOUT) {
    ESF_MAIN_INFO("Mutex acquisition timed out.");
    ESF_MAIN_ELOG_ERR(ESF_MAIN_ELOG_SYSTEM_ERROR);
    ESF_MAIN_TRACE("func end");
    return kEsfMainErrorTimeout;
  } else if (ret != 0) {
    ESF_MAIN_ERR("pthread_mutex_timedlock ret=%d", ret);
    ESF_MAIN_ELOG_ERR(ESF_MAIN_ELOG_SYSTEM_ERROR);
    ESF_MAIN_TRACE("func end");
    return kEsfMainErrorExternal;
  } else {
    // do nothing.
  }
  ESF_MAIN_DBG("mutex lock.");

  if (resource.is_initialized == false) {
    ESF_MAIN_INFO("resource.is_initialized=false");
    pthread_mutex_unlock(&resource.state_mutex);
    ESF_MAIN_DBG("mutex unlock.");
    ESF_MAIN_TRACE("func end");
    return kEsfMainErrorUninitialize;
  }

  int32_t sent_size = 0;
  ESF_MAIN_DBG("*msg=%p, msg=%d, size=%zu, prio=0", (void *)&type, type,
               sizeof(type));
  UtilityMsgErrCode utility_ret = UtilityMsgSend(
      resource.utility_msg_handle, &type, sizeof(type), 0, &sent_size);
  if (utility_ret != kUtilityMsgOk) {
    ESF_MAIN_ERR("UtilityMsgSend ret=%d", ret);
    ESF_MAIN_ELOG_ERR(ESF_MAIN_ELOG_SYSTEM_ERROR);
    pthread_mutex_unlock(&resource.state_mutex);
    ESF_MAIN_DBG("mutex unlock.");
    ESF_MAIN_TRACE("func end");
    return kEsfMainErrorExternal;
  } else if ((size_t)sent_size != sizeof(type)) {
    ESF_MAIN_ERR("sent_size=%zu, sizeof(type)=%zu", (size_t)sent_size,
                 sizeof(type));
    ESF_MAIN_ELOG_ERR(ESF_MAIN_ELOG_SYSTEM_ERROR);
    pthread_mutex_unlock(&resource.state_mutex);
    ESF_MAIN_DBG("mutex unlock.");
    ESF_MAIN_TRACE("func end");
    return kEsfMainErrorExternal;
  } else {
    // do nothing.
  }

  pthread_mutex_unlock(&resource.state_mutex);
  ESF_MAIN_DBG("mutex unlock.");

  ESF_MAIN_TRACE("func end");
  return kEsfMainOk;
}
