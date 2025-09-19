/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// Includes --------------------------------------------------------------------
#include "pl_network_util_os_impl.h"

#include <NetworkManager.h>
#include <errno.h>
#include <glib.h>
#include <linux/mii.h>
#include <linux/sockios.h>
#include <net/if.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "pl_network_log.h"

// Macros ----------------------------------------------------------------------
#define IPV4_MAX_PREFIX (32)
#define MII_PHYID2_OUI_SHIFT (10)  // Bits 10-15: OUI mask [24:19]
#define MII_PHYID2_OUI_MASK (0x3f << MII_PHYID2_OUI_SHIFT)
#define NETMASK_TOP_BIT (0x80000000)
#define PHY_ID1_DEFAULT (0x7)
#define PHY_ID2_DEFAULT (0x30 << MII_PHYID2_OUI_SHIFT)

// Types (typedef / enum / struct / union) -------------------------------------
typedef struct {
  GMainLoop *loop;
  gboolean success;
} NmCallbackData;

// Static variables ------------------------------------------------------------
static pthread_mutex_t s_mutex = PTHREAD_MUTEX_INITIALIZER;
#ifdef CONFIG_PL_NETWORK_UTIL_IMPL_IFDOWN_DISABLE
// This static variable maintains the virtual interface state when ifdown
// operations are disabled on the T4R platform.
static bool s_is_ifup = true;
#endif

// Private functions -----------------------------------------------------------
static int NmNetmaskToPrefix(uint32_t netmask) {
  uint32_t mask = ntohl(netmask);
  int prefix = 0;
  while (mask & NETMASK_TOP_BIT) {
    prefix++;
    mask <<= 1;
  }
  if (mask != 0) {
    return -1;
  }
  return prefix;
}

// -----------------------------------------------------------------------------
static uint32_t PrefixToNetmask(int prefix) {
  return htonl(UINT32_MAX << (IPV4_MAX_PREFIX - prefix));
}

// -----------------------------------------------------------------------------
static NMConnection *NmGetConnection(NMClient *client, const char *if_name) {
  if (!if_name) {
    DLOGE("Interface name is NULL");
    ELOGE(kElog_NmGetConnection);
    return NULL;
  }
  const GPtrArray *conns = nm_client_get_connections(client);
  if (!conns) {
    DLOGE("Failed to retrieve connection list from NMClient");
    ELOGE(kElog_NmGetConnection);
    return NULL;
  }
  for (guint i = 0; i < conns->len; i++) {
    NMConnection *conn = g_ptr_array_index(conns, i);
    const char *device = nm_connection_get_interface_name(conn);
    if (device && strcmp(device, if_name) == 0) {
      return conn;
    }
  }
  DLOGE("Active connection for the device not found");
  ELOGE(kElog_NmGetConnection);
  return NULL;
}

// -----------------------------------------------------------------------------
static NMIPConfig *NmGetIpv4Config(NMClient *client, const char *if_name) {
  NMDevice *device = nm_client_get_device_by_iface(client, if_name);
  if (!device) {
    DLOGE("Failed to find device for %s", if_name);
    ELOGE(kElog_NmGetIpv4Config);
    return NULL;
  }
  NMIPConfig *ip4 = nm_device_get_ip4_config(device);
  if (!ip4) {
    DLOGE("No IPv4 configuration found");
    ELOGE(kElog_NmGetIpv4Config);
    return NULL;
  }
  return ip4;
}

// -----------------------------------------------------------------------------
static void NmUpdateConnectionCallback(GObject *object, GAsyncResult *res,
                                       gpointer user_data) {
  NMRemoteConnection *rconn = NM_REMOTE_CONNECTION(object);
  NmCallbackData *data = (NmCallbackData *)user_data;
  GError *err = NULL;
  if (!nm_remote_connection_update2_finish(rconn, res, &err)) {
    DLOGE("Failed to update connection: %s", err->message);
    ELOGE(kElog_NmUpdateConnectionCallback);
    g_error_free(err);
    data->success = FALSE;
  } else {
    DLOGD("Successfully updated connection settings");
    data->success = TRUE;
  }
  g_main_loop_quit(data->loop);
}

