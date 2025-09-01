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
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>

#include "pl.h"
#include "pl_network.h"
#include "pl_network_impl.h"
#include "pl_network_internal.h"
#include "pl_network_util.h"
#ifdef  CONFIG_PL_NETWORK_HAVE_ETHER
#include "pl_ether.h"
#endif
#ifdef  CONFIG_PL_NETWORK_HAVE_WIFI
#include "pl_wifi.h"
#endif
#include "utility_log.h"
#include "utility_log_module_id.h"

// Macros ---------------------------------------------------------------------
#define EVENT_ID  (0x9600)
#define EVENT_UID_START (0x00)
#define EVENT_ID_START (EVENT_UID_START + 0x0D)

#define NETWORK_LOG_ERR(event_id, format, ...) \
  WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:" format, \
                   __FILE__, __LINE__, ##__VA_ARGS__); \
  WRITE_ELOG_ERROR(MODULE_ID_SYSTEM, (EVENT_ID | (EVENT_ID_START + event_id)));

#define NETWORK_LOG_ERR_WITH_ID(event_id, format, ...) \
  WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:" \
                   format, __FILE__, __LINE__, ##__VA_ARGS__); \
  WRITE_ELOG_ERROR(MODULE_ID_SYSTEM, (EVENT_ID | event_id));

