/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// Includes --------------------------------------------------------------------
#include "pl_wifi.h"

#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pl.h"
#include "pl_network.h"
#include "pl_network_internal.h"
#include "pl_network_log.h"
#include "pl_wifi_ap.h"
#include "pl_wifi_sta.h"
#include "utility_log.h"
#include "utility_log_module_id.h"

// Macros ---------------------------------------------------------------------
#define WIFI_SHOW_STATE(state) \
  ((state < kPlWifiStateMax) ? kWifiState[state] : "Unknown")

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
  char if_name[PL_NETWORK_IFNAME_LEN];
  pthread_mutex_t mutex;
  PlWifiState state;
  PlNetworkWifiMode mode;
} PlWifiInfo;

// Global Variables ------------------------------------------------------------
static const char *kWifiState[kPlWifiStateMax] = {
    "Standby",
    "Ready",
    "Running",
};

// Wi-Fi Operations
static const PlNetworkOps s_wifi_osp = {
    .set_config = PlWifiSetConfig,
    .get_config = PlWifiGetConfig,
    .get_status = PlWifiGetStatus,
    .reg_event = PlWifiRegisterEventHandler,
    .unreg_event = PlWifiUnregisterEventHandler,
    .start = PlWifiStart,
    .stop = PlWifiStop,
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
  int ret = 0;
  PlWifiInfo *info = NULL;

  // argument check
  if (net_info == NULL) {
    err_code = kPlErrInvalidParam;
    DLOGE("argument error.");
    ELOGE(kElog_PlErrInvalidParam);
    ELOGE(kElog_PlWifiInitialize);
    goto err_end1;
  }

  // initialized
  if (net_info->if_info != NULL) {
    err_code = kPlErrInternal;
    DLOGE("%s is initalized.", net_info->if_name);
    ELOGE(kElog_PlErrInternal);
    ELOGE(kElog_PlWifiInitialize);
    goto err_end1;
  }

  // malloc interface info
  info = (PlWifiInfo *)malloc(sizeof(PlWifiInfo));
  if (info == NULL) {
    err_code = kPlErrMemory;
    DLOGE("malloc() failed.");
    ELOGE(kElog_PlErrMemory);
    ELOGE(kElog_PlWifiInitialize);
    goto err_end1;
  }
  memset(info, 0, sizeof(PlWifiInfo));

  // set interface info
  memcpy(info->if_name, net_info->if_name, (PL_NETWORK_IFNAME_LEN - 1));
  info->if_name[(PL_NETWORK_IFNAME_LEN - 1)] = '\0';
  info->state = kPlWifiReady;
  info->mode = kPlNetworkWifiModeUnkown;

  // mutex initialize
  ret = pthread_mutex_init(&(info->mutex), NULL);
  if (ret != 0) {
    err_code = kPlErrLock;
    DLOGE("pthread_mutex_init() failed.(ret=%d, errno=%d)", ret, errno);
    ELOGE(kElog_PlErrLock);
    ELOGE(kElog_PlWifiInitialize);
    goto err_end2;
  }

  // mutex lock
  ret = pthread_mutex_lock(&(info->mutex));
  if (ret != 0) {
    err_code = kPlErrLock;
    DLOGE("pthread_mutex_lock() failed.(ret=%d, errno=%d)", ret, errno);
    ELOGE(kElog_PlErrLock);
    ELOGE(kElog_PlWifiInitialize);
    goto err_end3;
  }

  // set interface info
  net_info->ops = (PlNetworkOps *)&s_wifi_osp;
  net_info->if_info = (void *)info;

  // mutex unlock
  ret = pthread_mutex_unlock(&(info->mutex));
  if (ret != 0) {
    DLOGE("pthread_mutex_unlock() failed.(ret=%d, errno=%d)", ret, errno);
    ELOGE(kElog_PlErrUnlock);
    ELOGE(kElog_PlWifiInitialize);
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
    DLOGE("argument error.");
    ELOGE(kElog_PlErrInvalidParam);
    ELOGE(kElog_PlWifiFinalize);
    goto err_end;
  }

  // get wi-fi info
  info = (PlWifiInfo *)net_info->if_info;

  // interface error
  if (info == NULL) {
    err_code = kPlErrNotFound;
    DLOGE("%s is not found.", net_info->if_name);
    ELOGE(kElog_PlErrNotFound);
    ELOGE(kElog_PlWifiFinalize);
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
  int ret = 0;
  PlWifiInfo *info = NULL;

  // argument check
  if ((net_info == NULL) || (config == NULL)) {
    err_code = kPlErrInvalidParam;
    DLOGE("argument error.");
    ELOGE(kElog_PlErrInvalidParam);
    ELOGE(kElog_PlWifiSetConfig);
    goto err_end1;
  }

  // get wi-fi info
  info = (PlWifiInfo *)net_info->if_info;

  // interface error
  if (info == NULL) {
    err_code = kPlErrNotFound;
    DLOGE("%s is not found.", net_info->if_name);
    ELOGE(kElog_PlErrNotFound);
    ELOGE(kElog_PlWifiSetConfig);
    goto err_end1;
  }

  // mutex lock
  ret = pthread_mutex_lock(&(info->mutex));
  if (ret != 0) {
    err_code = kPlErrLock;
    DLOGE("pthread_mutex_lock() failed.(ret=%d, errno=%d)", ret, errno);
    ELOGE(kElog_PlErrLock);
    ELOGE(kElog_PlWifiSetConfig);
    goto err_end1;
  }

  // state check
  if (info->state != kPlWifiReady) {
    err_code = kPlErrInvalidState;
    DLOGE("state error.(state=%s)", WIFI_SHOW_STATE(info->state));
    ELOGE(kElog_PlErrInvalidState);
    ELOGE(kElog_PlWifiSetConfig);
    goto err_end2;
  }

  // set config
  switch (config->network.wifi.mode) {
    // AP mode
    case kPlNetworkWifiModeAp:
      err_code = PlWifiApSetConfig(net_info->if_name, config);
      break;

    // STA mode
    case kPlNetworkWifiModeSta:
      err_code = PlWifiStaSetConfig(net_info->if_name, config);
      break;

    default:
      // mode error
      err_code = kPlErrNoSupported;
      DLOGE("mode error.(mode=%d)", info->mode);
      ELOGE(kElog_PlErrNoSupported);
      ELOGE(kElog_PlWifiSetConfig);
      goto err_end2;
      break;
  }

  // store wi-fi mode
  info->mode = config->network.wifi.mode;

err_end2:
  // mutex unlock
  ret = pthread_mutex_unlock(&(info->mutex));
  if (ret != 0) {
    DLOGE("pthread_mutex_unlock() failed.(ret=%d, errno=%d)", ret, errno);
    ELOGE(kElog_PlErrUnlock);
    ELOGE(kElog_PlWifiSetConfig);
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
  int ret = 0;
  PlWifiInfo *info = NULL;

  // argument check
  if ((net_info == NULL) || (config == NULL)) {
    err_code = kPlErrInvalidParam;
    DLOGE("argument error.");
    ELOGE(kElog_PlErrInvalidParam);
    ELOGE(kElog_PlWifiGetConfig);
    goto err_end1;
  }

  // get wi-fi info
  info = (PlWifiInfo *)net_info->if_info;

  // interface error
  if (info == NULL) {
    err_code = kPlErrNotFound;
    DLOGE("%s is not found.", net_info->if_name);
    ELOGE(kElog_PlErrNotFound);
    ELOGE(kElog_PlWifiGetConfig);
    goto err_end1;
  }

  // mutex lock
  ret = pthread_mutex_lock(&(info->mutex));
  if (ret != 0) {
    err_code = kPlErrLock;
    DLOGE("pthread_mutex_lock() failed.(ret=%d)", ret);
    ELOGE(kElog_PlErrLock);
    ELOGE(kElog_PlWifiGetConfig);
    goto err_end1;
  }

  // get config
  switch (info->mode) {
    // AP mode
    case kPlNetworkWifiModeAp:
      err_code = PlWifiApGetConfig(net_info->if_name, config);
      break;

    // STA mode
    case kPlNetworkWifiModeSta:
      err_code = PlWifiStaGetConfig(net_info->if_name, config);
      break;

    default:
      // mode error
      err_code = kPlErrNoSupported;
      DLOGE("mode error.(mode=%d)", info->mode);
      ELOGE(kElog_PlErrNoSupported);
      ELOGE(kElog_PlWifiGetConfig);
      goto err_end2;
      break;
  }

  // set wi-fi mode
  config->network.wifi.mode = info->mode;

err_end2:
  // mutex unlock
  ret = pthread_mutex_unlock(&(info->mutex));
  if (ret != 0) {
    DLOGE("pthread_mutex_unlock() failed.(ret=%d, errno=%d)", ret, errno);
    ELOGE(kElog_PlErrUnlock);
    ELOGE(kElog_PlWifiGetConfig);
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
  int ret = 0;
  PlWifiInfo *info = NULL;

  // argument check
  if ((net_info == NULL) || (status == NULL)) {
    err_code = kPlErrInvalidParam;
    DLOGE("argument error.");
    ELOGE(kElog_PlErrInvalidParam);
    ELOGE(kElog_PlWifiGetStatus);
    goto err_end1;
  }

  // get wi-fi info
  info = (PlWifiInfo *)net_info->if_info;

  // interface error
  if (info == NULL) {
    err_code = kPlErrNotFound;
    DLOGE("%s is not found.", net_info->if_name);
    ELOGE(kElog_PlErrNotFound);
    ELOGE(kElog_PlWifiGetStatus);
    goto err_end1;
  }

  // mutex lock
  ret = pthread_mutex_lock(&(info->mutex));
  if (ret != 0) {
    err_code = kPlErrLock;
    DLOGE("pthread_mutex_lock() failed.(ret=%d, errno=%d)", ret, errno);
    ELOGE(kElog_PlErrLock);
    ELOGE(kElog_PlWifiGetStatus);
    goto err_end1;
  }

  // get status
  switch (info->mode) {
    // AP mode
    case kPlNetworkWifiModeAp:
      err_code = PlWifiApGetStatus(net_info->if_name, status);
      break;

    // STA mode
    case kPlNetworkWifiModeSta:
      err_code = PlWifiStaGetStatus(net_info->if_name, status);
      break;

    default:
      // mode error
      err_code = kPlErrNoSupported;
      DLOGE("mode error.(mode=%d)", info->mode);
      ELOGE(kElog_PlErrNoSupported);
      ELOGE(kElog_PlWifiGetStatus);
      goto err_end2;
      break;
  }

err_end2:
  // mutex unlock
  ret = pthread_mutex_unlock(&(info->mutex));
  if (ret != 0) {
    DLOGE("pthread_mutex_unlock() failed.(ret=%d, errno=%d)", ret, errno);
    ELOGE(kElog_PlErrUnlock);
    ELOGE(kElog_PlWifiGetStatus);
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
  int ret = 0;
  PlWifiInfo *info = NULL;

  // argument check
  if (net_info == NULL) {
    err_code = kPlErrInvalidParam;
    DLOGE("argument error.");
    ELOGE(kElog_PlErrInvalidParam);
    ELOGE(kElog_PlWifiRegisterEventHandler);
    goto err_end1;
  }

  // get wi-fi info
  info = (PlWifiInfo *)net_info->if_info;

  // interface error
  if (info == NULL) {
    err_code = kPlErrNotFound;
    DLOGE("%s is not found.", net_info->if_name);
    ELOGE(kElog_PlErrNotFound);
    ELOGE(kElog_PlWifiRegisterEventHandler);
    goto err_end1;
  }

  // mutex lock
  ret = pthread_mutex_lock(&(info->mutex));
  if (ret != 0) {
    err_code = kPlErrLock;
    DLOGE("pthread_mutex_lock() failed.(ret=%d, errno=%d)", ret, errno);
    ELOGE(kElog_PlErrLock);
    ELOGE(kElog_PlWifiRegisterEventHandler);
    goto err_end1;
  }

  // state check
  if (info->state != kPlWifiReady) {
    err_code = kPlErrInvalidState;
    DLOGE("state error.(state=%s)", WIFI_SHOW_STATE(info->state));
    ELOGE(kElog_PlErrInvalidState);
    ELOGE(kElog_PlWifiRegisterEventHandler);
    goto err_end2;
  }

  // register event
  switch (info->mode) {
    // AP mode
    case kPlNetworkWifiModeAp:
      err_code = PlWifiApRegisterEventHandler(net_info->if_name);
      break;

    // STA mode
    case kPlNetworkWifiModeSta:
      err_code = PlWifiStaRegisterEventHandler(net_info->if_name);
      break;

    default:
      // mode error
      err_code = kPlErrNoSupported;
      DLOGE("mode error.(mode=%d)", info->mode);
      ELOGE(kElog_PlErrNoSupported);
      ELOGE(kElog_PlWifiRegisterEventHandler);
      goto err_end2;
      break;
  }

err_end2:
  // mutex unlock
  ret = pthread_mutex_unlock(&(info->mutex));
  if (ret != 0) {
    DLOGE("pthread_mutex_unlock() failed.(ret=%d, errno=%d)", ret, errno);
    ELOGE(kElog_PlErrUnlock);
    ELOGE(kElog_PlWifiRegisterEventHandler);
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
  int ret = 0;
  PlWifiInfo *info = NULL;

  // argument check
  if (net_info == NULL) {
    err_code = kPlErrInvalidParam;
    DLOGE("argument error.");
    ELOGE(kElog_PlErrInvalidParam);
    ELOGE(kElog_PlWifiUnregisterEventHandler);
    goto err_end1;
  }

  // get wi-fi info
  info = (PlWifiInfo *)net_info->if_info;

  // interface error
  if (info == NULL) {
    err_code = kPlErrNotFound;
    DLOGE("%s is not found.", net_info->if_name);
    ELOGE(kElog_PlErrNotFound);
    ELOGE(kElog_PlWifiUnregisterEventHandler);
    goto err_end1;
  }

  // mutex lock
  ret = pthread_mutex_lock(&(info->mutex));
  if (ret != 0) {
    err_code = kPlErrLock;
    DLOGE("pthread_mutex_lock() failed.(ret=%d, errno=%d)", ret, errno);
    ELOGE(kElog_PlErrLock);
    ELOGE(kElog_PlWifiUnregisterEventHandler);
    goto err_end1;
  }

  // state check
  if (info->state != kPlWifiReady) {
    err_code = kPlErrInvalidState;
    DLOGE("state error.(state=%s)", WIFI_SHOW_STATE(info->state));
    ELOGE(kElog_PlErrInvalidState);
    ELOGE(kElog_PlWifiUnregisterEventHandler);
    goto err_end2;
  }

  // unregister event
  switch (info->mode) {
    // AP mode
    case kPlNetworkWifiModeAp:
      err_code = PlWifiApUnregisterEventHandler(net_info->if_name);
      break;

    // STA mode
    case kPlNetworkWifiModeSta:
      err_code = PlWifiStaUnregisterEventHandler(net_info->if_name);
      break;

    default:
      // mode error
      err_code = kPlErrNoSupported;
      DLOGE("mode error.(mode=%d)", info->mode);
      ELOGE(kElog_PlErrNoSupported);
      ELOGE(kElog_PlWifiUnregisterEventHandler);
      goto err_end2;
      break;
  }

err_end2:
  // mutex unlock
  ret = pthread_mutex_unlock(&(info->mutex));
  if (ret != 0) {
    DLOGE("pthread_mutex_unlock() failed.(ret=%d, errno=%d)", ret, errno);
    ELOGE(kElog_PlErrUnlock);
    ELOGE(kElog_PlWifiUnregisterEventHandler);
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
  int ret = 0;
  PlWifiInfo *info = NULL;

  // argument check
  if (net_info == NULL) {
    err_code = kPlErrInvalidParam;
    DLOGE("argument error.");
    ELOGE(kElog_PlErrInvalidParam);
    ELOGE(kElog_PlWifiStart);
    goto err_end1;
  }

  // get wi-fi info
  info = (PlWifiInfo *)net_info->if_info;

  // interface error
  if (info == NULL) {
    err_code = kPlErrNotFound;
    DLOGE("%s is not found.", net_info->if_name);
    ELOGE(kElog_PlErrNotFound);
    ELOGE(kElog_PlWifiStart);
    goto err_end1;
  }

  // mutex lock
  ret = pthread_mutex_lock(&(info->mutex));
  if (ret != 0) {
    err_code = kPlErrLock;
    DLOGE("pthread_mutex_lock() failed.(ret=%d, errno=%d)", ret, errno);
    ELOGE(kElog_PlErrLock);
    ELOGE(kElog_PlWifiStart);
    goto err_end1;
  }

  // state check
  if (info->state != kPlWifiReady) {
    err_code = kPlErrInvalidState;
    DLOGE("state error.(state=%s)", WIFI_SHOW_STATE(info->state));
    ELOGE(kElog_PlErrInvalidState);
    ELOGE(kElog_PlWifiStart);
    goto err_end2;
  }

  // start
  switch (info->mode) {
    // AP mode
    case kPlNetworkWifiModeAp:
      err_code = PlWifiApStart(net_info->if_name);
      break;

    // STA mode
    case kPlNetworkWifiModeSta:
      err_code = PlWifiStaStart(net_info->if_name);
      break;

    default:
      // mode error
      err_code = kPlErrNoSupported;
      DLOGE("mode error.(mode=%d)", info->mode);
      ELOGE(kElog_PlErrNoSupported);
      ELOGE(kElog_PlWifiStart);
      goto err_end2;
      break;
  }

  // Ready -> Running
  info->state = kPlWifiRunning;

err_end2:
  // mutex unlock
  ret = pthread_mutex_unlock(&(info->mutex));
  if (ret != 0) {
    DLOGE("pthread_mutex_unlock() failed.(ret=%d, errno=%d)", ret, errno);
    ELOGE(kElog_PlErrUnlock);
    ELOGE(kElog_PlWifiStart);
  }
  DLOGI("%s network start.(state=%s)", net_info->if_name,
        WIFI_SHOW_STATE(info->state));

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
  int ret = 0;
  PlWifiInfo *info = NULL;

  // argument check
  if (net_info == NULL) {
    err_code = kPlErrInvalidParam;
    DLOGE("argument error.");
    ELOGE(kElog_PlErrInvalidParam);
    ELOGE(kElog_PlWifiStop);
    goto err_end1;
  }

  // get wi-fi info
  info = (PlWifiInfo *)net_info->if_info;

  // interface error
  if (info == NULL) {
    err_code = kPlErrNotFound;
    DLOGE("%s is not found.", net_info->if_name);
    ELOGE(kElog_PlErrNotFound);
    ELOGE(kElog_PlWifiStop);
    goto err_end1;
  }

  // mutex lock
  ret = pthread_mutex_lock(&(info->mutex));
  if (ret != 0) {
    err_code = kPlErrLock;
    DLOGE("pthread_mutex_lock() failed.(ret=%d, errno=%d)", ret, errno);
    ELOGE(kElog_PlErrLock);
    ELOGE(kElog_PlWifiStop);
    goto err_end1;
  }

  // state check
  if (info->state != kPlWifiRunning) {
    err_code = kPlErrInvalidState;
    DLOGE("state error.(state=%s)", WIFI_SHOW_STATE(info->state));
    ELOGE(kElog_PlErrInvalidState);
    ELOGE(kElog_PlWifiStop);
    goto err_end2;
  }

  // stop
  switch (info->mode) {
    // AP mode
    case kPlNetworkWifiModeAp:
      err_code = PlWifiApStop(net_info->if_name);
      break;

    // STA mode
    case kPlNetworkWifiModeSta:
      err_code = PlWifiStaStop(net_info->if_name);
      break;

    default:
      // mode error
      err_code = kPlErrNoSupported;
      DLOGE("mode error.(mode=%d)", info->mode);
      ELOGE(kElog_PlErrNoSupported);
      ELOGE(kElog_PlWifiStop);
      goto err_end2;
      break;
  }

  // Running -> Ready
  info->state = kPlWifiReady;
  DLOGI("%s network stop.(state=%s)", net_info->if_name,
        WIFI_SHOW_STATE(info->state));

err_end2:
  // mutex unlock
  ret = pthread_mutex_unlock(&(info->mutex));
  if (ret != 0) {
    DLOGE("pthread_mutex_unlock() failed.(ret=%d, errno=%d)", ret, errno);
    ELOGE(kElog_PlErrUnlock);
    ELOGE(kElog_PlWifiStop);
  }

err_end1:
  return err_code;
}
