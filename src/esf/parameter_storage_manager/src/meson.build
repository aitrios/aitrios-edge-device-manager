# SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
#
# SPDX-License-Identifier: Apache-2.0

esf_sources += files([
	'parameter_storage_manager_buffer.c',
	'parameter_storage_manager_ext.c',
	'parameter_storage_manager_internal_work.c',
	'parameter_storage_manager_mutex.c',
	'parameter_storage_manager_resource.c',
	'parameter_storage_manager_storage_adapter.c',
	'parameter_storage_manager_storage_adapter_item_type.c',
	'parameter_storage_manager_storage_adapter_other.c',
	'parameter_storage_manager_storage_adapter_pl.c',
	'parameter_storage_manager_storage_adapter_settings.c',
	'parameter_storage_manager_utility.c',
])

# In this layer includes are given with the path from src/esf, for example:
#
#  #include "parameter_storage_manager/src/parameter_storage_manager_config.h"
#
# To accomodate that we add the esf directory as an include.

esf_includes_internal += include_directories('../..')