#define NETWORK_LOG_WRN(event_id, format, ...) \
  WRITE_DLOG_WARN(MODULE_ID_SYSTEM, "%s-%d:" \
                   format, __FILE__, __LINE__, ##__VA_ARGS__); \
  WRITE_ELOG_WARN(MODULE_ID_SYSTEM, (EVENT_ID | (EVENT_ID_START + event_id)));

#define NETWORK_LOG_DBG(format, ...) \
  WRITE_DLOG_DEBUG(MODULE_ID_SYSTEM, "%s-%d:" format, \
                   __FILE__, __LINE__, ##__VA_ARGS__)

#define PL_NETWORK_ELOG_SET_CONFIG_ERROR    (EVENT_UID_START + 0x00)
#define PL_NETWORK_ELOG_GET_CONFIG_ERROR    (EVENT_UID_START + 0x01)
#define PL_NETWORK_ELOG_GET_STATUS_ERROR    (EVENT_UID_START + 0x02)
#define PL_NETWORK_ELOG_REG_EVENT_ERROR     (EVENT_UID_START + 0x03)
#define PL_NETWORK_ELOG_UNREG_EVENT_ERROR   (EVENT_UID_START + 0x04)
#define PL_NETWORK_ELOG_START_ERROR         (EVENT_UID_START + 0x05)
#define PL_NETWORK_ELOG_STOP_ERROR          (EVENT_UID_START + 0x06)
#define PL_NETWORK_ELOG_ETHER_INIT_ERROR    (EVENT_UID_START + 0x07)
#define PL_NETWORK_ELOG_WIFI_INIT_ERROR     (EVENT_UID_START + 0x08)
#define PL_NETWORK_ELOG_SOCKET_ERROR        (EVENT_UID_START + 0x09)
#define PL_NETWORK_ELOG_SIOCSIFFLAGS_ERROR  (EVENT_UID_START + 0x0A)
#define PL_NETWORK_ELOG_SIOCGIFFLAGS_ERROR  (EVENT_UID_START + 0x0B)
#define PL_NETWORK_ELOG_OS_ERROR            (EVENT_UID_START + 0x0C)

// Message
#define NETWORK_MSG_MAX_SIZE              (6)

// Net Stat
#define NETWORK_NETSTAT_PATH              "/proc/net/stat"

// Show State String
#define NETIF_STATE_SHOW(state)           kNetIfState[state]

// Typedefs --------------------------------------------------------------------
// PL Network State
typedef enum {
  kPlNetworkIniStatReady = 0,
  kPlNetworkIniStatRunning,
  kPlNetworkIniStatMax
} PlNetworkIniStat;

// PL Netwrok Command
typedef enum {
  kPlNetworkCmdNone = 0,
  kPlNetworkCmdEvent,
  kPlNetworkCmdQuit,
  kPlNetworkCmdMax
} PlNetworkCmd;

// PL Network Message
TAILQ_HEAD(PlNetworkMsgList, PlNetworkMsg);

struct PlNetworkMsg {
  TAILQ_ENTRY(PlNetworkMsg) head;
  PlNetworkCmd cmd;
  char if_name[PL_NETWORK_IFNAME_LEN];
  PlNetworkEvent event;
  uint8_t reason;
};

typedef struct {
  struct PlNetworkMsgList list;
  struct PlNetworkMsg *msgs;
  uint32_t max_msg_size;
} PlNetworkMsgInfo;

SLIST_HEAD(PlNetworkList, PlNetworkData);

struct PlNetworkData {
  PlNetworkInfo  net_info;
  SLIST_ENTRY(PlNetworkData) next;
};

// Global Variables ------------------------------------------------------------
// Network Statement
static PlNetworkIniStat s_pl_network_init_state = kPlNetworkIniStatReady;

// Network Interface Statement
static const char *kNetIfState[kPlNetIfStateMax] = {
  "Stopped",
  "Started"
};

// Network System Informations
static const PlNetworkSystemInfo s_network_system_infos[] = {
#ifdef CONFIG_PL_ETHER_HAVE_ETH0
  { // eth0
    NETWORK_ETH0_IFNAME,
    kPlNetworkTypeEther,
    NETWORK_ETH0_CLOUD,
    NETWORK_ETH0_LOCAL,
  },
#endif  // CONFIG_PL_ETHER_HAVE_ETH0
#ifdef CONFIG_PL_WIFI_HAVE_WLAN0
  { // wlan0
    NETWORK_WLAN0_IFNAME,
    kPlNetworkTypeWifi,
    NETWORK_WLAN0_CLOUD,
    NETWORK_WLAN0_LOCAL,
  },
#endif  // CONFIG_PL_WIFI_HAVE_WLAN0
#ifdef CONFIG_PL_WIFI_HAVE_WLAN1
  { // wlan1
    NETWORK_WLAN1_IFNAME,
    kPlNetworkTypeWifi,
    NETWORK_WLAN1_CLOUD,
    NETWORK_WLAN1_LOCAL,
  },
#endif  // CONFIG_PL_WIFI_HAVE_WLAN1
  { // Terminate
    {'\0'},
    kPlNetworkTypeUnkown,
    false,
    false
  }
};
static const uint32_t kNetworkSystemInfoTotalNum =
  (sizeof(s_network_system_infos) / sizeof(PlNetworkSystemInfo) - 1);

// Network Configurations
static struct PlNetworkList s_network_info;

// Network Mutex
static pthread_mutex_t s_pl_network_mutex = PTHREAD_MUTEX_INITIALIZER;

// Netwrok Message List Info
static PlNetworkMsgInfo s_network_msg_info;

// Network Message Semaphore
static sem_t s_network_msg_sem;

// Network Event Thread
static pthread_t s_pl_network_thread_pid = 0;

// Local functions -------------------------------------------------------------
// Network Event Thread
void *PlNetworkEventThread(void *arg);

static PlErrCode NetworkDeviceInitialize(uint32_t thread_priority);
static PlErrCode NetworkCreateEventThread(uint32_t thread_priority);
static PlErrCode NetworkDeviceFinalize(void);
static PlErrCode NetworkGetData(const char *if_name,
                                struct PlNetworkData **net_data);
static PlErrCode NetworkSendMsg(PlNetworkCmd cmd, const char *if_name,
                                PlNetworkEvent event, uint8_t reason);

// Functions -------------------------------------------------------------------
// -----------------------------------------------------------------------------
//  PlNetworkGetSystemInfo
//
//  Get network system informations.
//
//  Args:
//    info_total_num(uint32_t *): Total number of network system informations.
//    infos(PlNetworkSystemInfo **): Network system informations.
//
//  Returns:
//    PlErrCode: kPlErrCodeOk on successful,
//                other than kPlErrCodeOk on failure.
//
//  Note:
//
// -----------------------------------------------------------------------------
PlErrCode PlNetworkGetSystemInfo(uint32_t *info_total_num,
                                 PlNetworkSystemInfo **infos) {
  PlErrCode err_code = kPlErrCodeOk;
  int ret = pthread_mutex_lock(&s_pl_network_mutex);
  if (ret != 0) {
    err_code =  kPlErrLock;
    NETWORK_LOG_ERR_WITH_ID(PL_NETWORK_ELOG_OS_ERROR,
                            "pthread_mutex_lock() failed.(ret=%d, errno=%d)",
                            ret, errno);
    goto err_end1;
  }

  // argument check
  if ((info_total_num == NULL) || (infos == NULL)) {
    err_code = kPlErrInvalidParam;
    NETWORK_LOG_ERR(0x00, "argument error.");
    goto err_end2;
  }

  // initialize check
  if (s_pl_network_init_state != kPlNetworkIniStatRunning) {
    err_code =  kPlErrInvalidState;
    NETWORK_LOG_ERR(0x01, "initialized error. state=%u",
                    s_pl_network_init_state);
    goto err_end2;
  }

  // Get system information
  *info_total_num = kNetworkSystemInfoTotalNum;
  *infos = (PlNetworkSystemInfo *)&s_network_system_infos[0];

err_end2:
  ret = pthread_mutex_unlock(&s_pl_network_mutex);
  if (ret != 0) {
    NETWORK_LOG_ERR_WITH_ID(PL_NETWORK_ELOG_OS_ERROR,
                    "pthread_mutex_unlock() failed.(ret=%d, errno=%d)",
                    ret, errno);
  }

err_end1:
  return err_code;
}

// -----------------------------------------------------------------------------
//  PlNetworkSetConfig
//
//  Set network interface configuration.
//
//  Args:
//    if_name(const char *): Interface name.
//    config(const PlNetworkConfig *): Network configuration.
//
//  Returns:
//    PlErrCode: kPlErrCodeOk on successful,
//                other than kPlErrCodeOk on failure.
//
//  Note:
//
// -----------------------------------------------------------------------------
PlErrCode PlNetworkSetConfig(const char *if_name,
                             const PlNetworkConfig *config) {
  PlErrCode err_code = kPlErrCodeOk;
  int ret = pthread_mutex_lock(&s_pl_network_mutex);
  if (ret != 0) {
    err_code =  kPlErrLock;
    NETWORK_LOG_ERR_WITH_ID(PL_NETWORK_ELOG_OS_ERROR,
                            "pthread_mutex_lock() failed.(ret=%d, errno=%d)",
                            ret, errno);
    goto err_end1;
  }

  // argument check
  if ((if_name == NULL) || (config == NULL)) {
    err_code = kPlErrInvalidParam;
    NETWORK_LOG_ERR(0x02, "argument error.");
    goto err_end2;
  }

  // initialize check
  if (s_pl_network_init_state != kPlNetworkIniStatRunning) {
    err_code = kPlErrInvalidState;
    NETWORK_LOG_ERR(0x03, "initialized error. state=%u",
                    s_pl_network_init_state);
    goto err_end2;
  }

  // interface check
  struct PlNetworkData *net_data = NULL;
  err_code = NetworkGetData(if_name, &net_data);
  if (err_code != kPlErrCodeOk) {
    NETWORK_LOG_ERR(0x04, "interface error.(if_name=%s, err_code=%u)",
                      if_name, err_code);
    err_code = kPlErrNotFound;
    goto err_end2;
  }
  PlNetworkInfo *net_info = &(net_data->net_info);

  // state check
  if (net_info->state != kPlNetIfStateStopped) {
    err_code = kPlErrInvalidState;
    NETWORK_LOG_ERR(0x05, "state error.(state=%s)",
                     NETIF_STATE_SHOW(net_info->state));
    goto err_end2;
  }

  // ops check
  if (net_info->ops == NULL) {
    err_code = kPlErrNotFound;
    NETWORK_LOG_ERR(0x06, "ops not found.");
    goto err_end2;
  }

  // api check
  if (net_info->ops->set_config == NULL) {
    err_code = kPlErrNotFound;
    NETWORK_LOG_ERR(0x07, "api not found.");
    goto err_end2;
  }

  // set config
  err_code = net_info->ops->set_config(net_info, config);
  // no supported
  if (err_code == kPlErrNoSupported) {
    NETWORK_LOG_WRN(0x3F, "set config api no supported.");
  // failed
  } else if (err_code != kPlErrCodeOk) {
    NETWORK_LOG_ERR_WITH_ID(PL_NETWORK_ELOG_SET_CONFIG_ERROR,
                            "set config failed. err_code=%u", err_code);
    err_code = kPlErrInvalidOperation;
  }

err_end2:
  // mutex unlock
  ret = pthread_mutex_unlock(&s_pl_network_mutex);
  if (ret != 0) {
    NETWORK_LOG_ERR_WITH_ID(PL_NETWORK_ELOG_OS_ERROR,
                    "pthread_mutex_unlock() failed.(ret=%d, errno=%d)",
                    ret, errno);
  }

err_end1:
  return err_code;
}

// -----------------------------------------------------------------------------
//  PlNetworkGetConfig
//
//  Get network interface configuration.
//
//  Args:
//    if_name(const char *): Interface name.
//    config(PlNetworkConfig *): Network configuration.
//
//  Returns:
//    PlErrCode: kPlErrCodeOk on successful,
//                other than kPlErrCodeOk on failure.
//
//  Note:
//
// -----------------------------------------------------------------------------
PlErrCode PlNetworkGetConfig(const char *if_name,
                             PlNetworkConfig *config) {
  PlErrCode err_code = kPlErrCodeOk;
  int ret = pthread_mutex_lock(&s_pl_network_mutex);
  if (ret != 0) {
    err_code =  kPlErrLock;
    NETWORK_LOG_ERR_WITH_ID(PL_NETWORK_ELOG_OS_ERROR,
                            "pthread_mutex_lock() failed.(ret=%d, errno=%d)",
                            ret, errno);
    goto err_end1;
  }

  // argument check
  if ((if_name == NULL) || (config == NULL)) {
    err_code = kPlErrInvalidParam;
    NETWORK_LOG_ERR(0x08, "argument error.");
    goto err_end2;
  }

  // initialize check
  if (s_pl_network_init_state != kPlNetworkIniStatRunning) {
    err_code = kPlErrInvalidState;
    NETWORK_LOG_ERR(0x09, "initialized error. state=%u",
                    s_pl_network_init_state);
    goto err_end2;
  }

  // interface check
  struct PlNetworkData *net_data = NULL;
  err_code = NetworkGetData(if_name, &net_data);
  if (err_code != kPlErrCodeOk) {
    NETWORK_LOG_ERR(0x0A, "interface error.(if_name=%s, err_code=%u)",
                      if_name, err_code);
    err_code = kPlErrNotFound;
    goto err_end2;
  }
  PlNetworkInfo *net_info = &(net_data->net_info);

  // ops check
  if (net_info->ops == NULL) {
    err_code = kPlErrNotFound;
    NETWORK_LOG_ERR(0x0B, "ops not found.");
    goto err_end2;
  }

  // api check
  if (net_info->ops->get_config == NULL) {
    err_code = kPlErrNotFound;
    NETWORK_LOG_ERR(0x0C, "api not found.");
    goto err_end2;
  }

  // get config
  err_code = net_info->ops->get_config(net_info, config);
  // no supported
  if (err_code == kPlErrNoSupported) {
    NETWORK_LOG_WRN(0x40, "get config api no supported.");
  // failed
  } else if (err_code != kPlErrCodeOk) {
    NETWORK_LOG_ERR_WITH_ID(PL_NETWORK_ELOG_GET_CONFIG_ERROR,
                            "get config failed. err_code=%u", err_code);
    err_code = kPlErrInvalidOperation;
  }

err_end2:
  ret = pthread_mutex_unlock(&s_pl_network_mutex);
  if (ret != 0) {
    NETWORK_LOG_ERR_WITH_ID(PL_NETWORK_ELOG_OS_ERROR,
                    "pthread_mutex_unlock() failed.(ret=%d, errno=%d)",
                    ret, errno);
  }

err_end1:
  return err_code;
}

// -----------------------------------------------------------------------------
//  PlNetworkGetStatus
//
//  Get network interface status.
//
//  Args:
//    if_name(const char *): Interface name.
//    status(PlNetworkStatus *): Network status.
//
//  Returns:
//    PlErrCode: kPlErrCodeOk on successful,
//                other than kPlErrCodeOk on failure.
//
//  Note:
//
// -----------------------------------------------------------------------------
PlErrCode PlNetworkGetStatus(const char *if_name,
                             PlNetworkStatus *status) {
  PlErrCode err_code = kPlErrCodeOk;
  int ret = pthread_mutex_lock(&s_pl_network_mutex);
  if (ret != 0) {
    err_code =  kPlErrLock;
    NETWORK_LOG_ERR_WITH_ID(PL_NETWORK_ELOG_OS_ERROR,
                            "pthread_mutex_lock() failed.(ret=%d, errno=%d)",
                            ret, errno);
    goto err_end1;
  }

  // argument check
  if ((if_name == NULL) || (status == NULL)) {
    err_code = kPlErrInvalidParam;
    NETWORK_LOG_ERR(0x0D, "argument error.");
    goto err_end2;
  }

  // initialize check
  if (s_pl_network_init_state != kPlNetworkIniStatRunning) {
    err_code = kPlErrInvalidState;
    NETWORK_LOG_ERR(0x0E, "initialized error. state=%u",
                    s_pl_network_init_state);
    goto err_end2;
  }

  // interface check
  struct PlNetworkData *net_data = NULL;
  err_code = NetworkGetData(if_name, &net_data);
  if (err_code != kPlErrCodeOk) {
    NETWORK_LOG_ERR(0x0F, "interface error.(if_name=%s, err_code=%u)",
                      if_name, err_code);
    err_code = kPlErrNotFound;
    goto err_end2;
  }
  PlNetworkInfo *net_info = &(net_data->net_info);

  // ops check
  if (net_info->ops == NULL) {
    err_code = kPlErrNotFound;
    NETWORK_LOG_ERR(0x10, "ops not found.");
    goto err_end2;
  }

  // api check
  if (net_info->ops->get_status == NULL) {
    err_code = kPlErrNotFound;
    NETWORK_LOG_ERR(0x11, "api not found.");
    goto err_end2;
  }

  // get status
  err_code = net_info->ops->get_status(net_info, status);
  if (err_code != kPlErrCodeOk) {
    NETWORK_LOG_ERR_WITH_ID(PL_NETWORK_ELOG_GET_STATUS_ERROR,
                            "get status failed. err_code=%u", err_code);
    err_code = kPlErrInvalidOperation;
    goto err_end2;
  }

err_end2:
  ret = pthread_mutex_unlock(&s_pl_network_mutex);
  if (ret != 0) {
    NETWORK_LOG_ERR_WITH_ID(PL_NETWORK_ELOG_OS_ERROR,
                    "pthread_mutex_unlock() failed.(ret=%d, errno=%d)",
                    ret, errno);
  }

err_end1:
  return err_code;
}

// -----------------------------------------------------------------------------
//  PlNetworkGetNetStat
//
//  Get the network status of the entire system as a string.
//
//  Args:
//    buf(char *): Buffer for storing strings.
//    buf_size(const uint32_t): Buffer size.
//
//  Returns:
//    PlErrCode: kPlErrCodeOk on successful,
//                other than kPlErrCodeOk on failure.
//
//  Note:
//
// -----------------------------------------------------------------------------
#ifdef __NuttX__
PlErrCode PlNetworkGetNetStat(char *buf, const uint32_t buf_size) {
  PlErrCode err_code = kPlErrCodeOk;
  // mutex lock
  int ret = pthread_mutex_lock(&s_pl_network_mutex);
  if (ret != 0) {
    err_code = kPlErrLock;
    NETWORK_LOG_ERR_WITH_ID(PL_NETWORK_ELOG_OS_ERROR,
                            "pthread_mutex_lock() failed.(ret=%d, errno=%d)",
                            ret, errno);
    goto err_end1;
  }

  // argument check
  if ((buf == NULL) || (buf_size < 1)) {
    err_code = kPlErrInvalidParam;
    NETWORK_LOG_ERR(0x12, "argument error.");
    goto err_end2;
  }

  // initialize check
  if (s_pl_network_init_state != kPlNetworkIniStatRunning) {
    err_code = kPlErrInvalidState;
    NETWORK_LOG_ERR(0x13, "initialized error. state=%u",
                    s_pl_network_init_state);
    goto err_end2;
  }

  // file open
  FILE *fp = fopen(NETWORK_NETSTAT_PATH, "r");
  if (fp == NULL) {
    err_code = kPlErrOpen;
    NETWORK_LOG_ERR_WITH_ID(PL_NETWORK_ELOG_OS_ERROR,
                            "fopen() error. errno=%d", errno);
    goto err_end2;
  }

  // file read
  size_t num = fread(buf, sizeof(char), buf_size, fp);
  if (num < 1) {
    err_code = kPlErrInvalidOperation;
    NETWORK_LOG_ERR_WITH_ID(PL_NETWORK_ELOG_OS_ERROR,
                            "fread() error.(%d) errno=%d", num, errno);
  }

  // file close
  ret = fclose(fp);
  if (ret != 0) {
    err_code = kPlErrClose;
    NETWORK_LOG_ERR_WITH_ID(PL_NETWORK_ELOG_OS_ERROR,
                            "fclose() error. errno=%d", errno);
  }

err_end2:
  ret = pthread_mutex_unlock(&s_pl_network_mutex);
  if (ret != 0) {
    NETWORK_LOG_ERR_WITH_ID(PL_NETWORK_ELOG_OS_ERROR,
                    "pthread_mutex_unlock() failed.(ret=%d, errno=%d)",
                    ret, errno);
  }

err_end1:
  return err_code;
}
#endif  // #ifdef __NuttX__

#ifndef __NuttX__
PlErrCode PlNetworkGetNetStat(char *buf, const uint32_t buf_size) {
  (void)buf;
  (void)buf_size;
  return kPlErrCodeOk;
}
#endif  // #ifndef __NuttX__

// -----------------------------------------------------------------------------
//  PlNetworkRegisterEventHandler
//
//  Register event handler and ethernet events.
//
//  Args:
//    if_name(const char *): Interface name.
//    handler(OsalEtherEventHandler): Event handler.
//    private_data(void *): User data.
//
//  Returns:
//    PlErrCode: kPlErrCodeOk on successful,
//                other than kPlErrCodeOk on failure.
//
//  Note:
//    Only one handler can be registered per interface.
//
// -----------------------------------------------------------------------------
PlErrCode PlNetworkRegisterEventHandler(const char *if_name,
                                        PlNetworkEventHandler handler,
                                        void *private_data) {
  PlErrCode err_code = kPlErrCodeOk;
  // mutex lock
  int ret = pthread_mutex_lock(&s_pl_network_mutex);
  if (ret != 0) {
    err_code = kPlErrLock;
    NETWORK_LOG_ERR_WITH_ID(PL_NETWORK_ELOG_OS_ERROR,
                            "pthread_mutex_lock() failed.(ret=%d, errno=%d)",
                            ret, errno);
    goto err_end1;
  }

  // parameter error
  if (if_name == NULL) {
    err_code = kPlErrInvalidParam;
    NETWORK_LOG_ERR(0x14, "argument error.");
    goto err_end2;
  }

  // initialize check
  if (s_pl_network_init_state != kPlNetworkIniStatRunning) {
    err_code = kPlErrInvalidState;
    NETWORK_LOG_ERR(0x15, "initialized error. state=%u",
                    s_pl_network_init_state);
    goto err_end2;
  }

  // interface check
  struct PlNetworkData *net_data = NULL;
  err_code = NetworkGetData(if_name, &net_data);
  if (err_code != kPlErrCodeOk) {
    NETWORK_LOG_ERR(0x16, "interface error.(if_name=%s, err_code=%u)",
                      if_name, err_code);
    err_code = kPlErrNotFound;
    goto err_end2;
  }
  PlNetworkInfo *net_info = &(net_data->net_info);

  // state check
  if (net_info->state != kPlNetIfStateStopped) {
    err_code = kPlErrInvalidState;
    NETWORK_LOG_ERR(0x17, "state error.(state=%s)",
                     NETIF_STATE_SHOW(net_info->state));
    goto err_end2;
  }

  // handler check
  if (net_info->handler != NULL) {
    err_code = kPlErrHandler;
    NETWORK_LOG_ERR(0x18, "handler is registerd.");
    goto err_end2;
  }

  // ops check
  if (net_info->ops == NULL) {
    err_code = kPlErrNotFound;
    NETWORK_LOG_ERR(0x19, "ops not found.");
    goto err_end2;
  }

  // api check
  if (net_info->ops->reg_event == NULL) {
    err_code = kPlErrNotFound;
    NETWORK_LOG_ERR(0x1A, "api not found.");
    goto err_end2;
  }

  net_info->handler = handler;
  net_info->private_data = private_data;
  err_code = net_info->ops->reg_event(net_info);
  if (err_code != kPlErrCodeOk) {
    NETWORK_LOG_ERR_WITH_ID(PL_NETWORK_ELOG_REG_EVENT_ERROR,
                            "register failed. err_code=%u", err_code);
    err_code = kPlErrInvalidOperation;
    net_info->handler = NULL;
    net_info->private_data = NULL;
    goto err_end2;
  }

err_end2:
  ret = pthread_mutex_unlock(&s_pl_network_mutex);
  if (ret != 0) {
    NETWORK_LOG_ERR_WITH_ID(PL_NETWORK_ELOG_OS_ERROR,
                    "pthread_mutex_unlock() failed.(ret=%d, errno=%d)",
                    ret, errno);
  }

err_end1:
  return err_code;
}

// -----------------------------------------------------------------------------
//  PlNetworkUnregisterEventHandler
//
//  Unregister event handler and ethernet events.
//
//  Args:
//    if_name(const char *): Interface name.
//
//  Returns:
//    PlErrCode: kPlErrCodeOk on successful,
//                other than kPlErrCodeOk on failure.
//
//  Note:
//
// -----------------------------------------------------------------------------
PlErrCode PlNetworkUnregisterEventHandler(const char *if_name) {
  PlErrCode err_code = kPlErrCodeOk;
  int ret = pthread_mutex_lock(&s_pl_network_mutex);
  if (ret != 0) {
    err_code = kPlErrLock;
    NETWORK_LOG_ERR_WITH_ID(PL_NETWORK_ELOG_OS_ERROR,
                            "pthread_mutex_lock() failed.(ret=%d, errno=%d)",
                            ret, errno);
    goto err_end1;
  }

  // parameter error
  if (if_name == NULL) {
    err_code = kPlErrInvalidParam;
    NETWORK_LOG_ERR(0x1B, "argument error.");
    goto err_end2;
  }

  // initialize check
  if (s_pl_network_init_state != kPlNetworkIniStatRunning) {
    err_code = kPlErrInvalidState;
    NETWORK_LOG_ERR(0x1C, "initialized error. state=%u",
                    s_pl_network_init_state);
    goto err_end2;
  }

  // interface check
  struct PlNetworkData *net_data = NULL;
  err_code = NetworkGetData(if_name, &net_data);
  if (err_code != kPlErrCodeOk) {
    NETWORK_LOG_ERR(0x1D, "interface error.(if_name=%s, err_code=%u)",
                      if_name, err_code);
    err_code = kPlErrNotFound;
    goto err_end2;
  }
  PlNetworkInfo *net_info = &(net_data->net_info);

  // state check
  if (net_info->state != kPlNetIfStateStopped) {
    err_code = kPlErrInvalidState;
    NETWORK_LOG_ERR(0x1E, "state error.(state=%s)",
                     NETIF_STATE_SHOW(net_info->state));
    goto err_end2;
  }

  // handler check
  if (net_info->handler == NULL) {
    err_code = kPlErrHandler;
    NETWORK_LOG_ERR(0x1F, "handler is not registerd.");
    goto err_end2;
  }

  // ops check
  if (net_info->ops == NULL) {
    err_code = kPlErrNotFound;
    NETWORK_LOG_ERR(0x20, "ops not found.");
    goto err_end2;
  }

  // api check
  if (net_info->ops->unreg_event == NULL) {
    err_code = kPlErrNotFound;
    NETWORK_LOG_ERR(0x21, "api not found.");
    goto err_end2;
  }

  // register event
  err_code = net_info->ops->unreg_event(net_info);
  if (err_code != kPlErrCodeOk) {
    NETWORK_LOG_ERR_WITH_ID(PL_NETWORK_ELOG_UNREG_EVENT_ERROR,
                            "unregister failed. err_code=%u", err_code);
    err_code = kPlErrInvalidOperation;
    goto err_end2;
  }

  // clear handler
  net_info->handler = NULL;
  net_info->private_data = NULL;

err_end2:
  ret = pthread_mutex_unlock(&s_pl_network_mutex);
  if (ret != 0) {
    NETWORK_LOG_ERR_WITH_ID(PL_NETWORK_ELOG_OS_ERROR,
                    "pthread_mutex_unlock() failed.(ret=%d, errno=%d)",
                    ret, errno);
  }

err_end1:
  return err_code;
}

// -----------------------------------------------------------------------------
//  PlNetworkStart
//
//  Enable network interface and start network connection.
//
//  Args:
//    if_name(const char *): Interface name.
//
//  Returns:
//    PlErrCode: kPlErrCodeOk on successful,
//                other than kPlErrCodeOk on failure.
//
//  Note:
//
// -----------------------------------------------------------------------------
PlErrCode PlNetworkStart(const char *if_name) {
  PlErrCode err_code = kPlErrCodeOk;
  int ret = pthread_mutex_lock(&s_pl_network_mutex);
  if (ret != 0) {
    err_code = kPlErrLock;
    NETWORK_LOG_ERR_WITH_ID(PL_NETWORK_ELOG_OS_ERROR,
                            "pthread_mutex_lock() failed.(ret=%d, errno=%d)",
                            ret, errno);
    goto err_end1;
  }

  // argument check
  if (if_name == NULL) {
    err_code = kPlErrInvalidParam;
    NETWORK_LOG_ERR(0x22, "argument error.");
    goto err_end2;
  }

  // initialize check
  if (s_pl_network_init_state != kPlNetworkIniStatRunning) {
    err_code = kPlErrInvalidState;
    NETWORK_LOG_ERR(0x23, "initialized error. state=%u",
                    s_pl_network_init_state);
    goto err_end2;
  }

  // interface check
  struct PlNetworkData *net_data = NULL;
  err_code = NetworkGetData(if_name, &net_data);
  if (err_code != kPlErrCodeOk) {
    NETWORK_LOG_ERR(0x24, "interface error.(if_name=%s, err_code=%u)",
                      if_name, err_code);
    err_code = kPlErrNotFound;
    goto err_end2;
  }
  PlNetworkInfo *net_info = &(net_data->net_info);

  // state check
  if (net_info->state != kPlNetIfStateStopped) {
    err_code = kPlErrInvalidState;
    NETWORK_LOG_ERR(0x25, "state error.(state=%s)",
                     NETIF_STATE_SHOW(net_info->state));
    goto err_end2;
  }

  // ops check
  if (net_info->ops == NULL) {
    err_code = kPlErrNotFound;
    NETWORK_LOG_ERR(0x26, "ops not found.");
    goto err_end2;
  }

  // api check
  if (net_info->ops->start == NULL) {
    err_code = kPlErrNotFound;
    NETWORK_LOG_ERR(0x27, "api not found.");
    goto err_end2;
  }

  // start
  net_info->state = kPlNetIfStateStarted;
  err_code = net_info->ops->start(net_info);
  if (kPlErrCodeOk != err_code) {
    NETWORK_LOG_ERR_WITH_ID(PL_NETWORK_ELOG_START_ERROR,
                            "start failed. err_code=%u", err_code);
    err_code = kPlErrInvalidOperation;
    // -> stooped
    net_info->state = kPlNetIfStateStopped;
    goto err_end2;
  }

err_end2:
  ret = pthread_mutex_unlock(&s_pl_network_mutex);
  if (ret != 0) {
    NETWORK_LOG_ERR_WITH_ID(PL_NETWORK_ELOG_OS_ERROR,
                    "pthread_mutex_unlock() failed.(ret=%d, errno=%d)",
                    ret, errno);
  }

err_end1:
  return err_code;
}

// -----------------------------------------------------------------------------
//  PlNetworkStop
//
//  Disable network interface and stop network connection.
//
//  Args:
//    if_name(const char *): Interface name.
//
//  Returns:
//    PlErrCode: kPlErrCodeOk on successful,
//                other than kPlErrCodeOk on failure.
//
//  Note:
//
// -----------------------------------------------------------------------------
PlErrCode PlNetworkStop(const char *if_name) {
  PlErrCode err_code = kPlErrCodeOk;
  int ret = pthread_mutex_lock(&s_pl_network_mutex);
  if (ret != 0) {
    NETWORK_LOG_ERR_WITH_ID(PL_NETWORK_ELOG_OS_ERROR,
                            "pthread_mutex_lock() failed.(ret=%d, errno=%d)",
                    ret, errno);
    err_code = kPlErrLock;
    goto err_end1;
  }

  // argument check
  if (if_name == NULL) {
    err_code = kPlErrInvalidParam;
    NETWORK_LOG_ERR(0x28, "argument error.");
    goto err_end2;
  }

  // initialize check
  if (s_pl_network_init_state != kPlNetworkIniStatRunning) {
    err_code = kPlErrInvalidState;
    NETWORK_LOG_ERR(0x29, "initialized error. state=%u",
                    s_pl_network_init_state);
    goto err_end2;
  }

  // interface check
  struct PlNetworkData *net_data = NULL;
  err_code = NetworkGetData(if_name, &net_data);
  if (err_code != kPlErrCodeOk) {
    NETWORK_LOG_ERR(0x2A, "interface error.(if_name=%s, err_code=%u)",
                      if_name, err_code);
    err_code = kPlErrNotFound;
    goto err_end2;
  }
  PlNetworkInfo *net_info = &(net_data->net_info);

  // state check
  if (net_info->state != kPlNetIfStateStarted) {
    err_code = kPlErrInvalidState;
    NETWORK_LOG_ERR(0x2B, "state error.(state=%s)",
                     NETIF_STATE_SHOW(net_info->state));
    goto err_end2;
  }

  // ops check
  if (net_info->ops == NULL) {
    err_code = kPlErrNotFound;
    NETWORK_LOG_ERR(0x2C, "ops not found.");
    goto err_end2;
  }

  // api check
  if (net_info->ops->stop == NULL) {
    err_code = kPlErrNotFound;
    NETWORK_LOG_ERR(0x2D, "api not found.");
    goto err_end2;
  }

  // stop
  net_info->state = kPlNetIfStateStopped;
  err_code = net_info->ops->stop(net_info);
  if (kPlErrCodeOk != err_code) {
    NETWORK_LOG_ERR_WITH_ID(PL_NETWORK_ELOG_STOP_ERROR,
                            "stop failed. err_code=%u", err_code);
    err_code = kPlErrInvalidOperation;
    // -> started
    net_info->state = kPlNetIfStateStarted;
    goto err_end2;
  }

err_end2:
  ret = pthread_mutex_unlock(&s_pl_network_mutex);
  if (ret != 0) {
    NETWORK_LOG_ERR_WITH_ID(PL_NETWORK_ELOG_OS_ERROR,
                    "pthread_mutex_unlock() failed.(ret=%d, errno=%d)",
                    ret, errno);
  }

err_end1:
  return err_code;
}

// -----------------------------------------------------------------------------
PlErrCode PlNetworkStopPre(const char *if_name) {
  PlErrCode err = PlNetworkEventSend(if_name, kPlNetworkEventIfDownPre, 0);
  NETWORK_LOG_DBG("%s:%d", __func__, err);
  return err;
}

// -----------------------------------------------------------------------------
//  PlNetworkStructInitialize
//
//  Perform network structure initialize.
//
//  Args:
//    structure(void *): Network structure.
//    type(PlNetworkStructType): Network structure type.
//
//  Returns:
//    PlErrCode: kPlErrCodeOk on successful,
//                other than kPlErrCodeOk on failure.
//
//  Note:
//
// -----------------------------------------------------------------------------
PlErrCode PlNetworkStructInitialize(void *structure,
                                    PlNetworkStructType type) {
  PlErrCode err_code = kPlErrCodeOk;
  // mutex lock
  int ret = pthread_mutex_lock(&s_pl_network_mutex);
  if (ret != 0) {
    NETWORK_LOG_ERR_WITH_ID(PL_NETWORK_ELOG_OS_ERROR,
                            "pthread_mutex_lock() failed.(ret=%d, errno=%d)",
                            ret, errno);
    err_code = kPlErrLock;
    goto err_end1;
  }

  PlNetworkSystemInfo *info = (PlNetworkSystemInfo *)NULL;
  PlNetworkWifiConfig *wifi_config = (PlNetworkWifiConfig *)NULL;
  PlNetworkWifiStatus *wifi_status = (PlNetworkWifiStatus *)NULL;

  // argument check
  if (structure == NULL) {
    err_code = kPlErrInvalidParam;
    NETWORK_LOG_ERR(0x2E, "argument error.");
    goto err_end2;
  }

  // initialize check
  if (s_pl_network_init_state != kPlNetworkIniStatRunning) {
    err_code = kPlErrInvalidState;
    NETWORK_LOG_ERR(0x2F, "initialized error. state=%u",
                    s_pl_network_init_state);
    goto err_end2;
  }

  // Network structure type
  switch (type) {
    // System Information
    case kPlNetworkStructTypeSystemInfo:
      info = (PlNetworkSystemInfo *)structure;
      memset(info->if_name, '\0', sizeof(info->if_name));
      info->type = kPlNetworkTypeUnkown;
      info->cloud_enable = false;
      info->local_enable = false;
      break;

    // Wi-Fi Config
    case kPlNetworkStructTypeWifiConfig:
      wifi_config = (PlNetworkWifiConfig *)structure;
      wifi_config->mode = kPlNetworkWifiModeUnkown;
      memset(wifi_config->ssid, '\0', sizeof(wifi_config->ssid));
      memset(wifi_config->pass, '\0', sizeof(wifi_config->pass));
      break;

    // Wi-Fi Status
    case kPlNetworkStructTypeWifiStatus:
      wifi_status = (PlNetworkWifiStatus *)structure;
      wifi_status->rssi = 0;
      wifi_status->band_width = kPlWifiBandWidthHt20;
      strncpy(wifi_status->country.cc, "01", sizeof(wifi_status->country.cc));
      wifi_status->country.schan = PL_NETWORK_SCHAN_DEFAULT;
      wifi_status->country.nchan = PL_NETWORK_NCHAN_DEFAULT;
      wifi_status->country.max_tx_power = 0;
      wifi_status->country.policy = kPlWifiCountryPolicyAuto;
      break;

    default:
      // Do nothing
      err_code = kPlErrInvalidParam;
      NETWORK_LOG_ERR(0x30, "type error.(type=%d)", type);
      goto err_end2;
      break;
  }

err_end2:
  ret = pthread_mutex_unlock(&s_pl_network_mutex);
  if (ret != 0) {
    NETWORK_LOG_ERR_WITH_ID(PL_NETWORK_ELOG_OS_ERROR,
                    "pthread_mutex_unlock() failed.(ret=%d, errno=%d)",
                    ret, errno);
  }

err_end1:
  return err_code;
}

// -----------------------------------------------------------------------------
//  PlNetworkInitialize
//
//  Perform network feature general initialize.
//
//  Args:
//    network_root(PlConfigObj *): Network configuration object.
//
//  Returns:
//    PlErrCode: kPlErrCodeOk on successful,
//                other than kPlErrCodeOk on failure.
//
//  Note:
//
// -----------------------------------------------------------------------------
PlErrCode PlNetworkInitialize(void) {
  PlErrCode err_code = kPlErrCodeOk;
  uint32_t thread_priority = NETWORK_THREAD_PRIORITY;

  // initialize check
  if (s_pl_network_init_state == kPlNetworkIniStatRunning) {
    err_code = kPlErrInvalidState;
    NETWORK_LOG_ERR(0x31, "initialized error. state=%u",
                    s_pl_network_init_state);
    goto err_end1;
  }

  // message list initialize
  TAILQ_INIT(&s_network_msg_info.list);

  // message buffer initialize
  s_network_msg_info.max_msg_size =
      (uint32_t)(sizeof(struct PlNetworkMsg) * NETWORK_MSG_MAX_SIZE);
  s_network_msg_info.msgs =
      (struct PlNetworkMsg *)malloc(s_network_msg_info.max_msg_size);
  if (s_network_msg_info.msgs == NULL) {
    err_code = kPlErrMemory;
    NETWORK_LOG_ERR_WITH_ID(PL_NETWORK_ELOG_OS_ERROR, "malloc() failed.");
    goto err_end2;
  }
  memset(s_network_msg_info.msgs, 0, s_network_msg_info.max_msg_size);

  // initialize network interface
  err_code = NetworkDeviceInitialize(thread_priority);
  if (err_code != kPlErrCodeOk) {
    NETWORK_LOG_ERR(0x32, "NetworkDeviceInitialize() failed."
                    "(err_code=%d)", err_code);
    goto err_end3;
  }

  // Create Network Event Thread
  err_code = NetworkCreateEventThread(thread_priority);
  if (err_code != kPlErrCodeOk) {
    NETWORK_LOG_ERR(0x33, "NetworkCreateEventThread() failed."
                    "(err_code=%d)", err_code);
    goto err_end3;
  }
  // Ready -> Running
  s_pl_network_init_state = kPlNetworkIniStatRunning;

  return err_code;

err_end3:
  // message buffer clear
  free(s_network_msg_info.msgs);
  s_network_msg_info.msgs = (struct PlNetworkMsg *)NULL;
  s_network_msg_info.max_msg_size = 0;

err_end2:
  // finalize network device
  NetworkDeviceFinalize();

err_end1:
  return err_code;
}

// -----------------------------------------------------------------------------
//  PlNetworkFinalize
//
//  Perform network feature general finalize.
//
//  Args:
//    void: -
//
//  Returns:
//    PlErrCode: kPlErrCodeOk on successful,
//                other than kPlErrCodeOk on failure.
//
//  Note:
//
// -----------------------------------------------------------------------------
PlErrCode PlNetworkFinalize(void) {
  PlErrCode err_code = kPlErrCodeOk;
  PlErrCode pl_ercd = kPlErrCodeOk;
  int ret = 0;
  void *result = NULL;
  struct PlNetworkMsg *entry, *temp;
  irqstate_t flags;

  // initialize check
  if (s_pl_network_init_state != kPlNetworkIniStatRunning) {
    err_code = kPlErrInvalidState;
    NETWORK_LOG_ERR(0x34, "initialized error. state=%u",
                    s_pl_network_init_state);
    goto err_end;
  }

  // Finalize Network Interfaces
  err_code = NetworkDeviceFinalize();
  if (err_code != kPlErrCodeOk) {
    NETWORK_LOG_ERR(0x35, "NetworkDeviceFinalize() failed."
                    "(err_code=%d)", err_code);
  }

  // Quit message send
  pl_ercd = NetworkSendMsg(kPlNetworkCmdQuit, NULL, 0, 0);
  if (pl_ercd != kPlErrCodeOk) {
    err_code = pl_ercd;
    NETWORK_LOG_ERR(0x36, "NetworkSendMsg() failed.(err_code=%d)", err_code);
    // PlNetworkEventThread cancel
    ret = pthread_cancel(s_pl_network_thread_pid);
    if (ret != 0) {
      NETWORK_LOG_ERR_WITH_ID(PL_NETWORK_ELOG_OS_ERROR,
                    "pthread_cancel() failed. errno=%d", errno);
    }
  }

  // Wait PlNetworkEventThread finish
  ret = pthread_join(s_pl_network_thread_pid, &result);
  if (ret != 0) {
    err_code = kPlThreadError;
    NETWORK_LOG_ERR_WITH_ID(PL_NETWORK_ELOG_OS_ERROR,
                            "pthread_join() failed. errno=%d", errno);
  }

  // message list clear
  flags = NetworkLock();
  TAILQ_FOREACH_SAFE(entry, &s_network_msg_info.list, head, temp) {
    TAILQ_REMOVE(&s_network_msg_info.list, entry, head);
    entry = (struct PlNetworkMsg *)NULL;
  }
  NetworkUnlock(flags);

  // message buffer clear
  free(s_network_msg_info.msgs);
  s_network_msg_info.msgs = (struct PlNetworkMsg *)NULL;
  s_network_msg_info.max_msg_size = 0;

  // Running -> Ready
  s_pl_network_init_state = kPlNetworkIniStatReady;

err_end:
  return err_code;
}

// -----------------------------------------------------------------------------
//  PlNetworkEventSend
//
//  Send network event message.
//
//  Args:
//    if_name(const char *): Interface name.
//    event(PlNetworkEvent): Network event.
//    reason(uint8_t): Reason for disconnection.
//
//  Returns:
//    PlErrCode: kPlErrCodeOk on successful,
//                other than kPlErrCodeOk on failure.
//
//  Note:
//
// -----------------------------------------------------------------------------
PlErrCode PlNetworkEventSend(const char *if_name,
                             PlNetworkEvent network_event_id,
                             uint8_t reason) {
  PlErrCode err_code = kPlErrCodeOk;

  // argument check
  if (if_name == NULL) {
    err_code = kPlErrInvalidParam;
    goto err_end;
  }

  // Event message send
  err_code = NetworkSendMsg(kPlNetworkCmdEvent, if_name,
                            network_event_id, reason);

err_end:
  return err_code;
}

// -----------------------------------------------------------------------------
//  PlNetworkEventThread
//
//  Network event thread.
//  When a message is received, an event handler is executed to notify
//  network events.
//
//  Args:
//    arg(void *): Argument.
//
//  Returns:
//    void *: NULL
//
//  Note:
//
// -----------------------------------------------------------------------------
void *PlNetworkEventThread(void *arg) {
  (void)arg;  // Avoid compiler warning
  PlErrCode pl_ercd = kPlErrCodeOk;
  int ret = 0;
  int i = 0;
  struct PlNetworkMsg *msg, *entry, *temp;
  struct PlNetworkMsg *msg_list[NETWORK_MSG_MAX_SIZE];
  uint8_t msg_num = 0;
  struct PlNetworkData *net_data = NULL;
  PlNetworkInfo *net_info = NULL;
  irqstate_t flags;

  // semaphore initialize
  ret = sem_init(&s_network_msg_sem, 0, 0);
  if (ret != 0) {
    NETWORK_LOG_ERR_WITH_ID(PL_NETWORK_ELOG_OS_ERROR,
                            "sem_init() failed. errno=%d", errno);
    return NULL;
  }

  // set thread name
  pthread_setname_np(s_pl_network_thread_pid, "PlNetworkEventThread");

  // main loop
  while (1) {
    // initalzie message
    msg_num = 0;

    // list is not empty
    flags = NetworkLock();
    if (!TAILQ_EMPTY(&s_network_msg_info.list)) {
      // search message list
      TAILQ_FOREACH_SAFE(entry, &s_network_msg_info.list, head, temp) {
        msg_list[msg_num] = entry;
        msg_num++;
      }
    }
    NetworkUnlock(flags);

    // found message
    for (i = 0; i < msg_num; i++) {
      msg = msg_list[i];
      NETWORK_LOG_DBG("cmd=%u if_name=%s network_event_id=%u reason=%d",
                      msg->cmd, msg->if_name, msg->event, msg->reason);

      // Message Command
      switch (msg->cmd) {
        // Network Events
        case kPlNetworkCmdEvent:
          // Get Event Handler
          pl_ercd = NetworkGetData(msg->if_name, &net_data);
          if (pl_ercd == kPlErrCodeOk) {
            net_info = &(net_data->net_info);
            if (net_info->handler != NULL) {
              // Execute Network Event Handler
              net_info->handler(msg->if_name, msg->event,
                                net_info->private_data);
            } else {
              NETWORK_LOG_DBG("handler is NULL !!!");
            }
          }
          break;

        // Thread Finalize
        case kPlNetworkCmdQuit:
          goto thread_end;
          break;

        default:
          // Do nothing
          break;
      }

      // remove message
      flags = NetworkLock();
      TAILQ_REMOVE(&s_network_msg_info.list, msg, head);
      NetworkUnlock(flags);

      // clear message
      msg->cmd = kPlNetworkCmdNone;
      memset(msg->if_name, '\0', PL_NETWORK_IFNAME_LEN);
      msg->event = 0;
      msg->reason = 0;
    }

    // get time
    struct timespec abstime;
    clock_gettime(CLOCK_REALTIME, &abstime);

    // 1sec
    abstime.tv_sec  += 1;
    abstime.tv_nsec += 0;

    ret = sem_timedwait(&s_network_msg_sem, &abstime);
    if ((ret == -1) && (errno == ETIMEDOUT)) {
      // Timeout 1sec
      // Note: If many network events occur, can not notify 1sec passed to upper layer.
      for (uint32_t j = 0; j < kNetworkSystemInfoTotalNum; j++) {
        pl_ercd = NetworkGetData(s_network_system_infos[j].if_name, &net_data);
        if (pl_ercd == kPlErrNotFound) {
          break;
        }
        net_info = &(net_data->net_info);
        if ((net_info == NULL) || (net_info->handler == NULL)) {
          break;
        }
        net_info->handler(s_network_system_infos[j].if_name,
                          kPlNetworkEvent1sec,
                          net_info->private_data);
      }
    }
  }

thread_end:
  // semaphore destroy
  sem_destroy(&s_network_msg_sem);

  // Exit Thread
  pthread_exit(0);

  return NULL;
}

// -----------------------------------------------------------------------------
//  NetworkDeviceInitialize
// -----------------------------------------------------------------------------
static PlErrCode NetworkDeviceInitialize(uint32_t thread_priority) {
  PlErrCode err_code = kPlErrCodeOk;
  PlErrCode pl_ercd = kPlErrCodeOk;
  struct PlNetworkData *net_data = NULL;
  PlNetworkInfo *net_info = NULL;

  // list initialize
  SLIST_INIT(&s_network_info);

  for (uint32_t i = 0; i < kNetworkSystemInfoTotalNum; i++) {
    net_data = (struct PlNetworkData *)malloc(sizeof(struct PlNetworkData));
    if (net_data == NULL) {
      err_code = kPlErrMemory;
    NETWORK_LOG_ERR_WITH_ID(PL_NETWORK_ELOG_OS_ERROR,
                            "malloc() failed. errno=%d", errno);
      goto err_end;
    }

    // set data
    net_info = &(net_data->net_info);
    memcpy(net_info->if_name,
           s_network_system_infos[i].if_name, (PL_NETWORK_IFNAME_LEN - 1));
    net_info->if_name[(PL_NETWORK_IFNAME_LEN - 1)] = '\0';
    net_info->if_type = s_network_system_infos[i].type;
    net_info->state = kPlNetIfStateStopped;
    net_info->handler = NULL;
    net_info->private_data = NULL;
    net_info->if_info = NULL;

    switch (s_network_system_infos[i].type) {
      // Ethernet
      case kPlNetworkTypeEther:
#ifdef  CONFIG_PL_NETWORK_HAVE_ETHER
        // initialize
        pl_ercd = PlEtherInitialize(net_info, thread_priority);
        if (pl_ercd != kPlErrCodeOk) {
          err_code = kPlErrInvalidOperation;
          NETWORK_LOG_ERR_WITH_ID(PL_NETWORK_ELOG_ETHER_INIT_ERROR,
                                  "PlEtherInitialize() failed."
                                  "(if_name=%s err_code=%d)",
                                  s_network_system_infos[i].if_name, pl_ercd);
          free(net_data);
          net_data = NULL;
          goto err_end;
        }
#else   // CONFIG_PL_NETWORK_HAVE_ETHER
        (void)pl_ercd;
        NETWORK_LOG_WRN(0x41, "No supported Ethernet.");
#endif  // CONFIG_PL_NETWORK_HAVE_ETHER
        break;

      // Wi-Fi
      case kPlNetworkTypeWifi:
#ifdef  CONFIG_PL_NETWORK_HAVE_WIFI
        // initialize
        pl_ercd = PlWifiInitialize(net_info, thread_priority);
        if (pl_ercd != kPlErrCodeOk) {
          err_code = kPlErrInvalidOperation;
          NETWORK_LOG_ERR_WITH_ID(PL_NETWORK_ELOG_WIFI_INIT_ERROR,
                                  "PlWifiInitialize() failed."
                                  "(if_name=%s err_code=%d)",
                                  s_network_system_infos[i].if_name, pl_ercd);
          free(net_data);
          net_data = NULL;
          goto err_end;
        }
#else   // CONFIG_PL_NETWORK_HAVE_WIFI
        NETWORK_LOG_WRN(0x42, "No supported Wi-Fi.");
#endif  // CONFIG_PL_NETWORK_HAVE_WIFI
        break;

      default:
        free(net_data);
        net_data = NULL;
        break;
    }

    if (net_data != NULL) {
      // add interface info
      SLIST_INSERT_HEAD(&s_network_info, net_data, next);
    }
  }

err_end:
  return err_code;
}

// -----------------------------------------------------------------------------
//  NetworkCreateEventThread
// -----------------------------------------------------------------------------
static PlErrCode NetworkCreateEventThread(uint32_t thread_priority) {
  PlErrCode err_code = kPlErrCodeOk;
  int ret = 0;
  pthread_attr_t thread_attr;

  // attr initialize
  ret = pthread_attr_init(&thread_attr);
  if (ret != 0) {
    err_code = kPlThreadError;
    NETWORK_LOG_ERR_WITH_ID(PL_NETWORK_ELOG_OS_ERROR,
                            "pthread_attr_init() failed.(ret=%d)", ret);
    goto err_end1;
  }

#ifdef __NuttX__
  // set priority
  struct sched_param sparam = {0};
  sparam.sched_priority = thread_priority;
  ret = pthread_attr_setschedparam(&thread_attr, &sparam);
  if (ret != 0) {
    err_code = kPlThreadError;
    NETWORK_LOG_ERR_WITH_ID(PL_NETWORK_ELOG_OS_ERROR,
                        "pthread_attr_setschedparam() failed.(ret=%d)", ret);
    goto err_end2;
  }
#endif

  // set stack
  ret = pthread_attr_setstacksize(&thread_attr,
                                  NETWORK_EVENT_THREAD_STACKSIZE);
  if (ret != 0) {
    err_code = kPlThreadError;
    NETWORK_LOG_ERR_WITH_ID(PL_NETWORK_ELOG_OS_ERROR,
                            "pthread_attr_setstack() failed.(ret=%d)", ret);
    goto err_end2;
  }

  // create thread
  ret = pthread_create(&s_pl_network_thread_pid, &thread_attr,
                       PlNetworkEventThread, NULL);
  if (ret != 0) {
    err_code = kPlThreadError;
    NETWORK_LOG_ERR_WITH_ID(PL_NETWORK_ELOG_OS_ERROR,
                      "pthread_create() failed.(ret=%d errno=%d)", ret, errno);
    goto err_end2;
  }

err_end2:
  // attr destroy
  ret = pthread_attr_destroy(&thread_attr);
  if (ret != 0) {
    err_code = kPlThreadError;
    NETWORK_LOG_ERR_WITH_ID(PL_NETWORK_ELOG_OS_ERROR,
                            "pthread_attr_destroy() failed.(ret=%d)", ret);
  }

err_end1:
  return err_code;
}

// -----------------------------------------------------------------------------
//  NetworkDeviceFinalize
// -----------------------------------------------------------------------------
static PlErrCode NetworkDeviceFinalize(void) {
  PlErrCode err_code = kPlErrCodeOk;
  PlErrCode pl_ercd = kPlErrCodeOk;
  struct PlNetworkData *net_data = NULL;

  for (uint32_t i = 0; i < kNetworkSystemInfoTotalNum; i++) {
    switch (s_network_system_infos[i].type) {
      // Ethernet
      case kPlNetworkTypeEther:
#ifdef  CONFIG_PL_NETWORK_HAVE_ETHER
        // get network info
        err_code = NetworkGetData(s_network_system_infos[i].if_name, &net_data);
        if (err_code == kPlErrCodeOk) {
          // finalize
          pl_ercd = PlEtherFinalize(&(net_data->net_info));
          if (pl_ercd != kPlErrCodeOk) {
            err_code = kPlErrInvalidOperation;
            NETWORK_LOG_ERR(0x39,
                            "PlEtherFinalize() failed.(if_name=%s err_code=%d)",
                            s_network_system_infos[i].if_name, pl_ercd);
          }
          // delete interface
          SLIST_REMOVE(&s_network_info, net_data, PlNetworkData, next);
          free(net_data);
          net_data = NULL;
        } else {
          NETWORK_LOG_ERR(0x3A, "%s Network Info not found.",
                          s_network_system_infos[i].if_name);
        }
#else   // CONFIG_PL_NETWORK_HAVE_ETHER
        (void)pl_ercd;
        (void)net_data;
        NETWORK_LOG_WRN(0x43, "No supported Ethernet.");
#endif  // CONFIG_PL_NETWORK_HAVE_ETHER
        break;

      // Wi-Fi
      case kPlNetworkTypeWifi:
#ifdef  CONFIG_PL_NETWORK_HAVE_WIFI
        // get network info
        err_code = NetworkGetData(s_network_system_infos[i].if_name, &net_data);
        if (err_code == kPlErrCodeOk) {
          // finalize
          pl_ercd = PlWifiFinalize(&(net_data->net_info));
          if (pl_ercd != kPlErrCodeOk) {
            err_code = kPlErrInvalidOperation;
            NETWORK_LOG_ERR(0x3B,
                            "PlWifiFinalize() failed.(if_name=%s err_code=%d)",
                            s_network_system_infos[i].if_name, pl_ercd);
            if (net_data->net_info.if_info) {
              free(net_data->net_info.if_info);
            }
            net_data->net_info.if_info = NULL;
            net_data->net_info.ops = NULL;
          }
          // delete interface
          SLIST_REMOVE(&s_network_info, net_data, PlNetworkData, next);
          free(net_data);
          net_data = NULL;
        } else {
          NETWORK_LOG_ERR(0x3C, "%s Network Info not found.",
                          s_network_system_infos[i].if_name);
        }
#else   // CONFIG_PL_NETWORK_HAVE_WIFI
        NETWORK_LOG_WRN(0x44, "No supported Wi-Fi.");
#endif  // CONFIG_PL_NETWORK_HAVE_WIFI
        break;

      default:
        // Do nothing
        break;
    }
  }

  return err_code;
}

// -----------------------------------------------------------------------------
//  NetworkGetData
// -----------------------------------------------------------------------------
static PlErrCode NetworkGetData(const char *if_name,
                                struct PlNetworkData **net_data) {
  PlErrCode err_code = kPlErrCodeOk;
  bool is_found = false;

  // search list
  struct PlNetworkData *entry, *temp;
  SLIST_FOREACH_SAFE(entry, &s_network_info, next, temp) {
    if (strncmp(if_name, entry->net_info.if_name, PL_NETWORK_IFNAME_LEN) == 0) {
      *net_data = entry;
      is_found = true;
    }
  }

  if (!is_found) {
    err_code = kPlErrNotFound;
  }

  return err_code;
}

// -----------------------------------------------------------------------------
//  NetworkSendMsg
// -----------------------------------------------------------------------------
static PlErrCode NetworkSendMsg(PlNetworkCmd cmd, const char *if_name,
                                PlNetworkEvent event, uint8_t reason) {
  PlErrCode err_code = kPlErrCodeOk;
  int ret = 0;
  struct PlNetworkMsg *msg = (struct PlNetworkMsg*)NULL;
  irqstate_t flags;

  // initialize check
  if (s_network_msg_info.msgs == NULL) {
    err_code = kPlErrInvalidState;
    goto err_end;
  }

  // message buffer check
  for (int i = 0; i < NETWORK_MSG_MAX_SIZE; i++) {
    // empty buffer
    if (s_network_msg_info.msgs[i].cmd == kPlNetworkCmdNone) {
      msg = &s_network_msg_info.msgs[i];
      break;
    }
  }

  // message buffer full
  if (msg == NULL) {
    err_code = kPlErrBufferOverflow;
    goto err_end;
  }

  // set message
  msg->cmd = cmd;
  if (cmd == kPlNetworkCmdEvent) {
    memcpy(msg->if_name, if_name, PL_NETWORK_IFNAME_LEN);
    msg->event = event;
    msg->reason = reason;
  }

  // insert message list
  flags = NetworkLock();
  TAILQ_INSERT_TAIL(&s_network_msg_info.list, msg, head);
  NetworkUnlock(flags);

  // Wake up PlNetworkEventThread
  ret = sem_post(&s_network_msg_sem);
  if (ret != 0) {
    err_code = kPlErrInternal;

    // remove message
    flags = NetworkLock();
    TAILQ_REMOVE(&s_network_msg_info.list, msg, head);
    NetworkUnlock(flags);
  }

err_end:
  return err_code;
}
