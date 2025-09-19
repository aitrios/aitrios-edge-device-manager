/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/


#ifndef __PL_USB_PORT_CTRL_CAM_IMPL_H__
#define __PL_USB_PORT_CTRL_CAM_IMPL_H__

#include "pl.h"
#include "pl_usb_port_controller.h"

PlErrCode PlUsbPCInitializeCamImpl(void);
PlErrCode PlUsbPCFinalizeCamImpl(void);
PlErrCode PlUsbPCRegEventCallbackCamImpl(uint32_t usb_pc_id,
                                          PlUsbPCEventCallback callback,
                                          void *private_data);

PlErrCode PlUsbPCUnregEventCallbackCamImpl(uint32_t usb_pc_id);

#endif  // _PL_USB_PORT_CTRL_CAM_IMPL_H_
