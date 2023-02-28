#include <Arduino.h>
#include <millisDelay.h>
#include <BfButton.h>
#include "NimbleTCode.h"

#define FIRMWAREVERSION "NimbleStroker_TCode_Serial_v0.4"

NimbleTCode nimble(FIRMWAREVERSION);

millisDelay ledUpdateDelay;
millisDelay logDelay;

BfButton btn(BfButton::STANDALONE_DIGITAL, ENC_BUTT, true, LOW);

void pressHandler(BfButton *btn, BfButton::press_pattern_t pattern)
{
    switch (pattern)
    {
    case BfButton::SINGLE_PRESS:
        nimble.toggle();
        if (!nimble.isRunning()) nimble.resetState();
        break;
    }
}

void logTimer()
{
    if (!logDelay.justFinished()) return;
    logDelay.repeat();

    nimble.printFrameState();
}

void updateLEDs()
{
    if (!ledUpdateDelay.justFinished()) return;
    ledUpdateDelay.repeat();

    nimble.updateEncoderLEDs();
    nimble.updateHardwareLEDs();
    nimble.updateNetworkLEDs(0, 0);
}

void setup()
{
    // NimbleStroker TCode setup
    nimble.init();
    while (!Serial);

#ifdef DEBUG
    Serial.setDebugOutput(true);
    delay(200);
    Serial.println("\nStarting");
#else
    Serial.setDebugOutput(false);
#endif

    // Button interface
    btn.onPress(pressHandler);

    // Timers setup
    ledUpdateDelay.start(30);
    logDelay.start(1000);
}

void loop()
{
    btn.read();
    while (Serial.available() > 0) {
        nimble.inputByte(Serial.read());
    }
    nimble.updateActuator();
    updateLEDs();
#ifdef DEBUG
    logTimer();
#endif
}