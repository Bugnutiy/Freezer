#pragma once
#include <Arduino.h>

/// @brief Небольшая библиотека для удобного управления модулем переключателя (реле)
class Relay
{
private:
    byte _pin;
    bool _truth, _state, _nextState;
    uint64_t _min_change_time, _tmr_min_change_time;
    void _setState(bool state, bool now = 0);

public:
    Relay(byte pin, bool truth = 1, uint64_t min_change_time = 0);
    ~Relay();
    bool set(bool state);
    bool setNow(bool state);
    bool getState();
    void change();
    void setMinChangeTime(uint64_t t);

    void resetTimer();

    bool ready();
    void tick();
    bool on();
    bool off();
};

/// @brief Конструктор класса реле
/// @param pin номер пина управления реле
/// @param truth Какой сигнал является сигналом включения 0 или 1 default = 1
/// @param min_change_time Задержка переключений в мс
Relay::Relay(byte pin, bool truth, uint64_t min_change_time)
{
    pinMode(pin, OUTPUT);
    _pin = pin;
    _truth = truth;
    digitalWrite(_pin, !_truth);
    _state = !_truth;
    _min_change_time = min_change_time;
    _nextState = !_truth;
}

Relay::~Relay()
{
}

/// @brief Управление реле
/// @param state true - включить. false - выключить
/// @return Возвращает true если произошло изменение состояния
bool Relay::set(bool state)
{
    if (getState() != state)
    {
        _setState(state);
        return true;
    }
    return false;
}

bool Relay::setNow(bool state)
{
    if (getState() != state)
    {
        _setState(state, 1);
        return true;
    }
    return false;
}
/// @brief Узнать состояние реле
/// @return true - включено, false - выключено
bool Relay::getState()
{
    return _state ^ !_truth;
}

/// @brief Переключить реле
void Relay::change()
{
    _setState(!getState());
}

/**
 * @brief Sets the minimum change time for the relay.
 *
 * This function sets the minimum time interval between two consecutive changes of the relay's state.
 * If the time elapsed since the last change is less than the minimum change time, the relay will not be changed.
 *
 * @param min_change_time The minimum change time in milliseconds.
 */
void Relay::setMinChangeTime(uint64_t min_change_time)
{
    _min_change_time = min_change_time;
}

void Relay::resetTimer()
{
    _tmr_min_change_time = -_min_change_time;
}

/**
 * @brief Checks if the relay is ready to be changed.
 *
 * This function checks if the relay is ready to be changed based on the minimum change time.
 * If the minimum change time is not set, the function returns true, indicating that the relay is ready.
 * If the minimum change time is set, the function checks if the time elapsed since the last change is greater than or equal to the minimum change time.
 * If the time elapsed is greater than or equal to the minimum change time, the function returns true, indicating that the relay is ready.
 * Otherwise, the function returns false, indicating that the relay is not ready.
 *
 * @return true if the relay is ready to be changed, false otherwise.
 */
bool Relay::ready()
{
    if (!_min_change_time)
        return true;
    if (millis() - _tmr_min_change_time >= _min_change_time)
    {
        return true;
    }
    return false;
}

void Relay::tick()
{
    if (ready())
    {
        if (_state != _nextState)
        {
            _state = _nextState;
            _tmr_min_change_time = millis();
            digitalWrite(_pin, _state);
        }
    }
}

bool Relay::on()
{
    return set(1);
}

bool Relay::off()
{
    return set(0);
}

void Relay::_setState(bool state, bool now)
{
    // if (!_min_change_time)
    // {
    //     _state = state ^ !_truth;
    //     digitalWrite(_pin, _state);
    // }
    if (ready() || now)
    {
        _state = state ^ !_truth;
        _tmr_min_change_time = millis();
        digitalWrite(_pin, _state);
    }
    else
    {
        _nextState = state ^ !_truth;
    }
}
