# SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
#
# SPDX-License-Identifier: Apache-2.0

esf_sources += files([
  'button_manager.c',
])

if not config_h.get('CONFIG_EXTERNAL_BUTTON_MANAGER_DISABLE', false)
  esf_sources += files([
    'button_manager_internal.c',
    'button_manager_internal.h',
    'button_manager_linked_list.c',
    'button_manager_linked_list.h',
  ])
endif
