/*
Copyright (C) 2024-2025 Ken Monteith.

This file is part of Bonus FFB.

Bonus FFB is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or any later version.

Bonus FFB is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Bonus FFB. If not, see <https://www.gnu.org/licenses/>.
*/

#include "SynchroGuard.h"

#include <QDebug>

HRESULT SynchroGuard::start(BonusFFB::DeviceInfo* device) {
    unsynchronizedSpringEff.dwSize = sizeof(DIEFFECT);
    unsynchronizedSpringEff.dwFlags = DIEFF_POLAR | DIEFF_OBJECTOFFSETS;
    unsynchronizedSpringEff.dwDuration = INFINITE;
    unsynchronizedSpringEff.dwSamplePeriod = 0;
    unsynchronizedSpringEff.dwGain = DI_FFNOMINALMAX;
    unsynchronizedSpringEff.dwTriggerButton = DIEB_NOTRIGGER;
    unsynchronizedSpringEff.dwTriggerRepeatInterval = 0;
    unsynchronizedSpringEff.cAxes = 2;
    unsynchronizedSpringEff.rgdwAxes = AXES;
    LONG dir[2] = { DI_DEGREES * 180, 0 };
    unsynchronizedSpringEff.rglDirection = dir;
    unsynchronizedSpringEff.lpEnvelope = 0;
    unsynchronizedSpringEff.cbTypeSpecificParams = sizeof(DICONDITION);
    unsynchronizedSpringEff.lpvTypeSpecificParams = &unsynchronizedSpring;
    unsynchronizedSpringEff.dwStartDelay = 0;

    HRESULT hr;
    if (lpdiUnsynchronizedSpringEff == nullptr) {
        hr = device->diDevice->CreateEffect(GUID_Spring,
            &unsynchronizedSpringEff, &lpdiUnsynchronizedSpringEff, nullptr);
        if (FAILED(hr)) {
            return hr;
        }
    }
    hr = lpdiUnsynchronizedSpringEff->Start(INFINITE, 0);

    /* Maybe we'll want a constant effect, but I think we're good for now 
    unsynchronizedConstantEff.dwSize = sizeof(DIEFFECT);
    unsynchronizedConstantEff.dwFlags = DIEFF_POLAR | DIEFF_OBJECTOFFSETS;
    unsynchronizedConstantEff.dwDuration = INFINITE;
    unsynchronizedConstantEff.dwSamplePeriod = 0;
    unsynchronizedConstantEff.dwGain = DI_FFNOMINALMAX;
    unsynchronizedConstantEff.dwTriggerButton = DIEB_NOTRIGGER;
    unsynchronizedConstantEff.dwTriggerRepeatInterval = 0;
    unsynchronizedConstantEff.cAxes = 2;
    unsynchronizedConstantEff.rgdwAxes = AXES;
    unsynchronizedConstantEff.rglDirection = FORWARD;
    unsynchronizedConstantEff.lpEnvelope = 0;
    unsynchronizedConstantEff.cbTypeSpecificParams = sizeof(DICONSTANTFORCE);
    unsynchronizedConstantEff.lpvTypeSpecificParams = &unsynchronizedConstant;
    unsynchronizedConstantEff.dwStartDelay = 0;

    if (lpdiUnsynchronizedConstantEff == nullptr) {
        hr = device->diDevice->CreateEffect(GUID_ConstantForce,
            &unsynchronizedConstantEff, &lpdiUnsynchronizedConstantEff, nullptr);
        if (FAILED(hr)) {
            return hr;
        }
    }
    hr = lpdiUnsynchronizedConstantEff->Start(INFINITE, 0);
    */

    keepInGearSpringEff.dwSize = sizeof(DIEFFECT);
    keepInGearSpringEff.dwFlags = DIEFF_POLAR | DIEFF_OBJECTOFFSETS;
    keepInGearSpringEff.dwDuration = INFINITE;
    keepInGearSpringEff.dwSamplePeriod = 0;
    keepInGearSpringEff.dwGain = DI_FFNOMINALMAX;
    keepInGearSpringEff.dwTriggerButton = DIEB_NOTRIGGER;
    keepInGearSpringEff.dwTriggerRepeatInterval = 0;
    keepInGearSpringEff.cAxes = 2;
    keepInGearSpringEff.rgdwAxes = AXES;
    keepInGearSpringEff.rglDirection = dir;
    keepInGearSpringEff.lpEnvelope = 0;
    keepInGearSpringEff.cbTypeSpecificParams = sizeof(DICONDITION);
    keepInGearSpringEff.lpvTypeSpecificParams = &keepInGearSpring;
    keepInGearSpringEff.dwStartDelay = 0;

    if (lpdiKeepInGearSpringEff == nullptr) {
        hr = device->diDevice->CreateEffect(GUID_Spring,
            &keepInGearSpringEff, &lpdiKeepInGearSpringEff, nullptr);
        if (FAILED(hr)) {
            return hr;
        }
    }
    hr = lpdiKeepInGearSpringEff->Start(INFINITE, 0);

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

    if (lpdiRumbleEff == nullptr) {
        hr = device->diDevice->CreateEffect(GUID_Sine,
            &rumbleEff, &lpdiRumbleEff, nullptr);
        if (FAILED(hr)) {
            qDebug() << "CreateEffect failed";
            return hr;
        }
    }
    hr = lpdiRumbleEff->Start(INFINITE, 0);
    return hr;
}

