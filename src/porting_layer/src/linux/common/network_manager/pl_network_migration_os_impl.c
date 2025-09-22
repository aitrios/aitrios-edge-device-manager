/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "pl_network_migration_os_impl.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pl.h"
#include "pl_network.h"
#include "pl_network_log.h"

// Macros ----------------------------------------------------------------------
#define LINE_SIZE_MAX (1024)
#define IPV4_MAX_PREFIX (32)
#define IPV4_ADDR_LEN (39 + 1)
#define PROXY_URL_LEN (256 + 1)
#define PROXY_USERNAME_LEN (32 + 1)
#define PROXY_PASSWORD_LEN (32 + 1)

#ifndef CONFIG_PL_NETWORK_IMPL_MIGRATION_DATA_PATH_WIRED_NETWORK
#define CONFIG_PL_NETWORK_IMPL_MIGRATION_DATA_PATH_WIRED_NETWORK \
  "/misc/etc/systemd/network/wired.network"
#endif

#ifndef CONFIG_PL_NETWORK_IMPL_MIGRATION_DATA_PATH_ENVIRONMENT
#define CONFIG_PL_NETWORK_IMPL_MIGRATION_DATA_PATH_ENVIRONMENT \
  "/misc/etc/transproxy/environment"
#endif

// Types (typedef / enum / struct / union) -------------------------------------
#define kIPAddress (kPlNetworkMigrationDataIdIPAddress)
#define kSubnetMask (kPlNetworkMigrationDataIdSubnetMask)
#define kGateway (kPlNetworkMigrationDataIdGateway)
#define kDNS (kPlNetworkMigrationDataIdDNS)
#define kIPMethod (kPlNetworkMigrationDataIdIPMethod)
#define kNetifKind (kPlNetworkMigrationDataIdNetifKind)
#define kProxyURL (kPlNetworkMigrationDataIdProxyURL)
#define kProxyPort (kPlNetworkMigrationDataIdProxyPort)
#define kProxyUserName (kPlNetworkMigrationDataIdProxyUserName)
#define kProxyPassword (kPlNetworkMigrationDataIdProxyPassword)
#define kDataIdMax (kPlNetworkMigrationDataIdMax)

typedef struct {
  int32_t ip_method;
  int32_t netif_kind;
  int32_t proxy_port;
  char ipv4_addr[IPV4_ADDR_LEN];
  char ipv4_mask[IPV4_ADDR_LEN];
  char ipv4_gateway[IPV4_ADDR_LEN];
  char ipv4_dns[IPV4_ADDR_LEN];
  char proxy_url[PROXY_URL_LEN];
  char proxy_username[PROXY_USERNAME_LEN];
  char proxy_password[PROXY_PASSWORD_LEN];
} CacheData;

typedef struct {
  bool parsed[kDataIdMax];
  void *cache[kDataIdMax];
  size_t size[kDataIdMax];
  CacheData data;
  char line_buf[LINE_SIZE_MAX];
} Handle;

typedef PlErrCode (*ParseTarget)(Handle *h, PlNetworkMigrationDataId id,
                                 const char *target_value);

typedef struct {
  PlNetworkMigrationDataId id;
  const char *pattern;
  ParseTarget parser;
} ParseTargetInfo;

// Private functions -----------------------------------------------------------
static char *FindValueSeparator(const char *line, char separator) {
  char *ret = strchr(line, separator);
  return ret;
}

// -----------------------------------------------------------------------------
static bool IsPatternMatch(const char *line, const char *pattern) {
  bool ret = strncmp(line, pattern, strlen(pattern)) == 0;
  return ret;
}

// -----------------------------------------------------------------------------
static void TruncateAtNewline(char *str) {
  char *rm_char = strpbrk(str, "\r\n");
  if (rm_char != NULL) {
    *rm_char = '\0';
  }
  return;
}

