# SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
#
# SPDX-License-Identifier: Apache-2.0

if config_h.get('CONFIG_PL_NETWORK_HAVE_ETHER', false)
  subdir('ether')
endif
if config_h.get('CONFIG_EXTERNAL_PL_NETWORK', false)
  subdir('network')
endif
if config_h.get('CONFIG_EXTERNAL_PL_NETWORK_UTIL', false)
  subdir('network_util')
endif
if config_h.get('CONFIG_PL_NETWORK_HAVE_WIFI', false)
  subdir('wifi')
endif
