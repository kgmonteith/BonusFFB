/*
Copyright (C) 2024-2026 Ken Monteith.

This file is part of Bonus FFB.

Bonus FFB is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or any later version.

Bonus FFB is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Bonus FFB. If not, see <https://www.gnu.org/licenses/>.
*/

#include "HeavyTruckSlotGuard.h"

#include <QDebug>

HRESULT HeavyTruckSlotGuard::start(DeviceInfo* devPtr) {
    device = devPtr;

    slotSpringEff.dwSize = sizeof(DIEFFECT);
    slotSpringEff.dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
    slotSpringEff.dwDuration = INFINITE;
    slotSpringEff.dwSamplePeriod = 0;
    slotSpringEff.dwGain = DI_FFNOMINALMAX;
    slotSpringEff.dwTriggerButton = DIEB_NOTRIGGER;
    slotSpringEff.dwTriggerRepeatInterval = 0;
    slotSpringEff.cAxes = 2;
    slotSpringEff.rgdwAxes = AXES;
    slotSpringEff.rglDirection = FORWARDBACK;
    slotSpringEff.lpEnvelope = 0;
    slotSpringEff.cbTypeSpecificParams = sizeof(DICONDITION) * 2;
    springConditions[0] = noSpring;
    springConditions[1] = keepFBCentered;
    slotSpringEff.lpvTypeSpecificParams = &springConditions;
    slotSpringEff.dwStartDelay = 0;
    device->addEffect("slotSpring", { GUID_Spring, &slotSpringEff });

    fbSlotPushEff.dwSize = sizeof(fbSlotPushEff);
    fbSlotPushEff.dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
    fbSlotPushEff.dwDuration = INFINITE;
    fbSlotPushEff.dwSamplePeriod = 0;
    fbSlotPushEff.dwGain = DI_FFNOMINALMAX; // Max gain applied to the effect
    fbSlotPushEff.dwTriggerButton = DIEB_NOTRIGGER;
    fbSlotPushEff.dwTriggerRepeatInterval = 0;
    fbSlotPushEff.cAxes = 1;
    fbSlotPushEff.rgdwAxes = &AXES[1];
    fbSlotPushEff.rglDirection = &FORWARDBACK[1];
    fbSlotPushEff.cbTypeSpecificParams = sizeof(DICONSTANTFORCE);
    fbSlotPushEff.lpvTypeSpecificParams = &fbcf;
    fbSlotPushEff.dwStartDelay = 0;
    //device->addEffect("fbSlotPush", { GUID_ConstantForce, &fbSlotPushEff });
    return DI_OK;
}

void HeavyTruckSlotGuard::updateSlotGuardState(HeavyTruckSlotState state) {
    slot_state = state;
}


void HeavyTruckSlotGuard::updateSlotGuardEffects(QPair<int, int> joystickValues) {
    if (slot_state == HeavyTruckSlotState::NEUTRAL_UNDER_SLOT) {
        // Disable the effect to prevent thrashing
        // We can scale the condition coefficients near the junctions instead, but good enough for now
        springConditions[0] = noSpring;
        springConditions[1] = noSpring;
        springConditions[1].lDeadBand = 500;
    } else if (slot_state == HeavyTruckSlotState::NEUTRAL) {
        springConditions[0] = noSpring;
        springConditions[1] = keepFBCentered;
        // Move the offset to increase force instead of adding a scaled constant force, which causes thrashing
        int offset = ((double(joystickValues.second) / 3.2767) - 10000) * -1.3;
        springConditions[1].lOffset = offset;
        springConditions[1].lDeadBand = 500;
    } else if (slot_state == HeavyTruckSlotState::SLOT_LEFT_FWD || slot_state == HeavyTruckSlotState::SLOT_LEFT_BACK) {
        springConditions[0] = keepLeft;
        // Think of a smarter way to implement this, but scaling the spring seems like a decent way to implement slot angling/rounding
        /*
        if (slot_state == HeavyTruckSlotState::SLOT_LEFT_BACK) {
            int offset = (((double(joystickValues.second) / 3.2767) - 10000));
            int scaledStrength = scaleRangeValue(offset, 0, 5000) * FFB_MAX;
            qDebug() << "joystickValues.second: " << joystickValues.second;
                qDebug() << scaledStrength;
            springConditions[0].lPositiveCoefficient = scaledStrength;
            springConditions[0].lNegativeCoefficient = scaledStrength;
        }*/
        springConditions[1] = noSpring;
    } else if (slot_state == HeavyTruckSlotState::SLOT_MIDDLE_FWD || slot_state == HeavyTruckSlotState::SLOT_MIDDLE_BACK) {
        springConditions[0] = keepLRCentered;
        springConditions[1] = noSpring;
    } else if (slot_state == HeavyTruckSlotState::SLOT_RIGHT_FWD || slot_state == HeavyTruckSlotState::SLOT_RIGHT_BACK) {
        springConditions[0] = keepRight;
        springConditions[1] = noSpring;
    }
    device->updateEffect("slotSpring");
}
