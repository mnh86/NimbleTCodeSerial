#include <Arduino.h>
// From https://github.com/ExploratoryDevices/NimbleConModule (with edits)
#include <ESP32Encoder.h>   // https://github.com/madhephaestus/ESP32Encoder
#include <HardwareSerial.h> // Arduino Core ESP32 Hardware serial library

// min() function needs this to work on ESP32
#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif

// Encoder pins
#define ENC_BUTT 2
#define ENC_A 35
#define ENC_B 34

ESP32Encoder encoder;

// Serial pins
#define PEND_RX 14
#define PEND_TX 15
#define ACT_RX 16
#define ACT_TX 17

#define SERIAL_BAUD 115200

HardwareSerial pendSerial(1);
HardwareSerial actSerial(2);

#define PACKET_TIMEOUT 50 // Time duration (ms) for packet timeout

// ADC Pins
#define ADC_REF 32
#define SENSOR_ADC 33

// Encoder LED PWM channels
#define ENC_LED_E 0
#define ENC_LED_SE 1
#define ENC_LED_S 2
#define ENC_LED_SW 3
#define ENC_LED_W 4
#define ENC_LED_NW 5
#define ENC_LED_N 6
#define ENC_LED_NE 7

// Other LED PWM channels
#define ACT_LED 8
#define PEND_LED 9
#define BT_LED 10
#define WIFI_LED 11

#define LED_MAX_DUTY 75 // No visible brightness difference with values between 75 and 255.

// Timers for sending serial data to actuator and pendant
#define SEND_INTERVAL 2000 // microseconds between packets sent.

int timeSinceLastActSend = 0;
int timeSinceLastPendSend = 0;

volatile int timerTriggered;

hw_timer_t *timer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

void IRAM_ATTR onTimer()
{
    portENTER_CRITICAL_ISR(&timerMux);
    timerTriggered = 1; // Set timer as triggered.
    portEXIT_CRITICAL_ISR(&timerMux);
}

bool checkTimer()
{
    if (timerTriggered == 1)
    {
        portENTER_CRITICAL(&timerMux);
        timerTriggered = 0; // Clear timer flag
        portEXIT_CRITICAL(&timerMux);
        return (1); // Return 1 to indicate timer has triggered
    }
    return (0);
}

// Pendant Variables
#define IDLE_FORCE 200 // Centering force to send when no value position signal is received.
#define MAX_FORCE 1023 // Pendant uses this value as a constant

struct Pendant
{
    bool present;
    // Signals from the pendant
    long positionCommand; // (range: -1000 to 1000)
    long forceCommand;
    bool activated;
    bool airOut;
    bool airIn;

    // Signals to the pendant
    long positionFeedback; // (range: -1000 to 1000)
    long forceFeedback;    // (range: -1023 to 1023)
    bool tempLimiting;     // Set high if the actuator is thermally limiting its performance.
    bool sensorFault;      // Set high if there's a fault in the position sensor.
};

struct Pendant pendant; // Declare pendant

// Actuator Variables
#define ACTUATOR_MAX_POS 1000

struct Actuator
{
    bool present;

    // Signals from the actuator
    long positionFeedback; // (range: -1000 to 1000)
                           // Errata: actuator delivered before January 2023 this signal always reads positive.
    long forceFeedback;    // (range: -1023 to 1023)
    bool tempLimiting;     // Reads high if the actuator is thermally limiting its performance.
    bool sensorFault;      // Reads high if there's a fault in the position sensor.

    // Signals to the actuator
    long positionCommand; // (range: -1000 to 1000)
    long forceCommand;    // (range: 0 to 1023)
    bool activated;       // Not used
    bool airOut;          // Set high to open air-out valve (looser)
    bool airIn;           // Set high to open air-in valve  (tighter)
};

struct Actuator actuator; // Declare actuator

