# NimbleTCodeSerial

Toy Code (T-Code) v0.2 compatible Serial Port implementation for the [NimbleStroker](https://shop.exploratorydevices.com/) and [NimbleConModule](https://shop.exploratorydevices.com/product/connectivity-module-dev-kit/).

## TCode Information

- `D0` - Identity device and firmware version: `NimbleStroker_TCode_Serial_v0.1`
- `D1` - Identify TCode version: `TCode v0.4`
- `D2` - List available axes and user range preferences:
  - `L0 0 9999 Up`: **Up/down position** linear motion (default: `5000`)
    - Maps to NimbleStroker positions: -1000 to 1000
  - `V0 0 9999 Vibe`: **Vibration intensity** (default: `0`)
    - An oscillation is applied to the position when sent to the NimbleStroker.
  - `A0 0 9999 Air`: **Auxilliary air in/out valve** (default `5000`)
    - Value `0000` = air-out valve (looser)
    - Value `5000` = stop valve (default)
    - Value `9999` = air-in valve (tighter)
  - `A1 0 9999 Force`: **Force command** (default `9999`)
    - Maps to Nimblestrocker force command values: 0 to 1023
    - Controls the air pressure force of the actuator

## Usage

1. Set up [VSCode with PlatformIO](https://randomnerdtutorials.com/vs-code-platformio-ide-esp32-esp8266-arduino/)
2. Clone this repo and open the project in VSCode
3. Build and upload this program into the NimbleConModule
4. Attach the NimbleConModule to the actuator (Label A)
   - Note: Pendant connection not supported
5. Long press the Encoder Dial (2 seconds) to send a `DSTOP` command.
6. Open the PlatformIO Serial Monitor. Enter a TCode command (ie. `D2`) to test.

## Attributions

- Utilized examples from: <https://github.com/tyrm/nimblestroker/>
- Modified NimbleConSDK from: <https://github.com/ExploratoryDevices/NimbleConModule>
- Includes (with minor mods): https://github.com/Dreamer2345/Arduino_TCode_Parser
- See also [platformio.ini](./platformio.ini) for other 3rd party OSS libraries used in this project
