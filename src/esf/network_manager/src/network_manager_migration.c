/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network_manager/network_manager_migration.h"

#include <stdlib.h>
#include <string.h>

#include "network_manager.h"
#include "network_manager/network_manager_log.h"
#include "porting_layer/include/pl.h"
#include "porting_layer/include/pl_network.h"

// -----------------------------------------------------------------------------
// NETWORK_MANAGER_DISABLE
// -----------------------------------------------------------------------------
#ifdef CONFIG_EXTERNAL_NETWORK_MANAGER_DISABLE
EsfNetworkManagerResult EsfNetworkManagerExecMigration(void) {
  return kEsfNetworkManagerResultSuccess;
}
#endif  // CONFIG_EXTERNAL_NETWORK_MANAGER_DISABLE

// -----------------------------------------------------------------------------
// NETWORK_MANAGER_ENABLE
// -----------------------------------------------------------------------------
#ifndef CONFIG_EXTERNAL_NETWORK_MANAGER_DISABLE

// Private functions -----------------------------------------------------------
static bool IsNullStringIPAddress(EsfNetworkManagerParameter* esfnm_param) {
  return (esfnm_param->normal_mode.dev_ip.ip[0] == '\0');
}

// -----------------------------------------------------------------------------
static bool IsNullStringProxyURL(EsfNetworkManagerParameter* esfnm_param) {
  return (esfnm_param->proxy.url[0] == '\0');
}

// -----------------------------------------------------------------------------
static void GetIPAddress(PlNetworkMigrationHandle pl_hdl,
                         EsfNetworkManagerParameter* esfnm_param,
                         EsfNetworkManagerParameterMask* esfnm_mask) {
  PlNetworkMigrationDataId id = kPlNetworkMigrationDataIdIPAddress;
  void* dst = &esfnm_param->normal_mode.dev_ip.ip;
  size_t size = sizeof(esfnm_param->normal_mode.dev_ip.ip);
  PlErrCode pl_ret = PlNetworkGetMigrationData(pl_hdl, id, dst, size);
  ESF_NETWORK_MANAGER_INFO("%s:%d:%s", __func__, pl_ret, (char*)dst);
  if (pl_ret == kPlErrCodeOk) {
    esfnm_mask->normal_mode.dev_ip.ip = 1;
  }
  return;
}

// -----------------------------------------------------------------------------
static void GetSubnetMask(PlNetworkMigrationHandle pl_hdl,
                          EsfNetworkManagerParameter* esfnm_param,
                          EsfNetworkManagerParameterMask* esfnm_mask) {
  PlNetworkMigrationDataId id = kPlNetworkMigrationDataIdSubnetMask;
  void* dst = &esfnm_param->normal_mode.dev_ip.subnet_mask;
  size_t size = sizeof(esfnm_param->normal_mode.dev_ip.subnet_mask);
  PlErrCode pl_ret = PlNetworkGetMigrationData(pl_hdl, id, dst, size);
  ESF_NETWORK_MANAGER_INFO("%s:%d:%s", __func__, pl_ret, (char*)dst);
  if (pl_ret == kPlErrCodeOk) {
    esfnm_mask->normal_mode.dev_ip.subnet_mask = 1;
  }
  return;
}

// -----------------------------------------------------------------------------
static void GetGateway(PlNetworkMigrationHandle pl_hdl,
                       EsfNetworkManagerParameter* esfnm_param,
                       EsfNetworkManagerParameterMask* esfnm_mask) {
  PlNetworkMigrationDataId id = kPlNetworkMigrationDataIdGateway;
  void* dst = &esfnm_param->normal_mode.dev_ip.gateway;
  size_t size = sizeof(esfnm_param->normal_mode.dev_ip.gateway);
  PlErrCode pl_ret = PlNetworkGetMigrationData(pl_hdl, id, dst, size);
  ESF_NETWORK_MANAGER_INFO("%s:%d:%s", __func__, pl_ret, (char*)dst);
  if (pl_ret == kPlErrCodeOk) {
    esfnm_mask->normal_mode.dev_ip.gateway = 1;
  }
  return;
}

