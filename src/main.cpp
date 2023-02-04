#include <Arduino.h>
#include "nimbleConModule.h"
#include "TCode.h"

#define FIRMWAREVERSION "NimbleStroker_TCode_Serial_POC"

TCode<2> tcode(FIRMWAREVERSION);

void setup() {
    while(!Serial){}
    initNimbleConModule();
    tcode.init();



}

void loop() {
    if (Serial.available())
    {
        String command = Serial.readStringUntil('\n');
        if (command == "ON") {
            ledLevelDisplay(255);
            Serial.println("Turn LED ON");
        } else if (command == "OFF") {
            ledLevelDisplay(0);
            Serial.println("Turn LED OFF");
        }
    }

    // Send packet of values to the actuator when time is ready
    if (checkTimer())
    {
        // actuator.positionCommand = framePos;
        // actuator.forceCommand = frameForce;
        sendToAct();
    }

    if (readFromAct()) // Read current state from actuator.
    { // If the function returns true, the values were updated.
        // if (actuator.tempLimiting) // stop
    }

    pendant.present ? ledcWrite(PEND_LED, 50) : ledcWrite(PEND_LED, 0); // Display pendant connection status on LED.
    actuator.present ? ledcWrite(ACT_LED, 50) : ledcWrite(ACT_LED, 0);  // Display actuator connection status on LED.
}