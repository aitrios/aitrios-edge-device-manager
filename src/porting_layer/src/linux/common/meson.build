# SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
#
# SPDX-License-Identifier: Apache-2.0

if config_h.get('CONFIG_EXTERNAL_PL_BUTTON', false)
  subdir('button_manager')
endif

subdir('parameter_storage_manager')
subdir('memory_manager')
if config_h.get('CONFIG_EXTERNAL_PL_NETWORK_IMPL', false)
  subdir('network_manager')
endif
subdir('power_manager')