// -----------------------------------------------------------------------------
static void NmActivateConnectionCallback(GObject *object, GAsyncResult *res,
                                         gpointer user_data) {
  NMClient *client = NM_CLIENT(object);
  NmCallbackData *data = (NmCallbackData *)user_data;
  GError *err = NULL;
  if (!nm_client_activate_connection_finish(client, res, &err)) {
    DLOGE("Failed to activate connection: %s", err->message);
    ELOGE(kElog_NmActivateConnectionCallback);
    g_error_free(err);
    data->success = FALSE;
  } else {
    DLOGD("Successfully activated connection");
    data->success = TRUE;
  }
  g_main_loop_quit(data->loop);
}

// -----------------------------------------------------------------------------
static int NmUpdateConnection(NMConnection *conn) {
  if (!NM_IS_REMOTE_CONNECTION(conn)) {
    DLOGE("Invalid cast: connection is not NMRemoteConnection");
    ELOGE(kElog_NmUpdateConnection);
    return -1;
  }
  NmCallbackData data = {.loop = g_main_loop_new(NULL, FALSE),
                         .success = FALSE};
  nm_remote_connection_update2(
      NM_REMOTE_CONNECTION(conn),
      nm_connection_to_dbus(conn, NM_CONNECTION_SERIALIZE_ALL), 0, NULL, NULL,
      NmUpdateConnectionCallback, &data);
  g_main_loop_run(data.loop);
  g_main_loop_unref(data.loop);
  return data.success ? 0 : -1;
}

// -----------------------------------------------------------------------------
static int NmActivateConnection(NMClient *client, NMConnection *conn) {
  NmCallbackData data = {.loop = g_main_loop_new(NULL, FALSE),
                         .success = FALSE};
  nm_client_activate_connection_async(client, conn, NULL, NULL, NULL,
                                      NmActivateConnectionCallback, &data);
  g_main_loop_run(data.loop);
  g_main_loop_unref(data.loop);
  return data.success ? 0 : -1;
}

// -----------------------------------------------------------------------------
static int NmSetIpv4addr(NMClient *client, const char *if_name,
                         const struct in_addr *addr,
                         const struct in_addr *netmask) {
  NMConnection *conn = NmGetConnection(client, if_name);
  if (!conn) {
    DLOGE("Failed to retrieve connection");
    ELOGE(kElog_NmSetIpv4addr);
    return -1;
  }
  NMSettingIPConfig *ip4 = nm_connection_get_setting_ip4_config(conn);
  if (!ip4) {
    DLOGE("Failed to get IPv4 setting");
    ELOGE(kElog_NmSetIpv4addr);
    return -1;
  }
  nm_setting_ip_config_clear_addresses(ip4);
  if (addr && addr->s_addr && netmask && netmask->s_addr) {
    char addr_str[INET_ADDRSTRLEN] = {0};
    if (!inet_ntop(AF_INET, addr, addr_str, sizeof(addr_str))) {
      DLOGE("inet_ntop failed for IPv4 address");
      ELOGE(kElog_NmSetIpv4addr);
      return -1;
    }
    int prefix = NmNetmaskToPrefix(netmask->s_addr);
    if (prefix < 0) {
      DLOGE("Invalid netmask provided");
      ELOGE(kElog_NmSetIpv4addr);
      return -1;
    }
    NMIPAddress *nm_addr = nm_ip_address_new(AF_INET, addr_str, prefix, NULL);
    if (!nm_addr) {
      DLOGE("Failed to create NMIPAddress");
      ELOGE(kElog_NmSetIpv4addr);
      return -1;
    }
    nm_setting_ip_config_add_address(ip4, nm_addr);
    nm_ip_address_unref(nm_addr);
  } else {
    // No address is configured, so clear the gateway to avoid invalid state
    g_object_set(ip4, NM_SETTING_IP_CONFIG_GATEWAY, NULL, NULL);
  }
  if (NmUpdateConnection(conn) != 0) {
    DLOGE("Failed to update connection");
    ELOGE(kElog_NmSetIpv4addr);
    return -1;
  }
  if (NmActivateConnection(client, conn) != 0) {
    DLOGE("Failed to activate connection");
    ELOGE(kElog_NmSetIpv4addr);
    return -1;
  }
  return 0;
}

