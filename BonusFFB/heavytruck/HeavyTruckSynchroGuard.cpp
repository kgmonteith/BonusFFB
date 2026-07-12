/*
Copyright (C) 2024-2026 Ken Monteith.

This file is part of Bonus FFB.

Bonus FFB is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or any later version.

Bonus FFB is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Bonus FFB. If not, see <https://www.gnu.org/licenses/>.
*/

#include "HeavyTruckSynchroGuard.h"

#include <QDebug>

HRESULT HeavyTruckSynchroGuard::start(DeviceConfiguration* devPtr, SlotPattern* spPtr, Telemetry* tPtr) {
    devices = devPtr;
    slotPattern = spPtr;
    telemetry = tPtr;

    rumbleUpdateTimer = new QTimer();
    rumbleUpdateTimer->setInterval(1);

    keepInGearSpringEff.dwSize = sizeof(DIEFFECT);
    keepInGearSpringEff.dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
    keepInGearSpringEff.dwDuration = INFINITE;
    keepInGearSpringEff.dwSamplePeriod = 0;
    keepInGearSpringEff.dwGain = DI_FFNOMINALMAX;
    keepInGearSpringEff.dwTriggerButton = DIEB_NOTRIGGER;
    keepInGearSpringEff.dwTriggerRepeatInterval = 0;
    keepInGearSpringEff.cAxes = 1;
    keepInGearSpringEff.rgdwAxes = &AXES[1];
    keepInGearSpringEff.rglDirection = &FORWARDBACK[1];
    keepInGearSpringEff.lpEnvelope = 0;
    keepInGearSpringEff.cbTypeSpecificParams = sizeof(DICONDITION);
    keepInGearSpringEff.lpvTypeSpecificParams = &keepInGearSpring;
    keepInGearSpringEff.dwStartDelay = 0;
    devices->joystick->addEffect("keepInGearSpring", { GUID_Spring, &keepInGearSpringEff, DIEP_TYPESPECIFICPARAMS | DIEP_NORESTART });

    torqueLoadSpringEff.dwSize = sizeof(DIEFFECT);
    torqueLoadSpringEff.dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
    torqueLoadSpringEff.dwDuration = INFINITE;
    torqueLoadSpringEff.dwSamplePeriod = 0;
    torqueLoadSpringEff.dwGain = DI_FFNOMINALMAX;
    torqueLoadSpringEff.dwTriggerButton = DIEB_NOTRIGGER;
    torqueLoadSpringEff.dwTriggerRepeatInterval = 0;
    torqueLoadSpringEff.cAxes = 1;
    torqueLoadSpringEff.rgdwAxes = &AXES[1];
    torqueLoadSpringEff.rglDirection = &FORWARDBACK[1];
    torqueLoadSpringEff.lpEnvelope = 0;
    torqueLoadSpringEff.cbTypeSpecificParams = sizeof(DICONDITION);
    torqueLoadSpringEff.lpvTypeSpecificParams = &torqueLoadSpring;
    torqueLoadSpringEff.dwStartDelay = 0;
    devices->joystick->addEffect("torqueLoadSpring", { GUID_Spring, &torqueLoadSpringEff, DIEP_TYPESPECIFICPARAMS | DIEP_NORESTART });
    
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
    devices->joystick->addEffect("rumble", { grindEffectShape, &rumbleEff, DIEP_TYPESPECIFICPARAMS | DIEP_NORESTART });

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
    devices->joystick->addEffect("rumblePushback", { GUID_ConstantForce, &rumblePushbackEff, DIEP_TYPESPECIFICPARAMS | DIEP_NORESTART });

    handsOffEff.dwSize = sizeof(DIEFFECT);
    handsOffEff.dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
    handsOffEff.dwDuration = INFINITE;
    handsOffEff.dwSamplePeriod = 0;
    handsOffEff.dwGain = DI_FFNOMINALMAX;
    handsOffEff.dwTriggerButton = DIEB_NOTRIGGER;
    handsOffEff.dwTriggerRepeatInterval = 0;
    handsOffEff.cAxes = 2;
    handsOffEff.rgdwAxes = AXES;
    handsOffEff.rglDirection = FORWARDBACK;
    handsOffEff.lpEnvelope = 0;
    handsOffEff.cbTypeSpecificParams = sizeof(DICONDITION) * 2;
    handsOffEff.lpvTypeSpecificParams = &handsOffCondition;
    handsOffEff.dwStartDelay = 0;
    devices->joystick->addEffect("handsOff", { GUID_Friction, &handsOffEff, DIEP_TYPESPECIFICPARAMS | DIEP_NORESTART });

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

    QObject::connect(rumbleUpdateTimer, &QTimer::timeout, this, &HeavyTruckSynchroGuard::setRumbleRPM);
    rumbleUpdateTimer->start();
    return DI_OK;
}

