/*
Copyright (C) 2024-2025 Ken Monteith.

This file is part of Bonus FFB.

Bonus FFB is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or any later version.

Bonus FFB is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Bonus FFB. If not, see <https://www.gnu.org/licenses/>.
*/

#include "PrndlSlotGuard.h"

#include <QDebug>

HRESULT PrndlSlotGuard::start(DeviceInfo* devicePtr) {
    device = devicePtr;

    HRESULT hr = DI_OK;
    keepCenteredSpringEff.dwSize = sizeof(DIEFFECT);
    keepCenteredSpringEff.dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
    keepCenteredSpringEff.dwDuration = INFINITE;
    keepCenteredSpringEff.dwSamplePeriod = 0;
    keepCenteredSpringEff.dwGain = DI_FFNOMINALMAX;
    keepCenteredSpringEff.dwTriggerButton = DIEB_NOTRIGGER;
    keepCenteredSpringEff.dwTriggerRepeatInterval = 0;
    keepCenteredSpringEff.cAxes = 2;
    keepCenteredSpringEff.rgdwAxes = AXES;
    keepCenteredSpringEff.rglDirection = FORWARDBACK;
    keepCenteredSpringEff.lpEnvelope = 0;
    keepCenteredSpringEff.cbTypeSpecificParams = sizeof(DICONDITION) * 2;
    keepCenteredSpringConditions[0] = keepLRCentered;
    keepCenteredSpringConditions[1] = noSpring;
    keepCenteredSpringEff.lpvTypeSpecificParams = &keepCenteredSpringConditions;
    keepCenteredSpringEff.dwStartDelay = 0;
    device->addEffect("keepCenteredSpring", { GUID_Spring, &keepCenteredSpringEff });
    
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
    
    shiftLockEff.dwSize = sizeof(shiftLockEff);
    shiftLockEff.dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
    shiftLockEff.dwDuration = INFINITE;
    shiftLockEff.dwSamplePeriod = 0;
    shiftLockEff.dwGain = DI_FFNOMINALMAX; // Max gain applied to the effect
    shiftLockEff.dwTriggerButton = DIEB_NOTRIGGER;
    shiftLockEff.dwTriggerRepeatInterval = 0;
    shiftLockEff.cAxes = 1;
    shiftLockEff.rgdwAxes = &AXES[1];
    shiftLockEff.rglDirection = &FORWARDBACK[1];
    shiftLockEff.cbTypeSpecificParams = sizeof(DICONSTANTFORCE);
    shiftLockEff.lpvTypeSpecificParams = &shiftLockForce;
    shiftLockEff.dwStartDelay = 0;
    device->addEffect("shiftLock", { GUID_ConstantForce, &shiftLockEff });

    qDebug() << "Started PrndlSlotGuard effect";
    return hr;
}

void PrndlSlotGuard::updateSlotSpringCenter(long newCenter) {
    keepInGearSpring.lOffset = newCenter;
    device->updateEffect("keepInGearSpring");
}

void PrndlSlotGuard::updateShiftLockEffectStrength(double strength) {
    long newStrength = (long)strength;
    if (lastShiftLockForce != newStrength) {
        qDebug() << "Setting shift lock strength to " << shiftLockForce.lMagnitude;
        shiftLockForce.lMagnitude = newStrength;
        device->updateEffect("shiftLock");
        lastShiftLockForce = newStrength;
    }
}
