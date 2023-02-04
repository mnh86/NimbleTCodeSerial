// TCode-Class-H v1.0,
// protocal by TempestMAx (https://www.patreon.com/tempestvr)
// implemented by Eve 05/02/2022
// usage of this class can be found at (https://github.com/Dreamer2345/Arduino_TCode_Parser)
// Please copy, share, learn, innovate, give attribution.
// Decodes T-code commands
// It can handle:
//   x linear channels (L0, L1, L2... L9)
//   x rotation channels (R0, R1, R2... L9)
//   x vibration channels (V0, V1, V2... V9)
//   x auxilliary channels (A0, A1, A2... A9)
// History:
//
#pragma once
#ifndef TCODE_H
#define TCODE_H
#include "Arduino.h"
#include "TCodeAxis.h"
#include "TCodeBuffer.h"

#define TCODE_MAX_CHANNEL_COUNT 10
#define TCODE_CHANNEL_TYPES 4
#define CURRENT_TCODE_VERSION "TCode v0.4"

#ifndef TCODE_EEPROM_MEMORY_OFFSET
#define TCODE_EEPROM_MEMORY_OFFSET 0
#endif

#define TCODE_EEPROM_MEMORY_ID "TCODE"
#define TCODE_EEPROM_MEMORY_ID_LENGTH 5

#ifndef TCODE_USE_EEPROM
#define TCODE_USE_EEPROM true
#endif

#ifndef TCODE_COMMAND_BUFFER_LENGTH
#define TCODE_COMMAND_BUFFER_LENGTH 255
#endif

#ifndef TCODE_MAX_AXIS
#define TCODE_MAX_AXIS 9999
#endif

#ifndef TCODE_MAX_AXIS_MAGNITUDE
#define TCODE_MAX_AXIS_MAGNITUDE 9999999
#endif

struct ChannelID
{
    char type;
    int channel;
    bool valid;
};

using TCODE_FUNCTION_PTR_T = void (*)(const String &input);

template <unsigned TCODE_CHANNEL_COUNT = 5>
class TCode
{
public:
    static_assert((TCODE_CHANNEL_COUNT > 0) && (TCODE_CHANNEL_COUNT <= TCODE_MAX_CHANNEL_COUNT), "TCode Channel Count must be larger than or equal to 1 but less than or equal to 10");
    static constexpr uintmax_t EEPROM_SIZE = TCODE_CHANNEL_COUNT * TCODE_CHANNEL_TYPES * sizeof(int) * 2 + TCODE_EEPROM_MEMORY_ID_LENGTH;

    TCode(const String &firmware);                                                // Constructor for class using defined TCode Version number
    TCode(const String &firmware, const String &TCode_version);                   // Constructor for class using user defined TCode Version number
    static ChannelID getIDFromStr(const String &input);                           // Function to convert string ID to a channel ID Type and if it is valid
    void inputByte(byte input);                                                   // Function to read off individual byte as input to the command buffer
    void inputChar(char input);                                                   // Function to read off individual char as input to the command buffer
    void inputString(const String &input);                                        // Function to take in a string as input to the command buffer
    int axisRead(const String &ID);                                               // Function to read the current position of an axis
    void axisWrite(const String &ID, int magnitude, char ext, long extMagnitude); // Function to set an axis
    void axisWrite(const String &ID, int magnitude);                              // Overloaded helper function for axisWrite without an extension
    unsigned long axisLastT(const String &ID);                                    // Function to query when an axis was last commanded
    void axisRegister(const String &ID, const String &Name);                      // Function to name and activate axis
    bool axisChanged(const String &ID);                                           // Function to check if an axis has changed
    void axisEasingType(const String &ID, EasingType e);                          // Function to set the easing type of an axis;

    void stop(); // Function stops all outputs

    void setMessageCallback(TCODE_FUNCTION_PTR_T function); // Function to set the used message callback this can be used to change the method of message transmition (if nullptr is passed to this function the default callback will be used)
    void sendMessage(const String &s);                      // Function which calls the callback (the default callback for TCode is Serial communication)

