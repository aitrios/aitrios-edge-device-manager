/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// Includes --------------------------------------------------------------------
#include "pl_ether.h"

#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pl.h"
#include "pl_ether_os_impl.h"
#include "pl_network.h"
#include "pl_network_config.h"
#include "pl_network_internal.h"
#include "pl_network_log.h"
#include "pl_network_util.h"
#include "utility_log.h"
#include "utility_log_module_id.h"

// Macros ----------------------------------------------------------------------
// Time unit convert
#define ETHER_TIME_CONV_MS2SEC(ms) (ms / 1000)
#define ETHER_TIME_CONV_MS2NS(ms) ((ms % 1000) * 1000000)
#define ETHER_TIME_CONV_SEC2NS(sec) (sec * 1000000000)

#define ETH_SHOW_STATE(state) kEtherState[state]
#define ETH_SHOW_IF(status) ((status == true) ? "Up" : "Down")
#define ETH_SHOW_LINK(status) ((status == true) ? "Up" : "Down")

// Global Variables ------------------------------------------------------------
static const char *kEtherState[kPlEtherStateMax] = {
    "Standby",
    "Ready",
    "Running",
};

// Ethernet Operations
static const PlNetworkOps s_ether_osp = {
    .set_config = PlEtherSetConfig,
    .get_config = PlEtherGetConfig,
    .get_status = PlEtherGetStatus,
    .reg_event = PlEtherRegisterEventHandler,
    .unreg_event = PlEtherUnregisterEventHandler,
    .start = PlEtherStart,
    .stop = PlEtherStop,
};

// Local functions -------------------------------------------------------------
// Monitor Thread
void *EtherMonitorThread(void *arg);
static PlErrCode EtherCreateMonitorThread(PlEtherInfo *ether_info,
                                          uint32_t monitor_thread_pri);
static PlErrCode EtherDestroyMonitorThread(PlEtherInfo *ether_info);