// -----------------------------------------------------------------------------
static int NmSetIpv4Gateway(NMClient *client, const char *if_name,
                            const struct in_addr *addr) {
  NMConnection *conn = NmGetConnection(client, if_name);
  if (!conn) {
    DLOGE("Failed to retrieve connection");
    ELOGE(kElog_NmSetIpv4Gateway);
    return -1;
  }
  NMSettingIPConfig *ip4 = nm_connection_get_setting_ip4_config(conn);
  if (!ip4) {
    DLOGE("Failed to get IPv4 setting");
    ELOGE(kElog_NmSetIpv4Gateway);
    return -1;
  }
  if (addr && addr->s_addr) {
    char addr_str[INET_ADDRSTRLEN] = {0};
    if (!inet_ntop(AF_INET, addr, addr_str, sizeof(addr_str))) {
      DLOGE("inet_ntop failed for gateway address");
      ELOGE(kElog_NmSetIpv4Gateway);
      return -1;
    }
    g_object_set(ip4, NM_SETTING_IP_CONFIG_GATEWAY, addr_str,
                 NM_SETTING_IP_CONFIG_NEVER_DEFAULT, FALSE, NULL);
  } else {
    g_object_set(ip4, NM_SETTING_IP_CONFIG_GATEWAY, NULL, NULL);
  }
  if (NmUpdateConnection(conn) != 0) {
    DLOGE("Failed to update connection");
    ELOGE(kElog_NmSetIpv4Gateway);
    return -1;
  }
  if (NmActivateConnection(client, conn) != 0) {
    DLOGE("Failed to activate connection");
    ELOGE(kElog_NmSetIpv4Gateway);
    return -1;
  }
  return 0;
}

// -----------------------------------------------------------------------------
static int NmSetIpv4dnsaddr(NMClient *client, const char *if_name,
                            const struct in_addr *addr) {
  NMConnection *conn = NmGetConnection(client, if_name);
  if (!conn) {
    DLOGE("Failed to retrieve connection");
    ELOGE(kElog_NmSetIpv4dnsaddr);
    return -1;
  }
  NMSettingIPConfig *ip4 = nm_connection_get_setting_ip4_config(conn);
  if (!ip4) {
    DLOGE("Failed to get IPv4 setting");
    ELOGE(kElog_NmSetIpv4dnsaddr);
    return -1;
  }
  if (addr && addr->s_addr) {
    char addr_str[INET_ADDRSTRLEN] = {0};
    if (!inet_ntop(AF_INET, addr, addr_str, sizeof(addr_str))) {
      DLOGE("inet_ntop failed for DNS address");
      ELOGE(kElog_NmSetIpv4dnsaddr);
      return -1;
    }
    const gchar *dns_list[] = {addr_str, NULL};
    g_object_set(ip4, NM_SETTING_IP_CONFIG_DNS, dns_list, NULL);
  } else {
    g_object_set(ip4, NM_SETTING_IP_CONFIG_DNS, NULL, NULL);
  }
  if (NmUpdateConnection(conn) != 0) {
    DLOGE("Failed to update connection");
    ELOGE(kElog_NmSetIpv4dnsaddr);
    return -1;
  }
  if (NmActivateConnection(client, conn) != 0) {
    DLOGE("Failed to activate connection");
    ELOGE(kElog_NmSetIpv4dnsaddr);
    return -1;
  }
  return 0;
}

// -----------------------------------------------------------------------------
static int NmSetIpv4method(NMClient *client, const char *if_name,
                           const char *method) {
  NMConnection *conn = NmGetConnection(client, if_name);
  if (!conn) {
    DLOGE("Failed to retrieve connection");
    ELOGE(kElog_NmSetIpv4method);
    return -1;
  }
  NMSettingIPConfig *ip4 = nm_connection_get_setting_ip4_config(conn);
  if (!ip4) {
    DLOGE("No IPv4 configuration found");
    ELOGE(kElog_NmSetIpv4method);
    return -1;
  }
  g_object_set(G_OBJECT(ip4), NM_SETTING_IP_CONFIG_METHOD, method, NULL);
  if (NmUpdateConnection(conn) != 0) {
    DLOGE("Failed to update connection");
    ELOGE(kElog_NmSetIpv4method);
    return -1;
  }
  if (NmActivateConnection(client, conn) != 0) {
    DLOGE("Failed to activate connection");
    ELOGE(kElog_NmSetIpv4method);
    return -1;
  }
  return 0;
}