void SynchroGuard::updatePedalEngagement(int clutchValue, int throttleValue) {
    clutchPercent = 1 - (double(clutchValue) / JOY_MAXPOINT);
    throttlePercent = double(throttleValue) / JOY_MAXPOINT;
    // Update kickout spring
    if (synchroState == SynchroState::ENTERING_SYNCH) {
        unsynchronizedSpring.lNegativeCoefficient = -10000 * clutchPercent;
        unsynchronizedSpringEff.lpvTypeSpecificParams = &unsynchronizedSpring;
        if (lpdiUnsynchronizedSpringEff != nullptr)
            lpdiUnsynchronizedSpringEff->SetParameters(&unsynchronizedSpringEff, DIEP_TYPESPECIFICPARAMS);
    }
    else if (synchroState == SynchroState::IN_SYNCH || synchroState == SynchroState::EXITING_SYNCH) {
        if (throttleValue > 100) {
            int scaledCoeff = keepInGearSpringMaxCoefficient * (throttlePercent * 3);
            if (scaledCoeff > keepInGearSpringMaxCoefficient)
                scaledCoeff = keepInGearSpringMaxCoefficient;
            else if (scaledCoeff < keepInGearSpringIdleCoefficient)
                scaledCoeff = keepInGearSpringIdleCoefficient;
            keepInGearSpring.lNegativeCoefficient = scaledCoeff * clutchPercent;
            keepInGearSpringEff.lpvTypeSpecificParams = &keepInGearSpring;
            if (lpdiKeepInGearSpringEff != nullptr)
                lpdiKeepInGearSpringEff->SetParameters(&keepInGearSpringEff, DIEP_TYPESPECIFICPARAMS);
        }
        else {
            keepInGearSpring.lNegativeCoefficient = keepInGearSpringIdleCoefficient * clutchPercent;
            keepInGearSpringEff.lpvTypeSpecificParams = &keepInGearSpring;
            if (lpdiKeepInGearSpringEff != nullptr)
                lpdiKeepInGearSpringEff->SetParameters(&keepInGearSpringEff, DIEP_TYPESPECIFICPARAMS);
        }
    }
}


void SynchroGuard::synchroStateChanged(SynchroState newState) {
    if (newState == SynchroState::IN_SYNCH) {
        qDebug() << "Disabling unsynch spring";
        // Activate keep-in-gear spring, disable unsynch spring
        unsynchronizedSpring.lNegativeCoefficient = 0;
        unsynchronizedSpringEff.lpvTypeSpecificParams = &unsynchronizedSpring;
        if (lpdiUnsynchronizedSpringEff != nullptr)
            lpdiUnsynchronizedSpringEff->SetParameters(&unsynchronizedSpringEff, DIEP_TYPESPECIFICPARAMS);
    }
    else if (newState == SynchroState::ENTERING_SYNCH) {
        // Deactivate keep-in-gear spring, enable unsynch spring
        unsynchronizedSpring.lNegativeCoefficient = -10000 * clutchPercent;
        unsynchronizedSpringEff.lpvTypeSpecificParams = &unsynchronizedSpring;
        if (lpdiUnsynchronizedSpringEff != nullptr)
            lpdiUnsynchronizedSpringEff->SetParameters(&unsynchronizedSpringEff, DIEP_TYPESPECIFICPARAMS);
    }
    synchroState = newState;
}

void SynchroGuard::grindingStateChanged(GrindingState newState) {
    if (newState != GrindingState::OFF) {
        // Start rumbling
        rumble.dwPeriod = int(6e7 / engineRPM);
        rumble.dwMagnitude = grindingIntensity * clutchPercent;
        rumbleEff.lpvTypeSpecificParams = &rumble;
        lpdiRumbleEff->SetParameters(&rumbleEff, DIEP_TYPESPECIFICPARAMS);
        // Enable constant pushback
        /*
        unsynchronizedConstant.lMagnitude = 10000;
        unsynchronizedConstantEff.lpvTypeSpecificParams = &unsynchronizedConstant;
        lpdiUnsynchronizedConstantEff->SetParameters(&unsynchronizedConstantEff, DIEP_TYPESPECIFICPARAMS);
        */
    }
    else {
        // Stop rumbling
        rumble.dwMagnitude = 0;
        rumbleEff.lpvTypeSpecificParams = &rumble;
        lpdiRumbleEff->SetParameters(&rumbleEff, DIEP_TYPESPECIFICPARAMS);
        // Disable constant pushback
        /*
        unsynchronizedConstant.lMagnitude = 0;
        unsynchronizedConstantEff.lpvTypeSpecificParams = &unsynchronizedConstant;
        lpdiUnsynchronizedConstantEff->SetParameters(&unsynchronizedConstantEff, DIEP_TYPESPECIFICPARAMS);
        */
    }
    grindingState = newState;
}

void SynchroGuard::updateEngineRPM(float newRPM) {
    if (grindingState != GrindingState::OFF) {
        rumble.dwPeriod = int(6e7 / newRPM);
        rumbleEff.lpvTypeSpecificParams = &rumble;
        lpdiRumbleEff->SetParameters(&rumbleEff, DIEP_TYPESPECIFICPARAMS);
    }
    engineRPM = newRPM;
}

void SynchroGuard::setGrindEffectIntensity(int value) {
    grindingIntensity = value * 100;    // Scale to 10000
}

void SynchroGuard::setGrindEffectRPM(int value) {
    updateEngineRPM(float(value));
}

void SynchroGuard::setKeepInGearIdleIntensity(int value) {
    keepInGearSpringIdleCoefficient = value * 100;
}