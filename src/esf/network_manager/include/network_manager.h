/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ESF_NETWORK_MANAGER_INCLUDE_NETWORK_MANAGER_NETWORK_MANAGER_H_
#define ESF_NETWORK_MANAGER_INCLUDE_NETWORK_MANAGER_NETWORK_MANAGER_H_

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__NuttX__)
#include <nuttx/config.h>
#endif  // __NuttX__

#include <stdbool.h>
#include <stdint.h>

// """Initializes the information used to start the network for Normal mode.

// Initialize with settings to connect in Normal mode, use Wi-Fi, and use DHCP.
// Please use the start by handle API in Normal mode.

// Args:
//     _os_info_ (EsfNetworkManagerOSInfo *): OS system information union.
//       NULL assignment not allowed.
//     _ssid_ (const char *): SSID to be set to EsfNetworkManagerOSInfo.
//       NULL assignment not allowed.
//     _password_ (const char *): Password to be set to EsfNetworkManagerOSInfo.
//       NULL assignment not allowed.

// Note:
// """
#define ESF_NETWORK_MANAGER_INIT_INFO_NORMAL_WIFI(_os_info_, _ssid_,        \
                                                  _password_)               \
  do {                                                                      \
    memset((_os_info_), 0, sizeof(EsfNetworkManagerOSInfo));                \
    snprintf((_os_info_)->normal_mode.wifi_sta.ssid,                        \
             sizeof((_os_info_)->normal_mode.wifi_sta.ssid), "%s", _ssid_); \
    snprintf((_os_info_)->normal_mode.wifi_sta.password,                    \
             sizeof((_os_info_)->normal_mode.wifi_sta.password), "%s",      \
             _password_);                                                   \
    (_os_info_)->normal_mode.ip_method = 0;                                 \
    (_os_info_)->normal_mode.netif_kind = 0;                                \
  } while (0)

// """Initializes the information used to start the network for Normal mode.

// Initialize with settings to connect in Normal mode, use Wi-Fi, and use
// Ethernet. Please use the start by handle API in Normal mode.

// Args:
//     _os_info_ (EsfNetworkManagerOSInfo *): OS system information union.
//       NULL assignment not allowed.

// Note:
// """
#define ESF_NETWORK_MANAGER_INIT_INFO_NORMAL_ETH(_os_info_)  \
  do {                                                       \
    memset((_os_info_), 0, sizeof(EsfNetworkManagerOSInfo)); \
    (_os_info_)->normal_mode.ip_method = 0;                  \
    (_os_info_)->normal_mode.netif_kind = 1;                 \
  } while (0)

// """Initializes the information used to start the network for AccessPoint
// mode.

// Initialize with settings to connect in AccessPoint mode.
// Set the specified values for ssid and password. Otherwise, set the default
// value. Please use the start by handle API in AccessPoint mode.

// Args:
//     _os_info_ (EsfNetworkManagerOSInfo *): OS system information union.
//       NULL assignment not allowed.
//     _ssid_ (const char *): SSID to be set to EsfNetworkManagerOSInfo.
//       NULL assignment not allowed.
//     _password_ (const char *): Password to be set to EsfNetworkManagerOSInfo.
//       NULL assignment not allowed.

// Note:
// """
#define ESF_NETWORK_MANAGER_INIT_INFO_AP(_os_info_, _ssid_, _password_)    \
  do {                                                                     \
    memset((_os_info_), 0, sizeof(EsfNetworkManagerOSInfo));               \
    snprintf((_os_info_)->accesspoint_mode.wifi_ap.ssid,                   \
             sizeof((_os_info_)->accesspoint_mode.wifi_ap.ssid), "%s",     \
             _ssid_);                                                      \
    snprintf((_os_info_)->accesspoint_mode.wifi_ap.password,               \
             sizeof((_os_info_)->accesspoint_mode.wifi_ap.password), "%s", \
             _password_);                                                  \
  } while (0)

// """Enables all masks.

// Enable all masks in a Network Manager-defined mask structure.
// Masks are paired with structure members, and when enabled,
// Network Manager will manipulate the structure member.

// Args:
//     obj (void*): Network Manager-defined mask structure.
//       NULL assignment not allowed.

// Examples:
//     EsfNetworkManagerParameterMask structure;
//     ESF_NETWORK_MANAGER_MASK_ENABLE_ALL(&structure);

// Note:
// """
#define ESF_NETWORK_MANAGER_MASK_ENABLE_ALL(obj) \
  (memset((obj), 0xFF, sizeof(*(obj))))

// """Disables all masks.

// Disables all masks in a Network Manager-defined mask structure.
// Masks are paired with structure members, and when disabled,
// Network Manager does not operate on the structure members.

// Args:
//     obj (void*): Network Manager-defined mask structure.
//       NULL assignment not allowed.

// Examples:
//     EsfNetworkManagerParameterMask structure;
//     ESF_NETWORK_MANAGER_MASK_DISABLE_ALL(&structure);

