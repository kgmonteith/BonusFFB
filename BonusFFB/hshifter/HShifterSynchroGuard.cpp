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

    engineVibrationEff.dwSize = sizeof(DIEFFECT);
    engineVibrationEff.dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
    engineVibrationEff.dwDuration = INFINITE;
    engineVibrationEff.dwSamplePeriod = 0;
    engineVibrationEff.dwGain = DI_FFNOMINALMAX;
    engineVibrationEff.dwTriggerButton = DIEB_NOTRIGGER;
    engineVibrationEff.dwTriggerRepeatInterval = 0;
    engineVibrationEff.cAxes = 1;
    engineVibrationEff.rgdwAxes = &AXES[0];
    engineVibrationEff.rglDirection = &FORWARDBACK[0];
    engineVibrationEff.lpEnvelope = 0;
    engineVibrationEff.cbTypeSpecificParams = sizeof(DIPERIODIC);
    engineVibrationEff.lpvTypeSpecificParams = &engineVibration;
    engineVibrationEff.dwStartDelay = 0;
    devices->joystick->addEffect("engineVibration", { GUID_Triangle, &engineVibrationEff, DIEP_TYPESPECIFICPARAMS | DIEP_NORESTART });

    torqueLockSpringEff.dwSize = sizeof(DIEFFECT);
    torqueLockSpringEff.dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
    torqueLockSpringEff.dwDuration = INFINITE;
    torqueLockSpringEff.dwSamplePeriod = 0;
    torqueLockSpringEff.dwGain = DI_FFNOMINALMAX;
    torqueLockSpringEff.dwTriggerButton = DIEB_NOTRIGGER;
    torqueLockSpringEff.dwTriggerRepeatInterval = 0;
    torqueLockSpringEff.cAxes = 1;
    torqueLockSpringEff.rgdwAxes = &AXES[1];
    torqueLockSpringEff.rglDirection = &FORWARDBACK[1];
    torqueLockSpringEff.lpEnvelope = 0;
    torqueLockSpringEff.cbTypeSpecificParams = sizeof(DICONDITION);
    torqueLockSpringEff.lpvTypeSpecificParams = &torqueLockSpring;
    torqueLockSpringEff.dwStartDelay = 0;
    // Too much to brain out right now, add this back later
    //devices->joystick->addEffect("torqueLockSpring", { GUID_Spring, &torqueLockSpringEff, DIEP_TYPESPECIFICPARAMS | DIEP_NORESTART });

    QObject::connect(rumbleUpdateTimer, &QTimer::timeout, this, &HShifterSynchroGuard::setRumbleRPM);
    rumbleUpdateTimer->start();

    return DI_OK;
}