// -----------------------------------------------------------------------------
static int NmGetIpv4addr(NMClient *client, const char *if_name,
                         struct in_addr *buf) {
  if (!buf) {
    DLOGE("buf is null");
    ELOGE(kElog_NmGetIpv4addr);
    return -1;
  }
  NMIPConfig *ip4 = NmGetIpv4Config(client, if_name);
  if (!ip4) {
    DLOGE("No IPv4 configuration found");
    ELOGE(kElog_NmGetIpv4addr);
    return -1;
  }
  const GPtrArray *addresses = nm_ip_config_get_addresses(ip4);
  if (!addresses || addresses->len == 0) {
    DLOGE("No IPv4 addresses found for interface '%s'", if_name);
    ELOGE(kElog_NmGetIpv4addr);
    return -1;
  }
  NMIPAddress *ip_addr = g_ptr_array_index(addresses, 0);
  const gchar *ip_str = nm_ip_address_get_address(ip_addr);
  if (inet_pton(AF_INET, ip_str, buf) != 1) {
    DLOGE("inet_pton failed for IP string: %s", ip_str);
    ELOGE(kElog_NmGetIpv4addr);
    return -1;
  }
  return 0;
}

// -----------------------------------------------------------------------------
static int NmGetIpv4netmask(NMClient *client, const char *if_name,
                            struct in_addr *buf) {
  if (!buf) {
    DLOGE("buf is null");
    ELOGE(kElog_NmGetIpv4netmask);
    return -1;
  }
  NMIPConfig *ip4 = NmGetIpv4Config(client, if_name);
  if (!ip4) {
    DLOGE("No IPv4 configuration found");
    ELOGE(kElog_NmGetIpv4netmask);
    return -1;
  }
  const GPtrArray *addresses = nm_ip_config_get_addresses(ip4);
  if (!addresses || addresses->len == 0) {
    DLOGE("No IPv4 addresses found for interface '%s'", if_name);
    ELOGE(kElog_NmGetIpv4netmask);
    return -1;
  }
  NMIPAddress *ip_addr = g_ptr_array_index(addresses, 0);
  gint prefix = nm_ip_address_get_prefix(ip_addr);
  buf->s_addr = PrefixToNetmask(prefix);
  return 0;
}

// -----------------------------------------------------------------------------
static int NmGetIpv4Gateway(NMClient *client, const char *if_name,
                            struct in_addr *buf) {
  if (!buf) {
    DLOGE("Output buffer is NULL");
    ELOGE(kElog_NmGetIpv4Gateway);
    return -1;
  }
  NMIPConfig *ip4 = NmGetIpv4Config(client, if_name);
  if (!ip4) {
    DLOGE("No IPv4 configuration found for interface '%s'", if_name);
    ELOGE(kElog_NmGetIpv4Gateway);
    return -1;
  }
  const char *gateway = nm_ip_config_get_gateway(ip4);
  if (!gateway) {
    DLOGE("No IPv4 gateway found for interface '%s'", if_name);
    ELOGE(kElog_NmGetIpv4Gateway);
    return -1;
  }
  if (inet_pton(AF_INET, gateway, buf) != 1) {
    DLOGE("inet_pton failed for gateway address: %s", gateway);
    ELOGE(kElog_NmGetIpv4Gateway);
    return -1;
  }
  return 0;
}

// -----------------------------------------------------------------------------
static int NmGetIpv4dnsaddr(NMClient *client, const char *if_name,
                            struct in_addr *buf) {
  if (!buf) {
    DLOGE("Output buffer is NULL");
    ELOGE(kElog_NmGetIpv4dnsaddr);
    return -1;
  }
  NMIPConfig *ip4 = NmGetIpv4Config(client, if_name);
  if (!ip4) {
    DLOGE("No IPv4 configuration found for interface '%s'", if_name);
    ELOGE(kElog_NmGetIpv4dnsaddr);
    return -1;
  }
  const char *const *dns_servers = nm_ip_config_get_nameservers(ip4);
  if (!dns_servers || !dns_servers[0]) {
    DLOGE("No DNS servers found for interface '%s'", if_name);
    ELOGE(kElog_NmGetIpv4dnsaddr);
    return -1;
  }
  if (inet_pton(AF_INET, dns_servers[0], buf) != 1) {
    DLOGE("inet_pton failed for DNS address: %s", dns_servers[0]);
    ELOGE(kElog_NmGetIpv4dnsaddr);
    return -1;
  }
  return 0;
}