// Note:
// """
#define ESF_NETWORK_MANAGER_MASK_DISABLE_ALL(obj) \
  (memset((obj), 0x00, sizeof(*(obj))))

// Defines an enumeration type for the result of executing an API.
typedef enum EsfNetworkManagerResult {
  // Success
  kEsfNetworkManagerResultSuccess = 0,

  // HAL API execution error.
  kEsfNetworkManagerResultHWIFError,

  // DHCP server error.
  kEsfNetworkManagerResultUtilityDHCPServerError,

  // IP address operation error.
  kEsfNetworkManagerResultUtilityIPAddressError,

  // External API execution error.
  kEsfNetworkManagerResultExternalError,

  // Not in a viable state.
  kEsfNetworkManagerResultStatusUnexecutable,

  // Already running.
  kEsfNetworkManagerResultStatusAlreadyRunning,

  // Parameter error.
  kEsfNetworkManagerResultInvalidParameter,

  // No connection information available.
  kEsfNetworkManagerResultNoConnectInfo,

  // Callback function is already registered.
  kEsfNetworkManagerResultAlreadyCallbackRegistered,

  // Callback function has already been unregistered.
  kEsfNetworkManagerResultAlreadyCallbackUnregistered,

  // Insufficient resources.
  kEsfNetworkManagerResultResourceExhausted,

  // Internal error.
  kEsfNetworkManagerResultInternalError,

  // Handle mismatch error.
  kEsfNetworkManagerResultNotFound,

  // Handle type error.
  kEsfNetworkManagerResultInvalidHandleType,

  // Operation rejected.
  kEsfNetworkManagerResultFailedPrecondition
} EsfNetworkManagerResult;

// Defines handle type for Network.
typedef int32_t EsfNetworkManagerHandle;

// Invalid value of EsfNetworkManagerHandle.
#define ESF_NETWORK_MANAGER_INVALID_HANDLE (-1)

// Defines the connection mode.
typedef enum EsfNetworkManagerMode {
  // Normal mode.
  kEsfNetworkManagerModeNormal,

  // AccessPoint mode.
  kEsfNetworkManagerModeAccessPoint,

  // The number of definitions.
  kEsfNetworkManagerModeNum,
} EsfNetworkManagerMode;

// Defines the handle type.
typedef enum EsfNetworkManagerHandleType {
  // Control type. API usage is not restricted.
  kEsfNetworkManagerHandleTypeControl,

  // Information type. API usage is partially restricted.
  kEsfNetworkManagerHandleTypeInfo,

  // The number of definitions.
  kEsfNetworkManagerHandleTypeNum,
} EsfNetworkManagerHandleType;

// Defines network connection state.
// Used with EsfNetworkManagerNotifyInfoCallback.
typedef enum EsfNetworkManagerNotifyInfo {
  // Device has connected to network.
  kEsfNetworkManagerNotifyInfoConnected = 0,

  // Device has been disconnected from network.
  kEsfNetworkManagerNotifyInfoDisconnected,

  // Device has started as Wi-Fi access point. Notified when in AccessPoint
  // mode.
  kEsfNetworkManagerNotifyInfoApStart,

  // The number of definitions.
  kSEsfNetworkManagerNotifyInfoNum
} EsfNetworkManagerNotifyInfo;

// Defines the network connection initiation type.
typedef enum EsfNetworkManagerStartType {
  // Use function-specified parameters.
  kEsfNetworkManagerStartTypeFuncParameter = 0,

  // Use the saved parameters.
  kEsfNetworkManagerStartTypeSaveParameter,

  // Use the parameters that were successful last time for Start.
  kEsfNetworkManagerStartTypeLastStartSuccessParameter,

  // The number of definitions.
  kEsfNetworkManagerStartTypeNum
} EsfNetworkManagerStartType;

// Defines the network connection information type.
typedef enum EsfNetworkManagerParameterType {
  // This is normal information.
  kEsfNetworkManagerParameterTypeNormal,

  // AccessPoint information. (Not supported)
  kEsfNetworkManagerParameterTypeAccessPoint,

  // Proxy information.
  kEsfNetworkManagerParameterTypeProxy,

  // The number of definitions.
  kEsfNetworkManagerParameterTypeNum,
} EsfNetworkManagerParameterType;

// IP address length.
#define ESF_NETWORK_MANAGER_IP_ADDRESS_LEN (39 + 1)

// WiFi SSID length.
#define ESF_NETWORK_MANAGER_WIFI_SSID_LEN (32 + 1)

// WiFi password length.
#define ESF_NETWORK_MANAGER_WIFI_PASSWORD_LEN (64 + 1)

// Proxy url length.
#define ESF_NETWORK_MANAGER_PROXY_URL_LEN (256 + 1)

// Proxy user name length.
#define ESF_NETWORK_MANAGER_PROXY_USER_NAME_LEN (32 + 1)

