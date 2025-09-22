# SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
#
# SPDX-License-Identifier: Apache-2.0

if config_h.get('CONFIG_EXTERNAL_PL_POWER_MGR', false)
  subdir('power_manager')
endif

if config_h.get('CONFIG_EXTERNAL_PL_SYSTEM_CONTROL', false)
  subdir('system_control')
endif

if config_h.get('CONFIG_EXTERNAL_PL_WDT', false)
  sudbdir('wdt')
endif

if config_h.get('CONFIG_EXTERNAL_PL_USB_PORT_CTRL', false)
  sudbdir('usb_port_controller')
endif