    void clearBuffer();

    void init(); // Initalizes the EEPROM and checks for the magic string
private:
    String versionID;
    String firmwareID;

    TCodeBuffer<TCODE_COMMAND_BUFFER_LENGTH> buffer;

    TCodeAxis Linear[TCODE_CHANNEL_COUNT];
    TCodeAxis Rotation[TCODE_CHANNEL_COUNT];
    TCodeAxis Vibration[TCODE_CHANNEL_COUNT];
    TCodeAxis Auxiliary[TCODE_CHANNEL_COUNT];

    TCODE_FUNCTION_PTR_T message_callback;

    static void defaultCallback(const String &input); // Function which is the default callback for TCode this uses the serial communication if it is setup with Serial.begin() if it is not setup then nothing happens

    void axisRow(const String &id, const String &name);      // Function to return the details of an axis stored in the EEPROM
    bool tryGetAxis(ChannelID decoded_id, TCodeAxis *&axis); // Function to get the axis specified by a decoded id if successfull returns true

    bool isnumber(char character);                         // Function which returns true if a char is a numerical
    String getNextIntStr(const String &input, int &index); // Function which goes through a string at an index and finds the first integer string returning it
    char getCurrentChar(const String &input, int index);   // Function which gets a char at a location and returns '\0' if over/underflow
    bool extentionValid(char c);                           // Function checks if a char is a valid extention char

    void executeNextBufferCommand();
    void executeString(String &input);   // Function to divide up and execute input string
    void readCommand(String &command);   // Function to process the individual commands
    void axisCommand(String &command);   // Function to read and interpret axis commands
    void deviceCommand(String &command); // Function to identify and execute device commands
    void setupCommand(String &command);  // Function to modify axis preference values

    bool checkMemoryKey();                                       // Function to check if a memory key "TCODE" has been placed in the EEPROM at the location TCODE_EEPROM_MEMORY_OFFSET
    void placeMemoryKey();                                       // Function to place the memory key at location TCODE_EEPROM_MEMORY_OFFSET
    void resetMemory();                                          // Function to reset the stored values area of the EEPROM
    int getHeaderEnd();                                          // Function to get the memory location of the start of the stored values area
    int getMemoryLocation(const String &id);                     // Function to get the memory location of an ID
    void updateSavedMemory(const String &id, int low, int high); // Function to update the memory location of an id

    void commitEEPROMChanges();           // Function abstracts the commit function for different board types;
    byte readEEPROM(int idx);             // Function abstracts the EEPROM read command so that it can be redefined if need be for different board types
    void writeEEPROM(int idx, byte byte); // Function abstracts the EEPROM write command so that it can be redefined if need be for different board types
    template <typename T>
    T &getEEPROM(int idx, T &t); // Function abstracts the EEPROM get command so that it can be redefined if need be for different board types
    template <typename T>
    void putEEPROM(int idx, T t); // Function abstracts the EEPROM put command so that it can be redefined if need be for different board types
};

template <unsigned TCODE_CHANNEL_COUNT>
TCode<TCODE_CHANNEL_COUNT>::TCode(const String &firmware)
{
    buffer.clear();
    firmwareID = firmware;
    versionID = CURRENT_TCODE_VERSION;
    stop();
    setMessageCallback(nullptr);
}

template <unsigned TCODE_CHANNEL_COUNT>
TCode<TCODE_CHANNEL_COUNT>::TCode(const String &firmware, const String &TCode_version)
{
    buffer.clear();
    firmwareID = firmware;
    versionID = TCode_version;
    stop();
    setMessageCallback(nullptr);
}

template <unsigned TCODE_CHANNEL_COUNT>
void TCode<TCODE_CHANNEL_COUNT>::clearBuffer()
{
    buffer.clear();
}