// Proxy password length.
#define ESF_NETWORK_MANAGER_PROXY_PASSWORD_LEN (32 + 1)

// IP information structure.
typedef struct EsfNetworkManagerIPInfo {
  char ip[ESF_NETWORK_MANAGER_IP_ADDRESS_LEN];
  char subnet_mask[ESF_NETWORK_MANAGER_IP_ADDRESS_LEN];
  char gateway[ESF_NETWORK_MANAGER_IP_ADDRESS_LEN];
  char dns[ESF_NETWORK_MANAGER_IP_ADDRESS_LEN];
  char dns2[ESF_NETWORK_MANAGER_IP_ADDRESS_LEN];
} EsfNetworkManagerIPInfo;

// Wi-Fi station information structure.
typedef struct EsfNetworkManagerWiFiStaInfo {
  char ssid[ESF_NETWORK_MANAGER_WIFI_SSID_LEN];
  char password[ESF_NETWORK_MANAGER_WIFI_PASSWORD_LEN];

  // Wi-Fi encryption method. (Not supported)
  // 0: WPA2-PSK
  // 1: WPA3-PSK
  // 2: WPA2_WPA3_PSK
  int32_t encryption;
} EsfNetworkManagerWiFiStaInfo;

// Normal mode information structure.
typedef struct EsfNetworkManagerNormalMode {
  // Static IP information for IPv4.
  EsfNetworkManagerIPInfo dev_ip;

  // Static IP information for IPv6.
  EsfNetworkManagerIPInfo dev_ip_v6;

  // Information about the Wi-Fi Station to connect.
  EsfNetworkManagerWiFiStaInfo wifi_sta;

  // Select the IP address method for the device.
  // 0: use DHCP.
  // 1: use static information.
  int32_t ip_method;

  // Select network interface for the device.
  // 0: Wi-Fi
  // 1: Ethernet
  int32_t netif_kind;
} EsfNetworkManagerNormalMode;

// Wi-Fi access point information structure.
typedef struct EsfNetworkManagerWiFiApInfo {
  char ssid[ESF_NETWORK_MANAGER_WIFI_SSID_LEN];
  char password[ESF_NETWORK_MANAGER_WIFI_PASSWORD_LEN];

  // Wi-Fi encryption method. (Not supported)
  // 0: WPA2-PSK
  // 1: WPA3-PSK
  // 2: WPA2_WPA3_PSK
  int32_t encryption;

  // Wi-Fi channel number. (Not supported)
  int32_t channel;
} EsfNetworkManagerWiFiApInfo;

// AccessPoint mode information structure.
typedef struct EsfNetworkManagerAccessPointMode {
  // Static IP information for IPv4.
  // The state after setting in ESF_NETWORK_MANAGER_INIT_INFO_AP uses the
  // following default values.
  //
  //  IP address: 192.168.4.1
  // Subnet mask: 255.255.255.0
  //     Gateway: 192.168.4.1
  //         DNS: 0.0.0.0
  EsfNetworkManagerIPInfo dev_ip;

  // Information for the device to act as a Wi-Fi access point.
  EsfNetworkManagerWiFiApInfo wifi_ap;
} EsfNetworkManagerAccessPointMode;

// Normal mode and AccessPoint mode information structure.
// For Normal mode, use normal_mode member.
// For AccessPoint mode, use the accesspoint_mode member.
typedef union EsfNetworkManagerOSInfo {
  EsfNetworkManagerNormalMode normal_mode;
  EsfNetworkManagerAccessPointMode accesspoint_mode;
} EsfNetworkManagerOSInfo;

// Network connection status structure.
typedef struct EsfNetworkManagerStatusInfo {
  // Network startup status.
  //  true: Up.
  // false: Down.
  bool is_if_up;

  // Network link status. Valid only for Ethernet.
  //  true: Up.
  // false: Down.
  bool is_link_up;
} EsfNetworkManagerStatusInfo;

// """Callback function to notify the network connection status.

// A callback function that is called when the network connection status
// changes. Notifications are made for each network mode.

// Args:
//     mode (EsfNetworkManagerMode): The mode of the handle in which the
//     callback
//       function was registered by EsfNetworkManagerRegisterCallback.
//     info (EsfNetworkManagerNotifyInfo): Network connection status.
//     private_data (void *): The pointer specified when registered the callback
//       function.

// Note:
// """
typedef void (*EsfNetworkManagerNotifyInfoCallback)(
    EsfNetworkManagerMode mode, EsfNetworkManagerNotifyInfo info,
    void *private_data);

// Mask structure of EsfNetworkManagerIPInfo.
// If 0 is set in the bit field, it will not be processed.
// Setting it to 1 will process it.
typedef struct EsfNetworkManagerIPInfoMask {
  uint8_t ip : 1;
  uint8_t subnet_mask : 1;
  uint8_t gateway : 1;
  uint8_t dns : 1;
  uint8_t dns2 : 1;
} EsfNetworkManagerIPInfoMask;

