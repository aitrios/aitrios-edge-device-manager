/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// Includes --------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <pthread.h>

#include "pl.h"
#include "pl_network.h"
#include "pl_network_internal.h"
#include "pl_wifi.h"
#include "pl_wifi_impl.h"
#include "utility_log.h"
#include "utility_log_module_id.h"

// Macros ---------------------------------------------------------------------
#define EVENT_ID  (0x9600)
#define EVENT_UID_START (0xB0)
#define EVENT_ID_START (EVENT_UID_START + 0x01)

#define NETWORK_LOG_ERR(event_id, format, ...) \
  WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:" \
                   format, __FILE__, __LINE__, ##__VA_ARGS__); \
  WRITE_ELOG_ERROR(MODULE_ID_SYSTEM, (EVENT_ID | (EVENT_ID_START + event_id)));

#define NETWORK_LOG_ERR_WITH_ID(event_id, format, ...) \
  WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:" \
                   format, __FILE__, __LINE__, ##__VA_ARGS__); \
  WRITE_ELOG_ERROR(MODULE_ID_SYSTEM, (EVENT_ID | event_id));

#define NETWORK_LOG_INF(format, ...) \
  WRITE_DLOG_INFO(MODULE_ID_SYSTEM, "%s-%d:" \
                   format, __FILE__, __LINE__, ##__VA_ARGS__)

#define PL_WIFI_ELOG_OS_ERROR              (EVENT_UID_START + 0x00)

#define WIFI_SHOW_STATE(state)          ((state < kPlWifiStateMax) ? \
                                        kWifiState[state] : "Unknown")

// Typedefs --------------------------------------------------------------------
// Wi-Fi Statement
typedef enum {
  kPlWifiStandby = 0,
  kPlWifiReady,
  kPlWifiRunning,
  kPlWifiStateMax
} PlWifiState;

// Wi-Fi info
typedef struct {
  char                    if_name[PL_NETWORK_IFNAME_LEN];
  pthread_mutex_t         mutex;
  PlWifiState             state;
  PlNetworkWifiMode       mode;
} PlWifiInfo;

// Global Variables ------------------------------------------------------------
static const char *kWifiState[kPlWifiStateMax] = {
  "Standby",
  "Ready",
  "Running"
};

// Wi-Fi Operations
static const PlNetworkOps s_wifi_osp = {
  .set_config  = PlWifiSetConfig,
  .get_config  = PlWifiGetConfig,
  .get_status  = PlWifiGetStatus,
  .reg_event   = PlWifiRegisterEventHandler,
  .unreg_event = PlWifiUnregisterEventHandler,
  .start       = PlWifiStart,
  .stop        = PlWifiStop,
};

// Local functions -------------------------------------------------------------