template <unsigned TCODE_CHANNEL_COUNT>
bool TCode<TCODE_CHANNEL_COUNT>::axisChanged(const String &ID)
{
    ChannelID decoded_id = TCode::getIDFromStr(ID);
    TCodeAxis *axis = nullptr;
    if (tryGetAxis(decoded_id, axis))
    {
        return axis->changed();
    }
    return false;
}

template <unsigned TCODE_CHANNEL_COUNT>
ChannelID TCode<TCODE_CHANNEL_COUNT>::getIDFromStr(const String &input)
{
    char type = input.charAt(0);
    int channel = input.charAt(1) - '0';
    bool valid = true;
    if ((channel < 0) || (channel >= TCODE_CHANNEL_COUNT))
        valid = false;
    switch (type)
    {
    case 'L':
    case 'R':
    case 'V':
    case 'A':
        break;
    default:
        valid = false;
    }
    return {type, channel, valid};
}

template <unsigned TCODE_CHANNEL_COUNT>
void TCode<TCODE_CHANNEL_COUNT>::axisEasingType(const String &ID, EasingType e)
{
    ChannelID decoded_id = TCode::getIDFromStr(ID);
    TCodeAxis *axis = nullptr;
    if (tryGetAxis(decoded_id, axis))
    {
        axis->setEasingType(e);
    }
}

template <unsigned TCODE_CHANNEL_COUNT>
void TCode<TCODE_CHANNEL_COUNT>::inputByte(byte input)
{
    inputChar(static_cast<char>(input));
}

template <unsigned TCODE_CHANNEL_COUNT>
void TCode<TCODE_CHANNEL_COUNT>::inputChar(char input)
{
    bool success = buffer.push(static_cast<char>(toupper(input)));
    if (!success && buffer.isFull())
    {
        executeNextBufferCommand();
        buffer.push(static_cast<char>(toupper(input)));
    }

    if (input == '\n')
    {
        while (!this->buffer.isEmpty())
            executeNextBufferCommand();
        buffer.clear();
    }
}

template <unsigned TCODE_CHANNEL_COUNT>
void TCode<TCODE_CHANNEL_COUNT>::inputString(const String &input)
{
    String bufferString = input + String('\n');
    bufferString.toUpperCase();
    bufferString.trim();
    executeString(bufferString);
}

template <unsigned TCODE_CHANNEL_COUNT>
int TCode<TCODE_CHANNEL_COUNT>::axisRead(const String &inputID)
{
    int x = TCODE_DEFAULT_AXIS_RETURN_VALUE; // This is the return variable
    ChannelID decoded_id = TCode::getIDFromStr(inputID);
    TCodeAxis *axis = nullptr;
    if (tryGetAxis(decoded_id, axis))
    {
        x = axis->getPosition();
    }
    return x;
}

template <unsigned TCODE_CHANNEL_COUNT>
void TCode<TCODE_CHANNEL_COUNT>::axisWrite(const String &inputID, int magnitude, char extension, long extMagnitude)
{
    ChannelID decoded_id = TCode::getIDFromStr(inputID);
    TCodeAxis *axis = nullptr;
    if (tryGetAxis(decoded_id, axis))
    {
        axis->set(magnitude, extension, extMagnitude);
    }
}

template <unsigned TCODE_CHANNEL_COUNT>
void TCode<TCODE_CHANNEL_COUNT>::axisWrite(const String &inputID, int magnitude)
{
    axisWrite(inputID, magnitude, ' ', 0);
}

template <unsigned TCODE_CHANNEL_COUNT>
unsigned long TCode<TCODE_CHANNEL_COUNT>::axisLastT(const String &inputID)
{
    unsigned long t = 0; // Return time
    ChannelID decoded_id = TCode::getIDFromStr(inputID);
    TCodeAxis *axis = nullptr;
    if (tryGetAxis(decoded_id, axis))
    {
        t = axis->lastT;
    }
    return t;
}

template <unsigned TCODE_CHANNEL_COUNT>
void TCode<TCODE_CHANNEL_COUNT>::axisRegister(const String &inputID, const String &axisName)
{
    ChannelID decoded_id = TCode::getIDFromStr(inputID);
    TCodeAxis *axis = nullptr;
    if (tryGetAxis(decoded_id, axis))
    {
        axis->axisName = axisName;
    }
}

