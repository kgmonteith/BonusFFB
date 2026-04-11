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
    slotSpringConditions[0] = noSpring;
    slotSpringConditions[1] = keepFBCentered;
    slotSpringEff.lpvTypeSpecificParams = &slotSpringConditions;
    slotSpringEff.dwStartDelay = 0;
    device->addEffect("slotSpring", { GUID_Spring, &slotSpringEff });

    leftSlotResistanceEff.dwSize = sizeof(DIEFFECT);
    leftSlotResistanceEff.dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
    leftSlotResistanceEff.dwDuration = INFINITE;
    leftSlotResistanceEff.dwSamplePeriod = 0;
    leftSlotResistanceEff.dwGain = DI_FFNOMINALMAX;
    leftSlotResistanceEff.dwTriggerButton = DIEB_NOTRIGGER;
    leftSlotResistanceEff.dwTriggerRepeatInterval = 0;
    leftSlotResistanceEff.cAxes = 1;
    leftSlotResistanceEff.rgdwAxes = &AXES[0];
    leftSlotResistanceEff.rglDirection = &FORWARDBACK[0];
    leftSlotResistanceEff.lpEnvelope = 0;
    leftSlotResistanceEff.cbTypeSpecificParams = sizeof(DICONDITION);
    leftSlotResistanceEff.lpvTypeSpecificParams = &leftSlotResistanceCondition;
    leftSlotResistanceEff.dwStartDelay = 0;
    device->addEffect("rightSlotWall", { GUID_Spring, &leftSlotResistanceEff });

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

void HeavyTruckSlotGuard::updateLeftSlotResistance(int value) {
    leftSlotResistance.lNegativeCoefficient = -100 * value;
    leftSlotResistance.lPositiveCoefficient = -100 * value;
}

void HeavyTruckSlotGuard::updateSlotGuardState(HeavyTruckSlotState state) {
    slot_state = state;
}

