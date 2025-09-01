# SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
#
# SPDX-License-Identifier: Apache-2.0

esf_includes_public += include_directories('include')

if config_h.get('CONFIG_EXTERNAL_CLOCK_MANAGER_RPI')
    subdir('src')
else
    subdir('src_t4')
endif
