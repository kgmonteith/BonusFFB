# Push/pull hand control

This mode simulates a [push/pull hand control](https://www.youtube.com/watch?v=YAAfDENDDC4), an assistive driving device that allows users to operate a vehicle's throttle and brakes with a single hand-operated lever. The PPHC mode works by splitting a force-feedback joystick's Y-axis into two virtual axes: pressing the joystick forward activates the brake axis, pulling back activates the throttle axis.

## Features

The PPHC mode was designed with racing in mind:

- The throttle axis is implemented by a simple spring effect, with its output value correlating to the joystick's deflection. The farther you pull the stick, the more throttle is applied.
- The brake axis simulates the feel of stiff racing brakes, with its output value approximating the amount of pressure applied to the stick. The harder you push forward on the stick, the more brake is applied.

SimHub integration for simulating ABS and wheel lockup effects is planned but not yet implemented.