#include <Arduino.h>
#include "Timer.h"
#include "GyverDS18Single.h"
#include "Relay.h"
#include "Potentiometr.h"
#include "SimpleLed.h"
#include <EEManager.h>
#define MY_DEBUG
#include "My_Debug.h"

#define PIN_SENS_FRIDGE 8
#define PIN_SENS_FREEZER 9

#define PIN_RELAY 7

#define PIN_LED_FRIDGE 10
#define PIN_LED_FREEZER 11

#define PIN_POT_L 5
#define PIN_POT_R 6

#define NO_FROST_ON_TIMER ((uint64_t)1000 * 60 * 60 * 24 * 7 * 2) // 604 800 000 неделя
#define NO_FROST_OFF_TIMER ((uint64_t)1000 * 60 * 60 * 2)         // 2 часа
#define NO_FROST_EEPROM_TIMER ((uint64_t)1000 * 60 * 60 * 24)     // как часто писать в EEPROM время работы без NO_FROST
#define NO_FROST_MAX_FRIDGE_TEMPERATURE 10
#define NO_FROST_MAX_FREEZER_TEMPERATURE -10

#define MAX_WORK_TIME ((uint64_t)1000 * 60 * 60 * 3) // 3 часа работает
#define BREAK_TIMER ((uint64_t)1000 * 60 * 10)       // Перерыв
#define BREAK_RESET ((uint64_t)1000 * 60 * 5)        // Сброс таймера работы

#define RELAY_CHANGE_TIMER ((uint64_t)1000 * 60 * 5) // Как часто можно переключать реле

#define TEMP_FREEZER_MIN -30
#define TEMP_FREEZER_MAX -10
#define TEMP_FREEZER_DEFAULT_RANGE 5
#define TEMP_FREEZER_MAX_RANGE 10
#define TEMP_FREEZER_MIN_RANGE 1

#define TEMP_FRIDGE_MIN -10
#define TEMP_FRIDGE_MAX 10
#define TEMP_FRIDGE_DEFAULT_RANGE 5
#define TEMP_FRIDGE_MAX_RANGE 10
#define TEMP_FRIDGE_MIN_RANGE 1

GyverDS18Single sensorFridge(PIN_SENS_FRIDGE, 0);
GyverDS18Single sensorFrizer(PIN_SENS_FREEZER, 0);

Relay compressor(PIN_RELAY, 1, RELAY_CHANGE_TIMER);

SimpleLed ledFridge(PIN_LED_FRIDGE);
SimpleLed ledFreezer(PIN_LED_FREEZER);

uint64_t FrostWorkTime = 0;
bool noFrostState = 0;

EEManager EE_FrostWorkTime(FrostWorkTime);
EEManager EE_noFrostState(noFrostState);
void setup()
{
#ifdef MY_DEBUG
    Serial.begin(115200);
#endif
    EE_FrostWorkTime.begin(0, 'b');
    EE_noFrostState.begin(EE_FrostWorkTime.nextAddr(), 'b');
}

uint8_t mode = 0;
#define MODE_NORMAL 0
#define MODE_NO_FROST 1
#define MODE_SETUP 2
#define MODE_EXTRA 3

void loop()
{
    EE_FrostWorkTime.tick();
    EE_noFrostState.tick();

    TMR64(NO_FROST_EEPROM_TIMER,
          {
              FrostWorkTime += millis();
              EE_FrostWorkTime.update();
          });
}