// Public functions ------------------------------------------------------------
PlNetworkUtilIrqstate PlNetworkUtilNetworkLockOsImpl(void) {
  int ret = pthread_mutex_lock(&s_mutex);
  if (ret != 0) {
    DLOGE("%s:%d", __func__, errno);
    ELOGE(kElog_PlErrLock);
  }
  return 0;
}

// -----------------------------------------------------------------------------
void PlNetworkUtilNetworkUnlockOsImpl(PlNetworkUtilIrqstate flags) {
  (void)flags;
  int ret = pthread_mutex_unlock(&s_mutex);
  if (ret != 0) {
    DLOGE("%s:%d", __func__, errno);
    ELOGE(kElog_PlErrLock);
  }
  return;
}

// -----------------------------------------------------------------------------
void PlNetworkUtilSchedLockOsImpl(void) {
  // Do nothing
  return;
}

// -----------------------------------------------------------------------------
void PlNetworkUtilSchedUnlockOsImpl(void) {
  // Do nothing
  return;
}

// -----------------------------------------------------------------------------
PlErrCode PlNetworkUtilGetNetStatOsImpl(char *buf, const uint32_t buf_size) {
  // Do nothing
  (void)buf;
  (void)buf_size;
  return kPlErrCodeOk;
}

// -----------------------------------------------------------------------------
PlNetworkCapabilities PlNetworkGetCapabilitiesOsImpl(void) {
  PlNetworkCapabilities ret = {0};
  ret.use_external_dhcpc = 1;
  return ret;
}

// -----------------------------------------------------------------------------
PlErrCode PlNetworkSetIfStatusOsImpl(const char *ifname, const bool is_ifup) {
  NMClient *client = nm_client_new(NULL, NULL);
  if (client == NULL) {
    DLOGE("Failed to create NMClient");
    ELOGE(kElog_PlNetworkSetIfStatusOsImpl);
    return kPlErrInternal;
  }
  NMDevice *device = nm_client_get_device_by_iface(client, ifname);
  if (device == NULL) {
    DLOGE("Device %s not found", ifname);
    ELOGE(kElog_PlNetworkSetIfStatusOsImpl);
    g_object_unref(client);
    return kPlErrInternal;
  }
  NMActiveConnection *active_conn = nm_device_get_active_connection(device);
  if (is_ifup == true) {
    if (active_conn == NULL) {
      DLOGD("Activating device %s", ifname);
      nm_client_activate_connection_async(client, NULL, device, NULL, NULL,
                                          NULL, NULL);
    } else {
      DLOGD("Device %s is already active", ifname);
    }
  }
  if (is_ifup == false) {
    if (active_conn == NULL) {
      DLOGD("Device %s is already inactive", ifname);
    } else {
      DLOGD("Deactivating device %s", ifname);
#ifndef CONFIG_PL_NETWORK_UTIL_IMPL_IFDOWN_DISABLE
      nm_client_deactivate_connection_async(client, active_conn, NULL, NULL,
                                            NULL);
#endif
    }
  }
  g_object_unref(client);
#ifdef CONFIG_PL_NETWORK_UTIL_IMPL_IFDOWN_DISABLE
  s_is_ifup = is_ifup;
#endif
  return kPlErrCodeOk;
}

// -----------------------------------------------------------------------------
#ifdef CONFIG_PL_NETWORK_UTIL_IMPL_IFDOWN_DISABLE
PlErrCode PlNetworkGetIfStatusOsImpl(const char *if_name, bool *is_if_up) {
  DLOGD("Device %s is %s (virtual state)", if_name, s_is_ifup ? "UP" : "DOWN");
  *is_if_up = s_is_ifup;
  return kPlErrCodeOk;
}
#endif  // CONFIG_PL_NETWORK_UTIL_IMPL_IFDOWN_DISABLE

