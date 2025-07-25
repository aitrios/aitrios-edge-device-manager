/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#include "led_manager/config/config.h"

// Power Led priority table.
static const EsfLedManagerPriority kPowerLedControlTable[] = {
    {
        .state = kEsfLedManagerLedStatusForcedOff,
        .led.color = CONFIG_EXTERNAL_LED_MANAGER_COLOR_CONFIG_LED_OFF,
    },
    {
        .state = kEsfLedManagerLedStatusResetting,
        .led.color = CONFIG_EXTERNAL_LED_MANAGER_COLOR_CONFIG_GREEN,
        .led.interval = kEsfLedManagerLedInterval1Hz,
    },
    {
        .state = kEsfLedManagerLedStatusErrorPeripheralDriversInitializationFailed,
        .led.color = CONFIG_EXTERNAL_LED_MANAGER_COLOR_CONFIG_RED,
        .led.interval = kEsfLedManagerLedInterval4Hz,
    },
    {
        .state = kEsfLedManagerLedStatusErrorNetworkInitializationFailed,
        .led.color = CONFIG_EXTERNAL_LED_MANAGER_COLOR_CONFIG_RED,
        .led.interval = kEsfLedManagerLedInterval4Hz,
    },
    {
        .state = kEsfLedManagerLedStatusErrorUpdateMemoryAllocateFailed,
        .led.color = CONFIG_EXTERNAL_LED_MANAGER_COLOR_CONFIG_RED,
        .led.interval = kEsfLedManagerLedInterval4Hz,
    },
    {
        .state = kEsfLedManagerLedStatusErrorDataFlashFailed,
        .led.color = CONFIG_EXTERNAL_LED_MANAGER_COLOR_CONFIG_RED,
        .led.interval = kEsfLedManagerLedInterval4Hz,
    },
    {
        .state = kEsfLedManagerLedStatusErrorLegacyUSB,
        .led.color = CONFIG_EXTERNAL_LED_MANAGER_COLOR_CONFIG_RED,
        .led.interval = kEsfLedManagerLedInterval1Hz,
    },
    {
        .state = kEsfLedManagerLedStatusErrorInvalidQRCode,
        .led.color = CONFIG_EXTERNAL_LED_MANAGER_COLOR_CONFIG_RED,
        .led.interval = kEsfLedManagerLedInterval1Hz,
    },
    {
        .state = kEsfLedManagerLedStatusErrorUploadFailed,
        .led.color = CONFIG_EXTERNAL_LED_MANAGER_COLOR_CONFIG_RED,
        .led.interval = kEsfLedManagerLedInterval1Hz,
    },
    {
        .state = kEsfLedManagerLedStatusErrorDownloadFailed,
        .led.color = CONFIG_EXTERNAL_LED_MANAGER_COLOR_CONFIG_RED,
        .led.interval = kEsfLedManagerLedInterval1Hz,
    },
    {
        .state = kEsfLedManagerLedStatusErrorAuthProxyFailed,
        .led.color = CONFIG_EXTERNAL_LED_MANAGER_COLOR_CONFIG_RED,
        .led.interval = kEsfLedManagerLedInterval1Hz,
    },
    {
        .state =
            kEsfLedManagerLedStatusWaitingForInputsToConnectConsoleGlobalProvisioner,
        .led.color = CONFIG_EXTERNAL_LED_MANAGER_COLOR_CONFIG_GREEN,
        .led.interval = kEsfLedManagerLedInterval4Hz,
    },
    {
        .state = kEsfLedManagerLedStatusWaitingForInputsToConnectConsole,
        .led.color = CONFIG_EXTERNAL_LED_MANAGER_COLOR_CONFIG_GREEN,
        .led.interval = kEsfLedManagerLedIntervalPattern,
    },
    {
        .state = kEsfLedManagerLedStatusAbleToAcceptInputs,
        .led.color = CONFIG_EXTERNAL_LED_MANAGER_COLOR_CONFIG_GREEN,
        .led.interval = kEsfLedManagerLedIntervalNone,
    },
    {
        .state = kEsfLedManagerLedStatusUnableToAcceptInputs,
        .led.color = CONFIG_EXTERNAL_LED_MANAGER_COLOR_CONFIG_GREEN,
        .led.interval = kEsfLedManagerLedInterval1Hz,
    },
};