// Functions -------------------------------------------------------------------
// -----------------------------------------------------------------------------
//  PlEtherInitialize
//
//  Perform ethernet feature general initialize.
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
PlErrCode PlEtherInitialize(struct network_info *net_info,
                            uint32_t thread_priority) {
  PlErrCode err_code = kPlErrCodeOk;
  PlErrCode pl_ercd = kPlErrCodeOk;
  int ret = 0;
  PlEtherInfo *info = NULL;

  // argument check
  if (net_info == NULL) {
    err_code = kPlErrInvalidParam;
    DLOGE("argument error.");
    ELOGE(kElog_PlErrInvalidParam);
    ELOGE(kElog_PlEtherInitialize);
    goto err_end1;
  }

  // initialized
  if (net_info->if_info != NULL) {
    err_code = kPlErrInternal;
    DLOGE("%s is initalized.", net_info->if_name);
    ELOGE(kElog_PlErrInternal);
    ELOGE(kElog_PlEtherInitialize);
    goto err_end1;
  }

  // malloc interface info
  info = (PlEtherInfo *)malloc(sizeof(PlEtherInfo));
  if (info == NULL) {
    err_code = kPlErrMemory;
    DLOGE("malloc() failed.");
    ELOGE(kElog_PlErrMemory);
    ELOGE(kElog_PlEtherInitialize);
    goto err_end1;
  }
  memset(info, 0, sizeof(PlEtherInfo));

  // set interface info
  snprintf(info->if_name, PL_NETWORK_IFNAME_LEN, "%s", net_info->if_name);
  info->state = kPlEtherReady;

  // mutex initialize
  ret = pthread_mutex_init(&(info->mutex), NULL);
  if (ret != 0) {
    err_code = kPlErrLock;
    DLOGE("pthread_mutex_init() failed.(ret=%d, errno=%d)", ret, errno);
    ELOGE(kElog_PlErrLock);
    ELOGE(kElog_PlEtherInitialize);
    goto err_end2;
  }

  // mutex lock
  ret = pthread_mutex_lock(&(info->mutex));
  if (ret != 0) {
    err_code = kPlErrLock;
    DLOGE("pthread_mutex_lock() failed.(ret=%d, errno=%d)", ret, errno);
    ELOGE(kElog_PlErrLock);
    ELOGE(kElog_PlEtherInitialize);
    goto err_end3;
  }

  // device initialize
  err_code = PlEtherInitializeOsImpl(net_info->if_name);
  if (err_code != kPlErrCodeOk) {
    DLOGE("%s:%d", __func__, err_code);
    ELOGE(kElog_PlEtherInitialize);
    err_code = kPlErrDevice;
    goto err_end4;
  }

  // create monitor thread
  err_code = EtherCreateMonitorThread(info, thread_priority);
  if (err_code != kPlErrCodeOk) {
    DLOGE("monitor thread create failed. err_code=%d", err_code);
    ELOGE(kElog_PlEtherInitialize);
    err_code = kPlThreadError;
    goto err_end5;
  }

  // set interface info
  net_info->ops = (PlNetworkOps *)&s_ether_osp;
  net_info->if_info = (void *)info;

  // mutex unlock
  ret = pthread_mutex_unlock(&(info->mutex));
  if (ret != 0) {
    DLOGE("pthread_mutex_unlock() failed.(ret=%d, errno=%d)", ret, errno);
    ELOGE(kElog_PlErrUnlock);
    ELOGE(kElog_PlEtherInitialize);
  }

  return kPlErrCodeOk;

err_end5:
  // device finalize
  pl_ercd = PlEtherFinalizeOsImpl(net_info->if_name);
  if (pl_ercd != kPlErrCodeOk) {
    DLOGE("%s:%u", __func__, pl_ercd);
    ELOGE(kElog_PlEtherInitialize);
  }

err_end4:
  // mutex unlock
  ret = pthread_mutex_unlock(&(info->mutex));
  if (ret != 0) {
    DLOGE("pthread_mutex_unlock() failed.(ret=%d, errno=%d)", ret, errno);
    ELOGE(kElog_PlErrUnlock);
    ELOGE(kElog_PlEtherInitialize);
  }

err_end3:
  // mutex destroy
  pthread_mutex_destroy(&info->mutex);

err_end2:
  free(info);

err_end1:
  return err_code;
}

