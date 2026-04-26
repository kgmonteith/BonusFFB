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
    slotSpringConditions[1] = noSpring;
    slotSpringEff.lpvTypeSpecificParams = &slotSpringConditions;
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

int HeavyTruckSlotGuard::isInCorner(int slot_num, QPair<int, int> joystickValues) {
    if ((joystickValues.first > slot->asJoystickValue(slot_num) - slot->rounding_factor && joystickValues.first < slot->asJoystickValue(slot_num) + slot->rounding_factor) && (joystickValues.second > JOY_MIDPOINT - slot->rounding_factor && joystickValues.second < JOY_MIDPOINT + slot->rounding_factor)) {
        if (slot_num == 1 && joystickValues.first < slot->asJoystickValue(slot_num))
            return CORNER_SQUARE;
        return CORNER_ROUNDED;
    }
    return NOT_IN_CORNER;
}

QPair<long, long> HeavyTruckSlotGuard::getCornerStrength(int slot_num, QPair<int, int> joystickValues) {
    // Calculate the origin point for the corner calculations
    double x0 = slot->asJoystickValue(slot_num), y0 = JOY_MIDPOINT;
    if (joystickValues.first < x0) {
        x0 -= slot->rounding_factor;
    }
    else {
        x0 += slot->rounding_factor;
    }
    if (joystickValues.second < y0) {
        y0 -= slot->rounding_factor;
    }
    else {
        y0 += slot->rounding_factor;
    }

    double x = joystickValues.first, y = joystickValues.second;
    // 1. Shift origin
    double dx = x - x0;
    double dy = y - y0;
    // 2. Compute Angle
    double angle = std::atan2(dy, dx);

    // 3. Compute sin/cos of the angle
    double sinA = std::abs(std::sin(angle));
    double cosA = std::abs(std::cos(angle));

    // Calculate linear scaling
    float scaleX = scaleRangeValue(x, slot->asJoystickValue(slot_num), x0);
    float scaleY = scaleRangeValue(y, JOY_MIDPOINT, y0);

    long xStrength = FFB_MAX * sinA * scaleX;
    long yStrength = FFB_MAX * cosA * scaleY;

    //qDebug() << "joystickValues: " << joystickValues.first << joystickValues.second << ", scaleX: " << scaleX << ", scaleY: " << scaleY;
    return QPair<long, long>(yStrength, xStrength);
}

void HeavyTruckSlotGuard::updateSlotGuardEffects(QPair<int, int> joystickValues) {
    //slot->getNearestSlot(joystickValues.first);
    int nearest_slot = slot->getNearestSlot(joystickValues.first);
    if (last_nearest_slot != nearest_slot && slot_state != HeavyTruckSlotState::NEUTRAL) {
        // Refusing to update slot effects without passing through neutral
        return;
    }
    last_nearest_slot = nearest_slot;
    int is_in_corner = isInCorner(nearest_slot, joystickValues);
    if (is_in_corner) {
        QPair<float, float> corner_strength = getCornerStrength(nearest_slot, joystickValues);
        slotSpringConditions[0].lPositiveCoefficient = corner_strength.first;
        slotSpringConditions[0].lNegativeCoefficient = corner_strength.first;
        slotSpringConditions[0].lOffset = slot->asFFBOffset(nearest_slot) + ((joystickPositionToFFBOffset(joystickValues.first) - slot->asFFBOffset(nearest_slot)) * -1.3);
        slotSpringConditions[1].lPositiveCoefficient = corner_strength.second;
        slotSpringConditions[1].lNegativeCoefficient = corner_strength.second;
        slotSpringConditions[1].lOffset = joystickPositionToFFBOffset(joystickValues.second) * -1.3;
    }
    else if (slot_state != HeavyTruckSlotState::UNKNOWN && slot_state != HeavyTruckSlotState::NEUTRAL) {
        // Keep stick centered L/R
        slotSpringConditions[0] = keepLRCentered;
        slotSpringConditions[0].lOffset = slot->asFFBOffset(nearest_slot) + ((joystickPositionToFFBOffset(joystickValues.first) - slot->asFFBOffset(nearest_slot)) * -1.3);
        slotSpringConditions[1] = noSpring;
    }
    else if (slot_state == HeavyTruckSlotState::NEUTRAL)
    {
        // Keep stick centered F/B
        slotSpringConditions[0] = noSpring;
        slotSpringConditions[1] = keepFBCentered;
        slotSpringConditions[1].lOffset = joystickPositionToFFBOffset(joystickValues.second) * -1.3;
    }
    else {
        qDebug() << "Slot state is unknown!";
    }

    // Set the wall effect strength
    if (joystickValues.first < slot->asJoystickValue(1) && joystickValues.first >= 5000 && (nearest_slot == 1 || slot_state == HeavyTruckSlotState::NEUTRAL)) {
        slotSpringConditions[0] = keepLRCentered;
        slotSpringConditions[0].lOffset = slot->asFFBOffset(1) + ((joystickPositionToFFBOffset(joystickValues.first) - slot->asFFBOffset(1)) * -1.3);
        slotSpringConditions[1] = noSpring;
        if (slot_state == HeavyTruckSlotState::NEUTRAL) {
            // Downscale wall effect when approaching the left slot
            double scalingDistance = 15000;
            slotSpringConditions[0].lNegativeCoefficient = slotSpringConditions[0].lNegativeCoefficient * scaleRangeValue(joystickValues.first, 5000, scalingDistance);
            slotSpringConditions[0].lPositiveCoefficient = slotSpringConditions[0].lPositiveCoefficient * scaleRangeValue(joystickValues.first, 5000, scalingDistance);
            //qDebug() << "slotSpringConditions[0].lPositiveCoefficient" << slotSpringConditions[0].lPositiveCoefficient;
        }
    }

    // Prevent the stick from being pushed too far right at any point
    if (joystickValues.first > slot->asJoystickValue(2)) {
        slotSpringConditions[0] = keepLRCentered;
        slotSpringConditions[0].lOffset = slot->asFFBOffset(2) + ((joystickPositionToFFBOffset(joystickValues.first) - slot->asFFBOffset(2)) * -1.3);
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

    device->updateEffect("slotSpring");
}