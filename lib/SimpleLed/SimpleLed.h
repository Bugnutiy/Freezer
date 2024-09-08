#pragma once
#include <Arduino.h>

#include "Timer.h"

class SimpleLed
{
private:
    uint8_t _pin;
    bool _state;

public:
    SimpleLed(uint8_t pin);
    ~SimpleLed();
    bool blink(uint16_t blink_time);
    bool blink(uint16_t blink_time, uint16_t k);

    void toggle()
    {
        _state = !_state;
        digitalWrite(_pin, _state);
    }
    void set(bool state)
    {
        _state = state;
        digitalWrite(_pin, _state);
    }
    bool getState()
    {
        return _state;
    }
};

SimpleLed::SimpleLed(uint8_t pin)
{
    _pin = pin;
    _state = 0;
    pinMode(_pin, OUTPUT);
    digitalWrite(_pin, _state);
}

SimpleLed::~SimpleLed()
{
}

bool SimpleLed::blink(uint16_t blink_time)
{
    TMR16(blink_time, {
        toggle();
    });
    return _state;
}

bool SimpleLed::blink(uint16_t blink_time, uint16_t k)
{
    static uint16_t i = 0;
    if (i >= k)
    {
        i = 0;
        return false;
    }
    TMR16(blink_time, {
        toggle();
        if (!_state)
        {
            i++;
        }
    });
    return true;
}