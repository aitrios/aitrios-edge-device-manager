# SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
#
# SPDX-License-Identifier: Apache-2.0

esf_includes_internal += include_directories('include')

esf_sources += files([
	'network_manager_accessor_parameter_storage_manager.c',
	'network_manager_accessor.c',
	'network_manager_resource.c',
	'network_manager.c'
])

if config_h.get('CONFIG_EXTERNAL_NETWORK_MANAGER_DISABLE', false)
  subdir('stub')
endif