void HeavyTruckSlotGuard::updateSlotGuardEffects(QPair<int, int> joystickValues) {
    if (slot_state == HeavyTruckSlotState::NEUTRAL_UNDER_SLOT) {
        // Disable the effect to prevent thrashing
        // We can scale the condition coefficients near the junctions instead, but good enough for now
        slotSpringConditions[0] = noSpring;
        slotSpringConditions[1] = noSpring;
        slotSpringConditions[1].lDeadBand = 750;
    } else if (slot_state == HeavyTruckSlotState::NEUTRAL) {
        slotSpringConditions[0] = noSpring;
        slotSpringConditions[1] = keepFBCentered;
        // Move the offset to increase force instead of adding a scaled constant force, which causes thrashing
        int offset = joystickPositionToFFBOffset(joystickValues.second) * -1;//-2;
        slotSpringConditions[1].lOffset = offset;
        slotSpringConditions[1].lDeadBand = 750;
    }
    else if (slot_state != HeavyTruckSlotState::UNKNOWN) {
        int slot_num = -1;
        if (slot_state == HeavyTruckSlotState::SLOT_LEFT_FWD || slot_state == HeavyTruckSlotState::SLOT_LEFT_BACK) {
            slot_num = 0;
        }
        else if (slot_state == HeavyTruckSlotState::SLOT_MIDDLE_FWD || slot_state == HeavyTruckSlotState::SLOT_MIDDLE_BACK) {
            slot_num = 1;
        }
        else if (slot_state == HeavyTruckSlotState::SLOT_RIGHT_FWD || slot_state == HeavyTruckSlotState::SLOT_RIGHT_BACK) {
            slot_num = 2;
        }
        slotSpringConditions[0] = keepLRCentered;
        //springConditions[0].lOffset = slot->asFFBOffset(slot_num);
        slotSpringConditions[0].lOffset = slot->asFFBOffset(slot_num) + ((joystickPositionToFFBOffset(joystickValues.first) - slot->asFFBOffset(slot_num)) * -1.3);
        // Try scaling down the L/R spring as it approaches the neutral channel
        // None of these techniques quite work, I think we need a full refactor of the slot guard, including the state
        //if ((slot_state == HeavyTruckSlotState::SLOT_MIDDLE_FWD && joystickValues.first > slot->asJoystickValue(slot_num)) || (slot_state == HeavyTruckSlotState::SLOT_RIGHT_FWD && joystickValues.first < slot->asJoystickValue(slot_num))) {
        /*
        if ((slot_state == HeavyTruckSlotState::SLOT_LEFT_FWD || slot_state == HeavyTruckSlotState::SLOT_MIDDLE_FWD || slot_state == HeavyTruckSlotState::SLOT_RIGHT_FWD)) {
            slotSpringConditions[0].lPositiveCoefficient = scaleRangeValue(joystickValues.second, JOY_MIDPOINT - 1400, JOY_MIDPOINT - 5000) * 10000;
            slotSpringConditions[0].lNegativeCoefficient = scaleRangeValue(joystickValues.second, JOY_MIDPOINT - 1400, JOY_MIDPOINT - 5000) * 10000;
            qDebug() << "slotSpringConditions[0].lPositiveCoefficient: " << slotSpringConditions[0].lPositiveCoefficient;
        }
        else {
            slotSpringConditions[0].lPositiveCoefficient = scaleRangeValue(joystickValues.second, JOY_MIDPOINT + 1400, JOY_MIDPOINT + 5000) * 10000;
            slotSpringConditions[0].lNegativeCoefficient = scaleRangeValue(joystickValues.second, JOY_MIDPOINT + 1400, JOY_MIDPOINT + 5000) * 10000;
        }*/
        slotSpringConditions[1] = noSpring;
    }
    
    // Prevent the stick from being pushed too far forward or back when the slot depth is less than 100%
    if ((slot_state == HeavyTruckSlotState::SLOT_LEFT_FWD || slot_state == HeavyTruckSlotState::SLOT_MIDDLE_FWD || slot_state == HeavyTruckSlotState::SLOT_RIGHT_FWD) && joystickValues.second <= JOY_MIDPOINT - slot->depthAsJoystick()) {
        slotSpringConditions[1] = keepFBCentered;
        int offset = slot->depthAsFFBOffsetFwd() + (std::abs(joystickPositionToFFBOffset(joystickValues.second) - slot->depthAsFFBOffsetFwd()) * 2.5);
        slotSpringConditions[1].lOffset = offset;
    }
    else if ((slot_state == HeavyTruckSlotState::SLOT_LEFT_BACK || slot_state == HeavyTruckSlotState::SLOT_MIDDLE_BACK || slot_state == HeavyTruckSlotState::SLOT_RIGHT_BACK) && joystickValues.second >= JOY_MIDPOINT + slot->depthAsJoystick()) {
        slotSpringConditions[1] = keepFBCentered;
        int offset = slot->depthAsFFBOffsetBack() - (std::abs(joystickPositionToFFBOffset(joystickValues.second) - slot->depthAsFFBOffsetBack()) * 2.5);
        slotSpringConditions[1].lOffset = offset;
    }
    // Prevent the stick from being pushed too far right at any point
    if ((slot_state == HeavyTruckSlotState::NEUTRAL || slot_state == HeavyTruckSlotState::NEUTRAL_UNDER_SLOT) && joystickValues.first > slot->asJoystickValue(2)) {
        slotSpringConditions[0] = keepLRCentered;
        slotSpringConditions[0].lOffset = slot->asFFBOffset(2) + ((joystickPositionToFFBOffset(joystickValues.first) - slot->asFFBOffset(2)) * -1.3);
        //int offset = (FFB_MIDPOINT + 7500) + ((joystickValueToFFBValue(joystickValues.first) - 7500) * -1.5);
        //springConditions[0].lOffset = offset;
    }
    device->updateEffect("slotSpring");

    // Set the "left slot wall" spring effect
    if ((slot_state != HeavyTruckSlotState::SLOT_LEFT_BACK && slot_state != HeavyTruckSlotState::SLOT_LEFT_FWD) && joystickValues.first < slot->asJoystickValue(1) - slot->middle_slot_half_width) {
    //if ((slot_state == HeavyTruckSlotState::NEUTRAL || slot_state == HeavyTruckSlotState::NEUTRAL_UNDER_SLOT) && joystickValues.first < slot->asJoystickValue(1) - slot->middle_slot_half_width) {
        leftSlotResistanceCondition = leftSlotResistance;
        leftSlotResistanceCondition.lNegativeCoefficient = leftSlotResistance.lNegativeCoefficient * scaleRangeValue(joystickValues.first, slot->asJoystickValue(1) - slot->middle_slot_half_width, slot->asJoystickValue(1) - (slot->middle_slot_half_width + 2000));
        leftSlotResistanceCondition.lPositiveCoefficient = leftSlotResistance.lPositiveCoefficient * scaleRangeValue(joystickValues.first, slot->asJoystickValue(1) - slot->middle_slot_half_width, slot->asJoystickValue(1) - (slot->middle_slot_half_width + 2000));
        //qDebug() << "rightSlotWallCondition.lPositiveCoefficient: "<< rightSlotWallCondition.lPositiveCoefficient;
    }
    else {
        leftSlotResistanceCondition = noSpring;
    }
    device->updateEffect("rightSlotWall");
}