/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// Includes --------------------------------------------------------------------
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>

#include "pl.h"
#include "pl_network.h"
#include "pl_network_internal.h"
#include "pl_network_util.h"
#include "pl_ether.h"
#include "pl_ether_impl.h"
#include "utility_log.h"
#include "utility_log_module_id.h"

// Macros ----------------------------------------------------------------------
// Time unit convert
#define ETHER_TIME_CONV_MS2SEC(ms)      (ms / 1000)
#define ETHER_TIME_CONV_MS2NS(ms)       ((ms % 1000) * 1000000)
#define ETHER_TIME_CONV_SEC2NS(sec)     (sec * 1000000000)

#define EVENT_ID  (0x9600)
#define EVENT_UID_START (0x60)
#define EVENT_ID_START (EVENT_UID_START + 0x04)

#define NETWORK_LOG_ERR_WITH_ID(event_id, format, ...) \
  WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:" \
                   format, __FILE__, __LINE__, ##__VA_ARGS__); \
  WRITE_ELOG_ERROR(MODULE_ID_SYSTEM, (EVENT_ID | event_id));

#define NETWORK_LOG_ERR(event_id, format, ...) \
  WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:" \
                   format, __FILE__, __LINE__, ##__VA_ARGS__); \
  WRITE_ELOG_ERROR(MODULE_ID_SYSTEM, (EVENT_ID | (EVENT_ID_START + event_id)));

