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

    damperEff.dwSize = sizeof(DIEFFECT);
    damperEff.dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
    damperEff.dwDuration = INFINITE;
    damperEff.dwSamplePeriod = 0;
    damperEff.dwGain = DI_FFNOMINALMAX;
    damperEff.dwTriggerButton = DIEB_NOTRIGGER;
    damperEff.dwTriggerRepeatInterval = 0;
    damperEff.cAxes = 2;
    damperEff.rgdwAxes = AXES;
    damperEff.rglDirection = FORWARDBACK;
    damperEff.lpEnvelope = 0;
    damperEff.cbTypeSpecificParams = sizeof(DICONDITION) * 2;
    damperEff.lpvTypeSpecificParams = &damperCondition;
    damperEff.dwStartDelay = 0;
    device->addEffect("damper", { GUID_Damper, &damperEff });

    inertiaEff.dwSize = sizeof(DIEFFECT);
    inertiaEff.dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
    inertiaEff.dwDuration = INFINITE;
    inertiaEff.dwSamplePeriod = 0;
    inertiaEff.dwGain = DI_FFNOMINALMAX;
    inertiaEff.dwTriggerButton = DIEB_NOTRIGGER;
    inertiaEff.dwTriggerRepeatInterval = 0;
    inertiaEff.cAxes = 2;
    inertiaEff.rgdwAxes = AXES;
    inertiaEff.rglDirection = FORWARDBACK;
    inertiaEff.lpEnvelope = 0;
    inertiaEff.cbTypeSpecificParams = sizeof(DICONDITION) * 2;
    inertiaEff.lpvTypeSpecificParams = &inertiaCondition;
    inertiaEff.dwStartDelay = 0;
    device->addEffect("inertia", { GUID_Inertia, &inertiaEff });

    frictionEff.dwSize = sizeof(DIEFFECT);
    frictionEff.dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
    frictionEff.dwDuration = INFINITE;
    frictionEff.dwSamplePeriod = 0;
    frictionEff.dwGain = DI_FFNOMINALMAX;
    frictionEff.dwTriggerButton = DIEB_NOTRIGGER;
    frictionEff.dwTriggerRepeatInterval = 0;
    frictionEff.cAxes = 2;
    frictionEff.rgdwAxes = AXES;
    frictionEff.rglDirection = FORWARDBACK;
    frictionEff.lpEnvelope = 0;
    frictionEff.cbTypeSpecificParams = sizeof(DICONDITION) * 2;
    frictionEff.lpvTypeSpecificParams = &frictionCondition;
    frictionEff.dwStartDelay = 0;
    device->addEffect("friction", { GUID_Friction, &frictionEff });

    return DI_OK;
}

void HeavyTruckSlotGuard::updateDamper(int value) {
    damperStrength = FFB_MAX * value * 0.01;
    damperCondition[0] = { 0, damperStrength, damperStrength };
    damperCondition[1] = { 0, damperStrength, damperStrength };
    if (device != nullptr && device->isAcquired) {
        device->updateEffect("damper");
        qDebug() << "damperStrength: " << damperStrength;
    }
}

void HeavyTruckSlotGuard::updateInertia(int value) {
    inertiaStrength = FFB_MAX * value * 0.1;
    inertiaCondition[0] = { 0, inertiaStrength, inertiaStrength };
    inertiaCondition[1] = { 0, inertiaStrength, inertiaStrength };
    if (device != nullptr && device->isAcquired) {
        device->updateEffect("inertia");
        qDebug() << "inertiaStrength: " << inertiaStrength;
    }
}

void HeavyTruckSlotGuard::updateFriction(int value) {
    frictionStrength = FFB_MAX * value * 0.01;
    frictionCondition[0] = { 0, frictionStrength, frictionStrength };
    frictionCondition[1] = { 0, frictionStrength, frictionStrength };
    if (device != nullptr && device->isAcquired) {
        device->updateEffect("friction");
        qDebug() << "frictionStrength: " << frictionStrength;
    }
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
        springConditions[1].lDeadBand = 1000;
    } else if (slot_state == HeavyTruckSlotState::NEUTRAL) {
        springConditions[0] = noSpring;
        springConditions[1] = keepFBCentered;
        // Move the offset to increase force instead of adding a scaled constant force, which causes thrashing
        int offset = joystickPositionToFFBOffset(joystickValues.second) * -.8;//-2;
        springConditions[1].lOffset = offset;
        springConditions[1].lDeadBand = 1000;
    } else if (slot_state == HeavyTruckSlotState::SLOT_LEFT_FWD || slot_state == HeavyTruckSlotState::SLOT_LEFT_BACK) {
        springConditions[0] = keepLRCentered;
        springConditions[0].lOffset = slot->asFFBOffset(0);
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
