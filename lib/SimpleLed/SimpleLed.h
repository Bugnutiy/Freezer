#pragma once
#include <Arduino.h>

// #include "Timer.h"

class SimpleLed
{
private:
    uint8_t _pin;
    bool _state;
    uint16_t _tmr_blink_time, _blink_i;

public:
    SimpleLed(uint8_t pin);
    ~SimpleLed();
    bool blink(uint16_t blink_time);
    bool blink(uint16_t blink_time, uint16_t k);
    void reset();

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
    if ((uint16_t)millis() - _tmr_blink_time >= blink_time)
    {
        _tmr_blink_time = millis();
        toggle();
        _blink_i = 0;
    }
    return _state;
}

bool SimpleLed::blink(uint16_t blink_time, uint16_t k)
{
    if (_blink_i == k)
    {
        _blink_i = 0;
        return false;
    }
    if ((uint16_t)millis() - _tmr_blink_time >= blink_time)
    {
        _tmr_blink_time = millis();
        toggle();
        if (!_state)
        {
            _blink_i++;
        }
    }
    return true;
}

void SimpleLed::reset()
{
    _tmr_blink_time = millis();
    _blink_i = 0;
}