template <unsigned TCODE_CHANNEL_COUNT>
void TCode<TCODE_CHANNEL_COUNT>::executeNextBufferCommand()
{
    String subbuf = String("");
    while ((!buffer.isEmpty()) && (buffer.peek() != ' ' && buffer.peek() != '\n'))
    {
        subbuf += buffer.pop();
    }

    if (!buffer.isEmpty())
    {
        if (buffer.peek() == ' ')
        {
            buffer.pop();
        }
        else if (buffer.peek() == '\n')
        {
            buffer.pop();
        }
    }

    readCommand(subbuf);
}

// Function to divide up and execute input string
template <unsigned TCODE_CHANNEL_COUNT>
void TCode<TCODE_CHANNEL_COUNT>::executeString(String &input)
{
    int lastIndex = 0;
    int index = input.indexOf(' '); // Look for spaces in string
    while (index > 0)
    {
        String sub = input.substring(lastIndex, index);
        readCommand(sub); // Read off first command
        lastIndex = index;
        index = input.indexOf(' '); // Look for next space
    }
    readCommand(input); // Read off last command
}

template <unsigned TCODE_CHANNEL_COUNT>
void TCode<TCODE_CHANNEL_COUNT>::readCommand(String &command)
{
    command.toUpperCase();

    // Switch between command types
    switch (command.charAt(0))
    {
    // Axis commands
    case 'L':
    case 'R':
    case 'V':
    case 'A':
        axisCommand(command);
        break;

    // Device commands
    case 'D':
        deviceCommand(command);
        break;

    // Setup commands
    case '$':
        setupCommand(command);
        break;
    }
}

template <unsigned TCODE_CHANNEL_COUNT>
bool TCode<TCODE_CHANNEL_COUNT>::isnumber(char c)
{
    switch (c)
    {
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
        return true;
    default:
        return false;
    }
}

template <unsigned TCODE_CHANNEL_COUNT>
bool TCode<TCODE_CHANNEL_COUNT>::extentionValid(char c)
{
    switch (c)
    {
    case 'I':
    case 'S':
        return true;
    default:
        return false;
    }
}

template <unsigned TCODE_CHANNEL_COUNT>
String TCode<TCODE_CHANNEL_COUNT>::getNextIntStr(const String &input, int &index)
{
    String accum = "";
    while (isnumber(getCurrentChar(input, index)))
    {
        accum += getCurrentChar(input, index++);
    }
    return accum;
}

template <unsigned TCODE_CHANNEL_COUNT>
char TCode<TCODE_CHANNEL_COUNT>::getCurrentChar(const String &input, int index)
{
    if (index >= input.length() || index < 0)
        return '\0';
    return input[index];
}

