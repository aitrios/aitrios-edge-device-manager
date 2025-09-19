/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// Includes --------------------------------------------------------------------
#include "pl_wifi_ap.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pl.h"
#include "pl_network.h"
#include "pl_network_log.h"
#include "utility_log.h"
#include "utility_log_module_id.h"

// Public functions ------------------------------------------------------------
PlErrCode PlWifiApSetConfig(const char *if_name,
                            const PlNetworkConfig *config) {
  // Do nothing
  PlErrCode err_code = kPlErrCodeOk;
  if ((if_name == NULL) || (config == NULL)) {
    err_code = kPlErrInvalidParam;
    DLOGE("argument error.");
    ELOGE(kElog_PlWifiApSetConfig);
    goto err_end;
  }

  err_code = kPlErrNoSupported;

err_end:
  return err_code;
}

// -----------------------------------------------------------------------------
PlErrCode PlWifiApGetConfig(const char *if_name, PlNetworkConfig *config) {
  // Do nothing
  PlErrCode err_code = kPlErrCodeOk;
  if ((if_name == NULL) || (config == NULL)) {
    err_code = kPlErrInvalidParam;
    DLOGE("argument error.");
    ELOGE(kElog_PlWifiApGetConfig);
    goto err_end;
  }

  err_code = kPlErrNoSupported;

err_end:
  return err_code;
}

// -----------------------------------------------------------------------------
PlErrCode PlWifiApGetStatus(const char *if_name, PlNetworkStatus *status) {
  // Do nothing
  PlErrCode err_code = kPlErrCodeOk;
  if ((if_name == NULL) || (status == NULL)) {
    err_code = kPlErrInvalidParam;
    DLOGE("argument error.");
    ELOGE(kElog_PlWifiApGetStatus);
    goto err_end;
  }

err_end:
  return err_code;
}

// -----------------------------------------------------------------------------
PlErrCode PlWifiApRegisterEventHandler(const char *if_name) {
  // Do nothing
  PlErrCode err_code = kPlErrCodeOk;
  if (if_name == NULL) {
    err_code = kPlErrInvalidParam;
    DLOGE("argument error.");
    ELOGE(kElog_PlWifiApRegisterEventHandler);
    goto err_end;
  }

err_end:
  return err_code;
}

// -----------------------------------------------------------------------------
PlErrCode PlWifiApUnregisterEventHandler(const char *if_name) {
  // Do nothing
  PlErrCode err_code = kPlErrCodeOk;
  if (if_name == NULL) {
    err_code = kPlErrInvalidParam;
    DLOGE("argument error.");
    ELOGE(kElog_PlWifiApUnregisterEventHandler);
    goto err_end;
  }

err_end:
  return err_code;
}

// -----------------------------------------------------------------------------
PlErrCode PlWifiApStart(const char *if_name) {
  // Do nothing
  PlErrCode err_code = kPlErrCodeOk;
  if (if_name == NULL) {
    err_code = kPlErrInvalidParam;
    DLOGE("argument error.");
    ELOGE(kElog_PlWifiApStart);
    goto err_end;
  }

err_end:
  return err_code;
}

// -----------------------------------------------------------------------------
PlErrCode PlWifiApStop(const char *if_name) {
  // Do nothing
  PlErrCode err_code = kPlErrCodeOk;
  if (if_name == NULL) {
    err_code = kPlErrInvalidParam;
    DLOGE("argument error.");
    ELOGE(kElog_PlWifiApStop);
    goto err_end;
  }

err_end:
  return err_code;
}