// -----------------------------------------------------------------------------
static void GetDNS(PlNetworkMigrationHandle pl_hdl,
                   EsfNetworkManagerParameter* esfnm_param,
                   EsfNetworkManagerParameterMask* esfnm_mask) {
  PlNetworkMigrationDataId id = kPlNetworkMigrationDataIdDNS;
  void* dst = &esfnm_param->normal_mode.dev_ip.dns;
  size_t size = sizeof(esfnm_param->normal_mode.dev_ip.dns);
  PlErrCode pl_ret = PlNetworkGetMigrationData(pl_hdl, id, dst, size);
  ESF_NETWORK_MANAGER_INFO("%s:%d:%s", __func__, pl_ret, (char*)dst);
  if (pl_ret == kPlErrCodeOk) {
    esfnm_mask->normal_mode.dev_ip.dns = 1;
  }
  return;
}

// -----------------------------------------------------------------------------
static void GetIPMethod(PlNetworkMigrationHandle pl_hdl,
                        EsfNetworkManagerParameter* esfnm_param,
                        EsfNetworkManagerParameterMask* esfnm_mask) {
  PlNetworkMigrationDataId id = kPlNetworkMigrationDataIdIPMethod;
  void* dst = &esfnm_param->normal_mode.ip_method;
  size_t size = sizeof(esfnm_param->normal_mode.ip_method);
  PlErrCode pl_ret = PlNetworkGetMigrationData(pl_hdl, id, dst, size);
  ESF_NETWORK_MANAGER_INFO("%s:%d:%d", __func__, pl_ret, *(int32_t*)dst);
  if (pl_ret == kPlErrCodeOk) {
    esfnm_mask->normal_mode.ip_method = 1;
  }
  return;
}

// -----------------------------------------------------------------------------
static void GetNetifKind(PlNetworkMigrationHandle pl_hdl,
                         EsfNetworkManagerParameter* esfnm_param,
                         EsfNetworkManagerParameterMask* esfnm_mask) {
  PlNetworkMigrationDataId id = kPlNetworkMigrationDataIdNetifKind;
  void* dst = &esfnm_param->normal_mode.netif_kind;
  size_t size = sizeof(esfnm_param->normal_mode.netif_kind);
  PlErrCode pl_ret = PlNetworkGetMigrationData(pl_hdl, id, dst, size);
  ESF_NETWORK_MANAGER_INFO("%s:%d:%d", __func__, pl_ret, *(int32_t*)dst);
  if (pl_ret == kPlErrCodeOk) {
    esfnm_mask->normal_mode.netif_kind = 1;
  }
  return;
}

// -----------------------------------------------------------------------------
static void GetProxyURL(PlNetworkMigrationHandle pl_hdl,
                        EsfNetworkManagerParameter* esfnm_param,
                        EsfNetworkManagerParameterMask* esfnm_mask) {
  PlNetworkMigrationDataId id = kPlNetworkMigrationDataIdProxyURL;
  void* dst = &esfnm_param->proxy.url;
  size_t size = sizeof(esfnm_param->proxy.url);
  PlErrCode pl_ret = PlNetworkGetMigrationData(pl_hdl, id, dst, size);
  ESF_NETWORK_MANAGER_INFO("%s:%d:%s", __func__, pl_ret, (char*)dst);
  if (pl_ret == kPlErrCodeOk) {
    esfnm_mask->proxy.url = 1;
  }
  return;
}

// -----------------------------------------------------------------------------
static void GetProxyPort(PlNetworkMigrationHandle pl_hdl,
                         EsfNetworkManagerParameter* esfnm_param,
                         EsfNetworkManagerParameterMask* esfnm_mask) {
  PlNetworkMigrationDataId id = kPlNetworkMigrationDataIdProxyPort;
  void* dst = &esfnm_param->proxy.port;
  size_t size = sizeof(esfnm_param->proxy.port);
  PlErrCode pl_ret = PlNetworkGetMigrationData(pl_hdl, id, dst, size);
  ESF_NETWORK_MANAGER_INFO("%s:%d:%d", __func__, pl_ret, *(int32_t*)dst);
  if (pl_ret == kPlErrCodeOk) {
    esfnm_mask->proxy.port = 1;
  }
  return;
}