// Mask structure of EsfNetworkManagerWiFiStaInfo.
// If 0 is set in the bit field, it will not be processed.
// Setting it to 1 will process it.
typedef struct EsfNetworkManagerWiFiStaInfoMask {
  uint8_t ssid : 1;
  uint8_t password : 1;
  uint8_t encryption : 1;
} EsfNetworkManagerWiFiStaInfoMask;

// Mask structure of EsfNetworkManagerWiFiApInfo.
// If 0 is set in the bit field, it will not be processed.
// Setting it to 1 will process it.
typedef struct EsfNetworkManagerWiFiApInfoMask {
  uint8_t ssid : 1;
  uint8_t password : 1;
  uint8_t encryption : 1;
  uint8_t channel : 1;
} EsfNetworkManagerWiFiApInfoMask;

// Mask structure of EsfNetworkManagerNormalMode.
// If 0 is set in the bit field, it will not be processed.
// Setting it to 1 will process it.
typedef struct EsfNetworkManagerNormalModeMask {
  EsfNetworkManagerIPInfoMask dev_ip;
  EsfNetworkManagerIPInfoMask dev_ip_v6;
  EsfNetworkManagerWiFiStaInfoMask wifi_sta;
  uint8_t ip_method : 1;
  uint8_t netif_kind : 1;
} EsfNetworkManagerNormalModeMask;

// Mask structure of EsfNetworkManagerAccessPointMode.
// If 0 is set in the bit field, it will not be processed.
// Setting it to 1 will process it.
typedef struct EsfNetworkManagerAccessPointModeMask {
  EsfNetworkManagerIPInfoMask dev_ip;
  EsfNetworkManagerWiFiApInfoMask wifi_ap;
} EsfNetworkManagerAccessPointModeMask;

// Proxy information structure.
typedef struct EsfNetworkManagerProxy {
  char url[ESF_NETWORK_MANAGER_PROXY_URL_LEN];
  int32_t port;
  char username[ESF_NETWORK_MANAGER_PROXY_USER_NAME_LEN];
  char password[ESF_NETWORK_MANAGER_PROXY_PASSWORD_LEN];
} EsfNetworkManagerProxy;

// Proxy information mask structure.
// If 0 is set in the bit field, it will not be processed.
// Setting it to 1 will process it.
typedef struct EsfNetworkManagerProxyMask {
  uint8_t url : 1;
  uint8_t port : 1;
  uint8_t username : 1;
  uint8_t password : 1;
} EsfNetworkManagerProxyMask;

// A network connection information structure.
typedef struct EsfNetworkManagerParameter {
  EsfNetworkManagerNormalMode normal_mode;
  EsfNetworkManagerAccessPointMode accesspoint_mode;
  EsfNetworkManagerProxy proxy;
} EsfNetworkManagerParameter;

// This is a mask structure for network connection information.
// Sets whether data is enabled or disabled when saving or retrieving via
// ParameterStorageManager.
typedef struct EsfNetworkManagerParameterMask {
  EsfNetworkManagerNormalModeMask normal_mode;
  EsfNetworkManagerAccessPointModeMask accesspoint_mode;
  EsfNetworkManagerProxyMask proxy;
} EsfNetworkManagerParameterMask;

// A network connection information union.
typedef union EsfNetworkManagerVariantParameter {
  EsfNetworkManagerNormalMode normal_mode;
  EsfNetworkManagerAccessPointMode accesspoint_mode;
  EsfNetworkManagerProxy proxy;
} EsfNetworkManagerVariantParameter;

// This is a mask union for network connection information union.
// Sets whether data is enabled or disabled when saving or retrieving via
// ParameterStorageManager.
typedef union EsfNetworkManagerVariantParameterMask {
  EsfNetworkManagerNormalModeMask normal_mode;
  EsfNetworkManagerAccessPointModeMask accesspoint_mode;
  EsfNetworkManagerProxyMask proxy;
} EsfNetworkManagerVariantParameterMask;

// """Initializes the Network module.

// Gets internal resources and initializes internal state. Gets network
// interfaces information from hardware and save it in internal area.
// If call this API again after it has been initialized, it will return
// kEsfNetworkManagerResultSuccess without doing anything.
// If initialization fails, the status will be notified to the Led Manager.

// Returns:
//     EsfNetworkManagerResult: The code returns one of the values
//     EsfNetworkManagerResult depending on the execution result.

// Yields:
//     kEsfNetworkManagerResultSuccess: Success.
//     kEsfNetworkManagerResultResourceExhausted: Failed to acquire memory.
//     kEsfNetworkManagerResultHWIFError: Hardware API responded with an error.

// Note:
//     Multiple calls cannot be made at the same time.
// """
EsfNetworkManagerResult EsfNetworkManagerInit(void);

// """Terminates the Network module.

