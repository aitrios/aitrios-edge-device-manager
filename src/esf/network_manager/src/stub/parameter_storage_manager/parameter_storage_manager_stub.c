/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>

#include "network_manager.h"
#include "network_manager/network_manager_accessor_parameter_storage_manager.h"
#include "parameter_storage_manager.h"

static EsfNetworkManagerParameterInternal *parameter = NULL;

EsfParameterStorageManagerStatus EsfParameterStorageManagerInit(void) {
  printf("[STUB] EsfParameterStorageManagerInit START\n");
  if (parameter != NULL) {
    printf("[STUB] EsfParameterStorageManagerInit parameter=%p\n", parameter);
    return kEsfParameterStorageManagerStatusOk;
  }
  parameter = calloc(1, sizeof(EsfNetworkManagerParameterInternal));
  if (parameter == NULL) {
    printf("[STUB] EsfParameterStorageManagerInit calloc error\n");
    return kEsfParameterStorageManagerStatusResourceExhausted;
  }
  printf("[STUB] EsfParameterStorageManagerInit END\n");
  return kEsfParameterStorageManagerStatusOk;
}

EsfParameterStorageManagerStatus EsfParameterStorageManagerDeinit(void) {
  printf("[STUB] EsfParameterStorageManagerDeinit START\n");
  if (parameter != NULL) {
    printf("[STUB] EsfParameterStorageManagerDeinit free parameter %p.\n",
           parameter);
    free(parameter);
    parameter = NULL;
  }
  printf("[STUB] EsfParameterStorageManagerDeinit END\n");
  return kEsfParameterStorageManagerStatusOk;
}

EsfParameterStorageManagerStatus EsfParameterStorageManagerOpen(
    EsfParameterStorageManagerHandle *handle) {
  printf("[STUB] EsfParameterStorageManagerOpen START handle=%p\n", handle);
  *handle = 7;
  printf("[STUB] EsfParameterStorageManagerOpen END\n");
  return kEsfParameterStorageManagerStatusOk;
}

EsfParameterStorageManagerStatus EsfParameterStorageManagerClose(
    EsfParameterStorageManagerHandle handle) {
  printf("[STUB] EsfParameterStorageManagerClose START handle=%d\n", handle);
  printf("[STUB] EsfParameterStorageManagerClose END\n");
  return kEsfParameterStorageManagerStatusOk;
}

#if 0
static void EsfStubPrintStructInfo(
    const EsfParameterStorageManagerStructInfo *info) {
  printf("[STUB] EsfStubPrintStructInfo START info=%p\n", info);
  printf("[STUB]   items_num=%zu\n", info->items_num);
  printf("[STUB]   items=%p\n", info->items);

  for (int i = 0; i < info->items_num; ++i) {
    printf(
        "[STUB]   i=%d id=%d type=%d offset=%zu size=%zu enabled=%p "
        "custom=%p\n",
        i, info->items[i].id, info->items[i].type, info->items[i].offset,
        info->items[i].size, info->items[i].enabled, info->items[i].custom);
  }
  printf("[STUB] EsfStubPrintStructInfo END\n");
}
#endif

