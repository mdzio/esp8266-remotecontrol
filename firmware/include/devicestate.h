#include <Arduino.h>

enum DeviceState : uint8_t
{
    INITIALIZING,
    COMMAND_TIMEOUT,
    LOW_BATTERY,
    FILESYSTEM_FAILED,
    WIFI_FAILED,
    DNS_FAILED,
    READY
};

extern const __FlashStringHelper *deviceStateText(DeviceState state);
