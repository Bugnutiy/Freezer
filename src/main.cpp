#include <Arduino.h>
#define MY_DEBUG
#include "My_Debug.h"
#include "Timer.h"
#include <GyverDS18Single.h>
#include "Relay.h"
#include "Potentiometr.h"
#include "SimpleLed.h"
#include <EEManager.h>
#include <GyverButton.h>

#define PIN_SENS_FRIDGE 2
#define PIN_SENS_FREEZER 3

#define PIN_COMPRESSOR 4
#define PIN_NO_FROST 5

#define PIN_LED_FRIDGE 6
#define PIN_LED_FREEZER 7

#define PIN_POT_L A0
#define PIN_POT_R A1
#define PIN_BUTTON 10

#define NO_FROST_ON_TEMP_TRIGGER_DEFAULT -15
#define NO_FROST_ON_TEMP_TRIGGER_MAX 5
#define NO_FROST_ON_TEMP_TRIGGER_MIN -20

#define NO_FROST_OFF_TEMP_TRIGGER_DEFAULT 5
#define NO_FROST_OFF_TEMP_TRIGGER_MAX 15
#define NO_FROST_OFF_TEMP_TRIGGER_MIN -10

#define NO_FROST_OFF_TEMP_FREEZER -6

#define NO_FROST_ON_TIME 24    // часа
#define NO_FROST_OFF_TIME 2    // часа
#define NO_FROST_EEPROM_TIME 3 // часа

const uint32_t MAX_WORK_TIME = ((uint32_t)1000 * 60 * 60 * 4); // 4 часа работает
const uint32_t BREAK_TIME = ((uint32_t)1000 * 60 * 20);        // Перерыв
const uint32_t BREAK_RESET = ((uint32_t)1000 * 60 * 10);       // Естесственный перерыв

const uint32_t RELAY_CHANGE_TIME = ((uint32_t)1000 * 60 * 5); // Как часто можно переключать реле

#define TEMP_FREEZER_REQ_MIN -30
#define TEMP_FREEZER_REQ_MAX -10
#define TEMP_FREEZER_DEFAULT_RANGE 1
#define TEMP_FREEZER_REQ_MAX_RANGE 10
#define TEMP_FREEZER_REQ_MIN_RANGE 1
// #define TEMP_FREEZER_EXTRA -5

#define TEMP_FRIDGE_REQ_MIN -10
#define TEMP_FRIDGE_REQ_MAX 10
#define TEMP_FRIDGE_DEFAULT_RANGE 1
#define TEMP_FRIDGE_REQ_MAX_RANGE 10
#define TEMP_FRIDGE_REQ_MIN_RANGE 1

#define EEKEY 'x'

const uint32_t hour = (uint32_t)1000 * 60 * 60;

uint16_t noFrostOnTimer, noFrostOffTimer, noFrostEEPROMTimer;
int16_t noFrostOnTempTrigger = NO_FROST_ON_TEMP_TRIGGER_DEFAULT, noFrostOffTempTrigger = NO_FROST_OFF_TEMP_TRIGGER_DEFAULT;
EEManager EE_noFrostOnTimer(noFrostOnTimer, 60000);
EEManager EE_noFrostOnTempTrigger(noFrostOnTempTrigger, 60000);
EEManager EE_noFrostOffTempTrigger(noFrostOffTempTrigger, 60000);

GyverDS18Single sensorFridge(PIN_SENS_FRIDGE, 0);
GyverDS18Single sensorFreezer(PIN_SENS_FREEZER, 0);
GButton button(PIN_BUTTON);

Potentiometr PotFreezerL(PIN_POT_L, true, 20, TEMP_FREEZER_REQ_MIN, TEMP_FREEZER_REQ_MAX, 10, 200);
Potentiometr PotFridgeR(PIN_POT_R, true, 20, TEMP_FRIDGE_REQ_MIN, TEMP_FRIDGE_REQ_MAX, 10, 200);

