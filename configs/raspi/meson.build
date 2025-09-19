# SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
#
# SPDX-License-Identifier: Apache-2.0

#####################
# common
#####################

# Informing the build process of the target, which is necessary to ensure proper
# compilation in a few places
config_h.set('CONFIG_EXTERNAL_TARGET_RPI', true)

config_h.set('CONFIG_ENABLE_REBOOT_ON_EDC_ASSERT', false)

#####################
# esf
#####################

# Currently necessary to ensure that some of the conditional compilation sites
# in time.h and pthread.h come out right, but possibly a candidate for a better
# solution elsewhere.
config_h.set('_GNU_SOURCE', true)

# button manager
config_h.set('CONFIG_ESF_BUTTON_MANAGER_THREAD_STACK_SIZE', 4096)
config_h.set('CONFIG_ESF_BUTTON_MANAGER_HANDLE_MAX_NUM', 3)
config_h.set('CONFIG_ESF_BUTTON_MANAGER_NOTIFICATION_CALLBACK_MAX_NUM', 3)
config_h.set('CONFIG_EXTERNAL_BUTTON_MANAGER_DISABLE', true)

#clock manager
config_h.set('CONFIG_EXTERNAL_CLOCK_MANAGER_SYNC_INTERVAL_MIN', 64)
config_h.set('CONFIG_EXTERNAL_CLOCK_MANAGER_SYNC_INTERVAL_MAX', 1024)
config_h.set('CONFIG_EXTERNAL_CLOCK_MANAGER_SYNC_INTERVAL_DEF', 64)
config_h.set('CONFIG_EXTERNAL_CLOCK_MANAGER_POLLING_TIME_MIN', 1)
config_h.set('CONFIG_EXTERNAL_CLOCK_MANAGER_POLLING_TIME_MAX', 1024)
config_h.set('CONFIG_EXTERNAL_CLOCK_MANAGER_POLLING_TIME_DEF', 1024)
config_h.set('CONFIG_EXTERNAL_CLOCK_MANAGER_LIMIT_PACKET_TIME_MIN', 0)
config_h.set('CONFIG_EXTERNAL_CLOCK_MANAGER_LIMIT_PACKET_TIME_MAX', 10000)
config_h.set('CONFIG_EXTERNAL_CLOCK_MANAGER_LIMIT_PACKET_TIME_DEF', 66)
config_h.set('CONFIG_EXTERNAL_CLOCK_MANAGER_SANITY_LIMIT_MIN', 0)
config_h.set('CONFIG_EXTERNAL_CLOCK_MANAGER_SANITY_LIMIT_MAX', 32767)
config_h.set('CONFIG_EXTERNAL_CLOCK_MANAGER_SANITY_LIMIT_DEF', 1000)
config_h.set('CONFIG_EXTERNAL_CLOCK_MANAGER_STABLE_RTC_MIN', 0)
config_h.set('CONFIG_EXTERNAL_CLOCK_MANAGER_STABLE_RTC_MAX', 1000)
config_h.set('CONFIG_EXTERNAL_CLOCK_MANAGER_STABLE_RTC_DEF', 33)
config_h.set('CONFIG_EXTERNAL_CLOCK_MANAGER_RTC_CORRECT_LIMIT_MIN', 0)
config_h.set('CONFIG_EXTERNAL_CLOCK_MANAGER_RTC_CORRECT_LIMIT_MAX', 1000)
config_h.set('CONFIG_EXTERNAL_CLOCK_MANAGER_RTC_CORRECT_LIMIT_DEF', 66)
config_h.set('CONFIG_EXTERNAL_CLOCK_MANAGER_STABLE_SYNC_CONT_MIN', 0)
config_h.set('CONFIG_EXTERNAL_CLOCK_MANAGER_STABLE_SYNC_CONT_MAX', 1000)
config_h.set('CONFIG_EXTERNAL_CLOCK_MANAGER_STABLE_SYNC_CONT_DEF', 5)
config_h.set('CONFIG_EXTERNAL_CLOCK_MANAGER_DEFAULT_NTP_SERVER', '"time.aitrios.sony-semicon.com"')
config_h.set('CONFIG_EXTERNAL_CLOCK_MANAGER_NOTIFIER_STACKSIZE', 4096)
config_h.set('CONFIG_EXTERNAL_CLOCK_MANAGER_DISABLE', true)