// Functions -------------------------------------------------------------------
// -----------------------------------------------------------------------------
//  PlWifiInitialize
//
//  Perform Wi-Fi feature general initialize.
//
//  Args:
//    net_info(struct network_info *): Network information.
//    thread_priority(uint32_t): Thread priority.
//
//  Returns:
//    PlErrCode: kPlErrCodeOk on successful,
//               other than kPlErrCodeOk on failure.
//
//  Note:
//
// -----------------------------------------------------------------------------
PlErrCode PlWifiInitialize(struct network_info *net_info,
                           uint32_t thread_priority) {
  (void)thread_priority;  // Avoid compiler warning
  PlErrCode err_code = kPlErrCodeOk;
  int ret = OK;
  PlWifiInfo *info = NULL;

  // argument check
  if (net_info == NULL) {
    err_code = kPlErrInvalidParam;
    NETWORK_LOG_ERR(0x00, "argument error.");
    goto err_end1;
  }

  // initialized
  if (net_info->if_info != NULL) {
    err_code = kPlErrInternal;
    NETWORK_LOG_ERR(0x01, "%s is initalized.", net_info->if_name);
    goto err_end1;
  }

  // malloc interface info
  info = (PlWifiInfo *)malloc(sizeof(PlWifiInfo));
  if (info == NULL) {
    err_code = kPlErrMemory;
    NETWORK_LOG_ERR_WITH_ID(PL_WIFI_ELOG_OS_ERROR, "malloc() failed.");
    goto err_end1;
  }
  memset(info, 0, sizeof(PlWifiInfo));

  // set interface info
  strncpy(info->if_name, net_info->if_name, (PL_NETWORK_IFNAME_LEN - 1));
  info->if_name[(PL_NETWORK_IFNAME_LEN - 1)] = '\0';
  info->state = kPlWifiReady;
  info->mode = kPlNetworkWifiModeUnkown;

  // mutex initialize
  ret = pthread_mutex_init(&(info->mutex), NULL);
  if (ret != OK) {
    err_code = kPlErrLock;
    NETWORK_LOG_ERR_WITH_ID(PL_WIFI_ELOG_OS_ERROR,
                            "pthread_mutex_init() failed.(ret=%d, errno=%d)",
                            ret, errno);
    goto err_end2;
  }

  // mutex lock
  ret = pthread_mutex_lock(&(info->mutex));
  if (ret != OK) {
    err_code = kPlErrLock;
    NETWORK_LOG_ERR_WITH_ID(PL_WIFI_ELOG_OS_ERROR,
                            "pthread_mutex_lock() failed.(ret=%d, errno=%d)",
                            ret, errno);
    goto err_end3;
  }

  // set interface info
  net_info->ops = (PlNetworkOps *)&s_wifi_osp;
  net_info->if_info = (void *)info;

  // mutex unlock
  ret = pthread_mutex_unlock(&(info->mutex));
  if (ret != OK) {
    NETWORK_LOG_ERR_WITH_ID(PL_WIFI_ELOG_OS_ERROR,
                        "pthread_mutex_unlock() failed.(ret=%d, errno=%d)",
                        ret, errno);
  }

  return kPlErrCodeOk;

err_end3:
  // mutex destroy
  pthread_mutex_destroy(&info->mutex);

err_end2:
  free(info);

err_end1:
  return err_code;
}

// -----------------------------------------------------------------------------
//  PlWifiFinalize
//
//  Perform Wi-Fi feature general finalize.
//
//  Args:
//    net_info(struct network_info *): Network information.
//
//  Returns:
//    PlErrCode: kPlErrCodeOk on successful,
//               other than kPlErrCodeOk on failure.
//
//  Note:
//
// -----------------------------------------------------------------------------
PlErrCode PlWifiFinalize(struct network_info *net_info) {
  PlErrCode err_code = kPlErrCodeOk;
  PlWifiInfo *info = NULL;

  // argument check
  if (net_info == NULL) {
    err_code = kPlErrInvalidParam;
    NETWORK_LOG_ERR(0x02, "argument error.");
    goto err_end;
  }

  // get wi-fi info
  info = (PlWifiInfo *)net_info->if_info;

  // interface error
  if (info == NULL) {
    err_code = kPlErrNotFound;
    NETWORK_LOG_ERR(0x03, "%s is not found.", net_info->if_name);
    goto err_end;
  }

  // mutex destroy
  pthread_mutex_destroy(&info->mutex);

  // clear interface info
  free(info);
  net_info->if_info = NULL;
  net_info->ops = NULL;

err_end:
  return err_code;
}