Relay relay_compressor(PIN_COMPRESSOR, 0, RELAY_CHANGE_TIME), relay_no_frost(PIN_NO_FROST, 0, 10000);
uint32_t TimerCompressorWork = 0, TimerChill = 0, TimerAutoChill = 0;
bool chilling = 0, AutoChill = false;

SimpleLed ledFridgeR(PIN_LED_FRIDGE);
SimpleLed ledFreezerL(PIN_LED_FREEZER);
SimpleLed ledBuiltin(13);

uint32_t SetupTimer = 0; ///

int16_t tempFreezer = -99, tempFridge = -99,
        tempFreezerReq, tempFridgeReq,
        tempFridgeRange = TEMP_FRIDGE_DEFAULT_RANGE, tempFreezerRange = TEMP_FREEZER_DEFAULT_RANGE,
        noFrostTempTrigger = NO_FROST_ON_TEMP_TRIGGER_DEFAULT;
EEManager EE_tempFreezerRange(tempFreezerRange, 60000);
EEManager EE_tempFridgeRange(tempFridgeRange, 60000);
EEManager EE_tempFreezerReq(tempFreezerReq, 60000);
EEManager EE_tempFridgeReq(tempFridgeReq, 60000);

uint8_t mode = 0;

#define MODE_NORMAL 0
#define MODE_NO_FROST 1
#define MODE_SETUP_FREEZER 2
#define MODE_SETUP_FRIDGE 3

// #define MODE_EXTRA 4

void setup()
{
#ifdef MY_DEBUG
    Serial.begin(115200);
#endif
    button.setDebounce(100);
    button.setDirection(NORM_OPEN);
    button.setTickMode(1);
    button.setType(HIGH_PULL);
    delay(1000);
    button.tick();

    PotFreezerL.tick();
    PotFridgeR.tick();

    tempFreezerReq = PotFreezerL.getValue();
    tempFridgeReq = PotFridgeR.getValue();
    sensorFridge.requestTemp();
    sensorFreezer.requestTemp();

    while (!sensorFridge.ready())
    {
        ledBuiltin.blink(200);
        ledFridgeR.blink(200);
        // DD("Waiting for sensorFridge to be ready", 20000);
    }
    if (sensorFridge.readTemp())
    { // если чтение успешно
        tempFridge = sensorFridge.getTempInt();
        // DDD("SensorFridge: ");
        // DD(tempFridge);
    }
    while (!sensorFreezer.ready())
    {
        ledBuiltin.blink(200);
        ledFreezerL.blink(200);
        // DD("Waiting for sensorFreezer to be ready", 20000);
    }
    if (sensorFreezer.readTemp())
    {
        tempFreezer = sensorFreezer.getTempInt();
        // DDD("SensorFreezer: ");
        // DD(tempFreezer);
    }
    sensorFridge.requestTemp();
    sensorFreezer.requestTemp();

    EE_noFrostOnTimer.begin(0, EEKEY);
    EE_noFrostOnTempTrigger.begin(EE_noFrostOnTimer.nextAddr(), EEKEY);
    EE_tempFreezerRange.begin(EE_noFrostOnTempTrigger.nextAddr(), EEKEY);
    EE_tempFridgeRange.begin(EE_tempFreezerRange.nextAddr(), EEKEY);
    EE_tempFridgeReq.begin(EE_tempFridgeRange.nextAddr(), EEKEY);
    EE_tempFreezerReq.begin(EE_tempFridgeReq.nextAddr(), EEKEY);
    EE_noFrostOffTempTrigger.begin(EE_tempFreezerReq.nextAddr(), EEKEY);

    // DD("EEPROM initialized");

    DD("Setup is DONE!");
    DDD("noFrostOnTimer: ");
    DD(noFrostOnTimer);
    DDD("noFrostOnTempTrigger: ");
    DD(noFrostOnTempTrigger);
    DDD("noFrostOffTempTrigger: ");
    DD(noFrostOffTempTrigger);
    DDD("tempFreezerRange: ");
    DD(tempFreezerRange);
    DDD("tempFridgeRange: ");
    DD(tempFridgeRange);
    DDD("tempFreezerReq: ");
    DD(tempFreezerReq);
    DDD("tempFridgeReq: ");
    DD(tempFridgeReq);

    ledFridgeR.set(0);
    while (millis() < 120000)
    {
        ledFreezerL.blink(2000);
        if (button.state())
        {
            ledFreezerL.reset();
            ledFridgeR.reset();
            while (button.state() && millis() < 5000)
            {
                ledFridgeR.blink(200);
                ledFreezerL.blink(200);
            }
            break;
        }
        // DD("Waiting 20s before starting", 1000);
    }
    ledFreezerL.reset();
    ledFridgeR.reset();
    ledBuiltin.reset();
    relay_compressor.resetTimer();
    relay_no_frost.resetTimer();
    // DD("Starting...");
}

