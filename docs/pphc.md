# Push/pull hand control

This mode simulates a [push/pull hand control](https://www.youtube.com/watch?v=YAAfDENDDC4), an assistive driving device that allows users to operate a vehicle's throttle and brakes with a single hand-operated lever. The PPHC mode works by splitting a force-feedback joystick's Y-axis into two virtual axes: pressing the joystick forward activates the brake axis, pulling back activates the throttle axis.

## Features

The PPHC mode was designed with racing in mind:

- The throttle axis is implemented by a simple spring effect, with its output value correlating to the joystick's deflection. The farther you pull the stick, the more throttle is applied.
- The brake axis simulates the feel of stiff racing brakes, with its output value approximating the amount of pressure applied to the stick. The harder you push forward on the stick, the more brake is applied.
    - This is an approximation; there is no support for actual force-sensing with the DirectInput force-feedback libraries. Thoughtful tuning of the brake spring and brake axis scaling is recommended.

SimHub integration for simulating ABS and wheel lockup effects is planned but not yet implemented.

The PPHC mode should work in every game. It's been tested in Assetto Corsa, ACC, and ATS/ETS2.

## Game configuration

Start the PPHC mode, then bind the axes in game as you would a regular pedal, by selecting the axis and moving the joystick. Push forward to assign the brake axis, pull back to assign the throttle axis.

## Settings descriptions

### Brake settings

- **<span id="brake-spring-scaling">Brake spring scaling:</span>** Controls how aggressively the brake spring effect increases when pushing the stick forward. The higher the value, the stiffer the spring will feel.
- **<span id="brake-axis-deadzone">Brake axis deadzone:</span>** Sets the deadzone for the brake axis. This is a portion of the approximated force, *not* joystick deflection. Tweak this value such that the initial brake bite point feels correct in-game.
- **<span id="brake-axis-scaling">Brake axis scaling:</span>** Controls the scaling of the output brake axis value. Lower values mean more stick deflection is needed for full braking force, higher values mean less deflection is needed. It's recommended to tweak this value such that maximum brake force occurs before the joystick pushes back with its maximum force.

### Throttle settings

- **<span id="throttle-spring-strength">Throttle spring strength:</span>** Sets the strength of the throttle spring. Higher values mean a stronger spring.
- **<span id="throttle-axis-deadzone">Throttle axis deadzone:</span>** Sets the throttle axis deadzone. Unlike the brake axis deadzone, this is proportional to joystick deflection.
- **<span id="throttle-slot-depth">Throttle slot depth:</span>** Sets the depth of the throttle slot. The throttle output value scales to 100% upon reaching the end of the throttle slot.