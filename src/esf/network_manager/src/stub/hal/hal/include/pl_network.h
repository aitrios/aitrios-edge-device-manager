/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PL_NETWORK_H_
#define PL_NETWORK_H_

// NetworkManagerStubEdit
#define FAR
#include <stdbool.h>
#include <stdint.h>

#include "hal/include/pl.h"
#include "netutils/dhcpc.h"

// Includes --------------------------------------------------------------------

// Macros ----------------------------------------------------------------------
// Interface name length
#define PL_NETWORK_IFNAME_LEN (32 + 1)
// SSID length
#define PL_NETWORK_SSID_LEN (32 + 1)
// Password length
#define PL_NETWORK_PASS_LEN (64 + 1)
// country code  length
#define PL_NETWORK_CC_LEN (3)
// start channel
#define PL_NETWORK_SCHAN_DEFAULT (1)
// total channel number
#define PL_NETWORK_NCHAN_DEFAULT (11)

// Sysytem info initalizer
#define PL_NETWORK_SYSTEM_INFO_INITIALIZER \
  {{'\0'}, kPlNetworkTypeUnkown, false, false}

// Wi-Fi config initalizer
#define PL_NETWORK_WIFI_CONFIG_INITIALIZER     \
  {                                            \
    kPlNetworkWifiModeUnkown, {'\0'}, { '\0' } \
  }

// Wi-Fi state initalizer
#define PL_NETWORK_WIFI_STATUS_INITIALIZER                         \
  {                                                                \
    0, kPlWifiBandWidthHt20, {                                     \
      "01", PL_NETWORK_SCHAN_DEFAULT, PL_NETWORK_NCHAN_DEFAULT, 0, \
          kPlWifiCountryPolicyAuto                                 \
    }                                                              \
  }

// Typdefs ---------------------------------------------------------------------
// PL Network Interface Type
typedef enum {
  kPlNetworkTypeEther = 0,
  kPlNetworkTypeWifi,
  kPlNetworkTypeUnkown,
  kPlNetworkTypeMax
} PlNetworkType;

// PL Network Wi-Fi Mode
typedef enum {
  kPlNetworkWifiModeSta = 0,
  kPlNetworkWifiModeAp,
  kPlNetworkWifiModeUnkown,
  kPlNetworkWifiModeMax
} PlNetworkWifiMode;

// PL Network Event
typedef enum {
  kPlNetworkEventLinkUp = 0,
  kPlNetworkEventLinkDown,
  kPlNetworkEventIfUp,
  kPlNetworkEventIfDown,
  kPlNetworkEventWifiReady,
  kPlNetworkEventWifiApStart,
  kPlNetworkEventWifiApStop,
  kPlNetworkEventWifiApConnected,
  kPlNetworkEventWifiApDisconnected,
  kPlNetworkEventWifiApProbeReqRecved,
  kPlNetworkEventWifiStaStart,
  kPlNetworkEventWifiStaStop,
  kPlNetworkEventWifiStaConnected,
  kPlNetworkEventWifiStaDisconnected,
  kPlNetworkEventWifiStaAuthmodeChange,
  kPlNetworkEventWifiStaRssiLow,
  kPlNetworkEventWifiStaBeaconTimeout,
  kPlNetworkEventMax
} PlNetworkEvent;

// PL Network Initializer Type
typedef enum {
  kPlNetworkStructTypeSystemInfo,
  kPlNetworkStructTypeWifiConfig,
  kPlNetworkStructTypeWifiStatus,
  kPlNetworkStructTypeMax
} PlNetworkStructType;

// PL Network System Information
typedef struct {
  char if_name[PL_NETWORK_IFNAME_LEN];
  PlNetworkType type;
  bool cloud_enable;
  bool local_enable;
} PlNetworkSystemInfo;

// PL Network Configuration for Wi-Fi
typedef struct {
  PlNetworkWifiMode mode;
  char ssid[PL_NETWORK_SSID_LEN];
  char pass[PL_NETWORK_PASS_LEN];
} PlNetworkWifiConfig;

// PL Network Configuration
typedef struct {
  PlNetworkType type;
  union {
    PlNetworkWifiConfig wifi;
    // Bluetooth, etc
  } network;
} PlNetworkConfig;

// Wi-Fi Country Policy
typedef enum {
  kPlWifiCountryPolicyAuto = 0,
  kPlWifiCountryPolicyManual,
  kPlWifiCountryPolicyMax,
} PlWifiCountryPolicy;

// Wi-Fi Country Code
typedef struct {
  char cc[PL_NETWORK_CC_LEN];
  uint8_t schan;
  uint8_t nchan;
  int8_t max_tx_power;
  PlWifiCountryPolicy policy;
} PlWifiCountry;

// Wi-Fi BandWidth
typedef enum {
  kPlWifiBandWidthHt20 = 0,
  kPlWifiBandWidthHt40,
  kPlWifiBandWidthMax,
} PlWifiBandWidth;

// PL Network Wi-Fi Status
typedef struct {
  int8_t rssi;
  PlWifiBandWidth band_width;
  PlWifiCountry country;
} PlNetworkWifiStatus;

// PL Network Status
typedef struct {
  union {
    PlNetworkWifiStatus wifi;
  } network;
  bool is_link_up;
  bool is_if_up;
} PlNetworkStatus;

// PL Network Event Handler
typedef void (*PlNetworkEventHandler)(const char *if_name, PlNetworkEvent event,
                                      void *private_data);

// Functions -------------------------------------------------------------------
PlErrCode PlNetworkGetSystemInfoStub(uint32_t *info_total_num,
                                     PlNetworkSystemInfo **infos);
PlErrCode PlNetworkSetConfigStub(const char *if_name,
                                 const PlNetworkConfig *config);
PlErrCode PlNetworkGetConfigStub(const char *if_name, PlNetworkConfig *config);
PlErrCode PlNetworkGetStatusStub(const char *if_name, PlNetworkStatus *status);
PlErrCode PlNetworkGetNetStatStub(char *buf, const uint32_t buf_size);
PlErrCode PlNetworkRegisterEventHandlerStub(const char *if_name,
                                            PlNetworkEventHandler handler,
                                            void *private_data);
PlErrCode PlNetworkUnregisterEventHandlerStub(const char *if_name);
PlErrCode PlNetworkStartStub(const char *if_name);
PlErrCode PlNetworkStopStub(const char *if_name);
PlErrCode PlNetworkStructInitializeStub(void *structure,
                                        PlNetworkStructType type);
PlErrCode PlNetworkInitializeStub(void);
PlErrCode PlNetworkFinalizeStub(void);

// NetworkManager DEBUG STUB
void PlNetworkInitStub(void);
void PlNetworkDeinitStub(void);

typedef enum {
  kPlNetworkIfTypeStubEther = 0,
  kPlNetworkIfTypeStubWifiSta,
  kPlNetworkIfTypeStubWifiAp,
  kPlNetworkIfTypeStubMax
} PlNetworkIfTypeStub;
void PlNetworkCallEventHandlerStub(PlNetworkIfTypeStub if_type,
                                   PlNetworkEvent event);

int dhcpc_request_stub(void *handle, struct dhcpc_state *p_result);

#endif  // PL_NETWORK_H_
