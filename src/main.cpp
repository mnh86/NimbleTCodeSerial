#include <Arduino.h>
#include <millisDelay.h>
#include <BfButton.h>
#include "nimbleConModule.h"
#include "TCode.h"

#define FIRMWAREVERSION "NimbleStroker_TCode_Serial_v0.2"

TCode<3> tcode(FIRMWAREVERSION);

millisDelay ledUpdateDelay;
millisDelay logDelay;

#define MAX_POSITION_DELTA 50

int16_t framePosition = 0; // next position to send to actuator (-1000 to 1000)
int16_t targetPos = 0; // position controlled via TCode
int16_t lastFramePos = 0;  // positon that was sent to the actuator last loop frame
int16_t frameForce = IDLE_FORCE; // next force to send to the actuator (0 to 1023)

#define VIBRATION_MAX_AMP 25
#define VIBRATION_MAX_SPEED 20.0 // hz

float vibrationSpeed = 10.0; // hz
uint16_t vibrationAmplitude = 0; // in position units (0 to 10)
int16_t vibrationPos = 0;

#define RUN_MODE_OFF 0
#define RUN_MODE_ON 1

bool runMode = RUN_MODE_ON;

void setRunMode(bool rm)
{
    if (runMode == rm) return;
    runMode = rm;
    if (runMode == RUN_MODE_OFF) {
        tcode.stop();
        Serial.println("Stopped");
    } else {
        Serial.println("Started");
    }
}

BfButton btn(BfButton::STANDALONE_DIGITAL, ENC_BUTT, true, LOW);

void pressHandler(BfButton *btn, BfButton::press_pattern_t pattern)
{
    switch (pattern)
    {
    case BfButton::DOUBLE_PRESS: // Start
        setRunMode(RUN_MODE_ON);
        break;
    case BfButton::LONG_PRESS: // Stop
        setRunMode(RUN_MODE_OFF);
        break;
    }
}

void printState() {
    Serial.printf("------------------\n");
    Serial.printf("   VibAmp: %5d\n", vibrationAmplitude);
    Serial.printf(" VibSpeed: %0.2f (hz)\n", vibrationSpeed);
    Serial.printf("   TarPos: %5d\n", targetPos);
    Serial.printf("      Pos: %5d\n", lastFramePos);
    Serial.printf("    Force: %5d\n", frameForce);
    Serial.printf("    AirIn: %5d\n", actuator.airIn);
    Serial.printf("   AirOut: %5d\n", actuator.airOut);
    Serial.printf("TempLimit: %s\n",  actuator.tempLimiting ? "true" : "false");
}

