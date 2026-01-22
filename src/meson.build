# SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
#
# SPDX-License-Identifier: Apache-2.0

subdir('esf')
subdir('hal')
subdir('porting_layer')
if get_option('target') == 't4r'
  subdir('security')
endif

subdir('utility')
subdir('wasm_binding')

# Compile the utilities library and declare it as a dependency to be available
# to the upper layers

utility = static_library(
  'utility',
  utility_sources,
  include_directories : [
    utility_includes_public,
    utility_includes_internal,
    esf_includes_public,
    esf_includes_internal,
  ],
)
utility_dep = declare_dependency(
  link_with : utility,
  include_directories : utility_includes_public
)

# The codecs library contains code to parse and create JSON, JPEG and Base64
# encoded data and needs some external libraries to help manage those.

codecs = static_library(
  'codecs',
  codec_sources,
  include_directories : [
    codec_includes_public,
    esf_includes_public,
  ],
  dependencies : [
    base64_dep,
    parson_dep,
    jpeg_dep,
    utility_dep,
    wamr_dep, # The ESF headers include WAMR headers...
  ],
)
codecs_dep = declare_dependency(
  link_with : codecs,
  include_directories : codec_includes_public
)

# And likewise the porting layer.

porting_layer = static_library(
  'pl',
  pl_sources,
  include_directories : [
    pl_includes_public,
    pl_includes_internal,
    utility_includes_public,
    esf_includes_public,
    hal_includes_public
  ],
  dependencies : [
    wamr_dep,
    codecs_dep,
    libnm_dep,
    sqlite3_dep,
    utility_dep,
  ] + (get_option('target') == 't4r' ? [libchrony_dep, mbedtls_dep] : []),
)
porting_layer_dep = declare_dependency(
  link_with : porting_layer,
  include_directories : pl_includes_public
)

# And likewise the Security Module if target is t4r
if get_option('target') == 't4r'
  security = static_library(
    'security',
    security_sources,
    include_directories : [
      security_includes_public,
      security_includes_internal,
      pl_includes_public,
      utility_includes_public,
    ],
    dependencies : [
    ]
  )
  security_dep = declare_dependency(
    link_with : security,
    include_directories : security_includes_public
  )
endif

# And finally the HAL (small as it is)

hal = static_library(
  'hal',
  hal_sources,
  include_directories : [
    hal_includes_public,
    hal_includes_internal,
    utility_includes_public,
  ],
  dependencies : [
    libnm_dep,
    utility_dep
  ],
)

hal_dep = declare_dependency(
  link_with : hal,
  include_directories : hal_includes_public
)