void HShifterSynchroGuard::updateTorqueLock() {
    // Get new joystick values
    JoystickValues joyValues = devices->getJoystickValues();

    // Get new pedal values
    PedalValues pedalValues = devices->getPedalValues();

    double clutchPercent = 1 - (double(pedalValues.clutch) / JOY_MAXPOINT);
    //throttlePercent = double(pedalValues.second) / JOY_MAXPOINT;
    double throttlePercent = double(pedalValues.throttle) / JOY_MAXPOINT;
    // Update keep-in-gear spring
    if (synchroState == SynchroState::IN_SYNCH || synchroState == SynchroState::EXITING_SYNCH) {
        // Assume throttle is applied, use pedal values for keep-in-gear force scaling
        double scaling = scaleRangeValue(throttlePercent, 0.01, 0.06);
        int maxStrength = FFB_MAX;
        double offsetScaling = 1.3;
        if (joyValues.fb < JOY_MIDPOINT && joyValues.fb > slotPattern->slotDepthAsJoystick(SLOT_ORIENTATION_FORWARD)) {
            /*
            torqueLockSpring.lOffset = slotParams->depthAsFFBOffsetFwd() - (std::abs(joystickPositionToFFBOffset(joyValues.fb) - slotParams->depthAsFFBOffsetFwd()) * offsetScaling);
            torqueLockSpring.lNegativeCoefficient = maxStrength * scaleRangeValue(joyValues.fb, slotParams->depthAsJoystickValueFwd(), slotParams->depthAsJoystickValueFwd() + 4000) * scaling * clutchPercent * -1; // AB9 1.1.3.4 firmware force inversion
            torqueLockSpring.lPositiveCoefficient = maxStrength * scaleRangeValue(joyValues.fb, slotParams->depthAsJoystickValueFwd(), slotParams->depthAsJoystickValueFwd() + 4000) * scaling * clutchPercent * -1; // AB9 1.1.3.4 firmware force inversion
            */
            double slot_depth_ffb = slotPattern->slotDepthAsFFBOffset(SLOT_ORIENTATION_FORWARD);
            double slot_depth_joystick = slotPattern->slotDepthAsJoystick(SLOT_ORIENTATION_FORWARD);
            torqueLockSpring.lOffset = slot_depth_ffb - (std::abs(joystickPositionToFFBOffset(joyValues.fb) - slot_depth_ffb) * offsetScaling);
            torqueLockSpring.lNegativeCoefficient = maxStrength * scaleRangeValue(joyValues.fb, slot_depth_joystick, slot_depth_joystick + (JOY_MIDPOINT * slotPattern->detent_zone_scale)) * scaling * clutchPercent * -1; // AB9 1.1.3.4 firmware force inversion
            torqueLockSpring.lPositiveCoefficient = maxStrength * scaleRangeValue(joyValues.fb, slot_depth_joystick, slot_depth_joystick + (JOY_MIDPOINT * slotPattern->detent_zone_scale)) * scaling * clutchPercent * -1; // AB9 1.1.3.4 firmware force inversion
        }
        else if (joyValues.fb > JOY_MIDPOINT && joyValues.fb < slotPattern->slotDepthAsJoystick(SLOT_ORIENTATION_BACK)) {
            /*
            torqueLockSpring.lOffset = slotParams->depthAsFFBOffsetBack() + (std::abs(joystickPositionToFFBOffset(joyValues.fb) - slotParams->depthAsFFBOffsetBack()) * offsetScaling);
            torqueLockSpring.lNegativeCoefficient = maxStrength * scaleRangeValue(joyValues.fb, slotParams->depthAsJoystickValueBack(), slotParams->depthAsJoystickValueBack() - 4000) * scaling * clutchPercent * -1; // AB9 1.1.3.4 firmware force inversion
            torqueLockSpring.lPositiveCoefficient = maxStrength * scaleRangeValue(joyValues.fb, slotParams->depthAsJoystickValueBack(), slotParams->depthAsJoystickValueBack() - 4000) * scaling * clutchPercent * -1; // AB9 1.1.3.4 firmware force inversion
            */
            double slot_depth_ffb = slotPattern->slotDepthAsFFBOffset(SLOT_ORIENTATION_BACK);
            double slot_depth_joystick = slotPattern->slotDepthAsJoystick(SLOT_ORIENTATION_BACK);
            torqueLockSpring.lOffset = slot_depth_ffb + (std::abs(joystickPositionToFFBOffset(joyValues.fb) - slot_depth_ffb) * offsetScaling);
            torqueLockSpring.lNegativeCoefficient = maxStrength * scaleRangeValue(joyValues.fb, slot_depth_joystick, slot_depth_joystick - (JOY_MIDPOINT * slotPattern->detent_zone_scale)) * scaling * clutchPercent * -1; // AB9 1.1.3.4 firmware force inversion
            torqueLockSpring.lPositiveCoefficient = maxStrength * scaleRangeValue(joyValues.fb, slot_depth_joystick, slot_depth_joystick - (JOY_MIDPOINT * slotPattern->detent_zone_scale)) * scaling * clutchPercent * -1; // AB9 1.1.3.4 firmware force inversion
        }
        else {
            torqueLockSpring.lNegativeCoefficient = 0;
            torqueLockSpring.lPositiveCoefficient = 0;
        }
    }
    else {
        torqueLockSpring.lNegativeCoefficient = 0;
        torqueLockSpring.lPositiveCoefficient = 0;
    }
    devices->joystick->updateEffect("torqueLockSpring");
}

/// <summary>
/// Invoked via a periodic timer set to run every millisecond
/// </summary>
void HShifterSynchroGuard::setRumbleRPM() {
    PedalValues pedalValues = devices->getPedalValues();
    JoystickValues joyValues = devices->getJoystickValues();

    double clutchPercent = 1 - (double(pedalValues.clutch) / JOY_MAXPOINT);
    double throttlePercent = double(pedalValues.throttle) / JOY_MAXPOINT;

    // Start rumbling
    if (grindingState != GrindingState::OFF) {
        double effectScaling = 0;
        JoystickValues joyValues = devices->getJoystickValues();
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
        long smoothedRPM = long(grindRPM) - long(grindRPM) % 10;
        // Apply some smoothing since the AB9 seems to struggle with changing periodic effects too frequently
        rumble.dwPeriod = 6e7 / std::abs(smoothedRPM);
        rumble.dwMagnitude = unsigned long(grind_strength * clutchPercent * std::abs(effectScaling));
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

void HShifterSynchroGuard::setEngineVibrationStrength(int value) {
    engine_vibration_strength = unsigned long(value) * 10;    // Scale to 1000
    engineVibration.dwMagnitude = engine_vibration_strength;
    if (devices != nullptr && devices->joystick->isAcquired) {
        devices->joystick->updateEffect("engineVibration");
    }
}

void HShifterSynchroGuard::setEngineRPM(int newRPM) {
    engineRPM = newRPM;
    engineVibration.dwPeriod = 6e7 / engineRPM;
    if (devices != nullptr && devices->joystick->isAcquired) {
        devices->joystick->updateEffect("engineVibration");
        //qDebug() << "newRPM: " << newRPM <<",  engineVibration.dwPeriod: " << engineVibration.dwPeriod << ", engineVibration.dwMagnitude: " << engineVibration.dwMagnitude;
    }
}

void HShifterSynchroGuard::synchroStateChanged(SynchroState newState) {
    synchroState = newState;
}

void HShifterSynchroGuard::grindingStateChanged(GrindingState newState) {
    grindingState = newState;
}

void HShifterSynchroGuard::updateGrindEffectRPM(float newRPM) {
    grindRPM = newRPM;
}

void HShifterSynchroGuard::setGrindEffectStrength(int value) {
    grind_strength = value * 100;    // Scale to 10000
}
