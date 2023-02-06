# Change log

## v0.3
- Reduced the maximum setting for the vibration amplitude from 25 to 20 (position units).

## v0.2 - 02/05/2023
- Added new aux axis for controlling vibration speed via tcode
  - `A2 0 9999 VibSpeed`: **Vibration speed** (default: `5000`)
  - Maps to an oscillation speed for vibration: 0 to 20.0hz (default 10.0hz)
- Changed Encoder button behavior:
  - `Long press (2sec)`: Stop sending commands to the actuator
  - `Double click`: Resume sending commands to the actuator
- Changed LED behavior so that vibration is more visible
- Added vibration control to buttplug-device.config.json example in README
- Removed TSTOP when tempLimiting is high, but added logging when it occurs

## v0.1 - 02/04/2023
- Initial PoC release
- RFC on axes spec
