/*
Copyright (C) 2024-2025 Ken Monteith.

This file is part of Bonus FFB.

Bonus FFB is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or any later version.

Bonus FFB is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Bonus FFB. If not, see <https://www.gnu.org/licenses/>.
*/

#include "HShifterSynchroGuard.h"

#include <QDebug>

HRESULT HShifterSynchroGuard::start(DeviceInfo* devPtr) {
    device = devPtr;

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
    device->addEffect("keepInGearEff", { GUID_ConstantForce, &keepInGearEff });

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
    device->addEffect("rumble", { GUID_Triangle, &rumbleEff, DIEP_TYPESPECIFICPARAMS });

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
    return DI_OK;
}

void HShifterSynchroGuard::updatePedalEngagement(QPair<int, int> pedalValues, QPair<int, int> joystickValues) { // int clutchValue, int throttleValue, int fbValue) {
    clutchPercent = 1 - (double(pedalValues.first) / JOY_MAXPOINT);
    throttlePercent = double(pedalValues.second) / JOY_MAXPOINT;
    int fbValue = joystickValues.second;
    // Update keep-in-gear spring
    if ((synchroState == SynchroState::IN_SYNCH || synchroState == SynchroState::EXITING_SYNCH)) {
        if (pedalValues.second > 100) {
            int scaledCoeff = keepInGearSpringMaxCoefficient * (throttlePercent);
            if (fbValue > JOY_MIDPOINT) {
                scaledCoeff *= scaleRangeValue(fbValue, JOY_MAXPOINT, JOY_MAXPOINT - 6000) * -1;
            }
            else {
                scaledCoeff *= scaleRangeValue(fbValue, 0, 6000);
            }
            keepInGearForce.lMagnitude = scaledCoeff * clutchPercent;
        }
        else {
            keepInGearForce.lMagnitude = 0;
        }
        device->updateEffect("keepInGearEff");
    }
}


void HShifterSynchroGuard::synchroStateChanged(SynchroState newState, int fbValue) {
    if (newState == SynchroState::IN_SYNCH) {
        // Activate keep-in-gear spring
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
    else if (newState == SynchroState::ENTERING_SYNCH) {
        // Deactivate keep-in-gear spring
        keepInGearSpring.lNegativeCoefficient = 0;
        device->updateEffect("keepInGearSpring");
    }
    synchroState = newState;
}

void HShifterSynchroGuard::grindingStateChanged(GrindingState newState, int fbValue) {
    if (newState != GrindingState::OFF) {
        // Start rumbling
        double effectScaling = scaleRangeValue(fbValue, JOY_MIDPOINT * 0.65, JOY_MIDPOINT * 0.65 - 7500) * -1;
        if (newState == GrindingState::GRINDING_BACK) {
            effectScaling = scaleRangeValue(fbValue, JOY_MAXPOINT - (JOY_MIDPOINT * 0.65), JOY_MAXPOINT - (JOY_MIDPOINT * 0.65 - 7500));
        }
        rumble.dwPeriod = int(6e7 / computeGrindRPM());
        rumble.dwMagnitude = grindingIntensity * clutchPercent * std::abs(effectScaling);
        if (synchroState == SynchroState::ENTERING_SYNCH)
        {
            rumblePushback.lMagnitude = FFB_MAX * effectScaling * clutchPercent;
            //qDebug() << "rumblePushback.lMagnitude: " << rumblePushback.lMagnitude;
        }
    }
    else {
        // Stop rumbling
        rumble.dwMagnitude = 0;
        rumblePushback.lMagnitude = 0;
    }
    device->updateEffect("rumble");
    device->updateEffect("rumblePushback");
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

void HShifterSynchroGuard::setGrindEffectBehavior(int index) {
    qDebug() << "New grind effect behavior: " << index;
    grindEffectBehavior = static_cast<GrindEffectBehavior>(index);
}

void HShifterSynchroGuard::setGrindEffectIntensity(int value) {
    grindingIntensity = value * 100;    // Scale to 10000
}

void HShifterSynchroGuard::setKeepInGearIdleIntensity(int value) {
    keepInGearSpringIdleCoefficient = value * 100;
}