#ifndef CONFIG_PL_NETWORK_UTIL_IMPL_IFDOWN_DISABLE
PlErrCode PlNetworkGetIfStatusOsImpl(const char *if_name, bool *is_if_up) {
  NMClient *client = nm_client_new(NULL, NULL);
  if (client == NULL) {
    DLOGE("Failed to create NMClient");
    ELOGE(kElog_PlNetworkGetIfStatusOsImpl);
    return kPlErrInternal;
  }
  NMDevice *device = nm_client_get_device_by_iface(client, if_name);
  if (device == NULL) {
    DLOGE("Device %s not found", if_name);
    ELOGE(kElog_PlNetworkGetIfStatusOsImpl);
    g_object_unref(client);
    return kPlErrInternal;
  }
  NMDeviceState state = nm_device_get_state(device);
  DLOGD("Device %s is in state %d", if_name, state);
  switch (state) {
    case NM_DEVICE_STATE_IP_CONFIG:
    case NM_DEVICE_STATE_IP_CHECK:
    case NM_DEVICE_STATE_ACTIVATED:
      DLOGD("Device %s is UP (ifup)", if_name);
      *is_if_up = true;
      break;
    default:
      DLOGD("Device %s is DOWN (ifdown)", if_name);
      *is_if_up = false;
      break;
  }
  g_object_unref(client);
  return kPlErrCodeOk;
}
#endif  // CONFIG_PL_NETWORK_UTIL_IMPL_IFDOWN_DISABLE

// -----------------------------------------------------------------------------
PlErrCode PlNetworkGetLinkStatusOsImpl(const char *if_name, bool *is_link_up,
                                       bool *is_phy_id_valid) {
  PlErrCode err_code = kPlErrCodeOk;
  int ret = 0;
  int sockfd = (-1);
  struct ifreq req = {0};
  struct mii_ioctl_data *mii = NULL;

  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    err_code = kPlErrOpen;
    DLOGE("socket() failed. ret=%d, errno=%d", ret, errno);
    ELOGE(kElog_socket);
    ELOGE(kElog_PlNetworkGetLinkStatusOsImpl);
    goto err_socket;
  }

  strncpy(req.ifr_name, if_name, IFNAMSIZ - 1);
  req.ifr_name[IFNAMSIZ - 1] = '\0';
  mii = (struct mii_ioctl_data *)&req.ifr_data;

  ret = ioctl(sockfd, SIOCGMIIPHY, (unsigned long)&req);  // NOLINT
  if (ret == -1) {
    err_code = kPlErrInvalidOperation;
    DLOGE("ioctl(SIOCGMIIPHY) failed. ret=%d, errno=%d", ret, errno);
    ELOGE(kElog_SIOCGMIIPHY);
    ELOGE(kElog_PlNetworkGetLinkStatusOsImpl);
    goto err_ioctl;
  }

  mii->reg_num = MII_BMSR;
  ret = ioctl(sockfd, SIOCGMIIREG, (unsigned long)&req);  // NOLINT
  if (ret == -1) {
    err_code = kPlErrInvalidOperation;
    DLOGE("ioctl(SIOCGMIIREG) failed. ret=%d, errno=%d", ret, errno);
    ELOGE(kElog_MII_BMSR);
    ELOGE(kElog_PlNetworkGetLinkStatusOsImpl);
    goto err_ioctl;
  }
  uint16_t val_bmsr = mii->val_out;
  DLOGD("register(MII_BMSR) : 0x%x", val_bmsr);

  mii->reg_num = MII_PHYSID1;
  ret = ioctl(sockfd, SIOCGMIIREG, (unsigned long)&req);  // NOLINT
  if (ret == -1) {
    err_code = kPlErrInvalidOperation;
    DLOGE("ioctl(SIOCGMIIREG) failed. ret=%d, errno=%d", ret, errno);
    ELOGE(kElog_MII_PHYSID1);
    ELOGE(kElog_PlNetworkGetLinkStatusOsImpl);
    goto err_ioctl;
  }
  uint16_t val_physid1 = mii->val_out;
  DLOGD("register(MII_PHYSID1) : 0x%x", val_physid1);

  mii->reg_num = MII_PHYSID2;
  ret = ioctl(sockfd, SIOCGMIIREG, (unsigned long)&req);  // NOLINT
  if (ret == -1) {
    err_code = kPlErrInvalidOperation;
    DLOGE("ioctl(SIOCGMIIREG) failed. ret=%d, errno=%d", ret, errno);
    ELOGE(kElog_MII_PHYSID2);
    ELOGE(kElog_PlNetworkGetLinkStatusOsImpl);
    goto err_ioctl;
  }
  uint16_t val_physid2 = mii->val_out;
  DLOGD("register(MII_PHYSID2) : 0x%x", val_physid2);

  if (val_bmsr & BMSR_LSTATUS) {
    *is_link_up = true;
  } else {
    *is_link_up = false;
  }

  // check Phy ID
  // phyid1 == 0000000000000111
  // phyid2 == 110000XXXXXXXXXX
  if (val_physid1 == PHY_ID1_DEFAULT && (val_physid2 == PHY_ID2_DEFAULT)) {
    *is_phy_id_valid = true;
  } else {
    *is_phy_id_valid = false;
  }
  *is_phy_id_valid = true;

