#include <Arduino.h>

#include "GyverDS18Single.h"
#include "Relay.h"
#define MY_DEBUG
#include "My_Debug.h"

#define PIN_SENS_FRIDGE 8
#define PIN_SENS_FREEZER 9

#define PIN_RELAY 7

#define PIN_LED_FRIDGE 10
#define PIN_LED_FREEZER 11

#define NO_FROST_ON_TIMER 1000 * 60 * 60 * 24 * 7 // 604 800 000 неделя
#define NO_FROST_OFF_TIMER 1000 * 60 * 60 * 2     // 2 часа 3600000
#define NO_FROST_EEPROM_TIMER 1000 * 60 * 60 * 24 // День
#define NO_FROST_MAX_FRIDGE_TEMPERATURE 10
#define NO_FROST_MAX_FREEZER_TEMPERATURE -5

#define MAX_WORK_TIME 1000 * 60 * 60 * 3 // 3 часа работает
#define BREAK_TIMER 1000 * 60 * 10       // Перерыв
#define BREAK_RESET 1000 * 60 * 5        // Сброс таймера работы

#define RELAY_SET_TIMER 1000 * 60 * 5 // Как часто можно переключать реле

GyverDS18Single sensorFridge();
GyverDS18Single sensorFrizer();

void setup()
{
}

void loop()
{
}