/**
 * @brief Checks if the temperature of the fridge is within the specified range or less then the specified value.
 *
 * @param[in] req (optional) If true, checks if the temperature is less then the specified value. If false, checks if the temperature is within the specified range.
 * @return True if the temperature is within the specified range or less then the specified value, false otherwise.
 */
bool freezerTest(bool req = 0)
{
    if (req)
        return tempFreezer <= tempFreezerReq;
    return (tempFreezer - tempFreezerReq) <= tempFreezerRange;
}

/**
 * @brief Checks if the temperature of the fridge is within the specified range or less then the specified value.
 *
 * @param[in] req (optional) If true, checks if the temperature is less then the specified value. If false, checks if the temperature is within the specified range.
 * @return True if the temperature is within the specified range or less then the specified value, false otherwise.
 */
bool fridgeTest(bool req = 0)
{
    if (req)
        return tempFridge <= tempFridgeReq;
    return (tempFridge - tempFridgeReq) <= tempFridgeRange;
}

uint8_t error;
void tempUpdate()
{
    TMR16(5000, {
        if (sensorFridge.ready())
        { // измерения готовы по таймеру
            if (sensorFridge.readTemp())
            { // если чтение успешно
                tempFridge = sensorFridge.getTempInt();
                // DDD("tempFridge:");
                // DD(tempFridge);
                error = 0;
            }
            else
            {
                tempFridge = tempFridgeReq;
                error = 1;
                // DD("Error sensorFridge.readTemp()  Loop");
            }
            sensorFridge.requestTemp(); // запрос следующего измерения
        }
        if (sensorFreezer.ready())
        { // измерения готовы по таймеру
            if (sensorFreezer.readTemp())
            { // если чтение успешно
                tempFreezer = sensorFreezer.getTempInt();
                // DDD("tempFreezer: ");
                // DD(tempFreezer);
                error = 0;
            }
            else
            {
                tempFreezer = tempFreezerReq;
                error = 2;
                // DD("Error sensorFreezer.readTemp()  Loop");
            }
            sensorFreezer.requestTemp(); // запрос следующего измерения
        }
    });
    if (error == 1)
    {
        ledFridgeR.blink(100);
    }
    if (error == 2)
    {
        ledFreezerL.blink(100);
    }
}
void potAndModUpdate()
{
    if (PotFreezerL.tick())
    {
        mode = MODE_SETUP_FREEZER;
        SetupTimer = millis();
    }
    if (PotFridgeR.tick())
    {
        SetupTimer = millis();
        mode = MODE_SETUP_FRIDGE;
    }
}

