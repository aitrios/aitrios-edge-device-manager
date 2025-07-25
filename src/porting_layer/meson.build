# SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
#
# SPDX-License-Identifier: Apache-2.0

pl_includes_public = [ include_directories('include') ]
pl_includes_internal = []

# Some places include "porting_layer/include/pl.h", so we need to account for
# that.
pl_includes_public += include_directories('..')

pl_sources = []

subdir('src')
