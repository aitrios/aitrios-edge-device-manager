/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ESF_FIRMWARE_MANAGER_SRC_COMMON_FIRMWARE_MANAGER_FACTORY_RESET_H_
#define ESF_FIRMWARE_MANAGER_SRC_COMMON_FIRMWARE_MANAGER_FACTORY_RESET_H_

#include <stdbool.h>

#include "firmware_manager.h"

EsfFwMgrResult EsfFwMgrStartFactoryResetInternal(
    EsfFwMgrFactoryResetCause cause);

#endif  // ESF_FIRMWARE_MANAGER_SRC_COMMON_FIRMWARE_MANAGER_FACTORY_RESET_H_