// Initialization fuction
void initNimbleConModule()
{
    // Setup encoder
    pinMode(ENC_BUTT, INPUT_PULLUP);
    pinMode(ENC_A, INPUT_PULLUP);
    pinMode(ENC_B, INPUT_PULLUP);

    ESP32Encoder::useInternalWeakPullResistors = UP;
    encoder.attachHalfQuad(ENC_A, ENC_B);
    encoder.setCount(0);

    // Actuator/Pendant defaults
    pendant.forceCommand = IDLE_FORCE;
    actuator.forceCommand = IDLE_FORCE;

    // Setup serial ports
    Serial.begin(SERIAL_BAUD);                                   // open serial port for USB connection
    pendSerial.begin(SERIAL_BAUD, SERIAL_8N1, PEND_RX, PEND_TX); // open serial port for pendant
    actSerial.begin(SERIAL_BAUD, SERIAL_8N1, ACT_RX, ACT_TX);    // open serial port for actuator

    // Set up timer interrupt.
    timer = timerBegin(0, 80, true);             // Set timer parameters
    timerAttachInterrupt(timer, &onTimer, true); // Attach interrupt to ISR
    timerAlarmWrite(timer, SEND_INTERVAL, true); // Configure timer threshold
    timerAlarmEnable(timer);                     // Enable timer

    // Attach PWM to LED pins (pin, PWM channel)
    ledcAttachPin(4, ENC_LED_E);
    ledcAttachPin(5, ENC_LED_SE);
    ledcAttachPin(12, ENC_LED_S);
    ledcAttachPin(13, ENC_LED_SW);
    ledcAttachPin(21, ENC_LED_W);
    ledcAttachPin(22, ENC_LED_NW);
    ledcAttachPin(23, ENC_LED_N);
    ledcAttachPin(25, ENC_LED_NE);
    ledcAttachPin(18, ACT_LED);
    ledcAttachPin(19, PEND_LED);
    ledcAttachPin(26, BT_LED);
    ledcAttachPin(27, WIFI_LED);

    // Configure PWM channels (PWM channel, PWM frequency, PWM counter bits)
    ledcSetup(ENC_LED_E, 1000, 8);
    ledcSetup(ENC_LED_SE, 1000, 8);
    ledcSetup(ENC_LED_S, 1000, 8);
    ledcSetup(ENC_LED_SW, 1000, 8);
    ledcSetup(ENC_LED_W, 1000, 8);
    ledcSetup(ENC_LED_NW, 1000, 8);
    ledcSetup(ENC_LED_N, 1000, 8);
    ledcSetup(ENC_LED_NE, 1000, 8);
    ledcSetup(ACT_LED, 1000, 8);
    ledcSetup(PEND_LED, 1000, 8);
    ledcSetup(BT_LED, 1000, 8);
    ledcSetup(WIFI_LED, 1000, 8);
}

void ledLevelDisplay(byte LEDScale)
{
    LEDScale > 0 ? ledcWrite(ENC_LED_N, map(LEDScale, 0, 32, 0, LED_MAX_DUTY)) : ledcWrite(ENC_LED_N, 0);
    LEDScale > 32 ? ledcWrite(ENC_LED_NE, map(LEDScale, 32, 64, 0, LED_MAX_DUTY)) : ledcWrite(ENC_LED_NE, 0);
    LEDScale > 64 ? ledcWrite(ENC_LED_E, map(LEDScale, 64, 96, 0, LED_MAX_DUTY)) : ledcWrite(ENC_LED_E, 0);
    LEDScale > 96 ? ledcWrite(ENC_LED_SE, map(LEDScale, 96, 128, 0, LED_MAX_DUTY)) : ledcWrite(ENC_LED_SE, 0);
    LEDScale > 128 ? ledcWrite(ENC_LED_S, map(LEDScale, 128, 160, 0, LED_MAX_DUTY)) : ledcWrite(ENC_LED_S, 0);
    LEDScale > 160 ? ledcWrite(ENC_LED_SW, map(LEDScale, 160, 192, 0, LED_MAX_DUTY)) : ledcWrite(ENC_LED_SW, 0);
    LEDScale > 192 ? ledcWrite(ENC_LED_W, map(LEDScale, 192, 224, 0, LED_MAX_DUTY)) : ledcWrite(ENC_LED_W, 0);
    LEDScale > 224 ? ledcWrite(ENC_LED_NW, map(LEDScale, 224, 255, 0, LED_MAX_DUTY)) : ledcWrite(ENC_LED_NW, 0);
}