// -----------------------------------------------------------------------------
static void GetProxyUserName(PlNetworkMigrationHandle pl_hdl,
                             EsfNetworkManagerParameter* esfnm_param,
                             EsfNetworkManagerParameterMask* esfnm_mask) {
  PlNetworkMigrationDataId id = kPlNetworkMigrationDataIdProxyUserName;
  void* dst = &esfnm_param->proxy.username;
  size_t size = sizeof(esfnm_param->proxy.username);
  PlErrCode pl_ret = PlNetworkGetMigrationData(pl_hdl, id, dst, size);
  ESF_NETWORK_MANAGER_INFO("%s:%d:%s", __func__, pl_ret, (char*)dst);
  if (pl_ret == kPlErrCodeOk) {
    esfnm_mask->proxy.username = 1;
  }
  return;
}

// -----------------------------------------------------------------------------
static void GetProxyPassword(PlNetworkMigrationHandle pl_hdl,
                             EsfNetworkManagerParameter* esfnm_param,
                             EsfNetworkManagerParameterMask* esfnm_mask) {
  PlNetworkMigrationDataId id = kPlNetworkMigrationDataIdProxyPassword;
  void* dst = &esfnm_param->proxy.password;
  size_t size = sizeof(esfnm_param->proxy.password);
  PlErrCode pl_ret = PlNetworkGetMigrationData(pl_hdl, id, dst, size);
  ESF_NETWORK_MANAGER_INFO("%s:%d:%s", __func__, pl_ret, (char*)dst);
  if (pl_ret == kPlErrCodeOk) {
    esfnm_mask->proxy.password = 1;
  }
  return;
}

// -----------------------------------------------------------------------------
static EsfNetworkManagerResult ExecMigrationCore(
    PlNetworkMigrationHandle pl_hdl, EsfNetworkManagerParameter* esfnm_param,
    EsfNetworkManagerParameterMask* esfnm_mask) {
  EsfNetworkManagerResult ret = kEsfNetworkManagerResultSuccess;
  // Must-write item
  ESF_NETWORK_MANAGER_MASK_DISABLE_ALL(esfnm_mask);
  GetNetifKind(pl_hdl, esfnm_param, esfnm_mask);
  ret = EsfNetworkManagerSaveParameter(esfnm_mask, esfnm_param);
  if (ret != kEsfNetworkManagerResultSuccess) {
    ESF_NETWORK_MANAGER_ERR("%s:%d", __func__, ret);
    ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_FLASH_FAILURE);
    return ret;
  }
  // Optional item
  ESF_NETWORK_MANAGER_MASK_DISABLE_ALL(esfnm_mask);
  GetIPAddress(pl_hdl, esfnm_param, esfnm_mask);
  if (!IsNullStringIPAddress(esfnm_param)) {
    GetSubnetMask(pl_hdl, esfnm_param, esfnm_mask);
    GetGateway(pl_hdl, esfnm_param, esfnm_mask);
    GetDNS(pl_hdl, esfnm_param, esfnm_mask);
    GetIPMethod(pl_hdl, esfnm_param, esfnm_mask);
  }
  GetProxyURL(pl_hdl, esfnm_param, esfnm_mask);
  if (!IsNullStringProxyURL(esfnm_param)) {
    GetProxyPort(pl_hdl, esfnm_param, esfnm_mask);
    GetProxyUserName(pl_hdl, esfnm_param, esfnm_mask);
    GetProxyPassword(pl_hdl, esfnm_param, esfnm_mask);
  }
  ret = EsfNetworkManagerSaveParameter(esfnm_mask, esfnm_param);
  if (ret != kEsfNetworkManagerResultSuccess) {
    ESF_NETWORK_MANAGER_WARN("%s:%d", __func__, ret);
    ESF_NETWORK_MANAGER_ELOG_WARN(ESF_NETWORK_MANAGER_ELOG_INTERNAL_ERROR_WARN);
    // Optional item is return success even fail
    ret = kEsfNetworkManagerResultSuccess;
  }
  return ret;
}