// Wifi Led priority table.
static const EsfLedManagerPriority kWifiLedControlTable[] = {
    {
        .state = kEsfLedManagerLedStatusForcedOff,
        .led.color = CONFIG_EXTERNAL_LED_MANAGER_COLOR_CONFIG_LED_OFF,
    },
    {
        .state = kEsfLedManagerLedStatusLinkEstablished,
        .led.color = CONFIG_EXTERNAL_LED_MANAGER_COLOR_CONFIG_GREEN,
        .led.interval = kEsfLedManagerLedIntervalNone,
    },
    {
        .state = kEsfLedManagerLedStatusAPFoundAndDoingAuthentication,
        .led.color = CONFIG_EXTERNAL_LED_MANAGER_COLOR_CONFIG_RED,
        .led.interval = kEsfLedManagerLedInterval4Hz,
    },
    {
        .state = kEsfLedManagerLedStatusSearchingAP,
        .led.color = CONFIG_EXTERNAL_LED_MANAGER_COLOR_CONFIG_RED,
        .led.interval = kEsfLedManagerLedInterval1Hz,
    },
    {
        .state = kEsfLedManagerLedStatusLoadingSSIDPassword,
        .led.color = CONFIG_EXTERNAL_LED_MANAGER_COLOR_CONFIG_RED,
        .led.interval = kEsfLedManagerLedInterval1Hz,
    },
};

// Service priority table.
static const EsfLedManagerPriority kServiceLedControlTable[] = {
    {
        .state = kEsfLedManagerLedStatusForcedOff,
        .led.color = CONFIG_EXTERNAL_LED_MANAGER_COLOR_CONFIG_LED_OFF,
    },
    {
        .state = kEsfLedManagerLedStatusConnectedWithTLS,
        .led.color = CONFIG_EXTERNAL_LED_MANAGER_COLOR_CONFIG_GREEN,
        .led.interval = kEsfLedManagerLedIntervalNone,
    },
    {
        .state = kEsfLedManagerLedStatusConnectedWithoutTLS,
        .led.color = CONFIG_EXTERNAL_LED_MANAGER_COLOR_CONFIG_ORANGE,
        .led.interval = kEsfLedManagerLedIntervalNone,
    },
    {
        .state = kEsfLedManagerLedStatusDisconnectedConnectingWithTLS,
        .led.color = CONFIG_EXTERNAL_LED_MANAGER_COLOR_CONFIG_GREEN,
        .led.interval = kEsfLedManagerLedInterval1Hz,
    },
    {
        .state = kEsfLedManagerLedStatusDisconnectedConnectingWithoutTLS,
        .led.color = CONFIG_EXTERNAL_LED_MANAGER_COLOR_CONFIG_ORANGE,
        .led.interval = kEsfLedManagerLedInterval1Hz,
    },
    {
        .state = kEsfLedManagerLedStatusDisconnectedConnectingProxy,
        .led.color = CONFIG_EXTERNAL_LED_MANAGER_COLOR_CONFIG_RED,
        .led.interval = kEsfLedManagerLedInterval4Hz,
    },
    {
        .state = kEsfLedManagerLedStatusDisconnectedConnectingDNSAndNTP,
        .led.color = CONFIG_EXTERNAL_LED_MANAGER_COLOR_CONFIG_RED,
        .led.interval = kEsfLedManagerLedInterval1Hz,
    },
    {
        .state = kEsfLedManagerLedStatusDisconnectedNoInternetConnection,
        .led.color = CONFIG_EXTERNAL_LED_MANAGER_COLOR_CONFIG_RED,
        .led.interval = kEsfLedManagerLedInterval4Hz,
    },
    {
        .state = kEsfLedManagerLedStatusDisconnectedEstablishingNetworkLinkOnPhysicalLayer,
        .led.color = CONFIG_EXTERNAL_LED_MANAGER_COLOR_CONFIG_RED,
        .led.interval = kEsfLedManagerLedIntervalNone,
    },
};

// LED priority table.
LED_MANAGER_STATIC LED_MANAGER_CONSTANT struct EsfLedManagerLedTables
    kLedControlTable[kEsfLedManagerTargetLedNum] = {
        ESF_LED_MANAGER_SET_TABLE(kPowerLedControlTable),
        ESF_LED_MANAGER_SET_TABLE(kWifiLedControlTable),
        ESF_LED_MANAGER_SET_TABLE(kServiceLedControlTable),
};