template <unsigned TCODE_CHANNEL_COUNT>
void TCode<TCODE_CHANNEL_COUNT>::axisCommand(String &input)
{
    ChannelID decoded_id = TCode::getIDFromStr(input);
    bool valid = decoded_id.valid;
    int channel = decoded_id.channel;
    char type = decoded_id.type;
    int Index = 2;
    int magnitude = 0;
    long extMagnitude = 0;

    String magnitudeStr = getNextIntStr(input, Index);
    while (magnitudeStr.length() < 4)
    {
        magnitudeStr += '0';
    }
    magnitude = magnitudeStr.toInt();
    magnitude = constrain(magnitude, 0, TCODE_MAX_AXIS);
    if (magnitude == 0 && magnitudeStr.charAt(magnitudeStr.length() - 1) != '0')
    {
        valid = false;
    }

    char extention = getCurrentChar(input, Index++);
    if (extentionValid(extention))
    {
        String extMagnitudeStr = getNextIntStr(input, Index);
        extMagnitude = extMagnitudeStr.toInt();
        extMagnitude = constrain(extMagnitude, 0, TCODE_MAX_AXIS_MAGNITUDE);
        if (extMagnitude == 0 && extMagnitudeStr.charAt(extMagnitudeStr.length() - 1) != '0')
        {
            extention = ' ';
            valid = false;
        }
    }
    else
    {
        extention = ' ';
        Index--;
    }
    // Uncommenting this line and commenting the other leads to = being the linear command and not leaving it blank as the command
    // EasingType rampType = EasingType::NONE;
    EasingType rampType = EasingType::LINEAR;
    if (extention != ' ')
    {
        char first = getCurrentChar(input, Index++);
        char second = getCurrentChar(input, Index);
        switch (first)
        {
        case '<':
            if (second == '>')
            {
                rampType = EasingType::EASEINOUT;
                Index++;
            }
            else
            {
                rampType = EasingType::EASEIN;
            }
            break;
        case '>':
            rampType = EasingType::EASEOUT;
            break;
        case '=':
            rampType = EasingType::LINEAR;
            break;
        }
    }

    char last = getCurrentChar(input, Index);
    if(last != '\0')
        valid = false;

    if (valid)
    {
        TCodeAxis *axis = nullptr;
        if (tryGetAxis(decoded_id, axis))
        {
            axis->set(magnitude, extention, extMagnitude);
            if (rampType != EasingType::NONE)
                axis->setEasingType(rampType);
        }
    }
}

template <unsigned TCODE_CHANNEL_COUNT>
void TCode<TCODE_CHANNEL_COUNT>::stop()
{
    for (int i = 0; i < TCODE_CHANNEL_COUNT; i++)
    {
        Linear[i].stop();
    }
    for (int i = 0; i < TCODE_CHANNEL_COUNT; i++)
    {
        Rotation[i].stop();
    }
    for (int i = 0; i < TCODE_CHANNEL_COUNT; i++)
    {
        Vibration[i].set(0, ' ', 0);
    }
    for (int i = 0; i < TCODE_CHANNEL_COUNT; i++)
    {
        Auxiliary[i].stop();
    }
}

template <unsigned TCODE_CHANNEL_COUNT>
void TCode<TCODE_CHANNEL_COUNT>::deviceCommand(String &input)
{
    input = input.substring(1);
    switch (input.charAt(0))
    {
    case 'S':
        stop();
        break;
    case '0':
        sendMessage(firmwareID + '\n');
        break;
    case '1':
        sendMessage(versionID + '\n');
        break;
    case '2':
        for (int i = 0; i < TCODE_CHANNEL_COUNT; i++)
        {
            axisRow("L" + String(i), Linear[i].axisName);
        }
        for (int i = 0; i < TCODE_CHANNEL_COUNT; i++)
        {
            axisRow("R" + String(i), Rotation[i].axisName);
        }
        for (int i = 0; i < TCODE_CHANNEL_COUNT; i++)
        {
            axisRow("V" + String(i), Vibration[i].axisName);
        }
        for (int i = 0; i < TCODE_CHANNEL_COUNT; i++)
        {
            axisRow("A" + String(i), Auxiliary[i].axisName);
        }
        break;
    }
}

template <unsigned TCODE_CHANNEL_COUNT>
int TCode<TCODE_CHANNEL_COUNT>::getHeaderEnd()
{
    return TCODE_EEPROM_MEMORY_OFFSET + TCODE_EEPROM_MEMORY_ID_LENGTH;
}

template <unsigned TCODE_CHANNEL_COUNT>
int TCode<TCODE_CHANNEL_COUNT>::getMemoryLocation(const String &id)
{
    ChannelID decoded_id = getIDFromStr(id);
    int memloc = -1;
    if (decoded_id.valid)
    {
        int typeoffset = -1;
        switch (decoded_id.type)
        {
        case 'L':
            typeoffset = 0;
            break;
        case 'R':
            typeoffset = 1;
            break;
        case 'V':
            typeoffset = 2;
            break;
        case 'A':
            typeoffset = 3;
            break;
        default:
            return -2;
        }
        int typebyteoffset = (sizeof(int) * 2) * TCODE_CHANNEL_COUNT * typeoffset;
        int entrybyteoffset = (sizeof(int) * 2) * decoded_id.channel;
        memloc = typebyteoffset + entrybyteoffset + getHeaderEnd();
    }
    return memloc;
}

