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

#define NO_FROST_ON_TIME ((uint64_t)1000 * 60 * 60 * 24 * 7 * 2) // 604 800 000 неделя
#define NO_FROST_OFF_TIME ((uint64_t)1000 * 60 * 60 * 2)         // 2 часа
#define NO_FROST_EEPROM_TIME ((uint64_t)1000 * 60 * 60 * 24)     // как часто писать в EEPROM время работы без NO_FROST
#define NO_FROST_MAX_FRIDGE_TEMPERATURE 10
#define NO_FROST_MAX_FREEZER_TEMPERATURE -10

#define MAX_WORK_TIME ((uint64_t)1000 * 60 * 60 * 3) // 3 часа работает
#define BREAK_TIMER ((uint64_t)1000 * 60 * 10)       // Перерыв
#define BREAK_RESET ((uint64_t)1000 * 60 * 5)        // Сброс таймера работы

#define RELAY_CHANGE_TIMER ((uint64_t)1000 * 60 * 5) // Как часто можно переключать реле

#define TEMP_FREEZER_MIN -30
#define TEMP_FREEZER_MAX -10
#define TEMP_FREEZER_DEFAULT_RANGE 1
#define TEMP_FREEZER_MAX_RANGE 5
#define TEMP_FREEZER_MIN_RANGE 1
#define TEMP_FREEZER_EXTRA 0

#define TEMP_FRIDGE_MIN -10
#define TEMP_FRIDGE_MAX 10
#define TEMP_FRIDGE_DEFAULT_RANGE 1
#define TEMP_FRIDGE_MAX_RANGE 5
#define TEMP_FRIDGE_MIN_RANGE 1

GyverDS18Single sensorFridge(PIN_SENS_FRIDGE, 0);
GyverDS18Single sensorFreezer(PIN_SENS_FREEZER, 0);

Potentiometr PotFreezerL(PIN_POT_L, true, 20, TEMP_FREEZER_MIN, TEMP_FREEZER_MAX, 5, 200);
Potentiometr PotFridgeR(PIN_POT_R, true, 20, TEMP_FRIDGE_MIN, TEMP_FRIDGE_MAX, 5, 200);

Relay compressor(PIN_RELAY, 1, RELAY_CHANGE_TIMER);
uint64_t compressorTimerWork = 0, compressorTimerChill = 0, compressorTimerAutoChill = 0;
bool chilling = 0;

SimpleLed ledFridgeR(PIN_LED_FRIDGE);
SimpleLed ledFreezerL(PIN_LED_FREEZER);

uint64_t FrostWorkTime = 0, FrostWorkTimer = 0, NoFrostTimer = 0, SetupTimer = 0; ///
bool noFrostState = 0;

EEManager EE_FrostWorkTime(FrostWorkTime, 60000);
EEManager EE_FrostWorkTimer(FrostWorkTimer, 60000);
EEManager EE_noFrostState(noFrostState, 60000);

int16_t tempFreezer = -99, tempFridge = -99,
        tempFreezerReq, tempFridgeReq,
        tempFridgeRange = TEMP_FRIDGE_DEFAULT_RANGE, tempFreezerRange = TEMP_FREEZER_DEFAULT_RANGE;
EEManager EE_tempFreezerRange(tempFreezerRange, 60000);
EEManager EE_tempFridgeRange(tempFridgeRange, 60000);
EEManager EE_tempFreezerReq(tempFreezerReq, 60000);
EEManager EE_tempFridgeReq(tempFridgeReq, 60000);

uint8_t mode = 0;
#define MODE_NORMAL 0
#define MODE_NO_FROST 1
#define MODE_SETUP_FREEZER 2
#define MODE_SETUP_FRIDGE 3

#define MODE_EXTRA 4

void setup()
{
    delay(1000);
#ifdef MY_DEBUG
    Serial.begin(115200);
#endif
    PotFreezerL.tick();
    PotFridgeR.tick();
    tempFreezerReq = PotFreezerL.getValue();
    tempFridgeReq = PotFridgeR.getValue();

    EE_FrostWorkTime.begin(0, 'b');
    EE_noFrostState.begin(EE_FrostWorkTime.nextAddr(), 'b');
    EE_tempFreezerRange.begin(EE_noFrostState.nextAddr(), 'b');
    EE_tempFridgeRange.begin(EE_tempFreezerRange.nextAddr(), 'b');
    EE_tempFridgeReq.begin(EE_tempFridgeRange.nextAddr(), 'b');
    EE_tempFreezerReq.begin(EE_tempFridgeReq.nextAddr(), 'b');
    EE_FrostWorkTimer.begin(EE_tempFreezerReq.nextAddr(), 'b');

    if (noFrostState)
    {
        mode = MODE_NO_FROST;
    }

    sensorFridge.requestTemp();
    sensorFreezer.requestTemp();
}