# if NTP synchronization fails for about 3 minutes, it is judged as an error.
# Because Chrony NTP synchronization cycle is 64 seconds.
config_h.set('CONFIG_EXTERNAL_CLOCK_MANAGER_NTP_ERROR_TIME', 192)

#clock manager stub netutils
config_h.set('CONFIG_NETUTILS_NTPCLIENT_NUM_SAMPLES', 5)
config_h.set('CONFIG_EXTERNAL_CLOCK_MANAGER_NTP_CLIENT_MONITOR_STACKSIZE', 4096)

# codec base64
config_h.set('CONFIG_EXTERNAL_CODEC_BASE64_FILEIO_WORK_SIZE', 7168)

# codec jpeg
config_h.set('CONFIG_EXTERNAL_CODEC_JPEG_OSS_LIBJPEG_TURBO', true)
config_h.set('CONFIG_EXTERNAL_CODEC_JPEG_FILE_IO_WRITE_BUFFER_SIZE', 4096)

# codec json
config_h.set('CONFIG_EXTERNAL_CODEC_JSON_BUFFER_SIZE', 4096)
config_h.set('CONFIG_EXTERNAL_CODEC_JSON_MEM_HANDLE_MAX', 3)

# firmware manager
config_h.set('CONFIG_EXTERNAL_FIRMWARE_MANAGER_AI_MODEL_SLOT_NUM', 4)
config_h.set('CONFIG_EXTERNAL_FIRMWARE_MANAGER_MAX_MEMORY_SIZE', 1048576)
config_h.set('CONFIG_EXTERNAL_FIRMWARE_MANAGER_USE_UTILITY_LOG', true)
config_h.set('CONFIG_EXTERNAL_FIRMWARE_MANAGER_USE_SENSOR_FW_UPDATE_LIB', true)
config_h.set('CONFIG_EXTERNAL_FIRMWARE_MANAGER_ENABLE_FACTORY_RESET', false)
config_h.set('CONFIG_EXTERNAL_FIRMWARE_MANAGER_ENABLE_SWITCH_PROCESSOR_FIRMWARE_SLOT', false)

# led manager
config_h.set('CONFIG_EXTERNAL_LED_MANAGER_COLOR_CONFIG_RED', 0)
config_h.set('CONFIG_EXTERNAL_LED_MANAGER_COLOR_CONFIG_GREEN', 1)
config_h.set('CONFIG_EXTERNAL_LED_MANAGER_COLOR_CONFIG_ORANGE', 2)

config_h.set('CONFIG_EXTERNAL_LED_MANAGER_COLOR_CONFIG_LED_OFF', 999)
config_h.set('CONFIG_EXTERNAL_LED_MANAGER_POWER_LED_ID', 0)
config_h.set('CONFIG_EXTERNAL_LED_MANAGER_WIFI_LED_ID', 1)
config_h.set('CONFIG_EXTERNAL_LED_MANAGER_SERVICE_LED_ID', 2)
config_h.set('CONFIG_EXTERNAL_LED_MANAGER_DISABLE', true)

# utility log & log manager
config_h.set('CONFIG_EXTERNAL_DLOG_DISABLE', true)
#config_h.set('CONFIG_EXTERNAL_ELOG_DISABLE', true)

# log manager
config_h.set('CONFIG_EXTERNAL_LOG_MANAGER_DLOG_THREAD_STACK_SIZE', 4096)
config_h.set('CONFIG_EXTERNAL_LOG_MANAGER_ELOG_THREAD_STACK_SIZE', 4096)
config_h.set('CONFIG_EXTERNAL_LOG_MANAGER_DLOG_NUM_OF_BUF', 2)
config_h.set('CONFIG_EXTERNAL_LOG_MANAGER_DLOG_SIZE_OF_BUF', 4096)
config_h.set('CONFIG_EXTERNAL_LOG_MANAGER_ELOG_NUM_OF_BUF', 1)
config_h.set('CONFIG_EXTERNAL_LOG_MANAGER_ELOG_SIZE_OF_BUF', 4096)
config_h.set('CONFIG_EXTERNAL_LOG_MANAGER_LOCAL_LIST_MAX_NUM', 5)
config_h.set('CONFIG_EXTERNAL_LOG_MANAGER_CLOUD_LIST_MAX_NUM', 5)
config_h.set('LOG_MANAGER_EVP_ENABLE', true)
config_h.set('CONFIG_EXTERNAL_LOG_MANAGER_PSM_DISABLE', true)