// -----------------------------------------------------------------------------
static bool IsTargetParsed(const Handle *h, PlNetworkMigrationDataId id) {
  bool ret = h->parsed[id];
  return ret;
}

// -----------------------------------------------------------------------------
static void TargetParsed(Handle *h, PlNetworkMigrationDataId id) {
  h->parsed[id] = true;
  return;
}

// -----------------------------------------------------------------------------
static PlErrCode ParseString(Handle *h, PlNetworkMigrationDataId id,
                             const char *target_value) {
  snprintf(h->cache[id], h->size[id], "%s", target_value);
  return kPlErrCodeOk;
}

// -----------------------------------------------------------------------------
static PlErrCode ParseStringToInt32(Handle *h, PlNetworkMigrationDataId id,
                                    const char *target_value) {
  char *endptr = NULL;
  int32_t value = (int32_t)strtol(target_value, &endptr, 10);
  if (*endptr != '\0') {
    DLOGE("%s:Contains non-numeric characters", __func__);
    ELOGE(kElog_ParseStringToInt32);
    return kPlErrInternal;
  }
  int32_t *cache = (int32_t *)h->cache[id];
  *cache = value;
  return kPlErrCodeOk;
}

// -----------------------------------------------------------------------------
static PlErrCode ParseIPAddress(Handle *h, PlNetworkMigrationDataId id,
                                const char *target_value) {
  const char *slash = FindValueSeparator(target_value, '/');
  if (slash == NULL) {
    DLOGE("%s:slash not found", __func__);
    ELOGE(kElog_ParseIPAddress);
    return kPlErrInternal;
  }
  size_t len = slash - target_value;
  snprintf(h->cache[id], h->size[id], "%.*s", (int)len, target_value);
  return kPlErrCodeOk;
}

// -----------------------------------------------------------------------------
static PlErrCode ParseSubnetMask(Handle *h, PlNetworkMigrationDataId id,
                                 const char *target_value) {
  const char *slash = FindValueSeparator(target_value, '/');
  if (slash == NULL) {
    DLOGE("%s:slash not found", __func__);
    ELOGE(kElog_ParseSubnetMask);
    return kPlErrInternal;
  }
  char *endptr;
  int32_t prefix = (int32_t)strtol(slash + 1, &endptr, 10);
  if (*endptr != '\0') {
    DLOGE("%s:Contains non-numeric characters", __func__);
    ELOGE(kElog_ParseSubnetMask);
    return kPlErrInternal;
  }
  if (prefix < 0 || prefix > IPV4_MAX_PREFIX) {
    DLOGE("%s:Invalid prefix length:%d", __func__, prefix);
    ELOGE(kElog_ParseSubnetMask);
    return kPlErrInternal;
  }
  uint32_t mask = 0;
  if (prefix) {
    mask = (UINT32_MAX << (IPV4_MAX_PREFIX - prefix));
  }
  snprintf(h->cache[id], h->size[id], "%u.%u.%u.%u", (mask >> 24) & 0xFF,
           (mask >> 16) & 0xFF, (mask >> 8) & 0xFF, mask & 0xFF);
  return kPlErrCodeOk;
}

// -----------------------------------------------------------------------------
static PlErrCode ParseIPMethod(Handle *h, PlNetworkMigrationDataId id,
                               const char *target_value) {
  int32_t *ip_method = h->cache[id];
  if (IsPatternMatch(target_value, "no")) {
    *ip_method = 1;  // manual
  } else {
    *ip_method = 0;  // auto
  }
  return kPlErrCodeOk;
}

// -----------------------------------------------------------------------------
static PlErrCode ParseTargetLine(Handle *h, const char *line,
                                 const ParseTargetInfo *target) {
  if (!IsPatternMatch(line, target->pattern)) {
    return kPlErrNotFound;
  }
  char *separator_position = FindValueSeparator(line, '=');
  TruncateAtNewline(separator_position);
  if (IsTargetParsed(h, target->id)) {
    return kPlErrAlready;
  }
  const char *value_position = separator_position + 1;
  PlErrCode ret = target->parser(h, target->id, value_position);
  if (ret == kPlErrCodeOk) {
    TargetParsed(h, target->id);
  }
  return ret;
}

