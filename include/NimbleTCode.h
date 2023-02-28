#include <Arduino.h>
#include <TCode.h>
#include "nimbleConModule.h"

#define MAX_POSITION_DELTA 50
#define VIBRATION_MAX_AMP 25
#define VIBRATION_MAX_SPEED 20.0 // hz

struct nimbleFrameState {
    int16_t targetPos = 0; // target position from tcode commands
    int16_t position = 0; // next position to send to actuator (-1000 to 1000)
    int16_t lastPos = 0; // previous frame's position
    int16_t force = IDLE_FORCE; // next force value to send to actuator (0 to 1023)
    int8_t air = 0; // next air state to send to actuator (-1 = air out, 0 = stop, 1 = air in)
    int16_t vibrationPos = 0; // next vibration position
};

class NimbleTCode {
    public:
        NimbleTCode(const String &firmware) { tcode = new TCode<3>(firmware); }
        ~NimbleTCode() { delete tcode; }
        void init();
        void resetState();
        void start() { running = true; }
        void stop() { tcode->stop(); running = false; }
        void toggle() { if (running) stop(); else start(); }
        void inputByte(byte input) { tcode->inputByte(input); }
        void updateActuator();
        void updateEncoderLEDs(bool isOn = true);
        void updateHardwareLEDs();
        void updateNetworkLEDs(uint32_t bluetooth, uint32_t wifi);
        void setVibrationSpeed(float v) { vibrationSpeed = min(max(v, (float)0), (float)VIBRATION_MAX_SPEED); }
        void setVibrationAmplitude(uint16_t v) { vibrationAmplitude = min(max(v, (uint16_t)0), (uint16_t)VIBRATION_MAX_AMP); }
        void printFrameState(Print& out = Serial);
        bool isRunning() { return running; }
        void setMessageCallback(TCODE_FUNCTION_PTR_T function) { tcode->setMessageCallback(function); }

    private:
        TCode<3> *tcode;
        bool running = true;
        float vibrationSpeed = VIBRATION_MAX_SPEED; // hz
        uint16_t vibrationAmplitude = 0; // amplitude in position units (0 to 25)
        nimbleFrameState frame;

        void handleAxisChanges();
        void handleVibrationSpeedChanges();
        void handlePositionChanges();
        void handleAirChanges();
        void handleForceChanges();
        int16_t clampPositionDelta();
};

void NimbleTCode::init()
{
    initNimbleConModule();
    resetState();

    tcode->init();

    tcode->axisRegister("L0", F("Up")); // Up stroke position
    tcode->axisWrite("L0", 5000, ' ', 0); // 5000: midpoint
    tcode->axisEasingType("L0", EasingType::EASEINOUT);

    tcode->axisRegister("V0", F("Vibe")); // Vibration Amplitude
    tcode->axisWrite("V0", 0, ' ', 0);    // 0: vibration off
    tcode->axisEasingType("V0", EasingType::EASEINOUT);

    tcode->axisRegister("A0", F("Air")); // Air in/out valve
    tcode->axisWrite("A0", 5000, ' ', 0); // 0: air out, 5000: stop, 9999: air in

    tcode->axisRegister("A1", F("Force"));
    tcode->axisWrite("A1", 9999, ' ', 0); // 9999: max force

    tcode->axisRegister("A2", F("VibeSpeed"));
    tcode->axisWrite("A2", 9999, ' ', 0); // 9999: max vibration speed
}

void NimbleTCode::resetState() {
    frame.targetPos = 0;
    frame.force = MAX_FORCE;
    frame.air = 0;
    vibrationSpeed = VIBRATION_MAX_SPEED;
    vibrationAmplitude = 0;
}

void NimbleTCode::handleAxisChanges()
{
    handleVibrationSpeedChanges();
    handlePositionChanges();
    handleAirChanges();
    handleForceChanges();
}

void NimbleTCode::handleVibrationSpeedChanges()
{
    if (!tcode->axisChanged("A2")) return;
    int val = tcode->axisRead("A2");
    int maxSpeed = VIBRATION_MAX_SPEED * 100;
    int newSpeed = map(val, 0, 9999, 0, maxSpeed);
    vibrationSpeed = float(newSpeed) / 100;
}