config_h.set('CONFIG_EXTERNAL_LOG_MANAGER_METRICS', false)
config_h.set('CONFIG_EXTERNAL_LOG_MANAGER_METRICS_QUEUE_NUM', 2)
config_h.set('CONFIG_EXTERNAL_LOG_MANAGER_METRICS_QUEUE_SIZE', 2048)
config_h.set('CONFIG_EXTERNAL_LOG_MANAGER_METRICS_GENERATE_THREAD_STACK_SIZE', 4096)
config_h.set('CONFIG_EXTERNAL_LOG_MANAGER_METRICS_GENERATE_PRIORITY', 100)
config_h.set('CONFIG_EXTERNAL_LOG_MANAGER_METRICS_GENERATE_INTERVAL', 1)
config_h.set('CONFIG_EXTERNAL_LOG_MANAGER_METRICS_SEND_THREAD_STACK_SIZE', 4096)
config_h.set('CONFIG_EXTERNAL_LOG_MANAGER_METRICS_SEND_PRIORITY', 100)
config_h.set('CONFIG_EXTERNAL_LOG_MANAGER_METRICS_SEND_INTERVAL', 1)

# main
config_h.set('CONFIG_EXTERNAL_MAIN_LOCKTIME_MS', 1000)
config_h.set('CONFIG_EXTERNAL_MAIN_OSAL_MSG_WAITTIME_MS', 100)
config_h.set('CONFIG_EXTERNAL_MAIN_APP_MEM_DIV_NUM', 1)
config_h.set('CONFIG_EXTERNAL_MEMORY_MANAGER', true)
config_h.set('CONFIG_EXTERNAL_PARAMETER_STORAGE_MANAGER', true)
config_h.set('CONFIG_EXTERNAL_LOG_MANAGER', true)
config_h.set('CONFIG_EXTERNAL_LED_MANAGER', true)
config_h.set('CONFIG_EXTERNAL_CLOCK_MANAGER', true)
config_h.set('CONFIG_EXTERNAL_FIRMWARE_MANAGER', true)
config_h.set('CONFIG_EXTERNAL_SYSTEM_MANAGER', true)
config_h.set('CONFIG_EXTERNAL_SYSTEMAPP', true)
config_h.set('CONFIG_EXTERNAL_MAIN_WAIT_SYSTEM_APP_TERM_TIME', 3000000)

# network manager
config_h.set('CONFIG_EXTERNAL_NETWORK_MANAGER', true)
config_h.set('CONFIG_EXTERNAL_NETWORK_MANAGER_LOCKTIME', 1000)
config_h.set('CONFIG_EXTERNAL_NETWORK_MANAGER_LOGCTL_ENABLE', true)
config_h.set('CONFIG_EXTERNAL_NETWORK_MANAGER_DISABLE', true)
config_h.set('CONFIG_EXTERNAL_NETWORK_MANAGER_WIFI_STA_MODE_DISABLE', true)
config_h.set('CONFIG_EXTERNAL_NETWORK_MANAGER_WIFI_AP_MODE_DISABLE', true)

# network manager stub
config_h.set('CONFIG_NET_IPv4', true)
config_h.set('CONFIG_NETDB_DNSCLIENT', true)
config_h.set('CONFIG_NET_ETHERNET', true)

# parameter storage manager
config_h.set('CONFIG_EXTERNAL_PARAMETER_STORAGE_MANAGER_HANDLE_MAX', 8)
config_h.set('CONFIG_EXTERNAL_PARAMETER_STORAGE_MANAGER_TIMEOUT_MS', 1000)
config_h.set('CONFIG_EXTERNAL_PARAMETER_STORAGE_MANAGER_FACTORY_RESET_MAX', 8)
config_h.set('CONFIG_EXTERNAL_PARAMETER_STORAGE_MANAGER_UPDATE_MAX', 4)
config_h.set('CONFIG_EXTERNAL_PARAMETER_STORAGE_MANAGER_BUFFER_LENGTH', 4096)