void noFrostFunc()
{
    // static bool nofrost_flag = 0;
    TMR32_NEXT(hour, {
        noFrostOnTimer++;
        noFrostEEPROMTimer++;
        // DD("noFrostFunc() -> hour timer");
    });
    if (noFrostOnTimer >= NO_FROST_ON_TIME && !relay_no_frost.getState() && !relay_compressor.getState())
    {
        // mode = MODE_NO_FROST;
        // //DD("noFrostOnTimer >= NO_FROST_ON_TIME && !relay_no_frost.getState()");
        noFrostOnTimer = 0;
        noFrostOffTimer = 0;
        relay_no_frost.on();
    }
    if (noFrostEEPROMTimer >= NO_FROST_EEPROM_TIME)
    {
        EE_noFrostOnTimer.update();
        // //DD("noFrostEEPROMTimer >= NO_FROST_EEPROM_TIME");
        noFrostEEPROMTimer = 0;
    }

    if ((tempFridge <= noFrostOnTempTrigger) && !relay_compressor.getState())
    {

        // DD("noFrostFunc() -> noFrostOnTempTrigger", 1000);
        noFrostOnTimer = 0;
        noFrostOffTimer = 0;
        // nofrost_flag=1;
        relay_no_frost.on();
    }
    if (relay_no_frost.getState())
    {
        TMR32_NEXT(hour, {
            // DD("noFrostFunc() -> noFrostOffTimer++");
            noFrostOffTimer++;
        });
        if (noFrostOffTimer >= NO_FROST_OFF_TIME)
        {
            // DD("noFrostFunc() -> noFrostOffTimer >= NO_FROST_OFF_TIME");
            relay_no_frost.off();
            noFrostOffTimer = 0;
            noFrostOnTimer = 0;
        }
        if (tempFridge >= noFrostOffTempTrigger)
        {
            // DD("noFrostFunc() -> noFrostOffTempTrigger");
            noFrostOnTimer = 0;
            noFrostOffTimer = 0;
            relay_no_frost.off();
        }
        if (tempFreezer >= NO_FROST_OFF_TEMP_FREEZER)
        {
            // DD("noFrostFunc() -> tempFreezer >= NO_FROST_OFF_TEMP_FREEZER");
            noFrostOnTimer = 0;
            noFrostOffTimer = 0;
            relay_no_frost.off();
        }
        ledFridgeR.blink(300);
    }
}