// Unregisters all event handlers in HAL.
// Frees internal resources.
// Updates the internal state to uninitialized.
// If termination fails due to something other than uninitialized, the status
// will be notified to the Led Manager.

// Returns:
//     EsfNetworkManagerResult: The code returns one of the values
//     EsfNetworkManagerResult depending on the execution result.

// Yields:
//     kEsfNetworkManagerResultSuccess: Success.
//     kEsfNetworkManagerResultStatusUnexecutable: Failed to acquire memory.
//     kEsfNetworkManagerResultHWIFError: Hardware API responded with an error.
//     kEsfNetworkManagerResultInternalError: Internal error.

// Note:
//     Multiple calls cannot be made at the same time.
// """
EsfNetworkManagerResult EsfNetworkManagerDeinit(void);

// """Gets the network module handle.

// Gets the handle of the specified mode and type.

// Args:
//     mode (EsfNetworkManagerMode): Connection mode.
//     handle_type (EsfNetworkManagerHandleType): Handle type.
//     handle (EsfNetworkManagerHandle*):
//       A pointer that receives the obtained handle.

// Returns:
//     EsfNetworkManagerResult: The code returns one of the values
//     EsfNetworkManagerResult depending on the execution result.

// Yields:
//     kEsfNetworkManagerResultSuccess: Success.
//     kEsfNetworkManagerResultInvalidParameter:
//       The specified parameter is invalid.
//     kEsfNetworkManagerResultStatusUnexecutable: Not initialized.
//     kEsfNetworkManagerResultResourceExhausted:
//       The maximum number of handles in use has been exceeded.
//     kEsfNetworkManagerResultInternalError: Internal error.

// Note:
// """
EsfNetworkManagerResult EsfNetworkManagerOpen(
    EsfNetworkManagerMode mode, EsfNetworkManagerHandleType handle_type,
    EsfNetworkManagerHandle *handle);

// """Frees the specified handle.

// The handle cannot be released while the API is being executed.
// Please execute after the API execution is completed.
// If the handle type is "control handle", it cannot be released while
// the handle is connected to the network.
// Please try again after disconnecting the network.

// Args:
//     handle (EsfNetworkManagerHandle*): A pointer that receives the obtained
//     handle.

// Returns:
//     EsfNetworkManagerResult: The code returns one of the values
//     EsfNetworkManagerResult depending on the execution result.

// Yields:
//     kEsfNetworkManagerResultSuccess: Success.
//     kEsfNetworkManagerResultInvalidParameter:
//       The specified parameter is invalid.
//     kEsfNetworkManagerResultStatusUnexecutable: Not initialized.
//     kEsfNetworkManagerResultHWIFError: Hardware API responded with an error.
//     kEsfNetworkManagerResultNotFound: The specified handle was not obtained.
//     kEsfNetworkManagerResultFailedPrecondition:The specified handle in use.
//     kEsfNetworkManagerResultInternalError: Internal error.

// Note:
// """
EsfNetworkManagerResult EsfNetworkManagerClose(EsfNetworkManagerHandle handle);

// """Starts connecting the network.

// Connects to the network using the mode specified by the handle.
// If os_info is NULL, the connection will be made using the information
// previously set in os_info. If os_info is not NULL, connection information
// will be saved inside the Network and will be made using the information.

// Args:
//     handle (EsfNetworkManagerHandle): Control handle.
//     start_type (EsfNetworkManagerStartType):
//       Specify the network connection initiation type.
//     os_info (EsfNetworkManagerOSInfo*): Network connection information.
//       If NULL is specified, the information specified in the previous
//       connection initiation is used.

// Returns:
//     EsfNetworkManagerResult: The code returns one of the values
//     EsfNetworkManagerResult depending on the execution result.

// Yields:
//     kEsfNetworkManagerResultSuccess: Success.
//     kEsfNetworkManagerResultStatusUnexecutable: Not initialized.
//     kEsfNetworkManagerResultHWIFError: Hardware API responded with an error.
//     kEsfNetworkManagerResultExternalError: Error response from
//       ParameterStorageManager API.
//     kEsfNetworkManagerResultNotFound: The specified handle was not obtained.
//     kEsfNetworkManagerResultStatusAlreadyRunning:
//       Network connection already started.
//     kEsfNetworkManagerResultNoConnectInfo:
//       Network connection information does not exist.
//     kEsfNetworkManagerResultUtilityDHCPServerError:
//       DHCP server operation failed.
//     kEsfNetworkManagerResultUtilityIPAddressError:
//       IP address operation failed.
//     kEsfNetworkManagerResultInvalidParameter:
//       The specified parameter is invalid.
//     kEsfNetworkManagerResultInternalError: Internal error.

// Note:
// """
EsfNetworkManagerResult EsfNetworkManagerStart(
    EsfNetworkManagerHandle handle, EsfNetworkManagerStartType start_type,
    EsfNetworkManagerOSInfo *os_info);

