#pragma once
#include <Arduino.h>
/// @brief Небольшая библиотека для удобного управления модулем переключателя (реле)
class Relay
{
private:
    byte _pin;
    bool _truth, _state, _nextState;
    uint64_t _min_change_time;
    void _setState(bool state);

public:
    Relay(byte pin, bool truth = 1, uint64_t min_change_time = 0);
    ~Relay();
    bool set(bool state);
    bool getState();
    void change();
    void setMinChangeTime(uint64_t t);
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
        if (!_min_change_time)
        {
            _setState(state);
            digitalWrite(_pin, _state);
            return true;
        }
        else
        {


        }
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
    digitalWrite(_pin, _state);
}

/// @brief 
/// @param t 
void Relay::setMinChangeTime(uint64_t t)
{
    _min_change_time = t;
}

void Relay::_setState(bool state)
{
    _state = state ^ !_truth;
}
