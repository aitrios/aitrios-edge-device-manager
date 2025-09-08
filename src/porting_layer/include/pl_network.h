/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#ifndef PL_NETWORK_H_
#define PL_NETWORK_H_

// Includes --------------------------------------------------------------------
#include <arpa/inet.h>
#include <stdbool.h>
#include "pl.h"

// Macros ----------------------------------------------------------------------
// Interface name length
#define PL_NETWORK_IFNAME_LEN    (32+1)
// SSID length
#define PL_NETWORK_SSID_LEN      (32+1)
// Password length
#define PL_NETWORK_PASS_LEN      (64+1)
// country code  length
#define PL_NETWORK_CC_LEN        (3)
// start channel
#define PL_NETWORK_SCHAN_DEFAULT (1)
// total channel number
#define PL_NETWORK_NCHAN_DEFAULT (11)

#define PL_NETWORK_IFHWADDRLEN   (6)

// Sysytem info initalizer
#define PL_NETWORK_SYSTEM_INFO_INITIALIZER {{'\0'}, kPlNetworkTypeUnkown, \
                                            false, false}

// Wi-Fi config initalizer
#define PL_NETWORK_WIFI_CONFIG_INITIALIZER {kPlNetworkWifiModeUnkown, \
                                            {'\0'}, {'\0'}}

// Wi-Fi state initalizer
#define PL_NETWORK_WIFI_STATUS_INITIALIZER {0, kPlWifiBandWidthHt20, \
                                            {"01", PL_NETWORK_SCHAN_DEFAULT, \
                                             PL_NETWORK_NCHAN_DEFAULT, 0, \
                                             kPlWifiCountryPolicyAuto}}

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
  kPlNetworkEventLinkUp = 0,             // 0
  kPlNetworkEventLinkDown,               // 1
  kPlNetworkEventIfUp,                   // 2
  kPlNetworkEventIfDown,                 // 3
  kPlNetworkEventWifiReady,              // 4
  kPlNetworkEventWifiApStart,            // 5
  kPlNetworkEventWifiApStop,             // 6
  kPlNetworkEventWifiApConnected,        // 7
  kPlNetworkEventWifiApDisconnected,     // 8
  kPlNetworkEventWifiApProbeReqRecved,   // 9
  kPlNetworkEventWifiStaStart,           // 10
  kPlNetworkEventWifiStaStop,            // 11
  kPlNetworkEventWifiStaConnected,       // 12
  kPlNetworkEventWifiStaDisconnected,    // 13
  kPlNetworkEventWifiStaAuthmodeChange,  // 14
  kPlNetworkEventWifiStaRssiLow,         // 15
  kPlNetworkEventWifiStaBeaconTimeout,   // 16
  kPlNetworkEventPhyIdValid,             // 17
  kPlNetworkEventPhyIdInvalid,           // 18
  kPlNetworkEvent1sec,                   // 19
  kPlNetworkEventIfDownPre,              // 20
  kPlNetworkEventMax                     // 21
} PlNetworkEvent;

// PL Network Initializer Type
typedef enum {
  kPlNetworkStructTypeSystemInfo,
  kPlNetworkStructTypeWifiConfig,
  kPlNetworkStructTypeWifiStatus,
  kPlNetworkStructTypeMax
} PlNetworkStructType;

typedef enum {
  kPlNetworkMigrationDataIdIPAddress = 0,
  kPlNetworkMigrationDataIdSubnetMask,
  kPlNetworkMigrationDataIdGateway,
  kPlNetworkMigrationDataIdDNS,
  kPlNetworkMigrationDataIdIPMethod,
  kPlNetworkMigrationDataIdNetifKind,
  kPlNetworkMigrationDataIdProxyURL,
  kPlNetworkMigrationDataIdProxyPort,
  kPlNetworkMigrationDataIdProxyUserName,
  kPlNetworkMigrationDataIdProxyPassword,
  kPlNetworkMigrationDataIdMax
} PlNetworkMigrationDataId;

// PL Network System Information
typedef struct {
  char          if_name[PL_NETWORK_IFNAME_LEN];
  PlNetworkType type;
  bool          cloud_enable;
  bool          local_enable;
} PlNetworkSystemInfo;