// ----------------------------------------------------------------------------
static EsfNetworkManagerResult IsNeedMigration(
    EsfNetworkManagerParameter* esfnm_param,
    EsfNetworkManagerParameterMask* esfnm_mask, bool* need_migration) {
  EsfNetworkManagerResult ret = kEsfNetworkManagerResultSuccess;

  ESF_NETWORK_MANAGER_MASK_ENABLE_ALL(esfnm_mask);
  ret = EsfNetworkManagerLoadParameter(esfnm_mask, esfnm_param);
  if (ret != kEsfNetworkManagerResultSuccess) {
    ESF_NETWORK_MANAGER_ERR("%s:%d", __func__, ret);
    ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_FLASH_FAILURE);
    return ret;
  }

  PlNetworkMigrationNeedParam param = {0};
  param.netif_kind = esfnm_param->normal_mode.netif_kind;
  PlErrCode pl_ret = PlNetworkIsNeedMigration(&param, need_migration);
  if (pl_ret != kPlErrCodeOk) {
    ESF_NETWORK_MANAGER_ERR("%s:%d", __func__, pl_ret);
    ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_PL_FAILURE);
    return kEsfNetworkManagerResultHWIFError;
  }
  return ret;
}

// -----------------------------------------------------------------------------
static EsfNetworkManagerResult ExecMigration(
    EsfNetworkManagerParameter* esfnm_param,
    EsfNetworkManagerParameterMask* esfnm_mask) {
  EsfNetworkManagerResult ret = kEsfNetworkManagerResultSuccess;

  bool need_migration = false;
  ret = IsNeedMigration(esfnm_param, esfnm_mask, &need_migration);
  if (ret != kEsfNetworkManagerResultSuccess) {
    ESF_NETWORK_MANAGER_ERR("%s:%d", __func__, ret);
    ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_INTERNAL_ERROR);
    return ret;
  }
  if (!need_migration) {
    ESF_NETWORK_MANAGER_INFO("%s:Done", __func__);
    return kEsfNetworkManagerResultSuccess;
  }

  PlErrCode pl_ret = kPlErrCodeOk;
  PlNetworkMigrationHandle pl_hdl = NULL;
  pl_ret = PlNetworkInitMigration(&pl_hdl);
  if (pl_ret != kPlErrCodeOk) {
    ESF_NETWORK_MANAGER_ERR("%s:%d", __func__, pl_ret);
    ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_PL_FAILURE);
    return kEsfNetworkManagerResultHWIFError;
  }
  ret = ExecMigrationCore(pl_hdl, esfnm_param, esfnm_mask);
  pl_ret = PlNetworkFinMigration(pl_hdl);
  if (pl_ret != kPlErrCodeOk) {
    ESF_NETWORK_MANAGER_WARN("%s:%d", __func__, pl_ret);
    ESF_NETWORK_MANAGER_ELOG_WARN(ESF_NETWORK_MANAGER_ELOG_INTERNAL_ERROR_WARN);
  }
  return ret;
}

// Public functions ------------------------------------------------------------
EsfNetworkManagerResult EsfNetworkManagerExecMigration(void) {
  EsfNetworkManagerParameterMask esfnm_mask = {0};
  EsfNetworkManagerParameter* esfnm_param =
      (EsfNetworkManagerParameter*)calloc(1, sizeof(*esfnm_param));
  if (esfnm_param == NULL) {
    ESF_NETWORK_MANAGER_ERR("%s:%p", __func__, esfnm_param);
    ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_INTERNAL_ERROR);
    return kEsfNetworkManagerResultInternalError;
  }
  EsfNetworkManagerResult ret = ExecMigration(esfnm_param, &esfnm_mask);
  free(esfnm_param);
  return ret;
}
#endif  // CONFIG_EXTERNAL_NETWORK_MANAGER_DISABLE