// -----------------------------------------------------------------------------
//  PlWifiSetConfig
//
//  Set network interface configuration.
//
//  Args:
//    net_info(struct network_info *): Network information.
//    config(const PlNetworkConfig *): Network configuration.
//
//  Returns:
//    PlErrCode: kPlErrCodeOk on successful,
//               other than kPlErrCodeOk on failure.
//
//  Note:
//    No configuration item exists.
//
// -----------------------------------------------------------------------------
PlErrCode PlWifiSetConfig(struct network_info *net_info,
                          const PlNetworkConfig *config) {
  PlErrCode err_code = kPlErrCodeOk;
  int ret = OK;
  PlWifiInfo *info = NULL;

  // argument check
  if ((net_info == NULL) || (config == NULL)) {
    err_code = kPlErrInvalidParam;
    NETWORK_LOG_ERR(0x04, "argument error.");
    goto err_end1;
  }

  // get wi-fi info
  info = (PlWifiInfo *)net_info->if_info;

  // interface error
  if (info == NULL) {
    err_code = kPlErrNotFound;
    NETWORK_LOG_ERR(0x05, "%s is not found.", net_info->if_name);
    goto err_end1;
  }

  // mutex lock
  ret = pthread_mutex_lock(&(info->mutex));
  if (ret != OK) {
    err_code = kPlErrLock;
    NETWORK_LOG_ERR_WITH_ID(PL_WIFI_ELOG_OS_ERROR,
                            "pthread_mutex_lock() failed.(ret=%d, errno=%d)",
                            ret, errno);
    goto err_end1;
  }

  // state check
  if (info->state != kPlWifiReady) {
    err_code = kPlErrInvalidState;
    NETWORK_LOG_ERR(0x06, "state error.(state=%s)",
                    WIFI_SHOW_STATE(info->state));
    goto err_end2;
  }

  // set config
  switch (config->network.wifi.mode) {
    // AP mode
    case kPlNetworkWifiModeAp:
      err_code = PlWifiApSetConfigImpl(net_info->if_name, config);
      break;

    // STA mode
    case kPlNetworkWifiModeSta:
      err_code = PlWifiStaSetConfigImpl(net_info->if_name, config);
      break;

    default:
      // mode error
      err_code = kPlErrNoSupported;
      NETWORK_LOG_ERR(0x07, "mode error.(mode=%d)", info->mode);
      goto err_end2;
      break;
  }

  // store wi-fi mode
  info->mode = config->network.wifi.mode;

err_end2:
  // mutex unlock
  ret = pthread_mutex_unlock(&(info->mutex));
  if (ret != OK) {
    NETWORK_LOG_ERR_WITH_ID(PL_WIFI_ELOG_OS_ERROR,
                        "pthread_mutex_unlock() failed.(ret=%d, errno=%d)",
                        ret, errno);
  }

err_end1:
  return err_code;
}

// -----------------------------------------------------------------------------
//  PlWifiGetConfig
//
//  Get network interface configuration.
//
//  Args:
//    net_info(struct network_info *): Network information.
//    config(PlNetworkConfig *): Network configuration.
//
//  Returns:
//    PlErrCode: kPlErrCodeOk on successful,
//               other than kPlErrCodeOk on failure.
//
//  Note:
//
// -----------------------------------------------------------------------------
PlErrCode PlWifiGetConfig(struct network_info *net_info,
                          PlNetworkConfig *config) {
  PlErrCode err_code = kPlErrCodeOk;
  int ret = OK;
  PlWifiInfo *info = NULL;

  // argument check
  if ((net_info == NULL) || (config == NULL)) {
    err_code = kPlErrInvalidParam;
    NETWORK_LOG_ERR(0x08, "argument error.");
    goto err_end1;
  }

  // get wi-fi info
  info = (PlWifiInfo *)net_info->if_info;

  // interface error
  if (info == NULL) {
    err_code = kPlErrNotFound;
    NETWORK_LOG_ERR(0x09, "%s is not found.", net_info->if_name);
    goto err_end1;
  }

  // mutex lock
  ret = pthread_mutex_lock(&(info->mutex));
  if (ret != OK) {
    err_code = kPlErrLock;
    NETWORK_LOG_ERR_WITH_ID(PL_WIFI_ELOG_OS_ERROR,
                            "pthread_mutex_lock() failed.(ret=%d)", ret);
    goto err_end1;
  }

  // get config
  switch (info->mode) {
    // AP mode
    case kPlNetworkWifiModeAp:
      err_code = PlWifiApGetConfigImpl(net_info->if_name, config);
      break;

    // STA mode
    case kPlNetworkWifiModeSta:
      err_code = PlWifiStaGetConfigImpl(net_info->if_name, config);
      break;

    default:
      // mode error
      err_code = kPlErrNoSupported;
      NETWORK_LOG_ERR(0x0A, "mode error.(mode=%d)", info->mode);
      goto err_end2;
      break;
  }

  // set wi-fi mode
  config->network.wifi.mode = info->mode;

err_end2:
  // mutex unlock
  ret = pthread_mutex_unlock(&(info->mutex));
  if (ret != OK) {
    NETWORK_LOG_ERR_WITH_ID(PL_WIFI_ELOG_OS_ERROR,
                        "pthread_mutex_unlock() failed.(ret=%d, errno=%d)",
                        ret, errno);
  }

err_end1:
  return err_code;
}

