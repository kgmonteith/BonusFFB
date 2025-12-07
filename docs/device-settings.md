# Device settings

## MOZA AB9

!!! success "Bonus FFB fully supports the MOZA AB9 FFB base"

??? warning "You must set `Force Feedback Mode` to `DirectInput`"
    If `Force Feedback Mode` is incorrect, Bonus FFB will silently fail to send to force feedback commands to the base, resulting a 'dead stick' effect.

??? warning "You must set `Base Force Model Selection` to `Flight Base`"
    Do NOT use the `Shifter` mode. The `Shifter` mode is for Moza's built-in shifter app and overrides Bonus FFB.

### Required MOZA Cockpit settings

Under Basic Settings, change these settings:

* `Force Feedback Mode` to `DirectInput`
* `Maximum Torque Output` to `100%`
* `Overall Force Feedback Intensity` to `100%`
* `Spring` to `0`
* `Game Force Feedback Gain` to `100%`

`Damper`, `Inertia`, and `Friction` can be set to personal preference.

Under Special, change these settings:

* `Base Force Model Selection` to `Flight Base`

It's recommended to close and fully exit Moza Cockpit after configuring these settings, to avoid any interference from Moza's built-in effects, and to avoid conflicts with Moza Pithouse.