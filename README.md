# Bonus FFB

Some notes to self...

ATS gear slot mapping (HShifterSlot property value vs. marking on shifter):
0 - Neutral
1 - Reverse
2 - '1'
3 - '2'
4 - '3'
5 - '4'
6 - '5'
So if we only implement 6 slots, we'll be sending values 0,2..7, skip 1. This should be fine since the real-world truck transmissions also use six slots, so slot 2/'1' is R1, 3/'2' is LL, 4/'3' is 1L, etc.
The HShifterSlot property is set when the shift gear is selected, regardless of whether the gear is correctly engaged. (I.e., it will report you are in first gear if you slot into it without pressing the clutch.)

The GearValues.Selected property is not set until the gear is correctly engaged. It reports the gear selected per the transmission setting, numerically. So the values don't match, but we know whether a gear is correctly engaged just this variable being != 0.
On an EF18 transmission, the values are -1 for R1, 1 for LL, 3 for 1L, 5 for 2L, etc. There does not seem to be a property that specifies the particular transmission type in use, so, we probably don't want to try to interpret the value ourselves.

The game has a clutch range setting that specifies how much the the clutch should be depressed before the gear can be engaged. When the value is set to 100%, the clutch must be pressed down 91% to engage first from neutral.

The gear cannot be disengaged when any amount of throttle is applied.

The gearbox grind frequency is proportional to the engine RPMs, possible the exactly value. In any case, fewer RPMs == slower vibration, higher RPM == faster vibration