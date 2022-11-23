#include <Arduino.h>
#include "blinker.h"

const auto HALF_PERIOD = 150; // milliseconds

void Blinker::blink(uint8_t count)
{
    if (count == 0 || state != 0)
    {
        return;
    }
    state = count * 2 - 1;
    digitalWrite(LED_BUILTIN, LOW); // led on
    // schedule on main thread after loop()
    ticker.once_ms_scheduled(HALF_PERIOD, std::bind(&Blinker::handler, this));
}

void Blinker::handler()
{
    // toggle led
    state--;
    digitalWrite(LED_BUILTIN, (state & 1) ? LOW : HIGH);
    // reschedule
    if (state)
    {
        ticker.once_ms_scheduled(HALF_PERIOD, std::bind(&Blinker::handler, this));
    }
}

Blinker::~Blinker()
{
    ticker.detach();
}