void ledPositionPulse(short int position, bool isOn)
{
    byte ledScale = map(abs(position), 1, ACTUATOR_MAX_POS, 1, LED_MAX_DUTY);
    byte ledState1 = 0;
    byte ledState2 = 0;

    if (isOn)
    {
        if (position < 0)
        {
            ledState1 = ledScale;
        }
        else
        {
            ledState2 = ledScale;
        }
    }

    ledcWrite(ENC_LED_N,  ledState1);
    ledcWrite(ENC_LED_SE, ledState1);
    ledcWrite(ENC_LED_SW, ledState1);

    ledcWrite(ENC_LED_NE, ledState2);
    ledcWrite(ENC_LED_NW, ledState2);
    ledcWrite(ENC_LED_S,  ledState2);

    ledcWrite(ENC_LED_W, 0);
    ledcWrite(ENC_LED_E, 0);
}

void sendToAct()
{
    // Parse settings for transmission ------------------------------------------------------------

    byte outgoingPacket[7], statusByte = 0;
    bool positionNegative = 0;
    int checkWord;

    if (actuator.positionCommand < 0)
    {
        actuator.positionCommand *= -1;
        positionNegative = 1;
    }
    else
        positionNegative = 0;

    statusByte |= actuator.activated;
    statusByte |= actuator.airOut << 1;
    statusByte |= actuator.airIn << 2;
    statusByte |= 0x80; // SYSTEM_TYPE: NimbleStroker

    outgoingPacket[0] = statusByte;
    outgoingPacket[1] = actuator.positionCommand & 0xFF;
    outgoingPacket[2] = actuator.positionCommand >> 8;
    outgoingPacket[2] |= positionNegative << 2;
    outgoingPacket[3] = actuator.forceCommand & 0xFF;
    outgoingPacket[4] = actuator.forceCommand >> 8;

    checkWord = 0;
    for (byte i = 0; i <= 4; i++)
    {
        checkWord += outgoingPacket[i];
    }

    outgoingPacket[5] = checkWord & 0x00FF;
    outgoingPacket[6] = checkWord >> 8;

    for (byte i = 0; i <= 6; i++)
    {
        actSerial.write(outgoingPacket[i]);
    }
}

bool readFromPend()
{
    static byte byteCounter = 0, errorCounter = 0, statusByte = 0;
    static long lastTime = 0, lastPacket = 0;
    static byte incomingPacket[7];
    int checkWord, checkSum;
    bool updated = 0;

    lastPacket = millis() - lastTime;

    if (lastPacket > PACKET_TIMEOUT) // If the last packet was more than the timeout ago, set everything to zero.
    {
        pendant.positionCommand = 0;
        pendant.forceCommand = IDLE_FORCE;
        pendant.present = false;
    }

    while (pendSerial.available()) // Clear pendant incoming serial buffer and fill the incomingPacket array with the first 10 bytes.
    {

        for (byte i = 1; i <= 6; i++) // Shift all bytes in the array to make room for the new one.
            incomingPacket[i - 1] = incomingPacket[i];

        incomingPacket[6] = pendSerial.read(); // put the new byte in the array.

        checkSum = 0; // Reset the checksum before proceeding
        for (byte i = 0; i <= 4; i++)
            checkSum += incomingPacket[i]; // Sum all elements in the array

        checkWord = (incomingPacket[6] << 8) | incomingPacket[5]; // Extract the sent checksum from the array
        if (checkWord == checkSum && checkWord != 0)              // If they match (and aren't zero), update all the variables from the values in the array.
        {
            lastTime = millis(); // Reset the time since the last packet was received.
            statusByte = incomingPacket[0];
            incomingPacket[2] &= 0x07; // Drop the NODE_TYPE designation from this byte.
            incomingPacket[4] &= 0x07; // Drop any random bits from this byte.

            if ((statusByte & 0xE0) == 0x80 && (incomingPacket[4] & 0xF8) == 0) // Verify that the system type and check bits are as expected. If not, don't process the packet.
            {
                pendant.positionCommand = (incomingPacket[2] << 8) | incomingPacket[1];
                if (pendant.positionCommand & 0x0400) // if negative bit is set
                {
                    pendant.positionCommand &= ~(0x0400); // clear negative bit
                    pendant.positionCommand *= -1;        // Set as negative
                }
                pendant.forceCommand = (incomingPacket[4] << 8) | incomingPacket[3];
                pendant.activated = (statusByte & 0x01) ? 1 : 0;
                pendant.airOut = (statusByte & 0x02) ? 1 : 0;
                pendant.airIn = (statusByte & 0x04) ? 1 : 0;
                pendant.present = true;
                updated = 1; // Return 1 since the struct was updated this call.
            }
        }
    }
    return (updated);
}