err_ioctl:
  ret = close(sockfd);
  if (ret == -1) {
    err_code = kPlErrClose;
    DLOGE("close() failed. ret=%d, errno=%d", ret, errno);
    ELOGE(kElog_close);
    ELOGE(kElog_PlNetworkGetLinkStatusOsImpl);
  }

err_socket:
  return err_code;
}

// -----------------------------------------------------------------------------
int PlNetworkSetIpv4AddrOsImpl(const char *if_name, const struct in_addr *addr,
                               const struct in_addr *netmask) {
  NMClient *client = nm_client_new(NULL, NULL);
  if (!client) {
    DLOGE("Failed to initialize NMClient");
    ELOGE(kElog_PlNetworkSetIpv4AddrOsImpl);
    return -1;
  }
  int ret = NmSetIpv4addr(client, if_name, addr, netmask);
  g_object_unref(client);
  return ret;
}

// -----------------------------------------------------------------------------
int PlNetworkSetIpv4GatewayOsImpl(const char *if_name,
                                  const struct in_addr *addr) {
  NMClient *client = nm_client_new(NULL, NULL);
  if (!client) {
    DLOGE("Failed to initialize NMClient");
    ELOGE(kElog_PlNetworkSetIpv4GatewayOsImpl);
    return -1;
  }
  int ret = NmSetIpv4Gateway(client, if_name, addr);
  g_object_unref(client);
  return ret;
}

// -----------------------------------------------------------------------------
int PlNetworkSetIpv4DnsOsImpl(const char *if_name, const struct in_addr *addr) {
  NMClient *client = nm_client_new(NULL, NULL);
  if (!client) {
    DLOGE("Failed to initialize NMClient");
    ELOGE(kElog_PlNetworkSetIpv4DnsOsImpl);
    return -1;
  }
  int ret = NmSetIpv4dnsaddr(client, if_name, addr);
  g_object_unref(client);
  return ret;
}

// -----------------------------------------------------------------------------
int PlNetworkSetIpv4MethodOsImpl(const char *if_name, bool is_dhcp) {
  NMClient *client = nm_client_new(NULL, NULL);
  if (!client) {
    DLOGE("Failed to initialize NMClient");
    ELOGE(kElog_PlNetworkSetIpv4MethodOsImpl);
    return -1;
  }
  const char *method_str = NM_SETTING_IP4_CONFIG_METHOD_MANUAL;
  if (is_dhcp) {
    method_str = NM_SETTING_IP4_CONFIG_METHOD_AUTO;
    if (NmSetIpv4Gateway(client, if_name, NULL) != 0) {
      DLOGW("Failed to clear gateway for DHCP mode");
      ELOGW(kElog_PlNetworkSetIpv4MethodOsImpl);
    }
    if (NmSetIpv4dnsaddr(client, if_name, NULL) != 0) {
      DLOGW("Failed to clear DNS for DHCP mode");
      ELOGW(kElog_PlNetworkSetIpv4MethodOsImpl);
    }
  }
  int ret = NmSetIpv4method(client, if_name, method_str);
  g_object_unref(client);
  return ret;
}

