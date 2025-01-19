# Device settings

## MOZA AB9

!!! success "Bonus FFB fully supports the MOZA AB9 FFB base"

??? warning "You must set `Force Feedback Mode` to `DirectInput` or `Integrated FFB`"
    If `Force Feedback Mode` is incorrect, Bonus FFB will silently fail to send to force feedback commands to the base, resulting a 'dead stick' effect.

As of January 2025, MOZA's Pit House software crashes when an AB9 FFB base is powered on. This usually doesn't matter, just be sure to fully configure your wheel before powering on the AB9 base. It does prevent devices that must be run through Pit House from running correctly&mdash;for example, the MOZA Stalks in ATS/ETS2 compatibility mode.

### Required MOZA Cockpit settings

Under Basic Settings, change these settings:

* `Force Feedback Mode` to `DirectInput` or `Integrated FFB`
* `Maximum Torque Output` to `100%`
* `Overall Force Feedback Intensity` to `100%`
* `Spring` to `0`
* `Game Force Feedback Gain` to `200%`&ndash;`250%`

`Damper`, `Inertia`, and `Friction` can be set to personal preference.