// Левый регулятор настраивает noFrostOnTempTrigger, правый - noFrostOffTempTrigger
void ButtonHold()
{
    // DD("Extra button holded");
    while (button.state())
    {
        ledFreezerL.blink(200);
        ledFridgeR.blink(200);
        // DD("Extra Button hold", 20000);
        relay_compressor.set(1);
        relay_no_frost.set(0);
        noFrostOffTimer = 0;
        noFrostOnTimer = 0;
        potAndModUpdate();
        if (mode == MODE_SETUP_FREEZER)
        {
            ledFridgeR.set(0);
            int16_t temp = 0;
            while ((uint32_t)millis() - SetupTimer <= 5000)
            {
                // DD("Setting up noFrostOnTriggerTemp", 1000);
                bool Pot1 = PotFreezerL.tick();
                // bool Pot2 = PotFridgeR.tick();
                uint16_t speedBlinkL = map(PotFreezerL.getRawValue(), 0, 1023, 1000, 100);
                ledFreezerL.blink(speedBlinkL);
                if (Pot1)
                {
                    SetupTimer = millis();
                }
                if (Pot1)
                {
                    temp = map(PotFreezerL.getRawValue(), 0, 1023, NO_FROST_ON_TEMP_TRIGGER_MIN, NO_FROST_ON_TEMP_TRIGGER_MAX);
                    // DDD("temp: ", 500);
                    // DD(temp, 500);
                }
            }
            if (noFrostOnTempTrigger != temp)
            {
                noFrostOnTempTrigger = temp;
                EE_noFrostOnTempTrigger.update();
            }
            // DDD("noFrostOnTempTrigger: ");
            // DD(noFrostOnTempTrigger);
            ledFreezerL.set(0);
            ledFridgeR.set(0);
            delay(2000);
            uint16_t speed = 500;
            if (temp < 0)
            {
                speed = 1000;
                temp = -temp;
            }
            uint8_t KL = (temp / 10) % 10, KR = temp % 10;

            while (ledFreezerL.blink(speed, KL))
            {
            }
            while (ledFridgeR.blink(speed, KR))
            {
            }
            delay(2000);
            ledFreezerL.set(1);
            ledFridgeR.set(1);
            delay(100);
            ledFreezerL.set(0);
            ledFridgeR.set(0);
            delay(1000);
            mode = MODE_NORMAL;
        }
        if (mode == MODE_SETUP_FRIDGE)
        {
            ledFreezerL.set(0);
            int16_t temp = 0;
            while (millis() - SetupTimer <= 5000)
            {
                // DD("Setting up noFrostOffTriggerTemp");
                bool Pot2 = PotFridgeR.tick();
                // bool Pot1 = PotFreezerL.tick();
                uint16_t speedBlinkR = map(PotFridgeR.getRawValue(), 0, 1023, 1000, 100);
                ledFridgeR.blink(speedBlinkR);
                if (Pot2)
                {
                    SetupTimer = millis();
                }
                if (Pot2)
                {
                    temp = map(PotFridgeR.getRawValue(), 0, 1023, NO_FROST_OFF_TEMP_TRIGGER_MIN, NO_FROST_OFF_TEMP_TRIGGER_MAX);
                    // DDD("temp: ", 500);
                    // DD(temp, 500);
                }
            }
            if (noFrostOffTempTrigger != temp)
            {
                noFrostOffTempTrigger = temp;
                EE_noFrostOffTempTrigger.update();
            }
            // DDD("noFrostOffTempTrigger: ");
            // DD(noFrostOffTempTrigger);
            ledFreezerL.set(0);
            ledFridgeR.set(0);
            delay(2000);
            uint16_t speed = 500;
            if (temp < 0)
            {
                speed = 1000;
                temp = -temp;
            }
            uint8_t KL = (temp / 10) % 10, KR = temp % 10;
            while (ledFreezerL.blink(speed, KL))
            {
            }
            while (ledFridgeR.blink(speed, KR))
            {
            }
            delay(2000);
            ledFreezerL.set(1);
            ledFridgeR.set(1);
            delay(100);
            ledFreezerL.set(0);
            ledFridgeR.set(0);
            delay(1000);
            mode = MODE_NORMAL;
        }
    }
    // DD("Extra button released");
    mode = MODE_NORMAL;
}

