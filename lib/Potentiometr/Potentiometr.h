#pragma once
#include <Arduino.h>

class Potentiometr
{
private:
    uint8_t _pin;
    bool _direction;
    uint16_t _map_min, _map_max;
    uint16_t _treshold, _changeTimer, _changeTime, _K_Reads, _prev_raw_value;
    int64_t _prev_value;

public:
    Potentiometr(uint8_t pin, bool direction = 1, uint16_t K_Reads = 20, uint16_t map_min = 0, uint16_t map_max = 1023, uint16_t treshold = 5, uint16_t changeTime = 200);
    ~Potentiometr();
    bool tick();
    uint16_t getValue();
    uint16_t readRaw();
    uint16_t getRawValue();
};

/**
 * @brief Constructs a new instance of the Potentiometr class.
 *
 * This constructor initializes a new instance of the Potentiometr class with the specified pin, direction, K_Reads, map_min, map_max, treshold, and changeTime.
 *
 * @param pin The Arduino pin connected to the potentiometer.
 * @param direction A boolean value indicating the direction of the potentiometer. If true, the potentiometer's value will be mapped from _map_min to _map_max. If false, the potentiometer's value will be mapped from _map_max to _map_min.
 * @param K_Reads The number of times to read the potentiometer value to improve stability.
 * @param map_min The minimum value to be mapped.
 * @param map_max The maximum value to be mapped.
 * @param treshold The threshold for detecting a change in the potentiometer value.
 * @param changeTime The time window for detecting a change in the potentiometer value.
 */
Potentiometr::Potentiometr(uint8_t pin, bool direction, uint16_t K_Reads, uint16_t map_min, uint16_t map_max, uint16_t treshold, uint16_t changeTime)
{
    _pin = pin;
    _direction = direction;
    _K_Reads = K_Reads;
    _map_min = map_min;
    _map_max = map_max;
    _treshold = treshold;
    _changeTime = changeTime;
    pinMode(_pin, INPUT_PULLUP);
    // pinMode(_pin, INPUT);
}

Potentiometr::~Potentiometr()
{
}

/**
 * @brief Checks for changes in the potentiometer value and updates the internal state accordingly.
 *
 * This function checks for changes in the potentiometer value by comparing the current value with the previous value. If the difference between the current and previous values is greater than the specified threshold, it updates the internal state and returns true. If the difference is less than or equal to the threshold and the time elapsed since the last change is less than the specified change time, it updates the internal state and returns true. Otherwise, it returns false.
 *
 * @param None
 * @return bool Returns true if a change is detected or if the time elapsed since the last change is less than the specified change time, false otherwise.
 */
bool Potentiometr::tick()
{
    uint16_t raw_value = readRaw();
    if (((int32_t)raw_value - _prev_raw_value < -_treshold) || ((int32_t)raw_value - _prev_raw_value > _treshold))
    {
        _changeTimer = millis();
        _prev_raw_value = raw_value;
        if (_direction)
            _prev_value = map(_prev_raw_value, 0, 1023, _map_min, _map_max);
        else
            _prev_value = map(_prev_raw_value, 0, 1023, _map_max, _map_min);
        return true;
    }
    if ((uint16_t)millis() - _changeTimer <= _changeTime && (((int32_t)raw_value - _prev_raw_value < -1) || ((int32_t)raw_value - _prev_raw_value > 1)))
    {
        _prev_raw_value = raw_value;
        if (_direction)
            _prev_value = map(_prev_raw_value, 0, 1023, _map_min, _map_max);
        else
            _prev_value = map(_prev_raw_value, 0, 1023, _map_max, _map_min);
        _changeTimer = millis();
        return true;
    }
    return false;
}

uint16_t Potentiometr::getValue()
{
    // tick();
    return _prev_value;
}
uint16_t Potentiometr::readRaw()
{
    uint64_t temp;
    for (uint16_t i = 0; i < _K_Reads; ++i)
    {
        temp += analogRead(_pin);
    }
    return temp / _K_Reads;
}
uint16_t Potentiometr::getRawValue()
{
    // tick();
    return _prev_raw_value;
}