# power manager
config_h.set('CONFIG_EXTERNAL_POWER_MANAGER_LOCKTIME', 1000)
config_h.set('CONFIG_EXTERNAL_POWER_MANAGER_DISABLE', true)
config_h.set('CONFIG_EXTERNAL_POWER_MANAGER_SW_WDT_ID_NUM', 3)
config_h.set('CONFIG_EXTERNAL_POWER_MANAGER_SW_WDT_ID_0', 0)
config_h.set('CONFIG_EXTERNAL_POWER_MANAGER_SW_WDT_ID_1', 1)
config_h.set('CONFIG_EXTERNAL_POWER_MANAGER_SW_WDT_ID_2', 2)
config_h.set('CONFIG_EXTERNAL_POWER_MANAGER_SW_WDT_TIMEOUT_SEC', 60)

# system manager
config_h.set('CONFIG_EXTERNAL_SYSTEM_MANAGER_RESET_CAUSE_DISABLE', true)
config_h.set('CONFIG_EXTERNAL_SYSTEM_MANAGER_EXCEPTION_UPLOAD_DISABLE', true)


#####################
# porting layer
#####################

# pl button manager
config_h.set('CONFIG_EXTERNAL_PL_BUTTON_NUM', 0)

# pl led manager
config_h.set('CONFIG_PL_LED_COLORS_NUM', 3)
config_h.set('CONFIG_PL_LED_LEDS_NUM', 3)

# firmware manager porting layer
config_h.set('CONFIG_FIRMWARE_MANAGER_PORTING_LAYER', false)

# pl system control
config_h.set('CONFIG_EXTERNAL_PL_SYSTEM_CONTROL', true)

#####################
# utility
#####################

# utility log
config_h.set('CONFIG_EXTERNAL_UTILITY_LOG', true)
config_h.set('CONFIG_UTILITY_LOG_HANDLE_MAX_NUM', 32)
config_h.set('CONFIG_UTILITY_LOG_BULK_DLOG_MAX_SIZE', 4096)
config_h.set('UTILITY_LOG_ENABLE_SYSLOG', false)

# Default Dlog level.
# The supported levels are as follows. 0:Critical, 1:Error, 2:Warning, 3:Info, 4:Debug, 5:Trace.
config_h.set('CONFIG_UTILITY_LOG_DEFAULT_DLOG_LEVEL', 3)

# Default Elog level.
# The supported levels are as follows. 0:Critical, 1:Error, 2:Warning, 3:Info, 4:Debug, 5:Trace.
config_h.set('CONFIG_UTILITY_LOG_DEFAULT_ELOG_LEVEL', 3)

# utility timer
config_h.set('CONFIG_NAME_MAX', 48)
config_h.set('CONFIG_UTILITY_TIMER_THREAD_PRIORITY', 65)

# These will need to be replaced more cleverly somehow, but for now let's just
# get everything to compile
config_h.set('TIMER_MAX', 32)
config_h.set('MSEC_PER_SEC',  '1000L')
config_h.set('USEC_PER_MSEC', '1000L')
config_h.set('NSEC_PER_USEC', '1000L')
config_h.set('NSEC_PER_MSEC', '1000000L')
config_h.set('USEC_PER_SEC',  '1000000L')
config_h.set('NSEC_PER_SEC',  '1000000000L')
config_h.set('CLOCKRES_MIN',  '100000L')

#####################
# wasm binding
#####################

# binding
config_h.set('CONFIG_EXTERNAL_WASM_BINDING_INIT', true)

# codec jpeg
config_h.set('CONFIG_EXTERNAL_CODEC_JPEG_WASM', true)
config_h.set('CONFIG_EXTERNAL_CODEC_JPEG_HANDLE_WASM', true)

# deviceid
config_h.set('CONFIG_EXTERNAL_DEVICE_ID_WASM', true)

# memory manager
config_h.set('CONFIG_EXTERNAL_MEMORY_MANAGER_WASM', true)