template <unsigned TCODE_CHANNEL_COUNT>
bool TCode<TCODE_CHANNEL_COUNT>::checkMemoryKey()
{
    char b[TCODE_EEPROM_MEMORY_ID_LENGTH + 1];
    for (int i = 0; i < TCODE_EEPROM_MEMORY_ID_LENGTH; i++)
        b[i] = (char)readEEPROM(TCODE_EEPROM_MEMORY_OFFSET + i);
    return (String(b) == String(TCODE_EEPROM_MEMORY_ID));
}

template <unsigned TCODE_CHANNEL_COUNT>
void TCode<TCODE_CHANNEL_COUNT>::placeMemoryKey()
{
    for (int i = 0; i < TCODE_EEPROM_MEMORY_ID_LENGTH; i++)
    {
        writeEEPROM(TCODE_EEPROM_MEMORY_OFFSET + i, TCODE_EEPROM_MEMORY_ID[i]);
    }
    commitEEPROMChanges();
}

template <unsigned TCODE_CHANNEL_COUNT>
void TCode<TCODE_CHANNEL_COUNT>::resetMemory()
{
    int headerEnd = getHeaderEnd();
    for (int j = 0; j < TCODE_CHANNEL_TYPES; j++)
    {
        for (int i = 0; i < TCODE_CHANNEL_COUNT; i++)
        {
            int memloc = ((sizeof(int) * 2) * TCODE_CHANNEL_COUNT * j) + ((sizeof(int) * 2) * i) + headerEnd;
            putEEPROM(memloc, ((int)0));
            putEEPROM(memloc + sizeof(int), ((int)TCODE_MAX_AXIS));
        }
    }
    commitEEPROMChanges();
}

template <unsigned TCODE_CHANNEL_COUNT>
void TCode<TCODE_CHANNEL_COUNT>::updateSavedMemory(const String &id, int low, int high)
{
    ChannelID decoded_id = getIDFromStr(id);
    if (decoded_id.valid)
    {
        int memloc = getMemoryLocation(id);
        if (memloc >= 0)
        {
            low = constrain(low, 0, TCODE_MAX_AXIS);
            high = constrain(high, 0, TCODE_MAX_AXIS);
            putEEPROM(memloc, low);
            memloc += sizeof(int);
            putEEPROM(memloc, high);
            commitEEPROMChanges();
        }
    }
}

template <unsigned TCODE_CHANNEL_COUNT>
void TCode<TCODE_CHANNEL_COUNT>::axisRow(const String &id, const String &axisName)
{
    if (axisName != "")
    {
        int memloc = getMemoryLocation(id);
        if (memloc >= 0)
        {
            int low, high;
            getEEPROM(memloc, low);
            memloc += sizeof(int);
            getEEPROM(memloc, high);
            low = constrain(low, 0, TCODE_MAX_AXIS);
            high = constrain(high, 0, TCODE_MAX_AXIS);
            sendMessage(id + " " + String(low) + " " + String(high) + " " + axisName + "\n");
        }
    }
}

template <unsigned TCODE_CHANNEL_COUNT>
bool TCode<TCODE_CHANNEL_COUNT>::tryGetAxis(ChannelID decoded_id, TCodeAxis *&axis)
{
    if (decoded_id.valid)
    {
        switch (decoded_id.type)
        {

        case 'L':
            axis = &Linear[decoded_id.channel];
            return true;
        case 'R':
            axis = &Rotation[decoded_id.channel];
            return true;
        case 'V':
            axis = &Vibration[decoded_id.channel];
            return true;
        case 'A':
            axis = &Auxiliary[decoded_id.channel];
            return true;
        default:
            return false;
        }
    }
    return false;
}

