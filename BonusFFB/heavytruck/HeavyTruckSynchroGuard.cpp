/*
Copyright (C) 2024-2026 Ken Monteith.

This file is part of Bonus FFB.

Bonus FFB is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or any later version.

Bonus FFB is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Bonus FFB. If not, see <https://www.gnu.org/licenses/>.
*/

#include "HeavyTruckSynchroGuard.h"

#include <QDebug>

HRESULT HeavyTruckSynchroGuard::start(DeviceInfo* devPtr, SlotParameters* sPtr) {
    device = devPtr;
    slot = sPtr;
    rumbleUpdateTimer = new QTimer();
    rumbleUpdateTimer->setInterval(20);

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
    device->addEffect("keepInGearSpring", { GUID_Spring, &keepInGearSpringEff });

    keepInGearEff.dwSize = sizeof(DIEFFECT);
    keepInGearEff.dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
    keepInGearEff.dwDuration = INFINITE;
    keepInGearEff.dwSamplePeriod = 0;
    keepInGearEff.dwGain = DI_FFNOMINALMAX;
    keepInGearEff.dwTriggerButton = DIEB_NOTRIGGER;
    keepInGearEff.dwTriggerRepeatInterval = 0;
    keepInGearEff.cAxes = 1;
    keepInGearEff.rgdwAxes = &AXES[1];
    keepInGearEff.rglDirection = &FORWARDBACK[1];
    keepInGearEff.lpEnvelope = 0;
    keepInGearEff.cbTypeSpecificParams = sizeof(DICONSTANTFORCE);
    keepInGearEff.lpvTypeSpecificParams = &keepInGearForce;
    keepInGearEff.dwStartDelay = 0;
    //device->addEffect("keepInGearEff", { GUID_ConstantForce, &keepInGearEff });

    rumbleEff.dwSize = sizeof(DIEFFECT);
    rumbleEff.dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
    rumbleEff.dwDuration = INFINITE;
    rumbleEff.dwSamplePeriod = 0;
    rumbleEff.dwGain = DI_FFNOMINALMAX;
    rumbleEff.dwTriggerButton = DIEB_NOTRIGGER;
    rumbleEff.dwTriggerRepeatInterval = 0;
    rumbleEff.cAxes = 1;
    rumbleEff.rgdwAxes = &AXES[0];
    rumbleEff.rglDirection = &FORWARDBACK[0];
    rumbleEff.lpEnvelope = 0;
    rumbleEff.cbTypeSpecificParams = sizeof(DIPERIODIC);
    rumbleEff.lpvTypeSpecificParams = &rumble;
    rumbleEff.dwStartDelay = 0;
    device->addEffect("rumble", { grindEffectShape, &rumbleEff, DIEP_TYPESPECIFICPARAMS });
    //device->addEffect("rumble", { GUID_Sine, &rumbleEff, DIEP_TYPESPECIFICPARAMS });
    //device->addEffect("rumble", { GUID_Square, &rumbleEff, DIEP_TYPESPECIFICPARAMS });
    //device->addEffect("rumble", { GUID_SawtoothUp, &rumbleEff, DIEP_TYPESPECIFICPARAMS });

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
    device->addEffect("rumblePushback", { GUID_ConstantForce, &rumblePushbackEff });

    QObject::connect(rumbleUpdateTimer, &QTimer::timeout, this, &HeavyTruckSynchroGuard::setRumbleRPM);
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

void HeavyTruckSynchroGuard::updatePedalEngagement(QPair<int, int> pedalValues, QPair<int, int> joystickValues) { // int clutchValue, int throttleValue, int fbValue) {
    clutchPercent = 1 - (double(pedalValues.first) / JOY_MAXPOINT);
    throttlePercent = double(pedalValues.second) / JOY_MAXPOINT;
    int fbValue = joystickValues.second;
    // Update keep-in-gear spring
    if ((synchroState == HeavyTruckSynchroState::IN_SYNCH || synchroState == HeavyTruckSynchroState::EXITING_SYNCH)) {
        if (throttlePercent > .01) {
            double pedalScaling = scaleRangeValue(throttlePercent, 0.01, 0.3);
            if (fbValue < JOY_MIDPOINT && fbValue > slot->depthAsJoystickValueFwd()) {
                keepInGearSpring.lOffset = slot->depthAsFFBOffsetFwd() - (std::abs(joystickPositionToFFBOffset(fbValue) - slot->depthAsFFBOffsetFwd()) * 3);
                keepInGearSpring.lNegativeCoefficient = 10000 * scaleRangeValue(fbValue, slot->depthAsJoystickValueFwd(), slot->depthAsJoystickValueFwd() + 4000) * pedalScaling;
                keepInGearSpring.lPositiveCoefficient = 10000 * scaleRangeValue(fbValue, slot->depthAsJoystickValueFwd(), slot->depthAsJoystickValueFwd() + 4000) * pedalScaling;
            }
            else if (fbValue > JOY_MIDPOINT && fbValue < slot->depthAsJoystickValueBack()) {
                keepInGearSpring.lOffset = slot->depthAsFFBOffsetBack() + (std::abs(joystickPositionToFFBOffset(fbValue) - slot->depthAsFFBOffsetBack()) * 3);
                keepInGearSpring.lNegativeCoefficient = 10000 * scaleRangeValue(fbValue, slot->depthAsJoystickValueBack(), slot->depthAsJoystickValueBack() - 4000) * pedalScaling;
                keepInGearSpring.lPositiveCoefficient = 10000 * scaleRangeValue(fbValue, slot->depthAsJoystickValueBack(), slot->depthAsJoystickValueBack() - 4000) * pedalScaling;
            }
            else {
                keepInGearSpring.lNegativeCoefficient = 0;
                keepInGearSpring.lPositiveCoefficient = 0;
            }
            //qDebug() << "keepInGearSpring.lOffset: " << keepInGearSpring.lOffset << ", keepInGearSpring.lPositiveCoefficient: " << keepInGearSpring.lPositiveCoefficient;
        }
        else {
            keepInGearSpring.lNegativeCoefficient = 0;
            keepInGearSpring.lPositiveCoefficient = 0;
        }
        device->updateEffect("keepInGearSpring");
    }
}


void HeavyTruckSynchroGuard::synchroStateChanged(HeavyTruckSynchroState newState, int fbValue) {
    /*
    if (newState == HeavyTruckSynchroState::IN_SYNCH) {
        if (fbValue < JOY_MIDPOINT) {
            keepInGearSpring.lOffset = slot->depthAsFFBOffsetFwd();
        }
        else {
            keepInGearSpring.lOffset = slot->depthAsFFBOffsetBack();
        }
        keepInGearSpring.lNegativeCoefficient = 10000;
        keepInGearSpring.lPositiveCoefficient = 10000;
        device->updateEffect("keepInGearSpring");
        // Activate keep-in-gear spring
        /*
        float phaseOut = 1.0;
        if (fbValue <= JOY_MIDPOINT) {
            phaseOut = scaleRangeValue(fbValue, JOY_QUARTERPOINT - 5000, JOY_QUARTERPOINT + 5000);
        }
        else {
            phaseOut = scaleRangeValue(fbValue, JOY_THREEQUARTERPOINT + 5000, JOY_THREEQUARTERPOINT - 5000);
        }
        keepInGearSpring.lNegativeCoefficient = keepInGearSpringIdleCoefficient * clutchPercent * -1;
        device->updateEffect("keepInGearSpring");
    }
    else if (newState == HeavyTruckSynchroState::ENTERING_SYNCH) {
        // Deactivate keep-in-gear spring
        keepInGearSpring.lNegativeCoefficient = 0;
        keepInGearSpring.lPositiveCoefficient = 0;
        device->updateEffect("keepInGearSpring");
    }*/
    synchroState = newState;
}

void HeavyTruckSynchroGuard::grindingStateChanged(HeavyTruckGrindingState newState) {
    grindingState = newState;
    if (grindingState != HeavyTruckGrindingState::OFF) {
        // Start rumbling
        setRumbleRPM();
        rumbleUpdateTimer->start();
    }
    else {
        // Stop rumbling
        rumbleUpdateTimer->stop();
        rumble.dwMagnitude = 0;
        rumble.dwPhase = 0;
        rumblePushback.lMagnitude = 0;
        device->updateEffect("rumble");
        device->updateEffect("rumblePushback");
    }
}

void HeavyTruckSynchroGuard::setRumbleRPM() {
    double effectScaling = scaleRangeValue(fbValue, grind_point_depth, grind_point_depth - grindPushbackScalingRange * 0.4) * -1;
    if (grindingState == HeavyTruckGrindingState::GRINDING_BACK) {
        effectScaling = scaleRangeValue(fbValue, JOY_MAXPOINT - (grind_point_depth), JOY_MAXPOINT - (grind_point_depth - grindPushbackScalingRange * 0.4));
    }
    double revMatchRumbleScaling = std::fmax(0, scaleRangeValue(std::abs(grindEffectRPM), 0, maxRevMatchRPM * 1.5));
    DWORD rumbleMag = grindingIntensity * clutchPercent * std::abs(effectScaling) * revMatchRumbleScaling;
    if (synchroState == HeavyTruckSynchroState::ENTERING_SYNCH)
    {
        effectScaling = scaleRangeValue(fbValue, grind_point_depth, grind_point_depth - grindPushbackScalingRange) * -1;
        if (grindingState == HeavyTruckGrindingState::GRINDING_BACK) {
            effectScaling = scaleRangeValue(fbValue, JOY_MAXPOINT - (grind_point_depth), JOY_MAXPOINT - (grind_point_depth - grindPushbackScalingRange));
        }
        double revMatchPushbackScaling = std::fmax(0.25, scaleRangeValue(std::abs(grindEffectRPM), 0, maxRevMatchRPM));
        rumblePushback.lMagnitude = FFB_MAX * effectScaling * clutchPercent * revMatchPushbackScaling;
        //qDebug() << "revMatchScaling: " << revMatchScaling << ", rumblePushback.lMagnitude: " << rumblePushback.lMagnitude;
        device->updateEffect("rumblePushback");
    }
    // I need anyone who finds this whole period/phase manipulation stuff to know that
    // I hate it, but there's something fucky going on with the AB9 when you update
    // periodic effects too frequently or by too much. It also resets the phase on a
    // period change, sigh.
    DWORD period = 6e7 / std::abs(grindEffectRPM);
    period -= period % 10000;
    if (period > 300000) {
        period -= period % 100000;
    }
    if (period > 1000000) {
        period -= period % 1000000;
    }
    if (period > 10000000) {
        period = 10000000;
    }
    if (rumble.dwPeriod != 0)
        rumblePhase += 720000000 / rumble.dwPeriod;
    if (rumblePhase >= 36000)
        rumblePhase = rumblePhase % 36000;
    if (period != rumble.dwPeriod || rumbleMag != rumble.dwMagnitude)
    {
        //qDebug() << "rumble.dwPeriod: " << rumble.dwPeriod << "grindEffectRPM: " << grindEffectRPM << ", rumble.dwPhase: " << rumble.dwPhase << "rumblePhase: " << rumblePhase;
        rumble.dwPeriod = period;
        rumble.dwPhase = (DWORD)rumblePhase;
        //qDebug() << "rumble.dwMagnitude: " << rumble.dwMagnitude;
        rumble.dwMagnitude = rumbleMag;
        device->updateEffect("rumble");
    }
}

void HeavyTruckSynchroGuard::updateGrindEffectRPM(float newRPM) {
    grindEffectRPM = newRPM;
}

void HeavyTruckSynchroGuard::setGrindEffectIntensity(int value) {
    grindingIntensity = value * 100;    // Scale to 10000
}

void HeavyTruckSynchroGuard::setKeepInGearIdleIntensity(int value) {
    keepInGearSpringIdleCoefficient = value * 100;
}

void HeavyTruckSynchroGuard::setJoystickFBValue(long value) {
    fbValue = value;
}

void HeavyTruckSynchroGuard::setMaxRevMatchRPM(int value) {
    maxRevMatchRPM = value;
}