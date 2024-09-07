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
    void blink(uint16_t blink_time);
    void toggle()
    {
        _state = !_state;
        digitalWrite(_pin, _state);
    }
    void setState(bool state)
    {
        _state = state;
        digitalWrite(_pin, _state);
    }
    bool getState(){
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

void SimpleLed::blink(uint16_t blink_time)
{
    TMR16(blink_time, {
        toggle();
    });
}