#define NETWORK_LOG_INF(format, ...) \
  WRITE_DLOG_INFO(MODULE_ID_SYSTEM, "%s-%d:" \
                   format, __FILE__, __LINE__, ##__VA_ARGS__)

#define NETWORK_LOG_DBG(format, ...) \
  WRITE_DLOG_DEBUG(MODULE_ID_SYSTEM, "%s-%d:" \
                   format, __FILE__, __LINE__, ##__VA_ARGS__)

#define PL_ETHER_ELOG_SOCKET_ERROR          (EVENT_UID_START + 0x00)
#define PL_ETHER_ELOG_SIOCGMIIPHY_ERROR     (EVENT_UID_START + 0x01)
#define PL_ETHER_ELOG_SIOCGMIIREG_ERROR     (EVENT_UID_START + 0x02)
#define PL_ETHER_ELOG_OS_ERROR              (EVENT_UID_START + 0x03)

#define ETH_SHOW_STATE(state)           ((state < kPlEtherStateMax) ? \
                                        kEtherState[state] : "Unknown")
#define ETH_SHOW_IF(status)             ((status == true) ? "Up" : "Down")
#define ETH_SHOW_LINK(status)           ((status == true) ? "Up" : "Down")

#ifndef __NuttX__
#define sched_lock()
#define sched_unlock()
#endif

// Typedefs --------------------------------------------------------------------
// Ethernet Statement
typedef enum {
  kPlEtherStandby = 0,
  kPlEtherReady,
  kPlEtherRunning,
  kPlEtherStateMax
} PlEtherState;

// Ethernet info
typedef struct {
  char              if_name[PL_NETWORK_IFNAME_LEN];
  pthread_mutex_t   mutex;
  PlEtherState      state;
  sem_t             sem;
  pthread_t         pid;
  bool              is_thread;
  bool              is_if_up;
  bool              is_link_up;
  bool              is_phy_id_valid;
} PlEtherInfo;

// Global Variables ------------------------------------------------------------
static const char *kEtherState[kPlEtherStateMax] = {
  "Standby",
  "Ready",
  "Running"
};

// Ethernet Operations
static const PlNetworkOps s_ether_osp = {
  .set_config  = PlEtherSetConfig,
  .get_config  = PlEtherGetConfig,
  .get_status  = PlEtherGetStatus,
  .reg_event   = PlEtherRegisterEventHandler,
  .unreg_event = PlEtherUnregisterEventHandler,
  .start       = PlEtherStart,
  .stop        = PlEtherStop,
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
  info = (PlEtherInfo *)malloc(sizeof(PlEtherInfo));
  if (info == NULL) {
    err_code = kPlErrMemory;
    NETWORK_LOG_ERR_WITH_ID(PL_ETHER_ELOG_OS_ERROR, "malloc() failed.");
    goto err_end1;
  }
  memset(info, 0, sizeof(PlEtherInfo));

  // set interface info
  memcpy(info->if_name, net_info->if_name, (PL_NETWORK_IFNAME_LEN - 1));
  info->if_name[(PL_NETWORK_IFNAME_LEN - 1)] = '\0';
  info->state = kPlEtherReady;

  // mutex initialize
  ret = pthread_mutex_init(&(info->mutex), NULL);
  if (ret != 0) {
    err_code = kPlErrLock;
    NETWORK_LOG_ERR_WITH_ID(PL_ETHER_ELOG_OS_ERROR,
                            "pthread_mutex_init() failed.(ret=%d, errno=%d)",
                            ret, errno);
    goto err_end2;
  }

  // mutex lock
  ret = pthread_mutex_lock(&(info->mutex));
  if (ret != 0) {
    err_code = kPlErrLock;
    NETWORK_LOG_ERR_WITH_ID(PL_ETHER_ELOG_OS_ERROR,
                            "pthread_mutex_lock() failed.(ret=%d, errno=%d)",
                            ret, errno);
    goto err_end3;
  }

  // device initialize
  err_code = PlEtherInitializeImpl(net_info->if_name);
  if (err_code != kPlErrCodeOk) {
    NETWORK_LOG_ERR(0x02, "PlEtherInitializeImpl() failed. err_code=%d",
                    err_code);
    err_code = kPlErrDevice;
    goto err_end4;
  }

  // create monitor thread
  err_code = EtherCreateMonitorThread(info, thread_priority);
  if (err_code != kPlErrCodeOk) {
    NETWORK_LOG_ERR(0x03, "monitor thread create failed. err_code=%d",
                    err_code);
    err_code = kPlThreadError;
    goto err_end5;
  }

  // set interface info
  net_info->ops = (PlNetworkOps *)&s_ether_osp;
  net_info->if_info = (void *)info;

  // mutex unlock
  ret = pthread_mutex_unlock(&(info->mutex));
  if (ret != 0) {
    NETWORK_LOG_ERR_WITH_ID(PL_ETHER_ELOG_OS_ERROR,
                    "pthread_mutex_unlock() failed.(ret=%d, errno=%d)",
                    ret, errno);
  }

  return kPlErrCodeOk;

err_end5:
  // device finalize
  pl_ercd = PlEtherFinalizeImpl(net_info->if_name);
  if (pl_ercd != kPlErrCodeOk) {
    NETWORK_LOG_ERR(0x12, "PlEtherFinalizeImpl() failed. err_code=%u", pl_ercd);
  }

err_end4:
  // mutex unlock
  ret = pthread_mutex_unlock(&(info->mutex));
  if (ret != 0) {
    NETWORK_LOG_ERR_WITH_ID(PL_ETHER_ELOG_OS_ERROR,
                    "pthread_mutex_unlock() failed.(ret=%d, errno=%d)",
                    ret, errno);
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
    NETWORK_LOG_ERR(0x04, "argument error.");
    goto err_end;
  }

  // get ethernet info
  info = (PlEtherInfo *)net_info->if_info;

  // interface error
  if (info == NULL) {
    err_code = kPlErrNotFound;
    NETWORK_LOG_ERR(0x05, "%s is not found.", net_info->if_name);
    goto err_end;
  }

  // mutex lock
  ret = pthread_mutex_lock(&(info->mutex));
  if (ret != 0) {
    err_code = kPlErrLock;
    NETWORK_LOG_ERR_WITH_ID(PL_ETHER_ELOG_OS_ERROR,
                            "pthread_mutex_lock() failed.(ret=%d, errno=%d)",
                            ret, errno);
  }

  // device finalize
  pl_ercd = PlEtherFinalizeImpl(net_info->if_name);
  if (pl_ercd != kPlErrCodeOk) {
    NETWORK_LOG_ERR(0x06, "PlEtherFinalizeImpl() failed. err_code=%d", pl_ercd);
    err_code = kPlErrDevice;
  }

  // mutex unlock
  ret = pthread_mutex_unlock(&(info->mutex));
  if (ret != 0) {
    NETWORK_LOG_ERR_WITH_ID(PL_ETHER_ELOG_OS_ERROR,
                    "pthread_mutex_unlock() failed.(ret=%d, errno=%d)",
                    ret, errno);
  }

  // destroy monitor thread
  pl_ercd = EtherDestroyMonitorThread(info);
  if (pl_ercd != kPlErrCodeOk) {
    NETWORK_LOG_ERR(0x07, " monitor thread destroy failed. pl_ercd=%d",
                    pl_ercd);
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
  (void)net_info;   // Avoid compiler warning
  (void)config;     // Avoid compiler warning
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
  (void)net_info;   // Avoid compiler warning
  (void)config;     // Avoid compiler warning
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
    NETWORK_LOG_ERR(0x08, "argument error.");
    goto err_end1;
  }

  // get ethernet info
  info = (PlEtherInfo *)net_info->if_info;

  // interface error
  if (info == NULL) {
    err_code = kPlErrNotFound;
    NETWORK_LOG_ERR(0x09, "%s is not found.", net_info->if_name);
    goto err_end1;
  }

  // mutex lock
  ret = pthread_mutex_lock(&(info->mutex));
  if (ret != 0) {
    err_code = kPlErrLock;
    NETWORK_LOG_ERR_WITH_ID(PL_ETHER_ELOG_OS_ERROR,
                            "pthread_mutex_lock() failed.(ret=%d, errno=%d)",
                            ret, errno);
    goto err_end1;
  }

  // get interface status
  status->is_if_up = info->is_if_up;

  // get link status
  status->is_link_up = info->is_link_up;

  // get phy id valid
  status->is_phy_id_valid = info->is_phy_id_valid;

  NETWORK_LOG_DBG("if_name=%s is_if_up=%s is_link_up=%s is_phy_id_valid=%s",
                  net_info->if_name,
                  ((status->is_if_up == true) ? "true" : "false"),
                  ((status->is_link_up == true) ? "true" : "false"),
                  ((status->is_phy_id_valid == true) ? "true" : "false"));

  // mutex unlock
  ret = pthread_mutex_unlock(&(info->mutex));
  if (ret != 0) {
    NETWORK_LOG_ERR_WITH_ID(PL_ETHER_ELOG_OS_ERROR,
                    "pthread_mutex_unlock() failed.(ret=%d, errno=%d)",
                    ret, errno);
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
#ifdef __NuttX__
PlErrCode PlEtherRegisterEventHandler(struct network_info *net_info) {
  (void)net_info;
  return kPlErrCodeOk;
}
#endif  // #ifdef __NuttX__

#ifndef __NuttX__
PlErrCode PlEtherRegisterEventHandler(struct network_info *net_info) {
  // Notifies the upper layer of the current state as an event.
  PlEtherInfo *info = (PlEtherInfo *)net_info->if_info;
  PlErrCode err = kPlErrCodeError;
  uint8_t reason = 0;
  PlNetworkEvent event = kPlNetworkEventMax;

  event = kPlNetworkEventIfDown;
  if (info->is_if_up) {
    event = kPlNetworkEventIfUp;
  }
  err = PlNetworkEventSend(info->if_name, event, reason);
  NETWORK_LOG_INF("%s:%d:%d", __func__, err, event);

  event = kPlNetworkEventLinkDown;
  if (info->is_link_up) {
    event = kPlNetworkEventLinkUp;
  }
  err = PlNetworkEventSend(info->if_name, event, reason);
  NETWORK_LOG_INF("%s:%d:%d", __func__, err, event);
  return kPlErrCodeOk;
}
#endif  // #ifndef __NuttX__

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
  (void)net_info;   // Avoid compiler warning
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
    NETWORK_LOG_ERR(0x0A, "argument error.");
    goto err_end1;
  }

  // get ethernet info
  info = (PlEtherInfo *)net_info->if_info;

  // interface error
  if (info == NULL) {
    err_code = kPlErrNotFound;
    NETWORK_LOG_ERR(0x0B, "%s is not found.", net_info->if_name);
    goto err_end1;
  }

  // mutex lock
  ret = pthread_mutex_lock(&(info->mutex));
  if (ret != 0) {
    err_code = kPlErrLock;
    NETWORK_LOG_ERR_WITH_ID(PL_ETHER_ELOG_OS_ERROR,
                            "pthread_mutex_lock() failed.(ret=%d, errno=%d)",
                            ret, errno);
    goto err_end1;
  }

  // state check
  if (info->state != kPlEtherReady) {
    err_code = kPlErrInvalidState;
    NETWORK_LOG_ERR(0x0C, "state error.(state=%s)",
                    ETH_SHOW_STATE(info->state));
    goto err_end2;
  }

  // start
  err_code = PlNetworkSetIfStatus(net_info->if_name, true);
  if (err_code != kPlErrCodeOk) {
    NETWORK_LOG_ERR(0x0D, "PlNetworkSetIfStatus() failed.(err_code=%d)",
                    err_code);
    err_code = kPlErrInvalidOperation;
    goto err_end2;
  }

  // Ready -> Running
  info->state = kPlEtherRunning;
  NETWORK_LOG_INF("%s network start.(state=%s)",
                  net_info->if_name, ETH_SHOW_STATE(info->state));

err_end2:
  // mutex unlock
  ret = pthread_mutex_unlock(&(info->mutex));
  if (ret != 0) {
    NETWORK_LOG_ERR_WITH_ID(PL_ETHER_ELOG_OS_ERROR,
                    "pthread_mutex_unlock() failed.(ret=%d, errno=%d)",
                    ret, errno);
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
    NETWORK_LOG_ERR(0x0E, "argument error.");
    goto err_end1;
  }

  // get ethernet info
  info = (PlEtherInfo *)net_info->if_info;

  // interface error
  if (info == NULL) {
    err_code = kPlErrNotFound;
    NETWORK_LOG_ERR(0x0F, "%s is not found.", net_info->if_name);
    goto err_end1;
  }

  // mutex lock
  ret = pthread_mutex_lock(&(info->mutex));
  if (ret != 0) {
    err_code = kPlErrLock;
    NETWORK_LOG_ERR_WITH_ID(PL_ETHER_ELOG_OS_ERROR,
                            "pthread_mutex_lock() failed.(ret=%d, errno=%d)",
                            ret, errno);
    goto err_end1;
  }

  // state check
  if (info->state != kPlEtherRunning) {
    err_code = kPlErrInvalidState;
    NETWORK_LOG_ERR(0x10, "state error.(state=%s)",
                    ETH_SHOW_STATE(info->state));
    goto err_end2;
  }

  // stop
  err_code = PlNetworkSetIfStatus(net_info->if_name, false);
  if (err_code != kPlErrCodeOk) {
    NETWORK_LOG_ERR(0x11, "PlNetworkSetIfStatus() failed.(err_code=%d)",
                    err_code);
    err_code = kPlErrInvalidOperation;
    goto err_end2;
  }

  // Running -> Ready
  info->state = kPlEtherReady;
  NETWORK_LOG_INF("%s network stop.(state=%s)",
                  net_info->if_name, ETH_SHOW_STATE(info->state));

err_end2:
  // mutex unlock
  ret = pthread_mutex_unlock(&(info->mutex));
  if (ret != 0) {
    NETWORK_LOG_ERR_WITH_ID(PL_ETHER_ELOG_OS_ERROR,
                    "pthread_mutex_unlock() failed.(ret=%d, errno=%d)",
                    ret, errno);
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
    NETWORK_LOG_DBG("state=%s if_status=%s link_status=%s",
                    ETH_SHOW_STATE(info->state),
                    ETH_SHOW_IF(info->is_if_up),
                    ETH_SHOW_LINK(info->is_link_up));

    is_change_ifup = false;

    // get interface status
    err_code = PlNetworkGetIfStatus(if_name, &is_if_up);
    if (err_code == kPlErrCodeOk) {
      // chnage status
      if (info->is_if_up != is_if_up) {
        if (is_if_up) {       // Down -> Up
          err_code = PlNetworkEventSend(if_name, kPlNetworkEventIfUp, reason);
         is_change_ifup = true;
        } else {              // Up -> Down
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
          NETWORK_LOG_ERR_WITH_ID(PL_ETHER_ELOG_OS_ERROR,
                    "pthread_mutex_unlock() failed.(ret=%d, errno=%d)",
                    ret, errno);
        }
      } else {
        NETWORK_LOG_ERR_WITH_ID(PL_ETHER_ELOG_OS_ERROR,
                              "pthread_mutex_lock() failed.(ret=%d, errno=%d)",
                              ret, errno);
      }
    }

    // get link status
    err_code = PlNetworkGetLinkStatus(if_name, &is_link_up, &is_phy_id_valid);
    if (err_code == kPlErrCodeOk) {
      // check phy
      if (is_phy_id_valid != info->is_phy_id_valid) {
        if (is_phy_id_valid) {   // Invalid -> Valid
          err_code = PlNetworkEventSend(if_name, kPlNetworkEventPhyIdValid,
                                        reason);
        } else {                // Valid -> Invalid
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
        NETWORK_LOG_ERR_WITH_ID(PL_ETHER_ELOG_OS_ERROR,
                            "pthread_mutex_lock() failed.(ret=%d, errno=%d)",
                            ret, errno);
      }

      // chnage status
      if (is_link_up != info->is_link_up) {
        if (is_link_up) {     // Down -> Up
          err_code = PlNetworkEventSend(if_name, kPlNetworkEventLinkUp, reason);
        } else {              // Up -> Down
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
          NETWORK_LOG_ERR_WITH_ID(PL_ETHER_ELOG_OS_ERROR,
                            "pthread_mutex_unlock() failed.(ret=%d, errno=%d)",
                            ret, errno);
        }
      } else {
        NETWORK_LOG_ERR_WITH_ID(PL_ETHER_ELOG_OS_ERROR,
                            "pthread_mutex_lock() failed.(ret=%d, errno=%d)",
                            ret, errno);
      }
    }

    // schedule lock
    sched_lock();

    // get time
    clock_gettime(CLOCK_REALTIME, &abstime);

    // calc wait time
    abstime.tv_sec  += ETHER_TIME_CONV_MS2SEC(ETHER_MONITOR_PERIODIC_MS);
    abstime.tv_nsec += ETHER_TIME_CONV_MS2NS(ETHER_MONITOR_PERIODIC_MS);
    // over 1sec
    if (abstime.tv_nsec >= ETHER_TIME_CONV_SEC2NS(1)) {
      abstime.tv_sec++;
      abstime.tv_nsec -= ETHER_TIME_CONV_SEC2NS(1);
    }

    // wait semaphore
    sem_timedwait(&(info->sem), &abstime);

    // schedule unlock
    sched_unlock();
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
    NETWORK_LOG_ERR_WITH_ID(PL_ETHER_ELOG_OS_ERROR,
                            "sem_init() failed. ret=%d, errno=%d", ret, errno);
    goto err_sem;
  }

  // attr initialize
  ret = pthread_attr_init(&attr);
  if (ret != 0) {
    NETWORK_LOG_ERR_WITH_ID(PL_ETHER_ELOG_OS_ERROR,
                  "pthread_attr_init() failed. ret=%d, errno=%d", ret, errno);
    goto err_init;
  }

#ifdef __NuttX__
  // set priority
  struct sched_param sparam = {0};
  sparam.sched_priority = monitor_thread_pri;
  ret = pthread_attr_setschedparam(&attr, &sparam);
  if (ret != 0) {
    NETWORK_LOG_ERR_WITH_ID(PL_ETHER_ELOG_OS_ERROR,
                      "pthread_attr_setschedparam() failed. ret=%d, errno=%d",
                      ret, errno);
    goto err_priority;
  }
#endif

  // set stack
  ret = pthread_attr_setstacksize(&attr, ETHER_MONITOR_THREAD_STACKSIZE);
  if (ret != 0) {
    NETWORK_LOG_ERR_WITH_ID(PL_ETHER_ELOG_OS_ERROR,
                        "pthread_attr_setstacksize() failed. ret=%d, errno=%d",
                        ret, errno);
    goto err_stack;
  }

  // create monitor thread
  ether_info->is_thread = true;
  ret = pthread_create(&ether_info->pid, &attr,
                       EtherMonitorThread, (void*)ether_info);
  if (ret != 0) {
    NETWORK_LOG_ERR_WITH_ID(PL_ETHER_ELOG_OS_ERROR, "pthread_create() failed.");
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
    NETWORK_LOG_ERR_WITH_ID(PL_ETHER_ELOG_OS_ERROR,
                      "pthread_join() failed. ret=%d, errno=%d", ret, errno);
  }

  // semaphore destroy
  sem_destroy(&ether_info->sem);

  return err_code;
}
