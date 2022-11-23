#ifndef BLINKER_H
#define BLINKER_H

#include <Ticker.h>

class Blinker
{
public:
    virtual ~Blinker();
    void blink(uint8_t count);

private:
    Ticker ticker;
    uint8_t state;

    void handler();
};

#endif // BLINKER_H