// -----------------------------------------------------------------------------
//  PlWifiGetStatus
//
//  Get network interface status.
//
//  Args:
//    net_info(struct network_info *): Network information.
//    status(PlNetworkStatus *): Network status.
//
//  Returns:
//    PlErrCode: kPlErrCodeOk on successful,
//               other than kPlErrCodeOk on failure.
//
//  Note:
//
// -----------------------------------------------------------------------------
PlErrCode PlWifiGetStatus(struct network_info *net_info,
                          PlNetworkStatus *status) {
  PlErrCode err_code = kPlErrCodeOk;
  int ret = OK;
  PlWifiInfo *info = NULL;

  // argument check
  if ((net_info == NULL) || (status == NULL)) {
    err_code = kPlErrInvalidParam;
    NETWORK_LOG_ERR(0x0B, "argument error.");
    goto err_end1;
  }

  // get wi-fi info
  info = (PlWifiInfo *)net_info->if_info;

  // interface error
  if (info == NULL) {
    err_code = kPlErrNotFound;
    NETWORK_LOG_ERR(0x0C, "%s is not found.", net_info->if_name);
    goto err_end1;
  }

  // mutex lock
  ret = pthread_mutex_lock(&(info->mutex));
  if (ret != OK) {
    err_code = kPlErrLock;
    NETWORK_LOG_ERR_WITH_ID(PL_WIFI_ELOG_OS_ERROR,
                            "pthread_mutex_lock() failed.(ret=%d, errno=%d)",
                            ret, errno);
    goto err_end1;
  }

  // get status
  switch (info->mode) {
    // AP mode
    case kPlNetworkWifiModeAp:
      err_code = PlWifiApGetStatusImpl(net_info->if_name, status);
      break;

    // STA mode
    case kPlNetworkWifiModeSta:
      err_code = PlWifiStaGetStatusImpl(net_info->if_name, status);
      break;

    default:
      // mode error
      err_code = kPlErrNoSupported;
      NETWORK_LOG_ERR(0x0D, "mode error.(mode=%d)", info->mode);
      goto err_end2;
      break;
  }

err_end2:
  // mutex unlock
  ret = pthread_mutex_unlock(&(info->mutex));
  if (ret != OK) {
    NETWORK_LOG_ERR_WITH_ID(PL_WIFI_ELOG_OS_ERROR,
                        "pthread_mutex_unlock() failed.(ret=%d, errno=%d)",
                        ret, errno);
  }

err_end1:
  return err_code;
}

// -----------------------------------------------------------------------------
//  PlWifiRegisterEventHandler
//
//  Register event handler and ethernet events.
//
//  Args:
//    net_info(struct network_info *): Network information.
//
//  Returns:
//    PlErrCode: kPlErrCodeOk on successful,
//               other than kPlErrCodeOk on failure.
//
//  Note:
//
// -----------------------------------------------------------------------------
PlErrCode PlWifiRegisterEventHandler(struct network_info *net_info) {
  PlErrCode err_code = kPlErrCodeOk;
  int ret = OK;
  PlWifiInfo *info = NULL;

  // argument check
  if (net_info == NULL) {
    err_code = kPlErrInvalidParam;
    NETWORK_LOG_ERR(0x0E, "argument error.");
    goto err_end1;
  }

  // get wi-fi info
  info = (PlWifiInfo *)net_info->if_info;

  // interface error
  if (info == NULL) {
    err_code = kPlErrNotFound;
    NETWORK_LOG_ERR(0x0F, "%s is not found.", net_info->if_name);
    goto err_end1;
  }

  // mutex lock
  ret = pthread_mutex_lock(&(info->mutex));
  if (ret != OK) {
    err_code = kPlErrLock;
    NETWORK_LOG_ERR_WITH_ID(PL_WIFI_ELOG_OS_ERROR,
                            "pthread_mutex_lock() failed.(ret=%d, errno=%d)",
                            ret, errno);
    goto err_end1;
  }

  // state check
  if (info->state != kPlWifiReady) {
    err_code = kPlErrInvalidState;
    NETWORK_LOG_ERR(0x10, "state error.(state=%s)",
                    WIFI_SHOW_STATE(info->state));
    goto err_end2;
  }

  // register event
  switch (info->mode) {
    // AP mode
    case kPlNetworkWifiModeAp:
      err_code = PlWifiApRegisterEventHandlerImpl(net_info->if_name);
      break;

    // STA mode
    case kPlNetworkWifiModeSta:
      err_code = PlWifiStaRegisterEventHandlerImpl(net_info->if_name);
      break;

    default:
      // mode error
      err_code = kPlErrNoSupported;
      NETWORK_LOG_ERR(0x11, "mode error.(mode=%d)", info->mode);
      goto err_end2;
      break;
  }

err_end2:
  // mutex unlock
  ret = pthread_mutex_unlock(&(info->mutex));
  if (ret != OK) {
    NETWORK_LOG_ERR_WITH_ID(PL_WIFI_ELOG_OS_ERROR,
                        "pthread_mutex_unlock() failed.(ret=%d, errno=%d)",
                        ret, errno);
  }

err_end1:
  return err_code;
}

