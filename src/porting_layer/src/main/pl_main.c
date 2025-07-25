/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "pl_main.h"

#include "pl_main_internal.h"
#include "pl_main_table.inc"

// Functions ------------------------------------------------------------------
PlErrCode PlMainEmmcFormat(PlMainKeepAliveCallback cb, void* user_data) {
  return PlMainInternalEmmcFormat(kDeviceInformation, kDeviceInformationSize,
                                  cb, user_data);
}
// -----------------------------------------------------------------------------
PlErrCode PlMainEmmcMount(void) {
  return PlMainInternalEmmcMount(kDeviceInformation, kDeviceInformationSize);
}
// -----------------------------------------------------------------------------
PlErrCode PlMainEmmcUnmount(void) {
  return PlMainInternalEmmcUnmount(kDeviceInformation, kDeviceInformationSize);
}
// -----------------------------------------------------------------------------
PlErrCode PlMainFlashFormat(PlMainKeepAliveCallback cb, void* user_data) {
  return PlMainInternalFlashFormat(kDeviceInformation, kDeviceInformationSize,
                                   cb, user_data);
}
// -----------------------------------------------------------------------------
PlErrCode PlMainFlashMount(void) {
  return PlMainInternalFlashMount(kDeviceInformation, kDeviceInformationSize);
}
// -----------------------------------------------------------------------------
PlErrCode PlMainFlashUnmount(void) {
  return PlMainInternalFlashUnmount(kDeviceInformation, kDeviceInformationSize);
}
// -----------------------------------------------------------------------------
PlErrCode PlMainIsFeatureSupported(PlMainFeatureType type) {
  return PlMainInternalIsFeatureSupported(kFeatureTypes, kFeatureTypesSize,
                                          type);
}