void NimbleTCode::handlePositionChanges()
{
    if (!tcode->axisChanged("L0")) {
        int val = tcode->axisRead("L0");
        frame.targetPos = map(val, 0, 9999, -ACTUATOR_MAX_POS, ACTUATOR_MAX_POS);
    }

    if (this->tcode->axisChanged("V0")) {
        int val = tcode->axisRead("V0");
        vibrationAmplitude = map(val, 0, 9999, 0, VIBRATION_MAX_AMP);
    }

    if (vibrationAmplitude > 0 && vibrationSpeed > 0) {
        int vibSpeedMillis = 1000 / vibrationSpeed;
        int vibModMillis = millis() % vibSpeedMillis;
        float tempPos = float(vibModMillis) / vibSpeedMillis;
        int vibWaveDeg = tempPos * 360;
        frame.vibrationPos = round(sin(radians(vibWaveDeg)) * vibrationAmplitude);
    } else {
        frame.vibrationPos = 0;
    }
    // Serial.printf("A:%5d S:%0.2f P:%5d\n",
    //     vibrationAmplitude,
    //     vibrationSpeed,
    //     vibrationPos
    // );

    int targetPosTmp = frame.targetPos;
    if (frame.targetPos - vibrationAmplitude < -ACTUATOR_MAX_POS) {
        targetPosTmp = frame.targetPos + vibrationAmplitude;
    } else if (frame.targetPos + vibrationAmplitude > ACTUATOR_MAX_POS) {
        targetPosTmp = frame.targetPos - vibrationAmplitude;
    }
    frame.position = targetPosTmp + frame.vibrationPos;
}

void NimbleTCode::handleAirChanges()
{
    if (!tcode->axisChanged("A0")) return;
    int val = tcode->axisRead("A0");

    if (val < 5000) {
        frame.air = -1;
    } else if (val > 5000) {
        frame.air = 1;
    } else {
        frame.air = 0;
    }
}

void NimbleTCode::handleForceChanges()
{
    if (!tcode->axisChanged("A1")) return;
    int val = tcode->axisRead("A1");
    frame.force = map(val, 0, 9999, 0, MAX_FORCE);
}

void NimbleTCode::updateActuator()
{
    handleAxisChanges();
    // Send packet of values to the actuator when time is ready
    if (checkTimer())
    {
        if (isRunning()) {
            frame.lastPos = clampPositionDelta();
            actuator.positionCommand = frame.lastPos;
            actuator.forceCommand = frame.force;
            actuator.airIn = (frame.air > 0);
            actuator.airOut = (frame.air < 0);
        } else {
            actuator.airIn = false;
            actuator.airOut = false;
            actuator.forceCommand = IDLE_FORCE;
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

        // Serial.printf("A P:%4d F:%4d T:%s\n",
        //     actuator.positionFeedback,
        //     actuator.forceFeedback,
        //     actuator.tempLimiting ? "true" : "false"
        // );
    }
}

void NimbleTCode::updateEncoderLEDs(bool isOn)
{
    int16_t pos = frame.lastPos;
    int16_t vibPos = frame.vibrationPos;

    byte ledScale = map(abs(pos), 0, ACTUATOR_MAX_POS, 1, LED_MAX_DUTY);
    byte ledState1 = 0;
    byte ledState2 = 0;
    byte vibScale = map(abs(vibPos), 0, VIBRATION_MAX_AMP, 1, LED_MAX_DUTY);
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

void NimbleTCode::updateHardwareLEDs()
{
    ledcWrite(PEND_LED, (pendant.present) ? 50 : 0);
    ledcWrite(ACT_LED, (actuator.present) ? 50 : 0);
}

void NimbleTCode::updateNetworkLEDs(uint32_t bluetooth, uint32_t wifi)
{
    ledcWrite(BT_LED, bluetooth);
    ledcWrite(WIFI_LED, wifi);
}

int16_t NimbleTCode::clampPositionDelta()
{
    int16_t delta = frame.position - frame.lastPos;
    if (delta >= 0) {
        return (delta > MAX_POSITION_DELTA) ? frame.lastPos + MAX_POSITION_DELTA : frame.position;
    } else {
        return (delta < -MAX_POSITION_DELTA) ? frame.lastPos - MAX_POSITION_DELTA : frame.position;
    }
}

void NimbleTCode::printFrameState(Print& out)
{
    out.printf("------------------\n");
    out.printf("   VibAmp: %5d\n", vibrationAmplitude);
    out.printf(" VibSpeed: %0.2f (hz)\n", vibrationSpeed);
    out.printf("   TarPos: %5d\n", frame.targetPos);
    out.printf("      Pos: %5d\n", frame.position);
    out.printf("    Force: %5d\n", actuator.forceCommand);
    out.printf("    AirIn: %s\n", actuator.airIn ? "true" : "false");
    out.printf("   AirOut: %s\n", actuator.airOut ? "true" : "false");
    out.printf("TempLimit: %s\n", actuator.tempLimiting ? "true" : "false");
}