// -----------------------------------------------------------------------------
//  PlWifiUnregisterEventHandler
//
//  Unregister event handler and ethernet events.
//
//  Args:
//    net_info(struct network_info *): Network information.
//
//  Returns:
//    PlErrCode: kPlErrCodeOk on successful,
//               other than kPlErrCodeOk on failure.
//
//  Note:
//
// -----------------------------------------------------------------------------
PlErrCode PlWifiUnregisterEventHandler(struct network_info *net_info) {
  PlErrCode err_code = kPlErrCodeOk;
  int ret = OK;
  PlWifiInfo *info = NULL;

  // argument check
  if (net_info == NULL) {
    err_code = kPlErrInvalidParam;
    NETWORK_LOG_ERR(0x12, "argument error.");
    goto err_end1;
  }

  // get wi-fi info
  info = (PlWifiInfo *)net_info->if_info;

  // interface error
  if (info == NULL) {
    err_code = kPlErrNotFound;
    NETWORK_LOG_ERR(0x13, "%s is not found.", net_info->if_name);
    goto err_end1;
  }

  // mutex lock
  ret = pthread_mutex_lock(&(info->mutex));
  if (ret != OK) {
    err_code = kPlErrLock;
    NETWORK_LOG_ERR_WITH_ID(PL_WIFI_ELOG_OS_ERROR,
                            "pthread_mutex_lock() failed.(ret=%d, errno=%d)",
                            ret, errno);
    goto err_end1;
  }

  // state check
  if (info->state != kPlWifiReady) {
    err_code = kPlErrInvalidState;
    NETWORK_LOG_ERR(0x14, "state error.(state=%s)",
                    WIFI_SHOW_STATE(info->state));
    goto err_end2;
  }

  // unregister event
  switch (info->mode) {
    // AP mode
    case kPlNetworkWifiModeAp:
      err_code = PlWifiApUnregisterEventHandlerImpl(net_info->if_name);
      break;

    // STA mode
    case kPlNetworkWifiModeSta:
      err_code = PlWifiStaUnregisterEventHandlerImpl(net_info->if_name);
      break;

    default:
      // mode error
      err_code = kPlErrNoSupported;
      NETWORK_LOG_ERR(0x15, "mode error.(mode=%d)", info->mode);
      goto err_end2;
      break;
  }

err_end2:
  // mutex unlock
  ret = pthread_mutex_unlock(&(info->mutex));
  if (ret != OK) {
    NETWORK_LOG_ERR_WITH_ID(PL_WIFI_ELOG_OS_ERROR,
                        "pthread_mutex_unlock() failed.(ret=%d, errno=%d)",
                        ret, errno);
  }

err_end1:
  return err_code;
}