// -----------------------------------------------------------------------------
static bool IsExpectedRet(PlErrCode e) {
  return (e == kPlErrCodeOk) || (e == kPlErrNotFound) || (e == kPlErrAlready);
}

// -----------------------------------------------------------------------------
static void ReadLine(Handle *h, const char *line, int current_line,
                     const ParseTargetInfo *targets, size_t target_num) {
  for (size_t i = 0; i < target_num; ++i) {
    PlErrCode ret = ParseTargetLine(h, line, &targets[i]);
    if (!IsExpectedRet(ret)) {
      DLOGW("%s:%d:%zu:%d", __func__, current_line, i, ret);
      ELOGW(kElog_ReadLine);
    }
  }
  return;
}

// -----------------------------------------------------------------------------
static void ReadFile(Handle *h, const char *path,
                     const ParseTargetInfo *targets, size_t target_num) {
  FILE *fp = fopen(path, "r");
  if (fp == NULL) {
    DLOGW("%s:%s", __func__, path);
    ELOGW(kElog_ReadFile);
    return;
  }
  int current_line = 0;
  while (fgets(h->line_buf, sizeof(h->line_buf), fp)) {
    ReadLine(h, h->line_buf, current_line, targets, target_num);
    current_line++;
  }
  int fret = fclose(fp);
  if (fret != 0) {
    DLOGW("%s:%d", __func__, fret);
    ELOGW(kElog_ReadFile);
    return;
  }
  return;
}

// -----------------------------------------------------------------------------
static void ReadFileWiredNetwork(Handle *h) {
  const char *kPath = CONFIG_PL_NETWORK_IMPL_MIGRATION_DATA_PATH_WIRED_NETWORK;
  DLOGI("%s:%s", __func__, kPath);
  const ParseTargetInfo targets[] = {
      {kIPAddress, "Address=", ParseIPAddress},
      {kSubnetMask, "Address=", ParseSubnetMask},
      {kGateway, "Gateway=", ParseString},
      {kDNS, "DNS=", ParseString},
      {kIPMethod, "DHCP=", ParseIPMethod},
  };
  size_t target_num = sizeof(targets) / sizeof(targets[0]);
  ReadFile(h, kPath, targets, target_num);
  return;
}

// -----------------------------------------------------------------------------
static void ReadFileEnvironment(Handle *h) {
  const char *kPath = CONFIG_PL_NETWORK_IMPL_MIGRATION_DATA_PATH_ENVIRONMENT;
  DLOGI("%s:%s", __func__, kPath);
  const ParseTargetInfo targets[] = {
      {kProxyURL, "NET_PROXY_ADDR=", ParseString},
      {kProxyPort, "NET_PROXY_PORT=", ParseStringToInt32},
      {kProxyUserName, "NET_PROXY_USER=", ParseString},
      {kProxyPassword, "NET_PROXY_PSWD=", ParseString},
  };
  size_t target_num = sizeof(targets) / sizeof(targets[0]);
  ReadFile(h, kPath, targets, target_num);
  return;
}

// -----------------------------------------------------------------------------
static void InitCache(Handle *h) {
  ReadFileWiredNetwork(h);
  ReadFileEnvironment(h);
  return;
}