// -----------------------------------------------------------------------------
int PlNetworkGetIpv4AddrOsImpl(const char *if_name, struct in_addr *ip,
                               struct in_addr *netmask) {
#ifdef CONFIG_PL_NETWORK_UTIL_IMPL_IFDOWN_DISABLE
  bool is_ifup = true;
  PlNetworkGetIfStatusOsImpl(if_name, &is_ifup);
  if (!is_ifup) {
    DLOGD("Device %s is DOWN (virtual state)", if_name);
    return -1;
  }
#endif
  NMClient *client = nm_client_new(NULL, NULL);
  if (!client) {
    DLOGE("Failed to initialize NMClient");
    ELOGE(kElog_PlNetworkGetIpv4AddrOsImpl);
    return -1;
  }
  int ret = 0;
  if (NmGetIpv4addr(client, if_name, ip) < 0) {
    DLOGE("Failed to get IPv4 address for interface '%s'", if_name);
    ELOGE(kElog_PlNetworkGetIpv4AddrOsImpl);
    ret = -1;
  }
  if (NmGetIpv4netmask(client, if_name, netmask) < 0) {
    DLOGE("Failed to get IPv4 netmask for interface '%s'", if_name);
    ELOGE(kElog_PlNetworkGetIpv4AddrOsImpl);
    ret = -1;
  }
  g_object_unref(client);
  return ret;
}

// -----------------------------------------------------------------------------
int PlNetworkGetIpv4GatewayOsImpl(const char *if_name, struct in_addr *addr) {
  NMClient *client = nm_client_new(NULL, NULL);
  if (!client) {
    DLOGE("Failed to initialize NMClient");
    ELOGE(kElog_PlNetworkGetIpv4GatewayOsImpl);
    return -1;
  }
  int ret = NmGetIpv4Gateway(client, if_name, addr);
  g_object_unref(client);
  return ret;
}

// -----------------------------------------------------------------------------
int PlNetworkGetIpv4DnsOsImpl(const char *if_name, struct in_addr *addr) {
  NMClient *client = nm_client_new(NULL, NULL);
  if (!client) {
    DLOGE("Failed to initialize NMClient");
    ELOGE(kElog_PlNetworkGetIpv4DnsOsImpl);
    return -1;
  }
  int ret = NmGetIpv4dnsaddr(client, if_name, addr);
  g_object_unref(client);
  return ret;
}

// -----------------------------------------------------------------------------
int PlNetworkGetMacAddrOsImpl(const char *if_name, uint8_t *macaddr) {
  // Do nothing
  if (!if_name || !macaddr) {
    return -1;
  }
  return 0;
}

// -----------------------------------------------------------------------------
void *PlNetworkDhcpcOpenOsImpl(const char *if_name, void *macaddr, int maclen) {
  // Do nothing
  if (!if_name || !macaddr) {
    return NULL;
  }
  (void)maclen;
  return (void *)UINTPTR_MAX;
}

// -----------------------------------------------------------------------------
int PlNetworkDhcpcRequestOsImpl(void *handle,
                                struct PlNetworkDhcpcState *state) {
  // Do nothing
  if (!handle || !state) {
    return -1;
  }
  state->serverid.s_addr = 0;
  state->ipaddr.s_addr = 0;
  state->netmask.s_addr = 0;
  state->dnsaddr.s_addr = 0;
  state->default_router.s_addr = 0;
  state->lease_time = UINT32_MAX;
  state->renewal_time = UINT32_MAX;
  state->rebinding_time = UINT32_MAX;
  return 0;
}

// -----------------------------------------------------------------------------
int PlNetworkDhcpcRenewOsImpl(void *handle, struct PlNetworkDhcpcState *state) {
  // Do nothing
  if (!handle || !state) {
    return -1;
  }
  return 0;
}

// -----------------------------------------------------------------------------
int PlNetworkDhcpcReleaseOsImpl(void *handle,
                                struct PlNetworkDhcpcState *state) {
  // Do nothing
  if (!handle || !state) {
    return -1;
  }
  return 0;
}

// -----------------------------------------------------------------------------
void PlNetworkDhcpcCloseOsImpl(void *handle) {
  // Do nothing
  (void)handle;
  return;
}

// -----------------------------------------------------------------------------
int PlNetworkResetDnsServerOsImpl(void) {
  // Do nothing
  return 0;
}
