/*
Copyright (C) 2024-2026 Ken Monteith.

This file is part of Bonus FFB.

Bonus FFB is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or any later version.

Bonus FFB is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Bonus FFB. If not, see <https://www.gnu.org/licenses/>.
*/

#include "HShifterSynchroGuard.h"

#include <QDebug>
    
HRESULT HShifterSynchroGuard::start(DeviceConfiguration* devPtr, SlotPattern* spPtr) {
    devices = devPtr;
    slotPattern = spPtr;

    rumbleUpdateTimer = new QTimer();
    rumbleUpdateTimer->setInterval(1);

    rumbleEff.dwSize = sizeof(DIEFFECT);
    rumbleEff.dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
    rumbleEff.dwDuration = INFINITE;
    rumbleEff.dwSamplePeriod = 0;
    rumbleEff.dwGain = DI_FFNOMINALMAX;
    rumbleEff.dwTriggerButton = DIEB_NOTRIGGER;
    rumbleEff.dwTriggerRepeatInterval = 0;
    rumbleEff.cAxes = 1;
    rumbleEff.rgdwAxes = &AXES[1];
    rumbleEff.rglDirection = &FORWARDBACK[1];
    rumbleEff.lpEnvelope = 0;
    rumbleEff.cbTypeSpecificParams = sizeof(DIPERIODIC);
    rumbleEff.lpvTypeSpecificParams = &rumble;
    rumbleEff.dwStartDelay = 0;
    devices->joystick->addEffect("rumble", { GUID_Triangle, &rumbleEff, DIEP_TYPESPECIFICPARAMS });

    rumblePushbackEff.dwSize = sizeof(rumblePushbackEff);
    rumblePushbackEff.dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
    rumblePushbackEff.dwDuration = INFINITE;
    rumblePushbackEff.dwSamplePeriod = 0;
    rumblePushbackEff.dwGain = DI_FFNOMINALMAX; // Max gain applied to the effect
    rumblePushbackEff.dwTriggerButton = DIEB_NOTRIGGER;
    rumblePushbackEff.dwTriggerRepeatInterval = 0;
    rumblePushbackEff.cAxes = 1;
    rumblePushbackEff.rgdwAxes = &AXES[1];
    rumblePushbackEff.rglDirection = &FORWARDBACK[1];
    rumblePushbackEff.cbTypeSpecificParams = sizeof(DICONSTANTFORCE);
    rumblePushbackEff.lpvTypeSpecificParams = &rumblePushback;
    rumblePushbackEff.dwStartDelay = 0;
    devices->joystick->addEffect("rumblePushback", { GUID_ConstantForce, &rumblePushbackEff });

    QObject::connect(rumbleUpdateTimer, &QTimer::timeout, this, &HShifterSynchroGuard::setRumbleRPM);
    rumbleUpdateTimer->start();

    return DI_OK;
}

/// <summary>
/// Invoked via a periodic timer set to run every millisecond
/// </summary>
void HShifterSynchroGuard::setRumbleRPM() {
    PedalValues pedalValues = devices->getPedalValues();
    JoystickValues joyValues = devices->getJoystickValues2();

    double clutchPercent = 1 - (double(pedalValues.clutch) / JOY_MAXPOINT);
    double throttlePercent = double(pedalValues.throttle) / JOY_MAXPOINT;

    // Start rumbling
    if (grindingState != GrindingState::OFF) {
        double effectScaling = 0;
        JoystickValues joyValues = devices->getJoystickValues2();
        double grind_depth_scaled = JOY_MIDPOINT * slotPattern->grind_zone_scale;
        if (grindingState == GrindingState::GRINDING_FWD)
        {
            //effectScaling = scaleRangeValue(joyValues.fb, slotParams->grindPointDepthAsJoystickValueFwd(), slotParams->grindPointDepthAsJoystickValueFwd() - grindPushbackScalingRange) * -1;
            double grind_point_fwd = JOY_MIDPOINT - grind_depth_scaled;
            effectScaling = scaleRangeValue(joyValues.fb, grind_point_fwd, grind_point_fwd - grindPushbackScalingRange) * -1;
        }
        else
        {
            //effectScaling = scaleRangeValue(joyValues.fb, slotParams->grindPointDepthAsJoystickValueBack(), slotParams->grindPointDepthAsJoystickValueBack() + grindPushbackScalingRange);
            double grind_point_back = JOY_MIDPOINT + grind_depth_scaled;
            effectScaling = scaleRangeValue(joyValues.fb, grind_point_back, grind_point_back + grindPushbackScalingRange);
        }
        rumblePushback.lMagnitude = FFB_MAX * effectScaling * clutchPercent * -1; // AB9 1.1.3.4 firmware force inversion
        devices->joystick->updateEffect("rumblePushback");
        long smoothedRPM = long(grindEffectRPM) - long(grindEffectRPM) % 10;
        // Apply some smoothing since the AB9 seems to struggle with changing periodic effects too frequently
        rumble.dwPeriod = 6e7 / std::abs(smoothedRPM);
        rumble.dwMagnitude = unsigned long(grindingIntensity * clutchPercent * std::abs(effectScaling));
        //qDebug() << "revMatchRumbleScaling: " << revMatchRumbleScaling << "smoothedRPM: " << smoothedRPM <<  "rumble.dwPeriod: " << rumble.dwPeriod << "rumble.dwMagnitude: " << rumble.dwMagnitude;
        devices->joystick->updateEffect("rumble");
    }
    else {
        // Stop rumbling
        if (rumble.dwMagnitude != 0) {
            rumble.dwMagnitude = 0;
            devices->joystick->updateEffect("rumble");
        }
        if (rumblePushback.lMagnitude != 0) {
            rumblePushback.lMagnitude = 0;
            devices->joystick->updateEffect("rumblePushback");
        }
    }
}

void HShifterSynchroGuard::synchroStateChanged(SynchroState newState) {
    synchroState = newState;
}

void HShifterSynchroGuard::grindingStateChanged(GrindingState newState) {
    grindingState = newState;
}

void HShifterSynchroGuard::updateEngineRPM(float newRPM) {
    engineRPM = newRPM;
}

float HShifterSynchroGuard::computeGrindRPM() {
    if (grindEffectBehavior == GrindEffectBehavior::MATCH_ENGINE_RPM && engineRPM) {
        return engineRPM;
    }
    else if (grindEffectBehavior == GrindEffectBehavior::ADD_ENGINE_RPM) {
        return engineRPM + grindEffectRPM;
    }
    return grindEffectRPM;
}

void HShifterSynchroGuard::updateGrindEffectRPM(float newRPM) {
    grindEffectRPM = newRPM;
}

void HShifterSynchroGuard::setGrindEffectIntensity(int value) {
    grindingIntensity = value * 100;    // Scale to 10000
}