// """Starts disconnecting the network.

// Disconnects to the network using the mode specified by the handle.
// Stop the DHCP server if it was running in AccessPoint mode.
// If network disconnection fails, the status will be notified to the Led
// Manager. After the disconnection is successfully started, the status is
// notified to the Led Manager according to the connection status.

// Args:
//     handle (EsfNetworkManagerHandle): Control handle.

// Returns:
//     EsfNetworkManagerResult: The code returns one of the values
//     EsfNetworkManagerResult depending on the execution result.

// Yields:
//     kEsfNetworkManagerResultSuccess: Success.
//     kEsfNetworkManagerResultStatusUnexecutable: Not initialized.
//     kEsfNetworkManagerResultHWIFError: Hardware API responded with an error.
//     kEsfNetworkManagerResultNotFound: The specified handle was not obtained.
//     kEsfNetworkManagerResultStatusAlreadyRunning: Network connection already
//       stopped.
//     kEsfNetworkManagerResultUtilityDHCPServerError: DHCP server operation
//     failed.
//     kEsfNetworkManagerResultInternalError: Internal error.

// Note:
// """
EsfNetworkManagerResult EsfNetworkManagerStop(EsfNetworkManagerHandle handle);

// """Gets network connection status.

// Gets the network connection status using the mode specified by the handle.
// If the network connection has started, get the connection status of the
// starting network. If connection information is saved within the network
// module, obtain the connection status of the network corresponding to the
// saved information. If connection information is not stored within network
// module, priority is given to WiFi to obtain the connection status.

// Args:
//     handle (EsfNetworkManagerHandle): A network handle.
//     status (EsfNetworkManagerStatusInfo*):
//       The acquired network connection status.
//       Must not be NULL.

// Returns:
//     EsfNetworkManagerResult: The code returns one of the values
//     EsfNetworkManagerResult depending on the execution result.

// Yields:
//     kEsfNetworkManagerResultSuccess: Success.
//     kEsfNetworkManagerResultStatusUnexecutable: Not initialized.
//     kEsfNetworkManagerResultHWIFError: Hardware API responded with an error.
//     kEsfNetworkManagerResultNotFound: The specified handle was not obtained.
//     kEsfNetworkManagerResultInvalidParameter: The specified parameter is
//       invalid.
//     kEsfNetworkManagerResultInternalError: Internal error.

// Note:
// """
EsfNetworkManagerResult EsfNetworkManagerGetIFStatus(
    EsfNetworkManagerHandle handle, EsfNetworkManagerStatusInfo *status);

// """Gets network information.

// Gets the network information using the mode specified by the handle.
// If a network connection has already been started, obtain the information used
// when starting the connection. If connection information is stored within the
// network, retrieve the stored information. If connection information is not
// stored within the network, kEsfNetworkManagerResultNoConnectInfo is returned.
// If the IP address information is specified by DHCP and the allocated address
// is in use, the IP address being used will be obtained.

// Args:
//     handle (EsfNetworkManagerHandle): A network handle.
//     ifinfo (EsfNetworkManagerOSInfo*): The acquired network connection
//     status.
//       Must not be NULL.

// Returns:
//     EsfNetworkManagerResult: The code returns one of the values
//     EsfNetworkManagerResult depending on the execution result.

// Yields:
//     kEsfNetworkManagerResultSuccess: Success.
//     kEsfNetworkManagerResultStatusUnexecutable: Not initialized.
//     kEsfNetworkManagerResultNotFound: The specified handle was not obtained.
//     kEsfNetworkManagerResultInvalidParameter: No connection information
//       available.
//     kEsfNetworkManagerResultNoConnectInfo: The specified parameter
//       is invalid.
//     kEsfNetworkManagerResultInternalError: Internal error.

// Note:
// """
EsfNetworkManagerResult EsfNetworkManagerGetIFInfo(
    EsfNetworkManagerHandle handle, EsfNetworkManagerOSInfo *ifinfo);

// """Gets NetStat information.

// Gets the network information using the mode specified by the handle.
// If a network connection has already been started, obtain the information used
// when starting the connection. If connection information is stored within the
// network, retrieve the stored information. If connection information is not
// stored within the network, kEsfNetworkManagerResultNoConnectInfo is returned.
// If the IP address information is specified by DHCP and the allocated address
// is in use, the IP address being used will be obtained.

// Args:
//     handle (EsfNetworkManagerHandle): A network handle.
//     netstat_buf_size (const int32_t): Size of netstat_buf.
//     netstat_buf (char*): Buffer for storing NetStat information acquisition.
//       Must not be NULL.

// Returns:
//     EsfNetworkManagerResult: The code returns one of the values
//     EsfNetworkManagerResult depending on the execution result.

