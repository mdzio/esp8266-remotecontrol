#include "devicestate.h"

const __FlashStringHelper *deviceStateText(DeviceState state)
{
    switch (state)
    {
    case INITIALIZING:
        return F("INITIALIZING");
        break;
    case COMMAND_TIMEOUT:
        return F("COMMAND_TIMEOUT");
        break;
    case LOW_BATTERY:
        return F("LOW_BATTERY");
        break;
    case FILESYSTEM_FAILED:
        return F("FILESYSTEM_FAILED");
        break;
    case WIFI_FAILED:
        return F("WIFI_FAILED");
        break;
    case DNS_FAILED:
        return F("DNS_FAILED");
        break;
    case READY:
        return F("READY");
        break;
    default:
        return F("?");
    }
}