static void EsfStubPrintMask(const EsfParameterStorageManagerMask mask) {
  printf("[STUB] EsfStubPrintMask START mask=0x%x\n", mask);
  EsfNetworkManagerParameterMask *param =
      (EsfNetworkManagerParameterMask *)mask;

  printf("normal_mode.dev_ip.ip=%d\n", param->normal_mode.dev_ip.ip);
  printf("normal_mode.dev_ip.subnet_mask=%d\n",
         param->normal_mode.dev_ip.subnet_mask);
  printf("normal_mode.dev_ip.gateway=%d\n", param->normal_mode.dev_ip.gateway);
  printf("normal_mode.dev_ip.dns=%d\n", param->normal_mode.dev_ip.dns);
  printf("normal_mode.dev_ip_v6.ip=%d\n", param->normal_mode.dev_ip_v6.ip);
  printf("normal_mode.dev_ip_v6.subnet_mask=%d\n",
         param->normal_mode.dev_ip_v6.subnet_mask);
  printf("normal_mode.dev_ip_v6.gateway=%d\n",
         param->normal_mode.dev_ip_v6.gateway);
  printf("normal_mode.dev_ip_v6.dns=%d\n", param->normal_mode.dev_ip_v6.dns);
  printf("normal_mode.ip_method=%d\n", param->normal_mode.ip_method);
  printf("normal_mode.netif_kind=%d\n", param->normal_mode.netif_kind);
  printf("normal_mode.wifi_sta.ssid=%d\n", param->normal_mode.wifi_sta.ssid);
  printf("normal_mode.wifi_sta.password=%d\n",
         param->normal_mode.wifi_sta.password);
  printf("normal_mode.wifi_sta.encryption=%d\n",
         param->normal_mode.wifi_sta.encryption);
  printf("accesspoint_mode.dev_ip.ip=%d\n", param->accesspoint_mode.dev_ip.ip);
  printf("accesspoint_mode.dev_ip.subnet_mask=%d\n",
         param->accesspoint_mode.dev_ip.subnet_mask);
  printf("accesspoint_mode.dev_ip.gateway=%d\n",
         param->accesspoint_mode.dev_ip.gateway);
  printf("accesspoint_mode.dev_ip.dns=%d\n",
         param->accesspoint_mode.dev_ip.dns);
  printf("accesspoint_mode.wifi_ap.ssid=%d\n",
         param->accesspoint_mode.wifi_ap.ssid);
  printf("accesspoint_mode.wifi_ap.password=%d\n",
         param->accesspoint_mode.wifi_ap.password);
  printf("accesspoint_mode.wifi_ap.encryption=%d\n",
         param->accesspoint_mode.wifi_ap.encryption);
  printf("accesspoint_mode.wifi_ap.channel=%d\n",
         param->accesspoint_mode.wifi_ap.channel);
  printf("proxy.url=%d\n", param->proxy.url);
  printf("proxy.port=%d\n", param->proxy.port);
  printf("proxy.username=%d\n", param->proxy.username);
  printf("proxy.password=%d\n", param->proxy.password);
  printf("[STUB] EsfStubPrintMask END\n");
}

static void EsfStubPrintData(const EsfParameterStorageManagerData data) {
  printf("[STUB] EsfStubPrintData START data=0x%x\n", data);
  EsfNetworkManagerParameterInternal *param =
      (EsfNetworkManagerParameterInternal *)data;

  printf("normal_mode.dev_ip.ip=%s\n", param->param.normal_mode.dev_ip.ip);
  printf("normal_mode.dev_ip.subnet_mask=%s\n",
         param->param.normal_mode.dev_ip.subnet_mask);
  printf("normal_mode.dev_ip.gateway=%s\n",
         param->param.normal_mode.dev_ip.gateway);
  printf("normal_mode.dev_ip.dns=%s\n", param->param.normal_mode.dev_ip.dns);
  printf("normal_mode.dev_ip_v6.ip=%s\n",
         param->param.normal_mode.dev_ip_v6.ip);
  printf("normal_mode.dev_ip_v6.subnet_mask=%s\n",
         param->param.normal_mode.dev_ip_v6.subnet_mask);
  printf("normal_mode.dev_ip_v6.gateway=%s\n",
         param->param.normal_mode.dev_ip_v6.gateway);
  printf("normal_mode.dev_ip_v6.dns=%s\n",
         param->param.normal_mode.dev_ip_v6.dns);
  printf("normal_mode.ip_method=%d\n", param->ip_method.data);
  printf("normal_mode.netif_kind=%d\n", param->netif_kind.data);
  printf("normal_mode.wifi_sta.ssid=%s\n",
         param->param.normal_mode.wifi_sta.ssid);
  printf("normal_mode.wifi_sta.password=%s\n",
         param->param.normal_mode.wifi_sta.password);
  printf("normal_mode.wifi_sta.encryption=%d\n",
         param->wifi_sta_encryption.data);
  printf("accesspoint_mode.dev_ip.ip=%s\n",
         param->param.accesspoint_mode.dev_ip.ip);
  printf("accesspoint_mode.dev_ip.subnet_mask=%s\n",
         param->param.accesspoint_mode.dev_ip.subnet_mask);
  printf("accesspoint_mode.dev_ip.gateway=%s\n",
         param->param.accesspoint_mode.dev_ip.gateway);
  printf("accesspoint_mode.dev_ip.dns=%s\n",
         param->param.accesspoint_mode.dev_ip.dns);
  printf("accesspoint_mode.wifi_ap.ssid=%s\n",
         param->param.accesspoint_mode.wifi_ap.ssid);
  printf("accesspoint_mode.wifi_ap.password=%s\n",
         param->param.accesspoint_mode.wifi_ap.password);
  printf("accesspoint_mode.wifi_ap.encryption=%d\n",
         param->wifi_ap_encryption.data);
  printf("accesspoint_mode.wifi_ap.channel=%d\n", param->wifi_ap_channel.data);
  printf("proxy.url=%s\n", param->param.proxy.url);
  printf("proxy.port=%s\n", param->proxy_port);
  printf("proxy.username=%s\n", param->param.proxy.username);
  printf("proxy.password=%s\n", param->param.proxy.password);
  printf("[STUB] EsfStubPrintData END\n");
}