// -----------------------------------------------------------------------------
//  PlWifiStart
//
//  Enable network interface and start network connection.
//
//  Args:
//    net_info(struct network_info *): Network information.
//
//  Returns:
//    PlErrCode: kPlErrCodeOk on successful,
//               other than kPlErrCodeOk on failure.
//
//  Note:
//
// -----------------------------------------------------------------------------
PlErrCode PlWifiStart(struct network_info *net_info) {
  PlErrCode err_code = kPlErrCodeOk;
  int ret = OK;
  PlWifiInfo *info = NULL;

  // argument check
  if (net_info == NULL) {
    err_code = kPlErrInvalidParam;
    NETWORK_LOG_ERR(0x16, "argument error.");
    goto err_end1;
  }

  // get wi-fi info
  info = (PlWifiInfo *)net_info->if_info;

  // interface error
  if (info == NULL) {
    err_code = kPlErrNotFound;
    NETWORK_LOG_ERR(0x17, "%s is not found.", net_info->if_name);
    goto err_end1;
  }

  // mutex lock
  ret = pthread_mutex_lock(&(info->mutex));
  if (ret != OK) {
    err_code = kPlErrLock;
    NETWORK_LOG_ERR_WITH_ID(PL_WIFI_ELOG_OS_ERROR,
                            "pthread_mutex_lock() failed.(ret=%d, errno=%d)",
                            ret, errno);
    goto err_end1;
  }

  // state check
  if (info->state != kPlWifiReady) {
    err_code = kPlErrInvalidState;
    NETWORK_LOG_ERR(0x18, "state error.(state=%s)",
                    WIFI_SHOW_STATE(info->state));
    goto err_end2;
  }

  // start
  switch (info->mode) {
    // AP mode
    case kPlNetworkWifiModeAp:
      err_code = PlWifiApStartImpl(net_info->if_name);
      break;

    // STA mode
    case kPlNetworkWifiModeSta:
      err_code = PlWifiStaStartImpl(net_info->if_name);
      break;

    default:
      // mode error
      err_code = kPlErrNoSupported;
      NETWORK_LOG_ERR(0x19, "mode error.(mode=%d)", info->mode);
      goto err_end2;
      break;
  }

  // Ready -> Running
  info->state = kPlWifiRunning;

err_end2:
  // mutex unlock
  ret = pthread_mutex_unlock(&(info->mutex));
  if (ret != OK) {
    NETWORK_LOG_ERR_WITH_ID(PL_WIFI_ELOG_OS_ERROR,
                        "pthread_mutex_unlock() failed.(ret=%d, errno=%d)",
                        ret, errno);
  }
  NETWORK_LOG_INF("%s network start.(state=%s)",
                  net_info->if_name, WIFI_SHOW_STATE(info->state));

err_end1:
  return err_code;
}

// -----------------------------------------------------------------------------
//  PlEtherStop
//
//  Disable network interface and stop network connection.
//
//  Args:
//    net_info(struct network_info *): Network information.
//
//  Returns:
//    PlErrCode: kPlErrCodeOk on successful,
//               other than kPlErrCodeOk on failure.
//
//  Note:
//
// -----------------------------------------------------------------------------
PlErrCode PlWifiStop(struct network_info *net_info) {
  PlErrCode err_code = kPlErrCodeOk;
  int ret = OK;
  PlWifiInfo *info = NULL;

  // argument check
  if (net_info == NULL) {
    err_code = kPlErrInvalidParam;
    NETWORK_LOG_ERR(0x1A, "argument error.");
    goto err_end1;
  }

  // get wi-fi info
  info = (PlWifiInfo *)net_info->if_info;

  // interface error
  if (info == NULL) {
    err_code = kPlErrNotFound;
    NETWORK_LOG_ERR(0x1B, "%s is not found.", net_info->if_name);
    goto err_end1;
  }

  // mutex lock
  ret = pthread_mutex_lock(&(info->mutex));
  if (ret != OK) {
    err_code = kPlErrLock;
    NETWORK_LOG_ERR_WITH_ID(PL_WIFI_ELOG_OS_ERROR,
                            "pthread_mutex_lock() failed.(ret=%d, errno=%d)",
                            ret, errno);
    goto err_end1;
  }

  // state check
  if (info->state != kPlWifiRunning) {
    err_code = kPlErrInvalidState;
    NETWORK_LOG_ERR(0x1C, "state error.(state=%s)",
                    WIFI_SHOW_STATE(info->state));
    goto err_end2;
  }

  // stop
  switch (info->mode) {
    // AP mode
    case kPlNetworkWifiModeAp:
      err_code = PlWifiApStopImpl(net_info->if_name);
      break;

    // STA mode
    case kPlNetworkWifiModeSta:
      err_code = PlWifiStaStopImpl(net_info->if_name);
      break;

    default:
      // mode error
      err_code = kPlErrNoSupported;
      NETWORK_LOG_ERR(0x1D, "mode error.(mode=%d)", info->mode);
      goto err_end2;
      break;
  }

  // Running -> Ready
  info->state = kPlWifiReady;
  NETWORK_LOG_INF("%s network stop.(state=%s)",
                  net_info->if_name, WIFI_SHOW_STATE(info->state));

err_end2:
  // mutex unlock
  ret = pthread_mutex_unlock(&(info->mutex));
  if (ret != OK) {
    NETWORK_LOG_ERR_WITH_ID(PL_WIFI_ELOG_OS_ERROR,
                        "pthread_mutex_unlock() failed.(ret=%d, errno=%d)",
                        ret, errno);
  }

err_end1:
  return err_code;
}