bool freezerTest()
{
    return (tempFreezer - tempFreezerReq) > tempFreezerRange;
}

bool fridgeTest()
{
    return (tempFridge - tempFridgeReq) > tempFridgeRange;
}

void tempUpdate()
{
    if (sensorFridge.ready())
    { // измерения готовы по таймеру
        if (sensorFridge.readTemp())
        { // если чтение успешно
            tempFridge = sensorFridge.getTempInt();
        }
        else
        {
            DD("Error sensorFridge.readTemp()  Loop");
        }
        sensorFridge.requestTemp(); // запрос следующего измерения
    }
    if (sensorFreezer.ready())
    { // измерения готовы по таймеру
        if (sensorFreezer.readTemp())
        { // если чтение успешно
            tempFreezer = sensorFreezer.getTempInt();
        }
        else
        {
            DD("Error sensorFreezer.readTemp()  Loop");
        }
        sensorFreezer.requestTemp(); // запрос следующего измерения
    }
}
void potAndModUpdate()
{

    if (PotFreezerL.tick())
    {
        mode = MODE_SETUP_FREEZER;
        SetupTimer = millis();
        DD("MODE_SETUP_FREEZER");
        DDD("POT_L: ");
        DDD(PotFreezerL.getRawValue());
        DDD(" (");
        DDD(PotFreezerL.getValue());
        DD(")");
    }
    if (PotFridgeR.tick())
    {
        SetupTimer = millis();
        mode = MODE_SETUP_FRIDGE;
        DD("MODE_SETUP_FRIDGE");
        DDD("POT_R: ");
        DDD(PotFridgeR.getRawValue());
        DDD(" (");
        DDD(PotFridgeR.getValue());
        DD(")");
    }
}
void loop()
{
    EE_FrostWorkTime.tick();
    EE_noFrostState.tick();
    EE_tempFreezerRange.tick();
    EE_tempFridgeRange.tick();
    EE_tempFreezerRange.tick();
    EE_tempFreezerReq.tick();
    EE_tempFridgeReq.tick();
    EE_FrostWorkTimer.tick();

    TMR64(NO_FROST_EEPROM_TIME,
          {
              FrostWorkTime += NO_FROST_EEPROM_TIME;
              EE_FrostWorkTime.update();
              DD("EE_FrostWorkTime.update");
          });
    if ((uint64_t)(FrostWorkTime - FrostWorkTimer) >= NO_FROST_ON_TIME)
    {
        FrostWorkTimer = FrostWorkTime;
        EE_FrostWorkTimer.update();
        DD("EE_FrostWorkTimer.update");
        noFrostState = 1;
        mode = MODE_NO_FROST;
        NoFrostTimer = millis();
        EE_noFrostState.update();
        DD("EE_noFrostState.update");
    }

    tempUpdate();
    potAndModUpdate();
    switch (mode)
    {
    case MODE_NO_FROST:
    {
        while (noFrostState)
        {
            compressor.tick();
            compressor.off();
            ledFridgeR.blink(500);
            ledFreezerL.blink(2000);
            tempUpdate();
            if ((tempFreezer >= NO_FROST_MAX_FREEZER_TEMPERATURE) || (tempFridge >= NO_FROST_MAX_FRIDGE_TEMPERATURE))
            {
                noFrostState = 0;
            }
            if ((uint64_t)(millis() - NoFrostTimer) >= NO_FROST_OFF_TIME)
            {
                noFrostState = 0;
            }
        }
        mode = MODE_NORMAL;
        DD("MODE_NORMAL");
        EE_noFrostState.update();
        NoFrostTimer = millis();
        FrostWorkTimer = millis();
        FrostWorkTime = millis();
        EE_FrostWorkTime.update();
        EE_FrostWorkTimer.update();
    }
    break;
    case MODE_SETUP_FREEZER:
    {
        int16_t temp = 0, range = 0;
        bool rangeChanged = false;
        while ((uint64_t)millis() - SetupTimer <= 5000)
        {
            ledFreezerL.blink(1000);
            bool Pot1 = PotFreezerL.tick();
            bool Pot2 = PotFridgeR.tick();
            if (Pot1 || Pot2)
            {
                SetupTimer = millis();
            }
            temp = PotFreezerL.getValue();
            if (Pot2)
            {
                ledFridgeR.blink(200);
                range = map(PotFridgeR.getRawValue(), 0, 1023, TEMP_FREEZER_MIN_RANGE, TEMP_FREEZER_MAX_RANGE);
                rangeChanged = 1;
            }
            if (!rangeChanged)
            {
                if (temp < tempFreezer)
                {
                    compressor.setNow(1);
                }
                else
                {
                    compressor.setNow(0);
                }
            }
        }
        if (rangeChanged)
        {
            tempFreezerRange = range;
            EE_tempFreezerRange.update();

            uint16_t speed = 200;
            if (range < 0)
            {
                speed = 500;
                range = -range;
            }
            uint8_t KL = (range / 10) % 10, KR = range % 10;
            while (ledFreezerL.blink(speed, KL))
            {
            }
            while (ledFridgeR.blink(speed, KR))
            {
            }
        }
        else
        {
            tempFreezerReq = temp;
            EE_tempFreezerReq.update();

            uint16_t speed = 200;
            if (temp < 0)
            {
                speed = 500;
                temp = -temp;
            }
            uint8_t KL = (temp / 10) % 10, KR = temp % 10;
            while (ledFreezerL.blink(speed, KL))
            {
            }
            while (ledFridgeR.blink(speed, KR))
            {
            }
        }
        mode = MODE_NORMAL;
    }
    break;
    case MODE_SETUP_FRIDGE:
    {
        int16_t temp = 0, range = 0;
        bool rangeChanged = false;
        while ((uint64_t)millis() - SetupTimer <= 5000)
        {
            ledFridgeR.blink(1000);
            bool Pot2 = PotFreezerL.tick();
            bool Pot1 = PotFridgeR.tick();
            if (Pot1 || Pot2)
            {
                SetupTimer = millis();
            }
            temp = PotFridgeR.getValue();
            if (Pot2)
            {
                ledFreezerL.blink(200);
                range = map(PotFreezerL.getRawValue(), 0, 1023, TEMP_FRIDGE_MIN_RANGE, TEMP_FRIDGE_MAX_RANGE);
                rangeChanged = 1;
            }
            if (!rangeChanged)
            {
                if (temp < tempFridge)
                {
                    compressor.setNow(1);
                }
                else
                {
                    compressor.setNow(0);
                }
            }
        }
        if (rangeChanged)
        {
            tempFreezerRange = range;
            EE_tempFreezerRange.update();

            uint16_t speed = 200;
            if (range < 0)
            {
                speed = 500;
                range = -range;
            }
            uint8_t KL = (range / 10) % 10, KR = range % 10;
            while (ledFreezerL.blink(speed, KL))
            {
            }
            while (ledFridgeR.blink(speed, KR))
            {
            }
        }
        else
        {
            tempFreezerReq = temp;
            EE_tempFreezerReq.update();

            uint16_t speed = 200;
            if (temp < 0)
            {
                speed = 500;
                temp = -temp;
            }
            uint8_t KL = (temp / 10) % 10, KR = temp % 10;
            while (ledFreezerL.blink(speed, KL))
            {
            }
            while (ledFridgeR.blink(speed, KR))
            {
            }
        }
        mode = MODE_NORMAL;
    }
    break;
    default:
    {
        if (freezerTest())
        {
            if (!chilling)
                compressor.on();

            if (compressor.getState())
            {
                ledFreezerL.set(1);
            }
            else
            {
                ledFreezerL.blink(1000);
            }
        }
        else
        {
            if (!fridgeTest())
            {
                compressor.off();
                static uint64_t tmrBREAK_RESET = 0;
                if ((uint64_t)(millis() - tmrBREAK_RESET) >= (BREAK_RESET + 10000))
                {
                    tmrBREAK_RESET = millis();
                    compressorTimerAutoChill = millis();
                }
            }
            ledFreezerL.set(0);
        }
        if (fridgeTest())
        {
            if (!chilling)
                compressor.on();
            if (compressor.getState())
            {
                ledFridgeR.set(1);
            }
            else
            {
                ledFridgeR.blink(1000);
            }
        }
        else
        {
            if (!freezerTest())
            {
                static uint64_t tmrBREAK_RESET = 0;
                if ((uint64_t)(millis() - tmrBREAK_RESET) >= (BREAK_RESET + 10000))
                {
                    tmrBREAK_RESET = millis();
                    compressorTimerAutoChill = millis();
                }
                compressor.off();
            }
            ledFridgeR.set(0);
        }
        if ((uint64_t)(millis() - compressorTimerWork) >= MAX_WORK_TIME)
        {
            chilling = 1;
            compressorTimerWork = millis() + BREAK_TIMER;
            compressorTimerChill = millis();
        }
        if (chilling && ((uint64_t)(millis() - compressorTimerChill) >= BREAK_TIMER))
        {
            chilling = 0;
            compressorTimerWork = millis();
        }
        if (!compressor.getState() && !chilling && ((uint64_t)(millis() - compressorTimerAutoChill) >= BREAK_RESET))
        {
            compressorTimerWork = millis();
        }
    }
    break;
    }
}
