#include <Arduino.h>
#include "nimbleConModule.h"
#include "TCode.h"
#include <millisDelay.h>

#define FIRMWAREVERSION "NimbleStroker_TCode_Serial_POC"

TCode<2> tcode(FIRMWAREVERSION);

millisDelay ledUpdateDelay;
millisDelay logDelay;

#define MAX_POSITION_DELTA 50

int16_t framePosition = 0; // -1000 to 1000
int16_t lastFramePos = 0;  // -1000 to 1000
int16_t frameForce = IDLE_FORCE; // (0 to 1023)

void printState() {
    Serial.printf("------------------\n");
    Serial.printf("      Pos: %5d\n", lastFramePos);
    Serial.printf("    Force: %5d\n", frameForce);
    Serial.printf("   AirOut: %5d\n", actuator.airIn);
    Serial.printf("   AirOut: %5d\n", actuator.airOut);
}

void setup()
{
    // NimbleStroker setup
    initNimbleConModule();

    // TCode setup
    tcode.init();
    tcode.axisRegister("L0", F("Up"));
    tcode.axisRegister("V0", F("Vibe0"));
    tcode.axisRegister("A0", F("Air"));
    tcode.axisRegister("A1", F("Force"));
    tcode.axisEasingType("L0", EasingType::EASEINOUT);
    tcode.axisWrite("L0", 5000);
    tcode.axisWrite("V0", 0);
    tcode.axisWrite("A0", 5000);
    tcode.axisWrite("A1", 9999);

    // Timers setup
    delay(3000);
    ledUpdateDelay.start(30);
    logDelay.start(1000);
    Serial.println("Ready!");
}

int16_t clampPositionDelta() {
    int16_t delta = framePosition - lastFramePos;
    if (delta >= 0) {
        return (delta > MAX_POSITION_DELTA) ? lastFramePos + MAX_POSITION_DELTA : framePosition;
    } else {
        return (delta < -MAX_POSITION_DELTA) ? lastFramePos - MAX_POSITION_DELTA : framePosition;
    }
}

void logTimer()
{
    if (!logDelay.justFinished()) return;
    logDelay.repeat();

    printState();
}

void updateLEDs()
{
    if (!ledUpdateDelay.justFinished()) return;
    ledUpdateDelay.repeat();

    ledPositionPulse(lastFramePos, true);
    pendant.present ? ledcWrite(PEND_LED, 50) : ledcWrite(PEND_LED, 0); // Display pendant connection status on LED.
    actuator.present ? ledcWrite(ACT_LED, 50) : ledcWrite(ACT_LED, 0);  // Display actuator connection status on LED.
}

void upPositionHandler()
{
    if (!tcode.axisChanged("L0")) return;
    int val = tcode.axisRead("L0");
    framePosition = map(val, 0, 9999, -ACTUATOR_MAX_POS, ACTUATOR_MAX_POS);
}

void vibrationHandler()
{
    if (!tcode.axisChanged("V0")) return;
    int val = tcode.axisRead("V0");
    // TODO
}

void airHandler()
{
    if (!tcode.axisChanged("A0")) return;
    int val = tcode.axisRead("A0");
    int airState = map(val, 0, 9999, -1, 1);
    actuator.airIn = (airState > 0);
    actuator.airOut = (airState < 0);
}

void forceHandler()
{
    if (!tcode.axisChanged("A1")) return;
    int val = tcode.axisRead("A1");
    frameForce = map(val, 0, 9999, 0, MAX_FORCE);
}

void loop()
{
    while (Serial.available() > 0)
    {
        tcode.inputByte(Serial.read());
    }

    upPositionHandler();
    vibrationHandler();
    airHandler();
    forceHandler();

    // Send packet of values to the actuator when time is ready
    if (checkTimer())
    {
        lastFramePos = clampPositionDelta();
        actuator.positionCommand = lastFramePos;
        actuator.forceCommand = frameForce;
        sendToAct();
    }

    if (readFromAct()) // Read current state from actuator.
    { // If the function returns true, the values were updated.
        if (actuator.tempLimiting) {
            tcode.stop();
        }
    }

    updateLEDs();
    logTimer();
}