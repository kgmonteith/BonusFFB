# Getting started

!!! danger "Using FFB devices for anything other than their intended purpose may result in damage or injury"
    The authors of Bonus FFB accept no liability for any loss or damage including, without limitation, indirect or consequential loss or damage arising out of or in connection with the use of the software. Use Bonus FFB at your own risk.

## Install vJoy

Install a recent version of [vJoy](https://github.com/jshafer817/vJoy/releases/tag/v2.1.9.1).

Run vJoyConf and set up at least one virtual device with a minimum of 8 buttons:

## Install optional telemetry plugins

### American Truck Simulator/Euro Truck Simulator 2

Install [RenCloud's scs-sdk-plugin](https://github.com/RenCloud/scs-sdk-plugin/releases) DLL to the `bin\win_x64\plugins` folder of your ATS and ETS2 installations. These are the default locations when using Steam:

* `C:\Program Files (x86)\Steam\steamapps\common\American Truck Simulator\bin\win_x64\plugins\`
* `C:\Program Files (x86)\Steam\steamapps\common\Euro Truck Simulator 2\bin\win_x64\plugins\`

??? tip "When installed correctly, ATS/ETS2 will start with a notice that the SDK has been activated."
    Unfortunately you will have to press OK for this message each time the game is launched.

## Configure your FFB joystick

Follow the [device settings guide](device-settings.md) for your specific FFB joystick.

!!! warning "Some FFB joysticks will silently fail to work with Bonus FFB until correctly configured."

## Install and configure Bonus FFB


Download and run the latest [Bonus FFB installer](https://github.com/kgmonteith/BonusFFB/releases).

Bonus FFB installs several executables, one for each application:

* [H-Shifter](hshifter.md)
* [Handbrake](handbrake.md)

Launch your desired application and open the `Input/output settings` tab. Select your FFB joystick and other devices as required by the application. Select the vJoy device you configured earlier.

If your FFB joystick and other devices are correctly detected and configured, you can start the app by pressing the ▶️ button.

Please read the application's guide for app-specific configuration, options, and features.