/*
Copyright (C) 2024-2026 Ken Monteith.

This file is part of Bonus FFB.

Bonus FFB is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or any later version.

Bonus FFB is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Bonus FFB. If not, see <https://www.gnu.org/licenses/>.
*/

#include "HeavyTruckSlotGuard.h"

#include <QDebug>

HRESULT HeavyTruckSlotGuard::start(DeviceInfo* devPtr, SlotParameters* sPtr) {
    device = devPtr;
    slot = sPtr;

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
        int offset = joystickPositionToFFBOffset(joystickValues.second) * -2;
        springConditions[1].lOffset = offset;
        springConditions[1].lDeadBand = 500;
    } else if (slot_state == HeavyTruckSlotState::SLOT_LEFT_FWD || slot_state == HeavyTruckSlotState::SLOT_LEFT_BACK) {
        springConditions[0] = keepLeft;
        springConditions[1] = noSpring;
    } else if (slot_state == HeavyTruckSlotState::SLOT_MIDDLE_FWD || slot_state == HeavyTruckSlotState::SLOT_MIDDLE_BACK) {
        springConditions[0] = keepLRCentered;
        springConditions[0].lOffset = slot->asFFBOffset(1);
        springConditions[1] = noSpring;
    } else if (slot_state == HeavyTruckSlotState::SLOT_RIGHT_FWD || slot_state == HeavyTruckSlotState::SLOT_RIGHT_BACK) {
        springConditions[0] = keepLRCentered;
        springConditions[0].lOffset = slot->asFFBOffset(2);
        springConditions[1] = noSpring;
    }
    
    // Prevent the stick from being pushed too far forward or back when the slot depth is less than 100%
    if ((slot_state == HeavyTruckSlotState::SLOT_LEFT_FWD || slot_state == HeavyTruckSlotState::SLOT_MIDDLE_FWD || slot_state == HeavyTruckSlotState::SLOT_RIGHT_FWD) && joystickValues.second <= JOY_MIDPOINT - slot->depthAsJoystick()) {
        springConditions[1] = keepFBCentered;
        int offset = slot->depthAsFFBOffsetFwd() + (std::abs(joystickPositionToFFBOffset(joystickValues.second) - slot->depthAsFFBOffsetFwd()) * 2.5);
        springConditions[1].lOffset = offset;
    }
    else if ((slot_state == HeavyTruckSlotState::SLOT_LEFT_BACK || slot_state == HeavyTruckSlotState::SLOT_MIDDLE_BACK || slot_state == HeavyTruckSlotState::SLOT_RIGHT_BACK) && joystickValues.second >= JOY_MIDPOINT + slot->depthAsJoystick()) {
        springConditions[1] = keepFBCentered;
        int offset = slot->depthAsFFBOffsetBack() - (std::abs(joystickPositionToFFBOffset(joystickValues.second) - slot->depthAsFFBOffsetBack()) * 2.5);
        springConditions[1].lOffset = offset;
    }
    // Prevent the stick from being pushed too far right at any point
    if ((slot_state == HeavyTruckSlotState::NEUTRAL || slot_state == HeavyTruckSlotState::NEUTRAL_UNDER_SLOT) && joystickValues.first > slot->asJoystickValue(2)) {
        springConditions[0] = keepLRCentered;
        springConditions[0].lOffset = slot->asFFBOffset(2);
        //int offset = (FFB_MIDPOINT + 7500) + ((joystickValueToFFBValue(joystickValues.first) - 7500) * -1.5);
        //springConditions[0].lOffset = offset;
    }
    device->updateEffect("slotSpring");
}