// -----------------------------------------------------------------------------
//  PlEtherFinalize
//
//  Perform ethernet feature general finalize.
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
PlErrCode PlEtherFinalize(struct network_info *net_info) {
  PlErrCode err_code = kPlErrCodeOk;
  PlErrCode pl_ercd = kPlErrCodeOk;
  PlEtherInfo *info = NULL;
  int ret = 0;

  // argument check
  if (net_info == NULL) {
    err_code = kPlErrInvalidParam;
    DLOGE("argument error.");
    ELOGE(kElog_PlErrInvalidParam);
    ELOGE(kElog_PlEtherFinalize);
    goto err_end;
  }

  // get ethernet info
  info = (PlEtherInfo *)net_info->if_info;

  // interface error
  if (info == NULL) {
    err_code = kPlErrNotFound;
    DLOGE("%s is not found.", net_info->if_name);
    ELOGE(kElog_PlErrNotFound);
    ELOGE(kElog_PlEtherFinalize);
    goto err_end;
  }

  // mutex lock
  ret = pthread_mutex_lock(&(info->mutex));
  if (ret != 0) {
    err_code = kPlErrLock;
    DLOGE("pthread_mutex_lock() failed.(ret=%d, errno=%d)", ret, errno);
    ELOGE(kElog_PlErrLock);
    ELOGE(kElog_PlEtherFinalize);
  }

  // device finalize
  pl_ercd = PlEtherFinalizeOsImpl(net_info->if_name);
  if (pl_ercd != kPlErrCodeOk) {
    DLOGE("%s:%d", __func__, pl_ercd);
    ELOGE(kElog_PlEtherFinalize);
    err_code = kPlErrDevice;
  }

  // mutex unlock
  ret = pthread_mutex_unlock(&(info->mutex));
  if (ret != 0) {
    DLOGE("pthread_mutex_unlock() failed.(ret=%d, errno=%d)", ret, errno);
    ELOGE(kElog_PlErrUnlock);
    ELOGE(kElog_PlEtherFinalize);
  }

  // destroy monitor thread
  pl_ercd = EtherDestroyMonitorThread(info);
  if (pl_ercd != kPlErrCodeOk) {
    DLOGE(" monitor thread destroy failed. pl_ercd=%d", pl_ercd);
    ELOGE(kElog_PlEtherFinalize);
    err_code = kPlThreadError;
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
//  PlEtherSetConfig
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
PlErrCode PlEtherSetConfig(struct network_info *net_info,
                           const PlNetworkConfig *config) {
  (void)net_info;  // Avoid compiler warning
  (void)config;    // Avoid compiler warning
  // No supported
  return kPlErrNoSupported;
}

// -----------------------------------------------------------------------------
//  PlEtherGetConfig
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
PlErrCode PlEtherGetConfig(struct network_info *net_info,
                           PlNetworkConfig *config) {
  (void)net_info;  // Avoid compiler warning
  (void)config;    // Avoid compiler warning
  // No supported
  return kPlErrNoSupported;
}

// -----------------------------------------------------------------------------
//  PlEtherGetStatus
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
PlErrCode PlEtherGetStatus(struct network_info *net_info,
                           PlNetworkStatus *status) {
  PlErrCode err_code = kPlErrCodeOk;
  int ret = 0;
  PlEtherInfo *info = NULL;

  // argument check
  if ((net_info == NULL) || (status == NULL)) {
    err_code = kPlErrInvalidParam;
    DLOGE("argument error.");
    ELOGE(kElog_PlErrInvalidParam);
    ELOGE(kElog_PlEtherGetStatus);
    goto err_end1;
  }

  // get ethernet info
  info = (PlEtherInfo *)net_info->if_info;

  // interface error
  if (info == NULL) {
    err_code = kPlErrNotFound;
    DLOGE("%s is not found.", net_info->if_name);
    ELOGE(kElog_PlErrNotFound);
    ELOGE(kElog_PlEtherGetStatus);
    goto err_end1;
  }

  // mutex lock
  ret = pthread_mutex_lock(&(info->mutex));
  if (ret != 0) {
    err_code = kPlErrLock;
    DLOGE("pthread_mutex_lock() failed.(ret=%d, errno=%d)", ret, errno);
    ELOGE(kElog_PlErrLock);
    ELOGE(kElog_PlEtherGetStatus);
    goto err_end1;
  }

  // get interface status
  status->is_if_up = info->is_if_up;

  // get link status
  status->is_link_up = info->is_link_up;

  // get phy id valid
  status->is_phy_id_valid = info->is_phy_id_valid;

  DLOGD("if_name=%s is_if_up=%s is_link_up=%s is_phy_id_valid=%s",
        net_info->if_name, ((status->is_if_up == true) ? "true" : "false"),
        ((status->is_link_up == true) ? "true" : "false"),
        ((status->is_phy_id_valid == true) ? "true" : "false"));

  // mutex unlock
  ret = pthread_mutex_unlock(&(info->mutex));
  if (ret != 0) {
    DLOGE("pthread_mutex_unlock() failed.(ret=%d, errno=%d)", ret, errno);
    ELOGE(kElog_PlErrUnlock);
    ELOGE(kElog_PlEtherGetStatus);
  }

err_end1:
  return err_code;
}

// -----------------------------------------------------------------------------
//  PlEtherRegisterEventHandler
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
PlErrCode PlEtherRegisterEventHandler(struct network_info *net_info) {
  return PlEtherRegisterEventHandlerOsImpl(net_info);
}

// -----------------------------------------------------------------------------
//  PlEtherUnregisterEventHandler
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
PlErrCode PlEtherUnregisterEventHandler(struct network_info *net_info) {
  (void)net_info;  // Avoid compiler warning
  // No unregister event
  return kPlErrCodeOk;
}

// -----------------------------------------------------------------------------
//  PlEtherStart
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
PlErrCode PlEtherStart(struct network_info *net_info) {
  PlErrCode err_code = kPlErrCodeOk;
  int ret = 0;
  PlEtherInfo *info = NULL;

  // argument check
  if (net_info == NULL) {
    err_code = kPlErrInvalidParam;
    DLOGE("argument error.");
    ELOGE(kElog_PlErrInvalidParam);
    ELOGE(kElog_PlEtherStart);
    goto err_end1;
  }

  // get ethernet info
  info = (PlEtherInfo *)net_info->if_info;

  // interface error
  if (info == NULL) {
    err_code = kPlErrNotFound;
    DLOGE("%s is not found.", net_info->if_name);
    ELOGE(kElog_PlErrNotFound);
    ELOGE(kElog_PlEtherStart);
    goto err_end1;
  }

  // mutex lock
  ret = pthread_mutex_lock(&(info->mutex));
  if (ret != 0) {
    err_code = kPlErrLock;
    DLOGE("pthread_mutex_lock() failed.(ret=%d, errno=%d)", ret, errno);
    ELOGE(kElog_PlErrLock);
    ELOGE(kElog_PlEtherStart);
    goto err_end1;
  }

  // state check
  if (info->state != kPlEtherReady) {
    err_code = kPlErrInvalidState;
    DLOGE("state error.(state=%s)", ETH_SHOW_STATE(info->state));
    ELOGE(kElog_PlErrInvalidState);
    ELOGE(kElog_PlEtherStart);
    goto err_end2;
  }

  // start
  err_code = PlNetworkSetIfStatus(net_info->if_name, true);
  if (err_code != kPlErrCodeOk) {
    DLOGE("PlNetworkSetIfStatus() failed.(err_code=%d)", err_code);
    ELOGE(kElog_PlEtherStart);
    err_code = kPlErrInvalidOperation;
    goto err_end2;
  }

  // Ready -> Running
  info->state = kPlEtherRunning;
  DLOGI("%s network start.(state=%s)", net_info->if_name,
        ETH_SHOW_STATE(info->state));

err_end2:
  // mutex unlock
  ret = pthread_mutex_unlock(&(info->mutex));
  if (ret != 0) {
    DLOGE("pthread_mutex_unlock() failed.(ret=%d, errno=%d)", ret, errno);
    ELOGE(kElog_PlErrUnlock);
    ELOGE(kElog_PlEtherStart);
  }

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
PlErrCode PlEtherStop(struct network_info *net_info) {
  PlErrCode err_code = kPlErrCodeOk;
  int ret = 0;
  PlEtherInfo *info = NULL;

  // argument check
  if (net_info == NULL) {
    err_code = kPlErrInvalidParam;
    DLOGE("argument error.");
    ELOGE(kElog_PlErrInvalidParam);
    ELOGE(kElog_PlEtherStop);
    goto err_end1;
  }

  // get ethernet info
  info = (PlEtherInfo *)net_info->if_info;

  // interface error
  if (info == NULL) {
    err_code = kPlErrNotFound;
    DLOGE("%s is not found.", net_info->if_name);
    ELOGE(kElog_PlErrNotFound);
    ELOGE(kElog_PlEtherStop);
    goto err_end1;
  }

  // mutex lock
  ret = pthread_mutex_lock(&(info->mutex));
  if (ret != 0) {
    err_code = kPlErrLock;
    DLOGE("pthread_mutex_lock() failed.(ret=%d, errno=%d)", ret, errno);
    ELOGE(kElog_PlErrLock);
    ELOGE(kElog_PlEtherStop);
    goto err_end1;
  }

  // state check
  if (info->state != kPlEtherRunning) {
    err_code = kPlErrInvalidState;
    DLOGE("state error.(state=%s)", ETH_SHOW_STATE(info->state));
    ELOGE(kElog_PlErrInvalidState);
    ELOGE(kElog_PlEtherStop);
    goto err_end2;
  }

  // stop
  err_code = PlNetworkSetIfStatus(net_info->if_name, false);
  if (err_code != kPlErrCodeOk) {
    DLOGE("PlNetworkSetIfStatus() failed.(err_code=%d)", err_code);
    ELOGE(kElog_PlEtherStop);
    err_code = kPlErrInvalidOperation;
    goto err_end2;
  }

  // Running -> Ready
  info->state = kPlEtherReady;
  DLOGI("%s network stop.(state=%s)", net_info->if_name,
        ETH_SHOW_STATE(info->state));

err_end2:
  // mutex unlock
  ret = pthread_mutex_unlock(&(info->mutex));
  if (ret != 0) {
    DLOGE("pthread_mutex_unlock() failed.(ret=%d, errno=%d)", ret, errno);
    ELOGE(kElog_PlErrUnlock);
    ELOGE(kElog_PlEtherStop);
  }

err_end1:
  return err_code;
}

// -----------------------------------------------------------------------------
//  EtherMonitorThread
//
//  Monitor thread for ethernet connecting.
//  When the connection status changes, send ethernet event.
//
//  Args:
//    arg(void *): Ethernet information.
//
//  Returns:
//    void *: NULL
//
//  Note:
//
// -----------------------------------------------------------------------------
void *EtherMonitorThread(void *arg) {
  PlErrCode err_code = kPlErrCodeOk;
  int ret = 0;
  PlEtherInfo *info = (PlEtherInfo *)arg;
  char *if_name = (char *)NULL;
  char short_name[6] = {'\0'};
  uint8_t reason = 0;
  bool is_if_up = false;
  bool is_link_up = false;
  bool is_change_ifup = false;
  bool is_phy_id_valid = false;
  struct timespec abstime;
  char thread_name[CONFIG_TASK_NAME_SIZE + 1] = {'\0'};

  // argument check
  if (arg == NULL) {
    goto end_proc;
  }

  // set thread name
  if_name = info->if_name;
  memcpy(short_name, if_name, (sizeof(short_name) - 1));
  snprintf(thread_name, CONFIG_TASK_NAME_SIZE, "%s(%s)", __func__, short_name);
  pthread_setname_np(info->pid, thread_name);

  // initialize
  info->is_if_up = false;
  info->is_link_up = false;

  // main loop
  while (info->is_thread) {
    DLOGD("state=%s if_status=%s link_status=%s", ETH_SHOW_STATE(info->state),
          ETH_SHOW_IF(info->is_if_up), ETH_SHOW_LINK(info->is_link_up));

    is_change_ifup = false;

    // get interface status
    err_code = PlNetworkGetIfStatus(if_name, &is_if_up);
    if (err_code == kPlErrCodeOk) {
      // chnage status
      if (info->is_if_up != is_if_up) {
        if (is_if_up) {  // Down -> Up
          err_code = PlNetworkEventSend(if_name, kPlNetworkEventIfUp, reason);
          is_change_ifup = true;
        } else {  // Up -> Down
          err_code = PlNetworkEventSend(if_name, kPlNetworkEventIfDown, reason);
        }
      }
      // mutex lock
      ret = pthread_mutex_lock(&(info->mutex));
      if (ret == 0) {
        // store interface status
        info->is_if_up = is_if_up;

        // mutex unlock
        ret = pthread_mutex_unlock(&(info->mutex));
        if (ret != 0) {
          DLOGE("pthread_mutex_unlock() failed.(ret=%d, errno=%d)", ret, errno);
          ELOGE(kElog_PlErrUnlock);
          ELOGE(kElog_EtherMonitorThread);
        }
      } else {
        DLOGE("pthread_mutex_lock() failed.(ret=%d, errno=%d)", ret, errno);
        ELOGE(kElog_PlErrLock);
        ELOGE(kElog_EtherMonitorThread);
      }
    }

    // get link status
    err_code = PlNetworkGetLinkStatus(if_name, &is_link_up, &is_phy_id_valid);
    if (err_code == kPlErrCodeOk) {
      // check phy
      if (is_phy_id_valid != info->is_phy_id_valid) {
        if (is_phy_id_valid) {  // Invalid -> Valid
          err_code = PlNetworkEventSend(if_name, kPlNetworkEventPhyIdValid,
                                        reason);
        } else {  // Valid -> Invalid
          err_code = PlNetworkEventSend(if_name, kPlNetworkEventPhyIdInvalid,
                                        reason);
        }
      }
      // mutex lock
      ret = pthread_mutex_lock(&(info->mutex));
      if (ret == 0) {
        // store alive status
        info->is_phy_id_valid = is_phy_id_valid;

        // mutex unlock
        pthread_mutex_unlock(&(info->mutex));
      } else {
        DLOGE("pthread_mutex_lock() failed.(ret=%d, errno=%d)", ret, errno);
        ELOGE(kElog_PlErrLock);
        ELOGE(kElog_EtherMonitorThread);
      }

      // chnage status
      if (is_link_up != info->is_link_up) {
        if (is_link_up) {  // Down -> Up
          err_code = PlNetworkEventSend(if_name, kPlNetworkEventLinkUp, reason);
        } else {  // Up -> Down
          err_code = PlNetworkEventSend(if_name, kPlNetworkEventLinkDown,
                                        reason);
        }
      } else {
        // If Down -> Up && Link Up
        if ((is_change_ifup) && (is_link_up)) {
          err_code = PlNetworkEventSend(if_name, kPlNetworkEventLinkUp, reason);
        }
      }
      // mutex lock
      ret = pthread_mutex_lock(&(info->mutex));
      if (ret == 0) {
        // store link status
        info->is_link_up = is_link_up;

        // mutex unlock
        ret = pthread_mutex_unlock(&(info->mutex));
        if (ret != 0) {
          DLOGE("pthread_mutex_unlock() failed.(ret=%d, errno=%d)", ret, errno);
          ELOGE(kElog_PlErrUnlock);
          ELOGE(kElog_EtherMonitorThread);
        }
      } else {
        DLOGE("pthread_mutex_lock() failed.(ret=%d, errno=%d)", ret, errno);
        ELOGE(kElog_PlErrLock);
        ELOGE(kElog_EtherMonitorThread);
      }
    }

    PlNetworkUtilSchedLock();

    // get time
    clock_gettime(CLOCK_REALTIME, &abstime);

    // calc wait time
    abstime.tv_sec += ETHER_TIME_CONV_MS2SEC(ETHER_MONITOR_PERIODIC_MS);
    abstime.tv_nsec += ETHER_TIME_CONV_MS2NS(ETHER_MONITOR_PERIODIC_MS);
    // over 1sec
    if (abstime.tv_nsec >= ETHER_TIME_CONV_SEC2NS(1)) {
      abstime.tv_sec++;
      abstime.tv_nsec -= ETHER_TIME_CONV_SEC2NS(1);
    }

    // wait semaphore
    sem_timedwait(&(info->sem), &abstime);

    PlNetworkUtilSchedUnlock();
  }

end_proc:
  pthread_exit(NULL);
  return NULL;
}

// -----------------------------------------------------------------------------
//  EtherCreateMonitorThread
//
//  Create monitor thread.
//
//  Args:
//    ether_info(PlEtherInfo *): Ethernet information.
//    monitor_thread_pri(uint32_t): Thread priority.
//
//  Returns:
//    PlErrCode: kPlErrCodeOk on successful,
//               other than kPlErrCodeOk on failure.
//
//  Note:
//
// -----------------------------------------------------------------------------
static PlErrCode EtherCreateMonitorThread(PlEtherInfo *ether_info,
                                          uint32_t monitor_thread_pri) {
  int ret = 0;
  pthread_attr_t attr = {0};

  // semaphore initialize
  ret = sem_init(&ether_info->sem, 0, 0);
  if (ret != 0) {
    DLOGE("sem_init() failed. ret=%d, errno=%d", ret, errno);
    ELOGE(kElog_PlThreadError);
    ELOGE(kElog_EtherCreateMonitorThread);
    goto err_sem;
  }

  // attr initialize
  ret = pthread_attr_init(&attr);
  if (ret != 0) {
    DLOGE("pthread_attr_init() failed. ret=%d, errno=%d", ret, errno);
    ELOGE(kElog_PlThreadError);
    ELOGE(kElog_EtherCreateMonitorThread);
    goto err_init;
  }

#ifdef __NuttX__
  // set priority
  struct sched_param sparam = {0};
  sparam.sched_priority = monitor_thread_pri;
  ret = pthread_attr_setschedparam(&attr, &sparam);
  if (ret != 0) {
    DLOGE("pthread_attr_setschedparam() failed. ret=%d, errno=%d", ret, errno);
    ELOGE(kElog_PlThreadError);
    ELOGE(kElog_EtherCreateMonitorThread);
    goto err_priority;
  }
#endif

  // set stack
  ret = pthread_attr_setstacksize(&attr, ETHER_MONITOR_THREAD_STACKSIZE);
  if (ret != 0) {
    DLOGE("pthread_attr_setstacksize() failed. ret=%d, errno=%d", ret, errno);
    ELOGE(kElog_PlThreadError);
    ELOGE(kElog_EtherCreateMonitorThread);
    goto err_stack;
  }

  // create monitor thread
  ether_info->is_thread = true;
  ret = pthread_create(&ether_info->pid, &attr, EtherMonitorThread,
                       (void *)ether_info);
  if (ret != 0) {
    DLOGE("pthread_create() failed.");
    ELOGE(kElog_PlThreadError);
    ELOGE(kElog_EtherCreateMonitorThread);
    goto err_thread;
  }

  // attr destroy
  pthread_attr_destroy(&attr);

  return kPlErrCodeOk;

err_thread:
  ether_info->is_thread = false;

err_stack:
#ifdef __NuttX__
err_priority:
#endif
  // attr destroy
  pthread_attr_destroy(&attr);

err_init:
  // semaphore destroy
  sem_destroy(&ether_info->sem);

err_sem:
  return kPlThreadError;
}

// -----------------------------------------------------------------------------
//  EtherDestroyMonitorThread
//
//  Destroy monitor thread.
//
//  Args:
//    ether_info(PlEtherInfo *): Ethernet information.
//
//  Returns:
//    PlErrCode: kPlErrCodeOk on successful,
//               other than kPlErrCodeOk on failure.
//
//  Note:
//
// -----------------------------------------------------------------------------
static PlErrCode EtherDestroyMonitorThread(PlEtherInfo *ether_info) {
  PlErrCode err_code = kPlErrCodeOk;
  int ret = 0;

  // destroy monitor thread
  ether_info->is_thread = false;
  ret = pthread_join(ether_info->pid, NULL);
  if (ret != 0) {
    err_code = kPlThreadError;
    DLOGE("pthread_join() failed. ret=%d, errno=%d", ret, errno);
    ELOGE(kElog_PlThreadError);
    ELOGE(kElog_EtherDestroyMonitorThread);
  }

  // semaphore destroy
  sem_destroy(&ether_info->sem);

  return err_code;
}