// PL Network Configuration for Wi-Fi
typedef struct {
  PlNetworkWifiMode mode;
  char              ssid[PL_NETWORK_SSID_LEN];
  char              pass[PL_NETWORK_PASS_LEN];
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
  char                cc[PL_NETWORK_CC_LEN];
  uint8_t             schan;
  uint8_t             nchan;
  int8_t              max_tx_power;
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
  int8_t           rssi;
  PlWifiBandWidth  band_width;
  PlWifiCountry    country;
} PlNetworkWifiStatus;

// PL Network Status
typedef struct {
  union {
    PlNetworkWifiStatus wifi;
  } network;
  bool is_link_up;
  bool is_if_up;
  bool is_phy_id_valid;
} PlNetworkStatus;

// PL Network Event Handler
typedef void (*PlNetworkEventHandler)(const char *if_name,
                                      PlNetworkEvent event,
                                      void *private_data);

typedef void *PlNetworkMigrationHandle;
struct PlNetworkDhcpcState {
  struct in_addr serverid;
  struct in_addr ipaddr;
  struct in_addr netmask;
  struct in_addr dnsaddr;
  struct in_addr default_router;
  uint32_t lease_time;
  uint32_t renewal_time;
  uint32_t rebinding_time;
};

typedef struct {
  uint32_t use_external_dhcpc : 1;
  uint32_t reserve : 31;
} PlNetworkCapabilities;

typedef struct {
  // Currently only netif_kind
  int32_t netif_kind;
} PlNetworkMigrationNeedParam;

// Functions -------------------------------------------------------------------
PlErrCode PlNetworkGetSystemInfo(uint32_t *info_total_num,
                                 PlNetworkSystemInfo **infos);
PlErrCode PlNetworkSetConfig(const char *if_name,
                             const PlNetworkConfig *config);
PlErrCode PlNetworkGetConfig(const char *if_name,
                             PlNetworkConfig *config);
PlErrCode PlNetworkGetStatus(const char *if_name,
                             PlNetworkStatus *status);
PlErrCode PlNetworkGetNetStat(char *buf, const uint32_t buf_size);
PlErrCode PlNetworkRegisterEventHandler(const char *if_name,
                                        PlNetworkEventHandler handler,
                                        void *private_data);
PlErrCode PlNetworkUnregisterEventHandler(const char *if_name);
PlErrCode PlNetworkStart(const char *if_name);
PlErrCode PlNetworkStop(const char *if_name);
PlErrCode PlNetworkStopPre(const char *if_name);
PlErrCode PlNetworkStructInitialize(void *structure,
                                    PlNetworkStructType type);
PlErrCode PlNetworkInitialize(void);
PlErrCode PlNetworkFinalize(void);
PlErrCode PlNetworkInitMigration(PlNetworkMigrationHandle *handle);
PlErrCode PlNetworkFinMigration(PlNetworkMigrationHandle handle);
PlErrCode PlNetworkGetMigrationData(PlNetworkMigrationHandle handle,
                                    PlNetworkMigrationDataId id, void *dst,
                                    size_t dst_size);
PlErrCode PlNetworkIsNeedMigration(PlNetworkMigrationNeedParam *param,
                                   bool *need_migration);
void PlNetworkEraseMigrationSrcData(void);
PlNetworkCapabilities PlNetworkGetCapabilities(void);
int PlNetworkSetIpv4Addr(const char *ifname, const struct in_addr *addr,
                         const struct in_addr *mask);
int PlNetworkSetIpv4Gateway(const char *ifname, const struct in_addr *addr);
int PlNetworkSetIpv4Dns(const char *ifname, const struct in_addr *addr);
int PlNetworkSetIpv4Method(const char *ifname, bool is_dhcp);
int PlNetworkGetIpv4Addr(const char *ifname, struct in_addr *addr,
                         struct in_addr *mask);
int PlNetworkGetIpv4Gateway(const char *ifname, struct in_addr *addr);
int PlNetworkGetIpv4Dns(const char *ifname, struct in_addr *addr);
int PlNetworkGetMacAddr(const char *ifname, uint8_t *mac);
void *PlNetworkDhcpcOpen(const char *ifname, void *mac, int maclen);
int PlNetworkDhcpcRequest(void *handle, struct PlNetworkDhcpcState *state);
int PlNetworkDhcpcRenew(void *handle, struct PlNetworkDhcpcState *state);
int PlNetworkDhcpcRelease(void *handle, struct PlNetworkDhcpcState *state);
void PlNetworkDhcpcClose(void *handle);
int PlNetworkResetDnsServer(void);

#endif  // PL_NETWORK_H_