// Yields:
//     kEsfNetworkManagerResultSuccess: Success.
//     kEsfNetworkManagerResultStatusUnexecutable: Not initialized.
//     kEsfNetworkManagerResultHWIFError: Hardware API responded with an error.
//     kEsfNetworkManagerResultNotFound: The specified handle was not obtained.
//     kEsfNetworkManagerResultInvalidParameter: No connection information
//       available.
//     kEsfNetworkManagerResultInternalError: Internal error.

// Note:
// """
EsfNetworkManagerResult EsfNetworkManagerGetNetstat(
    EsfNetworkManagerHandle handle, const int32_t netstat_buf_size,
    char *netstat_buf);

// """Gets RSSI information.

// Gets the RSSI information using the mode specified by the handle.
// This can only be executed when the handle is in Normal mode and Wi-Fi
// connection is started.

// Args:
//     handle (EsfNetworkManagerHandle): A network handle.
//     rssi_buf (int8_t*): Buffer for storing RSSI information acquisition.
//       Must not be NULL.

// Returns:
//     EsfNetworkManagerResult: The code returns one of the values
//     EsfNetworkManagerResult depending on the execution result.

// Yields:
//     kEsfNetworkManagerResultSuccess: Success.
//     kEsfNetworkManagerResultStatusUnexecutable: Not initialized.
//     kEsfNetworkManagerResultHWIFError: Hardware API responded with an error.
//     kEsfNetworkManagerResultNotFound: The specified handle was not obtained.
//     kEsfNetworkManagerResultInvalidParameter: No connection information
//       available.
//     kEsfNetworkManagerResultInternalError: Internal error.

// Note:
// """
EsfNetworkManagerResult EsfNetworkManagerGetRssi(EsfNetworkManagerHandle handle,
                                                 int8_t *rssi_buf);

// """Registers a callback function to notify the network connection status.

// Registers a callback function using the mode specified by the handle.
// The callback function is called when the network connection status changes.
// If private_data is set, specify it when executing the callback function.
// Calls the callback function to notify the latest status when registering.
// In that case, make the call in the context in which this API was executed.

// Args:
//     handle (EsfNetworkManagerHandle): A network handle.
//     notify_callback (EsfNetworkManagerNotifyInfoCallback):
//       A callback function pointer to notify the network connection status.
//       Must not be NULL.
//     private_data (void*): User data to be passed as a parameter when calling
//       the callback function. Can be specified as NULL.

// Returns:
//     EsfNetworkManagerResult: The code returns one of the values
//     EsfNetworkManagerResult depending on the execution result.

// Yields:
//     kEsfNetworkManagerResultSuccess: Success.
//     kEsfNetworkManagerResultStatusUnexecutable: Not initialized.
//     kEsfNetworkManagerResultNotFound: The specified handle was not obtained.
//     kEsfNetworkManagerResultInvalidParameter: No connection information
//       available.
//     kEsfNetworkManagerResultAlreadyCallbackRegistered:
//       Callback function is already registered.
//     kEsfNetworkManagerResultInternalError: Internal error.

// Note:
// """
EsfNetworkManagerResult EsfNetworkManagerRegisterCallback(
    EsfNetworkManagerHandle handle,
    EsfNetworkManagerNotifyInfoCallback notify_callback, void *private_data);

// """Unregisters a callback function that notifies network connection status.

// Unregisters a callback function that notifies network connection status.

// Args:
//     handle (EsfNetworkManagerHandle): A network handle.

// Returns:
//     EsfNetworkManagerResult: The code returns one of the values
//     EsfNetworkManagerResult depending on the execution result.

// Yields:
//     kEsfNetworkManagerResultSuccess: Success.
//     kEsfNetworkManagerResultStatusUnexecutable: Not initialized.
//     kEsfNetworkManagerResultNotFound: The specified handle was not obtained.
//     kEsfNetworkManagerResultInvalidParameter: No connection information
//       available.
//     kEsfNetworkManagerResultAlreadyCallbackUnregistered:
//       Callback function is already unregistered.
//     kEsfNetworkManagerResultInternalError: Internal error.

// Note:
// """
EsfNetworkManagerResult EsfNetworkManagerUnregisterCallback(
    EsfNetworkManagerHandle handle);

// """Saves network connection information via ParameterStorageManager.

// Checks the validity of the information specified for a parameter with a valid
// mask. Saves a parameter with a valid mask via ParameterStorageManager.

// Args:
//     mask (const EsfNetworkManagerParameterMask *):
//       This is a mask structure for network connection information.
//       Only data for which the mask setting is enabled will be saved.
//       Do not specify NULL.
//     parameter (const EsfNetworkManagerParameter *):
//       Network connection information.
//       Please set the information to be saved for data with mask settings
//       enabled. Do not specify NULL.

// Returns:
//     EsfNetworkManagerResult: The code returns one of the values
//     EsfNetworkManagerResult depending on the execution result.