void setup()
{
    // NimbleStroker setup
    initNimbleConModule();

    // TCode setup
    tcode.init();
    tcode.axisRegister("L0", F("Up"));
    tcode.axisRegister("V0", F("Vibe"));
    tcode.axisRegister("A0", F("Air"));
    tcode.axisRegister("A1", F("Force"));
    tcode.axisRegister("A2", F("VibeSpeed"));
    tcode.axisEasingType("L0", EasingType::EASEINOUT);
    tcode.axisWrite("L0", 5000);
    tcode.axisWrite("V0", 0);
    tcode.axisWrite("A0", 5000);
    tcode.axisWrite("A1", 9999);
    tcode.axisWrite("A2", 5000);

    // Button interface
    btn.onDoublePress(pressHandler)
        .onPressFor(pressHandler, 2000);

    // Timers setup
#ifdef DEBUG
    delay(3000);
    Serial.println("Ready!");
#endif
    ledUpdateDelay.start(30);
    logDelay.start(1000);
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

void ledPulse(short int pos, short int vibPos, bool isOn = true)
{
    byte ledScale = map(abs(pos), 1, ACTUATOR_MAX_POS, 1, LED_MAX_DUTY);
    byte ledState1 = 0;
    byte ledState2 = 0;
    byte vibScale = map(abs(vibPos), 1, VIBRATION_MAX_AMP, 1, LED_MAX_DUTY);
    byte vibState1 = 0;
    byte vibState2 = 0;

    if (isOn)
    {
        if (pos < 0) {
            ledState1 = ledScale;
        } else if (pos > 0) {
            ledState2 = ledScale;
        }

        if (vibPos < 0) {
            vibState1 = vibScale;
        } else if (vibPos > 0) {
            vibState2 = vibScale;
        }
    }

    ledcWrite(ENC_LED_N,  ledState1);
    ledcWrite(ENC_LED_SE, ledState1);
    ledcWrite(ENC_LED_SW, ledState1);

    ledcWrite(ENC_LED_NE, ledState2);
    ledcWrite(ENC_LED_NW, ledState2);
    ledcWrite(ENC_LED_S,  ledState2);

    ledcWrite(ENC_LED_W, vibState1);
    ledcWrite(ENC_LED_E, vibState2);
}

void updateLEDs()
{
    if (!ledUpdateDelay.justFinished()) return;
    ledUpdateDelay.repeat();

    ledPulse(lastFramePos, vibrationPos);
    pendant.present ? ledcWrite(PEND_LED, 50) : ledcWrite(PEND_LED, 0); // Display pendant connection status on LED.
    actuator.present ? ledcWrite(ACT_LED, 50) : ledcWrite(ACT_LED, 0);  // Display actuator connection status on LED.
}

/**
 * Up position is mixed with a vibration oscillator wave
 */
void positionHandler()
{
    if (!tcode.axisChanged("L0")) {
        int val = tcode.axisRead("L0");
        targetPos = map(val, 0, 9999, -ACTUATOR_MAX_POS, ACTUATOR_MAX_POS);
    }

    if (tcode.axisChanged("V0")) {
        int val = tcode.axisRead("V0");
        vibrationAmplitude = map(val, 0, 9999, 0, VIBRATION_MAX_AMP);
    }

    int vibSpeedMillis = 1000 / vibrationSpeed;
    int vibModMillis = millis() % vibSpeedMillis;
    float tempPos = float(vibModMillis) / vibSpeedMillis;
    int vibWaveDeg = tempPos * 360;
    vibrationPos = sin(radians(vibWaveDeg)) * vibrationAmplitude;

    int targetPosTmp = targetPos;
    if (targetPos - vibrationAmplitude < -ACTUATOR_MAX_POS) {
        targetPosTmp = targetPos + vibrationAmplitude;
    } else if (targetPos + vibrationAmplitude > ACTUATOR_MAX_POS) {
        targetPosTmp = targetPos - vibrationAmplitude;
    }
    framePosition = targetPosTmp + vibrationPos;
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

void vibrationSpeedHandler()
{
    if (!tcode.axisChanged("A2")) return;
    int val = tcode.axisRead("A2");
    int maxSpeed = VIBRATION_MAX_SPEED * 100;
    int newSpeed = map(val, 0, 9999, 0, maxSpeed);
    vibrationSpeed = float(newSpeed) / 100;
}

void loop()
{
    btn.read();

    while (Serial.available() > 0)
    {
        tcode.inputByte(Serial.read());
    }

    vibrationSpeedHandler();
    positionHandler();
    airHandler();
    forceHandler();

    // Send packet of values to the actuator when time is ready
    if (checkTimer())
    {
        if (runMode == RUN_MODE_ON) {
            lastFramePos = clampPositionDelta();
            actuator.positionCommand = lastFramePos;
            actuator.forceCommand = frameForce;
        }
        sendToAct();
    }

    if (readFromAct()) // Read current state from actuator.
    { // If the function returns true, the values were updated.

        // Unclear yet if any action is required when tempLimiting is occurring.
        // A comparison is needed with the Pendant behavior.
        // if (actuator.tempLimiting) {
        //     setRunMode(RUN_MODE_OFF);
        // }
    }

    updateLEDs();
#ifdef DEBUG
    logTimer();
#endif
}