bool flag = 0;
void loop()
{
    // ledBuiltin.blink(1000);
    EE_noFrostOnTimer.tick();
    EE_noFrostOnTempTrigger.tick();
    EE_tempFreezerRange.tick();
    EE_tempFridgeRange.tick();
    EE_tempFreezerReq.tick();
    EE_tempFridgeReq.tick();
    EE_noFrostOffTempTrigger.tick();

    relay_compressor.tick();
    relay_no_frost.tick();

    tempUpdate();
    potAndModUpdate();
    noFrostFunc();

    if (button.state())
    {
        ledFreezerL.reset();
        ledFridgeR.reset();
        ButtonHold();
    }
    switch (mode)
    {
    default:
    {
        if (!fridgeTest() || !freezerTest())
        {
            if (!chilling && !relay_no_frost.getState())
            {
                relay_compressor.on();
                AutoChill = 0;
            }
        }
        if (((fridgeTest(1) || freezerTest(1)) && (fridgeTest() && freezerTest())) && relay_compressor.getState())
        {
            relay_compressor.off();
            ledFreezerL.set(0);
            ledFridgeR.set(0);
        }

        if (!fridgeTest())
        {
            if (!relay_compressor.getState() && relay_compressor.getNextState())
            {
                ledFridgeR.blink(1000);
            }
            else if (relay_compressor.getState())
            {
                ledFridgeR.set(1);
            }
            else
            {
                ledFridgeR.set(0);
            }
        }
        if (!freezerTest())
        {
            if (!relay_compressor.getState() && relay_compressor.getNextState())
            {
                ledFreezerL.blink(1000);
            }
            else if (relay_compressor.getState())
            {
                ledFreezerL.set(1);
            }
            else
            {
                ledFreezerL.set(0);
            }
        }
        if (relay_compressor.getState())
        {
            if (((uint32_t)(millis() - TimerCompressorWork) >= MAX_WORK_TIME) && !chilling)
            {
                chilling = 1;
                relay_compressor.off();
                TimerCompressorWork = millis() + BREAK_TIME;
                TimerChill = millis();
            }
            if (chilling)
            {
                TimerChill = millis();
                relay_compressor.off();
            }
        }
        else
        {
            if (!AutoChill)
            {
                AutoChill = 1;
                TimerAutoChill = millis();
            }
            if (chilling && ((uint32_t)(millis() - TimerChill) >= BREAK_TIME))
            {
                chilling = 0;
                TimerAutoChill = millis();
                TimerChill = millis();
                TimerCompressorWork = millis();
            }
            if (!chilling && ((uint32_t)(millis() - TimerAutoChill) >= BREAK_RESET))
            {
                TimerAutoChill = millis();
                TimerChill = millis();
                TimerCompressorWork = millis();
                AutoChill = 0;
            }
            if (relay_no_frost.getState())
            {
                ledFridgeR.blink(500);
            }
        }
    }

    break;

    case MODE_SETUP_FREEZER:
    {
        int16_t temp = 0, range = 0;
        bool rangeChanged = false;
        ledFridgeR.set(0);
        while ((uint32_t)millis() - SetupTimer <= 5000)
        {
            // DD("Setting up Freezer...", 1000);
            bool Pot1 = PotFreezerL.tick();
            bool Pot2 = PotFridgeR.tick();
            uint16_t speedBlinkL = map(PotFreezerL.getRawValue(), 0, 1023, 1000, 100);
            ledFreezerL.blink(speedBlinkL);
            if (Pot1 || Pot2)
            {
                SetupTimer = millis();
            }
            temp = PotFreezerL.getValue();
            if (Pot2)
            {
                uint16_t speedBlinkR = map(PotFridgeR.getRawValue(), 0, 1023, 1000, 100);
                ledFridgeR.blink(speedBlinkR);
                range = map(PotFridgeR.getRawValue(), 0, 1023, TEMP_FREEZER_REQ_MIN_RANGE, TEMP_FREEZER_REQ_MAX_RANGE);
                rangeChanged = 1;
            }
            if (!rangeChanged)
            {
                if (temp < tempFreezer)
                {
                    relay_compressor.setNow(1);
                }
                else
                {
                    relay_compressor.setNow(0);
                }
            }
        }
        ledFreezerL.set(0);
        ledFridgeR.set(0);
        delay(2000);
        if (rangeChanged)
        {
            tempFreezerRange = range;
            // DDD("rangeChanged: ");
            // DD(tempFreezerRange);
            EE_tempFreezerRange.update();
            uint16_t speed = 500;

            uint8_t KL = (range / 10) % 10, KR = range % 10;

            while (ledFreezerL.blink(speed, KL))
            {
                // DD("rangeChanged: ledFreezerL.blink(speed, KL)", 1000);
            }
            while (ledFridgeR.blink(speed, KR))
            {
                // DD("rangeChanged: ledFridgeR.blink(speed, KR)", 1000);
            }
            delay(1000);
            ledFreezerL.set(1);
            ledFridgeR.set(1);
            delay(100);
            ledFreezerL.set(0);
            ledFridgeR.set(0);
            delay(1000);
        }
        else
        {
            tempFreezerReq = temp;
            // DDD("tempFreezerReq: ");
            // DD(tempFreezerReq);
            EE_tempFreezerReq.update();
            uint16_t speed = 500;
            if (temp < 0)
            {
                speed = 1000;
                temp = -temp;
            }
            uint8_t KL = (temp / 10) % 10, KR = temp % 10;

            while (ledFreezerL.blink(speed, KL))
            {
                // DD("tempFreezerReq: ledFreezerL.blink(speed, KL)", 1000);
            }
            while (ledFridgeR.blink(speed, KR))
            {
                // DD("tempFreezerReq: ledFridgeR.blink(speed, KR)", 1000);
            }
            delay(1000);
            ledFreezerL.set(1);
            ledFridgeR.set(1);
            delay(100);
            ledFreezerL.set(0);
            ledFridgeR.set(0);
            delay(1000);
        }
        mode = MODE_NORMAL;
    }
    break;

    case MODE_SETUP_FRIDGE:
    {
        int16_t temp = 0, range = 0;
        bool rangeChanged = false;
        ledFreezerL.set(0);
        while ((uint32_t)millis() - SetupTimer <= 5000)
        {

            bool Pot2 = PotFreezerL.tick();
            bool Pot1 = PotFridgeR.tick();
            uint16_t speedBlinkR = map(PotFridgeR.getRawValue(), 0, 1023, 1000, 100);
            ledFridgeR.blink(speedBlinkR);
            if (Pot1 || Pot2)
            {
                SetupTimer = millis();
            }
            temp = PotFridgeR.getValue();
            if (Pot2)
            {
                uint16_t speedBlinkL = map(PotFreezerL.getRawValue(), 0, 1023, 1000, 100);
                ledFreezerL.blink(speedBlinkL);
                range = map(PotFreezerL.getRawValue(), 0, 1023, TEMP_FRIDGE_REQ_MIN_RANGE, TEMP_FRIDGE_REQ_MAX_RANGE);
                rangeChanged = 1;
            }
            if (!rangeChanged)
            {
                if (temp < tempFridge)
                {
                    relay_compressor.setNow(1);
                }
                else
                {
                    relay_compressor.setNow(0);
                }
            }
        }
        ledFreezerL.set(0);
        ledFridgeR.set(0);
        delay(2000);
        if (rangeChanged)
        {
            tempFridgeRange = range;
            // DDD("rangeChanged: ");
            // DD(tempFridgeRange);
            EE_tempFridgeRange.update();

            uint16_t speed = 500;

            uint8_t KL = (range / 10) % 10, KR = range % 10;
            // DDD("KL: ");
            // DD(KL);
            // DDD("KR: ");
            // DD(KR);
            while (ledFreezerL.blink(speed, KL))
            {
                // DD("rangeChanged: ledFreezerL.blink(speed, KL)", 1000);
            }
            while (ledFridgeR.blink(speed, KR))
            {
                // DD("rangeChanged: ledFridgeR.blink(speed, KR)", 1000);
            }
            delay(1000);
            ledFreezerL.set(1);
            ledFridgeR.set(1);
            delay(100);
            ledFreezerL.set(0);
            ledFridgeR.set(0);
            delay(1000);
        }
        else
        {
            tempFridgeReq = temp;
            // DDD("tempFridgeReq: ");
            // DD(tempFridgeReq);
            EE_tempFridgeReq.update();

            uint16_t speed = 500;
            if (temp < 0)
            {
                speed = 1000;
                temp = -temp;
            }
            uint8_t KL = (temp / 10) % 10, KR = temp % 10;

            while (ledFreezerL.blink(speed, KL))
            {
                // DD("tempFridgeReq: ledFreezerL.blink(speed, KL)", 1000);
            }
            while (ledFridgeR.blink(speed, KR))
            {
                // DD("tempFridgeReq: ledFridgeR.blink(speed, KR)", 1000);
            }
            delay(1000);
            ledFreezerL.set(1);
            ledFridgeR.set(1);
            delay(100);
            ledFreezerL.set(0);
            ledFridgeR.set(0);
            delay(1000);
        }
        mode = MODE_NORMAL;
    }
    break;
    }
}