// Yields:
//     kEsfNetworkManagerResultSuccess: Success.
//     kEsfNetworkManagerResultStatusUnexecutable: Not initialized.
//     kEsfNetworkManagerResultExternalError: Error response from
//       ParameterStorageManager API.
//     kEsfNetworkManagerResultInvalidParameter: Input parameter error.
//     kEsfNetworkManagerResultInternalError: Internal
//     error.

// Note:
// """
EsfNetworkManagerResult EsfNetworkManagerSaveParameter(
    const EsfNetworkManagerParameterMask *mask,
    const EsfNetworkManagerParameter *parameter);

// """Load network connection information via ParameterStorageManager.

// The mask is used to obtain valid data via ParameterStorageManager and set it
// to parameter.

// Args:
//     mask (const EsfNetworkManagerParameterMask *):
//       This is a mask structure for network connection information.
//       Only data for which the mask setting is enabled will be load.
//       Do not specify NULL.
//     parameter (EsfNetworkManagerParameter *):
//       This is the acquired information.
//       Do not specify NULL.

// Returns:
//     EsfNetworkManagerResult: The code returns one of the values
//     EsfNetworkManagerResult depending on the execution result.

// Yields:
//     kEsfNetworkManagerResultSuccess: Success.
//     kEsfNetworkManagerResultStatusUnexecutable: Not initialized.
//     kEsfNetworkManagerResultExternalError: Error response from
//       ParameterStorageManager API.
//     kEsfNetworkManagerResultInvalidParameter: Input parameter error.
//     kEsfNetworkManagerResultInternalError: Internal
//     error.

// Note:
// """
EsfNetworkManagerResult EsfNetworkManagerLoadParameter(
    const EsfNetworkManagerParameterMask *mask,
    EsfNetworkManagerParameter *parameter);

// """Saves network connection information via ParameterStorageManager.

// Checks the validity of the information specified for a parameter with a valid
// mask. Saves a parameter with a valid mask via ParameterStorageManager.

// Args:
//     parameter_type (EsfNetworkManagerParameterType):
//       Specify the network connection information to be used.
//       Save the elements specified for mask and parameter.
//     mask (const EsfNetworkManagerVariantParameterMask *):
//       This is a mask union for network connection information.
//       Only data for which the mask setting is enabled will be saved.
//       Do not specify NULL.
//     parameter (const EsfNetworkManagerVariantParameter *):
//       Network connection information.
//       Please set the information to be saved for data with mask settings
//       enabled. Do not specify NULL.

// Returns:
//     EsfNetworkManagerResult: The code returns one of the values
//     EsfNetworkManagerResult depending on the execution result.

// Yields:
//     kEsfNetworkManagerResultSuccess: Success.
//     kEsfNetworkManagerResultStatusUnexecutable: Not initialized.
//     kEsfNetworkManagerResultExternalError: Error response from
//       ParameterStorageManager API.
//     kEsfNetworkManagerResultInvalidParameter: Input parameter error.
//     kEsfNetworkManagerResultInternalError: Internal
//     error.

// Note:
// """
EsfNetworkManagerResult EsfNetworkManagerSaveVariantParameter(
    EsfNetworkManagerParameterType parameter_type,
    const EsfNetworkManagerVariantParameterMask *mask,
    const EsfNetworkManagerVariantParameter *parameter);

// """Load network connection information via ParameterStorageManager.

// The mask is used to obtain valid data via ParameterStorageManager and set it
// to parameter.

// Args:
//     parameter_type (EsfNetworkManagerParameterType):
//       Specify the network connection information to be used.
//       Load the elements specified for mask and parameter.
//     mask (const EsfNetworkManagerVariantParameterMask *):
//       This is a mask union for network connection information.
//       Only data for which the mask setting is enabled will be load.
//       Do not specify NULL.
//     parameter (EsfNetworkManagerVariantParameter *):
//       This is the acquired information.
//       Do not specify NULL.

// Returns:
//     EsfNetworkManagerResult: The code returns one of the values
//     EsfNetworkManagerResult depending on the execution result.

// Yields:
//     kEsfNetworkManagerResultSuccess: Success.
//     kEsfNetworkManagerResultStatusUnexecutable: Not initialized.
//     kEsfNetworkManagerResultExternalError: Error response from
//       ParameterStorageManager API.
//     kEsfNetworkManagerResultInvalidParameter: Input parameter error.
//     kEsfNetworkManagerResultInternalError: Internal
//     error.

// Note:
// """
EsfNetworkManagerResult EsfNetworkManagerLoadVariantParameter(
    EsfNetworkManagerParameterType parameter_type,
    const EsfNetworkManagerVariantParameterMask *mask,
    EsfNetworkManagerVariantParameter *parameter);

EsfNetworkManagerResult EsfNetworkManagerExecMigration(void);
void EsfNetworkManagerEraseMigrationSrcData(void);

#ifdef __cplusplus
}
#endif
#endif  // ESF_NETWORK_MANAGER_INCLUDE_NETWORK_MANAGER_NETWORK_MANAGER_H_