void HeavyTruckSynchroGuard::setGrindEffectShape(int index) {
    if (index == 0) {
        grindEffectShape = GUID_Triangle;
    }
    else if (index == 1) {
        grindEffectShape = GUID_Sine;
    }
    else if (index == 2) {
        grindEffectShape = GUID_SawtoothUp;
    }
    else {
        grindEffectShape = GUID_Square;
    }
}

void HeavyTruckSynchroGuard::setTorqueLoadStrength(int value) {
    torqueLoadSpringStrength = value * -100;
}

void HeavyTruckSynchroGuard::updateTorqueLock() { // int clutchValue, int throttleValue, int fbValue) {
    // Get new joystick values
    JoystickValues joyValues = devices->getJoystickValues2();

    // Get new pedal values
    PedalValues pedalValues = devices->getPedalValues();

    clutchPercent = 1 - (double(pedalValues.clutch) / JOY_MAXPOINT);
    //throttlePercent = double(pedalValues.second) / JOY_MAXPOINT;
    throttlePercent = telemetry->getThrottlePercent();
    // Update keep-in-gear spring
    if ((synchroState == HeavyTruckSynchroState::IN_SYNCH || synchroState == HeavyTruckSynchroState::EXITING_SYNCH) && applyIdleTorqueLock) {
        // Assume throttle is applied, use pedal values for keep-in-gear force scaling
        double scaling = scaleRangeValue(throttlePercent, 0.01, 0.06);
        int maxStrength = keepInGearSpringMaxCoefficient;
        double offsetScaling = 1.3;
        if ((FFB_MAX * scaling) < keepInGearSpringIdleCoefficient) {
            // Throttle is not actually applied, scale the keep-in-gear effect by the truck speed
            // If not moving, apply a minimum of 33% of the idle torque lock to keep the stick in gear
            scaling = max(scaleRangeValue(telemetry->getSpeed(), 0, 2.2), 0.33);
            float nearNeutralScaling = scaleRangeValue(std::abs(joystickPositionToFFBOffset(joyValues.fb)), 1000, 2500);
            maxStrength = keepInGearSpringIdleCoefficient * nearNeutralScaling;
            offsetScaling = 0;
            //qDebug() << "nearNeutralScaling: " << nearNeutralScaling << "speed: " << speed << ", maxStrength: " << maxStrength << ", scaling: " << scaling;
        }
        //if (joyValues.fb < JOY_MIDPOINT && joyValues.fb > slotParams->depthAsJoystickValueFwd()) {
        if (joyValues.fb < JOY_MIDPOINT && joyValues.fb > slotPattern->slotDepthAsJoystick(SLOT_ORIENTATION_FORWARD)) {
            /*
            keepInGearSpring.lOffset = slotParams->depthAsFFBOffsetFwd() - (std::abs(joystickPositionToFFBOffset(joyValues.fb) - slotParams->depthAsFFBOffsetFwd()) * offsetScaling);
            keepInGearSpring.lNegativeCoefficient = maxStrength * scaleRangeValue(joyValues.fb, slotParams->depthAsJoystickValueFwd(), slotParams->depthAsJoystickValueFwd() + 4000) * scaling * clutchPercent * -1; // AB9 1.1.3.4 firmware force inversion
            keepInGearSpring.lPositiveCoefficient = maxStrength * scaleRangeValue(joyValues.fb, slotParams->depthAsJoystickValueFwd(), slotParams->depthAsJoystickValueFwd() + 4000) * scaling * clutchPercent * -1; // AB9 1.1.3.4 firmware force inversion
            */
            double slot_depth_ffb = slotPattern->slotDepthAsFFBOffset(SLOT_ORIENTATION_FORWARD);
            double slot_depth_joystick = slotPattern->slotDepthAsJoystick(SLOT_ORIENTATION_FORWARD);
            keepInGearSpring.lOffset = slot_depth_ffb - (std::abs(joystickPositionToFFBOffset(joyValues.fb) - slot_depth_ffb) * offsetScaling);
            keepInGearSpring.lNegativeCoefficient = maxStrength * scaleRangeValue(joyValues.fb, slot_depth_joystick, slot_depth_joystick + 4000) * scaling * clutchPercent * -1; // AB9 1.1.3.4 firmware force inversion
            keepInGearSpring.lPositiveCoefficient = maxStrength * scaleRangeValue(joyValues.fb, slot_depth_joystick, slot_depth_joystick + 4000) * scaling * clutchPercent * -1; // AB9 1.1.3.4 firmware force inversion
        }
        else if (joyValues.fb > JOY_MIDPOINT && joyValues.fb < slotPattern->slotDepthAsJoystick(SLOT_ORIENTATION_BACK)) {
            /*
            keepInGearSpring.lOffset = slotParams->depthAsFFBOffsetBack() + (std::abs(joystickPositionToFFBOffset(joyValues.fb) - slotParams->depthAsFFBOffsetBack()) * offsetScaling);
            keepInGearSpring.lNegativeCoefficient = maxStrength * scaleRangeValue(joyValues.fb, slotParams->depthAsJoystickValueBack(), slotParams->depthAsJoystickValueBack() - 4000) * scaling * clutchPercent * -1; // AB9 1.1.3.4 firmware force inversion
            keepInGearSpring.lPositiveCoefficient = maxStrength * scaleRangeValue(joyValues.fb, slotParams->depthAsJoystickValueBack(), slotParams->depthAsJoystickValueBack() - 4000) * scaling * clutchPercent * -1; // AB9 1.1.3.4 firmware force inversion
            */
            double slot_depth_ffb = slotPattern->slotDepthAsFFBOffset(SLOT_ORIENTATION_BACK);
            double slot_depth_joystick = slotPattern->slotDepthAsJoystick(SLOT_ORIENTATION_BACK);
            keepInGearSpring.lOffset = slot_depth_ffb + (std::abs(joystickPositionToFFBOffset(joyValues.fb) - slot_depth_ffb) * offsetScaling);
            keepInGearSpring.lNegativeCoefficient = maxStrength * scaleRangeValue(joyValues.fb, slot_depth_joystick, slot_depth_joystick - 4000) * scaling * clutchPercent * -1; // AB9 1.1.3.4 firmware force inversion
            keepInGearSpring.lPositiveCoefficient = maxStrength * scaleRangeValue(joyValues.fb, slot_depth_joystick, slot_depth_joystick - 4000) * scaling * clutchPercent * -1; // AB9 1.1.3.4 firmware force inversion
        }
        else {
            keepInGearSpring.lNegativeCoefficient = 0;
            keepInGearSpring.lPositiveCoefficient = 0;
        }
        //qDebug() << "keepInGearSpring.lOffset: " << keepInGearSpring.lOffset << ", keepInGearSpring.lPositiveCoefficient: " << keepInGearSpring.lPositiveCoefficient;
        // Apply engine torque load spring
        scaling = torqueLoadSpringStrength * scaleRangeValue(throttlePercent, 0.01, 1) * clutchPercent  * -1; // AB9 1.1.3.4 firmware force inversion
        torqueLoadSpring.lNegativeCoefficient = scaling;
        torqueLoadSpring.lPositiveCoefficient = scaling;
        int handsOffDamper = FFB_MAX * scaleRangeValue(throttlePercent, 0.01, 1) * clutchPercent;
        handsOffCondition[0] = { 0, handsOffDamper, handsOffDamper };
        handsOffCondition[1] = { 0, handsOffDamper, handsOffDamper };
    }
    else {
        if (synchroState == HeavyTruckSynchroState::IN_SYNCH && !applyIdleTorqueLock && (joyValues.fb <= slotPattern->slotDepthAsJoystick(SLOT_ORIENTATION_FORWARD) || joyValues.fb >= slotPattern->slotDepthAsJoystick(SLOT_ORIENTATION_BACK))) {
            // Only start applying the idle torque lock if the stick has reached the end of the slot
            applyIdleTorqueLock = true;
            //qDebug() << "Enabling idle torque lock";
        }
        else {
            applyIdleTorqueLock = false;
            //qDebug() << "Disabling idle torque lock";
        }
        keepInGearSpring.lNegativeCoefficient = 0;
        keepInGearSpring.lPositiveCoefficient = 0;
        torqueLoadSpring.lNegativeCoefficient = 0;
        torqueLoadSpring.lPositiveCoefficient = 0;
        handsOffCondition[0] = { 0, 0, 0 };
        handsOffCondition[1] = { 0, 0, 0 };
    }
    devices->joystick->updateEffect("keepInGearSpring");
    devices->joystick->updateEffect("torqueLoadSpring");
    devices->joystick->updateEffect("handsOff");
}