bool readFromAct()
{
    static byte byteCounter = 0, errorCounter = 0, statusByte = 0;
    static long lastTime = 0, lastPacket = 0;
    static byte incomingPacket[7];
    int checkWord, checkSum;
    bool updated = 0;

    lastPacket = millis() - lastTime;

    if (lastPacket > PACKET_TIMEOUT) // If the last packet was more than the timeout ago, set everything to zero.
        actuator.present = false;

    while (actSerial.available()) // Clear pendant incoming serial buffer and fill the incomingPacket array with the first 10 bytes.
    {
        for (byte i = 1; i <= 6; i++) // Shift all bytes in the array to make room for the new one.
            incomingPacket[i - 1] = incomingPacket[i];

        incomingPacket[6] = actSerial.read(); // put the new byte in the array.

        checkSum = 0; // Reset the checksum before proceeding
        for (byte i = 0; i <= 4; i++)
            checkSum += incomingPacket[i]; // Sum all elements in the array

        checkWord = (incomingPacket[6] << 8) | incomingPacket[5]; // Extract the sent checksum from the array
        if (checkWord == checkSum && checkWord != 0)              // If they match (and aren't zero), update all the variables from the values in the array.
        {
            lastTime = millis(); // Reset the time since the last packet was received.
            statusByte = incomingPacket[0];
            incomingPacket[2] &= 0x07; // Drop the NODE_TYPE designation from this byte.
            incomingPacket[4] &= 0x07; // Drop any random bits from this byte.

            if ((statusByte & 0xE0) == 0x80 && (incomingPacket[4] & 0xF8) == 0) // Verify that the system type and check bits are as expected. If not, don't process the packet.
            {
                actuator.positionFeedback = (incomingPacket[2] << 8) | incomingPacket[1];
                if (actuator.positionFeedback & 0x0400) // if negative bit is set
                {
                    actuator.positionFeedback &= ~(0x0400); // clear negative bit
                    actuator.positionFeedback *= -1;        // Set as negative
                }
                actuator.forceFeedback = (incomingPacket[4] << 8) | incomingPacket[3];
                if (actuator.forceFeedback & 0x0400) // if negative bit is set
                {
                    actuator.forceFeedback &= ~(0x0400); // clear negative bit
                    actuator.forceFeedback *= -1;        // Set as negative
                }
                actuator.activated = (statusByte & 0x01) ? 1 : 0;
                actuator.sensorFault = (statusByte & 0x02) ? 1 : 0;
                actuator.tempLimiting = (statusByte & 0x04) ? 1 : 0;
                actuator.present = true;
                updated = 1; // Return 1 since the struct was updated this call.
            }
        }
    }
    return (updated);
}
