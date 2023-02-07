# Change log

## v0.3 - 02/06/2023
- Fix: Divide by zero crash during vibration calculations.
- `L0` Up Position: mapped range was changed from (-1000 -> 1000) to (-750 -> 750). Based on analysis of Pendant behavior which does not send values higher than 750. Testing values at 1000 causes slamming of the actuator piston to occur against its enclosure.
- `V0` Vibration Amplitude: internal range was changed from (0 -> 20) to (0 -> 25) position units. Its default is now 25, which matches more closely with the NimbleStroker Pendant behavior.
- `V0` Vibration Amplitude: changes will now ease in and out rather than a linear change.
- `A2` Vibration Speed: default was changed from 10hz to 20hz.

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