template <unsigned TCODE_CHANNEL_COUNT>
void TCode<TCODE_CHANNEL_COUNT>::setupCommand(String &input)
{
    input = input.substring(1);
    int index = 3;
    ChannelID decoded_id = getIDFromStr(input);
    bool valid = decoded_id.valid;
    int low = 0;
    int high = 0;

    String lowStr = getNextIntStr(input, index);
    index++;
    low = lowStr.toInt();
    low = constrain(low, 0, TCODE_MAX_AXIS);
    if (low == 0 && lowStr.charAt(lowStr.length() - 1) != '0')
    {
        valid = false;
    }

    String highStr = getNextIntStr(input, index);
    high = highStr.toInt();
    high = constrain(high, 0, TCODE_MAX_AXIS);
    if (high == 0 && highStr.charAt(highStr.length() - 1) != '0')
    {
        valid = false;
    }

    if (valid)
    {
        if (TCODE_USE_EEPROM)
        {
            updateSavedMemory(input, low, high);
            switch (decoded_id.type)
            {
            case 'L':
                axisRow("L" + String(decoded_id.channel), Linear[decoded_id.channel].axisName);
                break;
            case 'R':
                axisRow("R" + String(decoded_id.channel), Rotation[decoded_id.channel].axisName);
                break;
            case 'V':
                axisRow("V" + String(decoded_id.channel), Vibration[decoded_id.channel].axisName);
                break;
            case 'A':
                axisRow("A" + String(decoded_id.channel), Auxiliary[decoded_id.channel].axisName);
                break;
            }
        }
        else
        {
            sendMessage(F("EEPROM NOT IN USE\n"));
        }
    }
}

template <unsigned TCODE_CHANNEL_COUNT>
void TCode<TCODE_CHANNEL_COUNT>::defaultCallback(const String &input)
{
    if (Serial)
    {
        Serial.print(input);
    }
}

template <unsigned TCODE_CHANNEL_COUNT>
void TCode<TCODE_CHANNEL_COUNT>::setMessageCallback(TCODE_FUNCTION_PTR_T f)
{
    if (f == nullptr)
    {
        message_callback = &defaultCallback;
    }
    else
    {
        message_callback = f;
    }
}

template <unsigned TCODE_CHANNEL_COUNT>
void TCode<TCODE_CHANNEL_COUNT>::sendMessage(const String &s)
{
    if (message_callback != nullptr)
        message_callback(s);
}

// PER BOARD CODE AREA
#if defined(ARDUINO_ESP32_DEV)
#include <EEPROM.h>
template <unsigned TCODE_CHANNEL_COUNT>
void TCode<TCODE_CHANNEL_COUNT>::init()
{
    if (!EEPROM.begin(EEPROM_SIZE))
    {
        sendMessage(F("EEPROM failed to initialise\n"));
    }
    else
    {
        sendMessage(F("EEPROM initialised\n"));
    }

    if (!checkMemoryKey() && TCODE_USE_EEPROM)
    {
        placeMemoryKey();
        resetMemory();
        commitEEPROMChanges();
    }
}

template <unsigned TCODE_CHANNEL_COUNT>
void TCode<TCODE_CHANNEL_COUNT>::commitEEPROMChanges()
{
    if (EEPROM.commit())
    {
        sendMessage(F("EEPROM successfully committed!\n"));
    }
    else
    {
        sendMessage(F("ERROR! EEPROM commit failed!\n"));
    }
}

template <unsigned TCODE_CHANNEL_COUNT>
byte TCode<TCODE_CHANNEL_COUNT>::readEEPROM(int idx)
{
    return EEPROM.read(idx);
}

template <unsigned TCODE_CHANNEL_COUNT>
void TCode<TCODE_CHANNEL_COUNT>::writeEEPROM(int idx, byte b)
{
    EEPROM.write(idx, b);
}