void HeavyTruckSynchroGuard::synchroStateChanged(HeavyTruckSynchroState newState) {
    synchroState = newState;
}

void HeavyTruckSynchroGuard::grindingStateChanged(HeavyTruckGrindingState newState) {
    grindingState = newState;
}

/// <summary>
/// Invoked via a periodic timer set to run every millisecond
/// </summary>
void HeavyTruckSynchroGuard::setRumbleRPM() {
    // Start rumbling
    if (grindingState != HeavyTruckGrindingState::OFF) {
        double effectScaling = 0;
        JoystickValues joyValues = devices->getJoystickValues2();
        double grind_depth_scaled = JOY_MIDPOINT * slotPattern->grind_zone_scale;
        if (grindingState == HeavyTruckGrindingState::GRINDING_FWD)
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
        double revMatchPushbackScaling = max(0.20, scaleRangeValue(std::abs(grindEffectRPM), 0, maxRevMatchRPM));
        rumblePushback.lMagnitude = FFB_MAX * effectScaling * clutchPercent * revMatchPushbackScaling * -1; // AB9 1.1.3.4 firmware force inversion
        devices->joystick->updateEffect("rumblePushback");
        long smoothedRPM = long(grindEffectRPM) - long(grindEffectRPM) % 10;
        double revMatchRumbleScaling = scaleRangeValue(std::abs(smoothedRPM), 0, min(300.0, maxRevMatchRPM * 2.0));
        // Apply some smoothing since the AB9 seems to struggle with changing periodic effects too frequently
        rumble.dwPeriod = 6e7 / std::abs(smoothedRPM);
        rumble.dwMagnitude = unsigned long(grindingIntensity * clutchPercent * std::abs(effectScaling) * revMatchRumbleScaling);
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

void HeavyTruckSynchroGuard::updateEngineRPM(float newRPM) {
    engineVibration.dwPeriod = 6e7 / newRPM;
    engineVibration.dwMagnitude = engineVibrationIntensity;
    if (devices != nullptr && devices->joystick->isAcquired) {
        devices->joystick->updateEffect("engineVibration");
        //qDebug() << "newRPM: " << newRPM <<",  engineVibration.dwPeriod: " << engineVibration.dwPeriod << ", engineVibration.dwMagnitude: " << engineVibration.dwMagnitude;
    }
}

void HeavyTruckSynchroGuard::updateGrindEffectRPM(float newRPM) {
    grindEffectRPM = newRPM;
}

void HeavyTruckSynchroGuard::setGrindEffectIntensity(int value) {
    grindingIntensity = value * 100;    // Scale to 10000
}

void HeavyTruckSynchroGuard::setEngineVibrationIntensity(int value) {
    engineVibrationIntensity = unsigned long(value) * 10;    // Scale to 1000
    if (devices != nullptr && devices->joystick->isAcquired) {
        engineVibration.dwMagnitude = engineVibrationIntensity;
        devices->joystick->updateEffect("engineVibration");
    }
}

void HeavyTruckSynchroGuard::setKeepInGearIdleIntensity(int value) {
    keepInGearSpringIdleCoefficient = value * 100;
}

void HeavyTruckSynchroGuard::setMaxRevMatchRPM(int value) {
    maxRevMatchRPM = value;
}