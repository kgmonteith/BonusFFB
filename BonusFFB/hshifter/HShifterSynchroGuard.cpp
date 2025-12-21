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

    unsynchronizedSpringEff.dwSize = sizeof(DIEFFECT);
    unsynchronizedSpringEff.dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
    unsynchronizedSpringEff.dwDuration = INFINITE;
    unsynchronizedSpringEff.dwSamplePeriod = 0;
    unsynchronizedSpringEff.dwGain = DI_FFNOMINALMAX;
    unsynchronizedSpringEff.dwTriggerButton = DIEB_NOTRIGGER;
    unsynchronizedSpringEff.dwTriggerRepeatInterval = 0;
    unsynchronizedSpringEff.cAxes = 1;
    unsynchronizedSpringEff.rgdwAxes = &AXES[1];
    unsynchronizedSpringEff.rglDirection = &FORWARDBACK[1];
    unsynchronizedSpringEff.lpEnvelope = 0;
    unsynchronizedSpringEff.cbTypeSpecificParams = sizeof(DICONDITION);
    unsynchronizedSpringEff.lpvTypeSpecificParams = &unsynchronizedSpring;
    unsynchronizedSpringEff.dwStartDelay = 0;
    device->addEffect("unsynchronizedSpring", { GUID_Spring, &unsynchronizedSpringEff });
    
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

    LONG rglDirection[2] = { 90 * DI_DEGREES, DI_DEGREES };
    rumbleEff.dwSize = sizeof(DIEFFECT);
    rumbleEff.dwFlags = DIEFF_POLAR | DIEFF_OBJECTOFFSETS;
    rumbleEff.dwDuration = INFINITE;
    rumbleEff.dwSamplePeriod = 0;
    rumbleEff.dwGain = DI_FFNOMINALMAX;
    rumbleEff.dwTriggerButton = DIEB_NOTRIGGER;
    rumbleEff.dwTriggerRepeatInterval = 0;
    rumbleEff.cAxes = 2;
    rumbleEff.rgdwAxes = AXES;
    rumbleEff.rglDirection = rglDirection;
    rumbleEff.lpEnvelope = 0;
    rumbleEff.cbTypeSpecificParams = sizeof(DIPERIODIC);
    rumbleEff.lpvTypeSpecificParams = &rumble;
    rumbleEff.dwStartDelay = 0;
    device->addEffect("rumble", { GUID_Sine, &rumbleEff });

    return createEffects();
}

HRESULT HShifterSynchroGuard::createEffects() {
    HRESULT hr = DI_OK;
    return hr;
}

void HShifterSynchroGuard::updatePedalEngagement(int clutchValue, int throttleValue) {
    clutchPercent = 1 - (double(clutchValue) / JOY_MAXPOINT);
    throttlePercent = double(throttleValue) / JOY_MAXPOINT;
    // Update kickout spring
    if (synchroState == SynchroState::ENTERING_SYNCH) {
        unsynchronizedSpring.lNegativeCoefficient = 10000 * clutchPercent;
        device->updateEffect("unsynchronizedSpring");
    }
    else if (synchroState == SynchroState::IN_SYNCH || synchroState == SynchroState::EXITING_SYNCH) {
        if (throttleValue > 100) {
            int scaledCoeff = keepInGearSpringMaxCoefficient * (throttlePercent * 3);
            if (scaledCoeff > keepInGearSpringMaxCoefficient)
                scaledCoeff = keepInGearSpringMaxCoefficient;
            else if (scaledCoeff < keepInGearSpringIdleCoefficient)
                scaledCoeff = keepInGearSpringIdleCoefficient;
            keepInGearSpring.lNegativeCoefficient = scaledCoeff * clutchPercent * -1;
        }
        else {
            keepInGearSpring.lNegativeCoefficient = keepInGearSpringIdleCoefficient * clutchPercent * -1;
        }
        device->updateEffect("keepInGearSpring");
    }
}


void HShifterSynchroGuard::synchroStateChanged(SynchroState newState) {
    if (newState == SynchroState::IN_SYNCH) {
        // Activate keep-in-gear spring, disable unsynch spring
        unsynchronizedSpring.lNegativeCoefficient = 0;
        device->updateEffect("unsynchronizedSpring");
        keepInGearSpring.lNegativeCoefficient = keepInGearSpringIdleCoefficient * clutchPercent * -1;
        device->updateEffect("keepInGearSpring");
    }
    else if (newState == SynchroState::ENTERING_SYNCH) {
        // Deactivate keep-in-gear spring, enable unsynch spring
        unsynchronizedSpring.lNegativeCoefficient = 10000 * clutchPercent;
        device->updateEffect("unsynchronizedSpring");
        keepInGearSpring.lNegativeCoefficient = 0;
        device->updateEffect("keepInGearSpring");
    }
    synchroState = newState;
}

void HShifterSynchroGuard::grindingStateChanged(GrindingState newState) {
    if (newState != GrindingState::OFF) {
        // Start rumbling
        rumble.dwPeriod = int(6e7 / computeGrindRPM());
        rumble.dwMagnitude = grindingIntensity * clutchPercent;
        
        // Enable constant pushback
        /*
        unsynchronizedConstant.lMagnitude = 10000;
        lpdiUnsynchronizedConstantEff->SetParameters(&unsynchronizedConstantEff, DIEP_TYPESPECIFICPARAMS);
        */
    }
    else {
        // Stop rumbling
        rumble.dwMagnitude = 0;
        // Disable constant pushback
        /*
        unsynchronizedConstant.lMagnitude = 0;
        lpdiUnsynchronizedConstantEff->SetParameters(&unsynchronizedConstantEff, DIEP_TYPESPECIFICPARAMS);
        */
    }
    device->updateEffect("rumble");
    grindingState = newState;
}

void HShifterSynchroGuard::updateEngineRPM(float newRPM) {
    if (grindingState != GrindingState::OFF) {
        rumble.dwPeriod = int(6e7 / newRPM);
        device->updateEffect("rumble");
    }
    engineRPM = newRPM;
}

float HShifterSynchroGuard::computeGrindRPM() {
    if (grindEffectBehavior == GrindEffectBehavior::MATCH_ENGINE_RPM) {
        //qDebug() << "GrindEffectBehavior MATCH_ENGINE_RPM " << engineRPM;
        return engineRPM;
    }
    else if (grindEffectBehavior == GrindEffectBehavior::ADD_ENGINE_RPM) {
        //qDebug() << "GrindEffectBehavior ADD_ENGINE_RPM" << engineRPM + grindEffectRPM;
        return engineRPM + grindEffectRPM;
    }
    //qDebug() << "GrindEffectBehavior OVERRIDE_ENGINE_RPM " << grindEffectRPM;
    return grindEffectRPM;
}

void HShifterSynchroGuard::updateGrindEffectRPM(float newRPM) {
    grindEffectRPM = newRPM;
}

void HShifterSynchroGuard::setGrindEffectBehavior(int index) {
    grindEffectBehavior = static_cast<GrindEffectBehavior>(index);
}

void HShifterSynchroGuard::setGrindEffectIntensity(int value) {
    grindingIntensity = value * 100;    // Scale to 10000
}

void HShifterSynchroGuard::setKeepInGearIdleIntensity(int value) {
    keepInGearSpringIdleCoefficient = value * 100;
}