// -----------------------------------------------------------------------------
static Handle *InitHandle(void) {
  Handle *h = calloc(1, sizeof(*h));
  if (h == NULL) {
    DLOGE("%s:%p", __func__, h);
    ELOGE(kElog_PlErrMemory);
    return NULL;
  }
  h->data.ipv4_addr[0] = '\0';
  h->data.ipv4_mask[0] = '\0';
  h->data.ipv4_gateway[0] = '\0';
  h->data.ipv4_dns[0] = '\0';
  h->data.ip_method = 0;   // auto
  h->data.netif_kind = 1;  // ether
  h->data.proxy_url[0] = '\0';
  h->data.proxy_port = 0;
  h->data.proxy_username[0] = '\0';
  h->data.proxy_password[0] = '\0';

  h->cache[kIPAddress] = h->data.ipv4_addr;
  h->cache[kSubnetMask] = h->data.ipv4_mask;
  h->cache[kGateway] = h->data.ipv4_gateway;
  h->cache[kDNS] = h->data.ipv4_dns;
  h->cache[kIPMethod] = &h->data.ip_method;
  h->cache[kNetifKind] = &h->data.netif_kind;
  h->cache[kProxyURL] = h->data.proxy_url;
  h->cache[kProxyPort] = &h->data.proxy_port;
  h->cache[kProxyUserName] = h->data.proxy_username;
  h->cache[kProxyPassword] = h->data.proxy_password;

  h->size[kIPAddress] = sizeof(h->data.ipv4_addr);
  h->size[kSubnetMask] = sizeof(h->data.ipv4_mask);
  h->size[kGateway] = sizeof(h->data.ipv4_gateway);
  h->size[kDNS] = sizeof(h->data.ipv4_dns);
  h->size[kIPMethod] = sizeof(h->data.ip_method);
  h->size[kNetifKind] = sizeof(h->data.netif_kind);
  h->size[kProxyURL] = sizeof(h->data.proxy_url);
  h->size[kProxyPort] = sizeof(h->data.proxy_port);
  h->size[kProxyUserName] = sizeof(h->data.proxy_username);
  h->size[kProxyPassword] = sizeof(h->data.proxy_password);
  return h;
}

// Public functions ------------------------------------------------------------
PlErrCode PlNetworkInitMigrationOsImpl(PlNetworkMigrationHandle *handle) {
  if (handle == NULL) {
    DLOGE("%s:%p", __func__, handle);
    ELOGE(kElog_PlNetworkInitMigrationOsImpl);
    return kPlErrInvalidParam;
  }
  Handle *h = InitHandle();
  if (h == NULL) {
    DLOGE("%s:%p", __func__, h);
    ELOGE(kElog_PlNetworkInitMigrationOsImpl);
    return kPlErrMemory;
  }
  InitCache(h);
  *handle = h;
  return kPlErrCodeOk;
}

// -----------------------------------------------------------------------------
PlErrCode PlNetworkFinMigrationOsImpl(PlNetworkMigrationHandle handle) {
  free(handle);
  return kPlErrCodeOk;
}

// -----------------------------------------------------------------------------
PlErrCode PlNetworkGetMigrationDataOsImpl(PlNetworkMigrationHandle handle,
                                          PlNetworkMigrationDataId id,
                                          void *dst, size_t dst_size) {
  if (handle == NULL || id >= kDataIdMax || dst == NULL || dst_size == 0) {
    DLOGE("%s:%p:%d:%p:%zu", __func__, handle, id, dst, dst_size);
    ELOGE(kElog_PlNetworkGetMigrationDataOsImpl);
    return kPlErrInvalidParam;
  }
  Handle *h = handle;
  memcpy(dst, h->cache[id], dst_size);
  return kPlErrCodeOk;
}

// -----------------------------------------------------------------------------
PlErrCode PlNetworkIsNeedMigrationOsImpl(PlNetworkMigrationNeedParam *param,
                                         bool *need_migration) {
  if (param->netif_kind == 1) {
    *need_migration = false;
  } else {
    *need_migration = true;
  }
  return kPlErrCodeOk;
}

// -----------------------------------------------------------------------------
void PlNetworkEraseMigrationSrcDataOsImpl(void) {
  remove(CONFIG_PL_NETWORK_IMPL_MIGRATION_DATA_PATH_ENVIRONMENT);
  return;
}