template <unsigned TCODE_CHANNEL_COUNT>
template <typename T>
T &TCode<TCODE_CHANNEL_COUNT>::getEEPROM(int idx, T &t)
{
    return EEPROM.get(idx, t);
}

template <unsigned TCODE_CHANNEL_COUNT>
template <typename T>
void TCode<TCODE_CHANNEL_COUNT>::putEEPROM(int idx, T t)
{
    EEPROM.put(idx, t);
}

#elif defined(ARDUINO_SAMD_NANO_33_IOT) // Nano 33 IOT
#include <FlashAsEEPROM.h>
template <unsigned TCODE_CHANNEL_COUNT>
void TCode<TCODE_CHANNEL_COUNT>::init()
{
    if (!EEPROM.begin())
    {
        sendMessage(F("EEPROM failed to initialise\n"));
    }
    else
    {
        sendMessage(F("EEPROM initialised\n"));
    }

    if (!checkMemoryKey() && TCODE_USE_EEPROM)
    {
        placeMemoryKey();
        resetMemory();
        commitEEPROMChanges();
    }
}

template <unsigned TCODE_CHANNEL_COUNT>
void TCode<TCODE_CHANNEL_COUNT>::commitEEPROMChanges()
{
    if (EEPROM.commit())
    {
        sendMessage(F("EEPROM successfully committed!\n"));
    }
    else
    {
        sendMessage(F("ERROR! EEPROM commit failed!\n"));
    }
}

template <unsigned TCODE_CHANNEL_COUNT>
byte TCode<TCODE_CHANNEL_COUNT>::readEEPROM(int idx)
{
    return EEPROM.read(idx);
}

template <unsigned TCODE_CHANNEL_COUNT>
void TCode<TCODE_CHANNEL_COUNT>::writeEEPROM(int idx, byte b)
{
    EEPROM.write(idx, b);
}

template <unsigned TCODE_CHANNEL_COUNT>
template <typename T>
T &TCode<TCODE_CHANNEL_COUNT>::getEEPROM(int idx, T &t)
{
    uint8_t *ptr = (uint8_t *)&t;
    for (int count = sizeof(T); count; --count)
        *ptr++ = EEPROM.read(idx);
    return t;
}

template <unsigned TCODE_CHANNEL_COUNT>
template <typename T>
void TCode<TCODE_CHANNEL_COUNT>::putEEPROM(int idx, T t)
{
    const uint8_t *ptr = (const uint8_t *)&t;
    size_t e = 0;
    for (int count = sizeof(T); count; --count, e++)
        EEPROM.update(idx + e, *ptr++);
}
#else // Uses the default arduino methods for setting EEPROM
#include <EEPROM.h>
template <unsigned TCODE_CHANNEL_COUNT>
void TCode<TCODE_CHANNEL_COUNT>::init()
{
    if (!checkMemoryKey() && TCODE_USE_EEPROM)
    {
        placeMemoryKey();
        resetMemory();
        commitEEPROMChanges();
    }
    sendMessage(F("EEPROM initialised\n"));
}

template <unsigned TCODE_CHANNEL_COUNT>
void TCode<TCODE_CHANNEL_COUNT>::commitEEPROMChanges()
{
}

template <unsigned TCODE_CHANNEL_COUNT>
byte TCode<TCODE_CHANNEL_COUNT>::readEEPROM(int idx)
{
    return EEPROM.read(idx);
}

template <unsigned TCODE_CHANNEL_COUNT>
void TCode<TCODE_CHANNEL_COUNT>::writeEEPROM(int idx, byte b)
{
    EEPROM.write(idx, b);
}

template <unsigned TCODE_CHANNEL_COUNT>
template <typename T>
T &TCode<TCODE_CHANNEL_COUNT>::getEEPROM(int idx, T &t)
{
    return EEPROM.get(idx, t);
}

template <unsigned TCODE_CHANNEL_COUNT>
template <typename T>
void TCode<TCODE_CHANNEL_COUNT>::putEEPROM(int idx, T t)
{
    EEPROM.put(idx, t);
}
#endif

#endif