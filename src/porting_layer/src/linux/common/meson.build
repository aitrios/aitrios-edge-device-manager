# SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
#
# SPDX-License-Identifier: Apache-2.0

if config_h.get('CONFIG_EXTERNAL_PL_BUTTON', false)
  subdir('button_manager')
endif

if config_h.get('CONFIG_FIRMWARE_MANAGER_PORTING_LAYER', false)
  subdir('firmware_manager')
endif

subdir('parameter_storage_manager')
subdir('memory_manager')
if config_h.get('CONFIG_EXTERNAL_PL_NETWORK_IMPL', false)
  subdir('network_manager')
endif
subdir('power_manager')
subdir('main')
subdir('log_manager')
subdir('system_manager')
if config_h.get('CONFIG_EXTERNAL_PL_CLOCK_MANAGER', false)
  subdir('clock_manager')
endif

if get_option('target') == 't4r'
  subdir('cipher_util')
  subdir('nvm_mgr')
endif