EsfParameterStorageManagerStatus EsfParameterStorageManagerSave(
    EsfParameterStorageManagerHandle handle,
    EsfParameterStorageManagerMask mask, EsfParameterStorageManagerData data,
    const EsfParameterStorageManagerStructInfo *info, void *private_data) {
  printf(
      "[STUB] EsfParameterStorageManagerSave START handle=%d mask=0x%x "
      "data=0x%x "
      "info=%p private_data=%p\n",
      handle, mask, data, info, private_data);
  if (parameter == NULL) {
    printf("[STUB] EsfParameterStorageManagerSave error parameter=%p\n",
           parameter);
    return kEsfParameterStorageManagerStatusFailedPrecondition;
  }
  // EsfStubPrintStructInfo(info);
  printf("[STUB] Save data print.\n");
  EsfStubPrintMask((EsfParameterStorageManagerMask)mask);
  EsfStubPrintData(data);
  int i = 0;
  for (i = 0; i < info->items_num; ++i) {
    if (info->items[i].enabled(mask)) {
      uint8_t *src_p = (uint8_t *)data + info->items[i].offset;
      uint8_t *dst_p = (uint8_t *)parameter + info->items[i].offset;
      memcpy(dst_p, src_p, info->items[i].size);
    }
  }
  printf("[STUB] Saved data print.\n");
  EsfStubPrintData((EsfParameterStorageManagerData)parameter);
  printf("[STUB] EsfParameterStorageManagerSave END\n");
  return kEsfParameterStorageManagerStatusOk;
}

EsfParameterStorageManagerStatus EsfParameterStorageManagerLoad(
    EsfParameterStorageManagerHandle handle,
    EsfParameterStorageManagerMask mask, EsfParameterStorageManagerData data,
    const EsfParameterStorageManagerStructInfo *info, void *private_data) {
  printf(
      "[STUB] EsfParameterStorageManagerLoad START handle=%d mask=0x%x "
      "data=0x%x "
      "info=%p private_data=%p\n",
      handle, mask, data, info, private_data);
  if (parameter == NULL) {
    printf("[STUB] EsfParameterStorageManagerLoad error parameter=%p\n",
           parameter);
    return kEsfParameterStorageManagerStatusFailedPrecondition;
  }
  // EsfStubPrintStructInfo(info);
  printf("[STUB] Saved data print.\n");
  EsfStubPrintData((EsfParameterStorageManagerData)parameter);
  int i = 0;
  for (i = 0; i < info->items_num; ++i) {
    if (info->items[i].enabled(mask)) {
      uint8_t *dst_p = (uint8_t *)data + info->items[i].offset;
      uint8_t *src_p = (uint8_t *)parameter + info->items[i].offset;
      memcpy(dst_p, src_p, info->items[i].size);
      if (info->items[i].type == kEsfParameterStorageManagerItemTypeRaw) {
        // printf("[STUB] dst_p:%p\n", dst_p);
        uint32_t *size_p = (uint32_t *)dst_p;
        *size_p = info->items[i].size - sizeof(uint32_t);
        // printf("[STUB] size_p:%p size:%u\n", size_p, *size_p);
      }
    }
  }
  printf("[STUB] Loaded data print.\n");
  EsfStubPrintMask((EsfParameterStorageManagerMask)mask);
  EsfStubPrintData(data);
  printf("[STUB] EsfParameterStorageManagerLoad END\n");
  return kEsfParameterStorageManagerStatusOk;
}
