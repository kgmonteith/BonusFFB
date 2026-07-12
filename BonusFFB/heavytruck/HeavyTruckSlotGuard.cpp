/*
Copyright (C) 2024-2026 Ken Monteith.

This file is part of Bonus FFB.

Bonus FFB is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or any later version.

Bonus FFB is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Bonus FFB. If not, see <https://www.gnu.org/licenses/>.
*/

#include "HeavyTruckSlotGuard.h"

#include <QDebug>

HRESULT HeavyTruckSlotGuard::start(DeviceConfiguration* devPtr, SlotPattern* spPtr) {
    devices = devPtr;
    slotPattern = spPtr;

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
    devices->joystick->addEffect("slotSpring", { GUID_Spring, &slotSpringEff });
    
    clickPushBackEff.dwSize = sizeof(clickPushBackEff);
    clickPushBackEff.dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
    clickPushBackEff.dwDuration = .025 * DI_SECONDS;
    clickPushBackEff.dwSamplePeriod = 0;
    clickPushBackEff.dwGain = DI_FFNOMINALMAX; // Max gain applied to the effect
    clickPushBackEff.dwTriggerButton = DIEB_NOTRIGGER;
    clickPushBackEff.dwTriggerRepeatInterval = 0;
    clickPushBackEff.cAxes = 1;
    clickPushBackEff.rgdwAxes = &AXES[1];
    clickPushBackEff.rglDirection = &FORWARDBACK[1];
    clickPushBackEff.cbTypeSpecificParams = sizeof(DIRAMPFORCE);
    clickPushBackEff.lpvTypeSpecificParams = &clickPushBack;
    clickPushBackEff.dwStartDelay = 0;
    devices->joystick->addEffect("clickPushBack", { GUID_RampForce, &clickPushBackEff, false });

    clickPushForwardEff.dwSize = sizeof(clickPushForwardEff);
    clickPushForwardEff.dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
    clickPushForwardEff.dwDuration = .025 * DI_SECONDS;
    clickPushForwardEff.dwSamplePeriod = 0;
    clickPushForwardEff.dwGain = DI_FFNOMINALMAX; // Max gain applied to the effect
    clickPushForwardEff.dwTriggerButton = DIEB_NOTRIGGER;
    clickPushForwardEff.dwTriggerRepeatInterval = 0;
    clickPushForwardEff.cAxes = 1;
    clickPushForwardEff.rgdwAxes = &AXES[1];
    clickPushForwardEff.rglDirection = &FORWARDBACK[1];
    clickPushForwardEff.cbTypeSpecificParams = sizeof(DIRAMPFORCE);
    clickPushForwardEff.lpvTypeSpecificParams = &clickPushForward;
    clickPushForwardEff.dwStartDelay = 0;
    devices->joystick->addEffect("clickPushForward", { GUID_RampForce, &clickPushForwardEff, false });

    return DI_OK;
}

void HeavyTruckSlotGuard::updateSlotGuardState(HeavyTruckSlotState state) {
    slot_state = state;
}

QPair<long, long> HeavyTruckSlotGuard::getCornerStrength(double slot_pos_x) {
    // Calculate the origin point for the corner calculations
    double x0 = slot_pos_x, y0 = JOY_MIDPOINT;
    if (joyValues.lr < x0) {
        x0 -= slotPattern->roundingFactorAsJoystick();
    }
    else {
        x0 += slotPattern->roundingFactorAsJoystick();
    }
    if (joyValues.fb < y0) {
        y0 -= slotPattern->roundingFactorAsJoystick();
    }
    else {
        y0 += slotPattern->roundingFactorAsJoystick();
    }

    double x = joyValues.lr, y = joyValues.fb;
    // 1. Shift origin
    double dx = x - x0;
    double dy = y - y0;
    // 2. Compute Angle
    double angle = std::atan2(dy, dx);

    // 3. Compute sin/cos of the angle
    double sinA = std::abs(std::sin(angle));
    double cosA = std::abs(std::cos(angle));

    // Calculate linear scaling
    float scaleX = scaleRangeValue(x, slot_pos_x, x0);
    float scaleY = scaleRangeValue(y, JOY_MIDPOINT, y0);

    long xStrength = FFB_MAX * sinA * scaleX;
    long yStrength = FFB_MAX * cosA * scaleY;

    //qDebug() << "joystickValues: " << joyValues.lr << joyValues.fb << ", scaleX: " << scaleX << ", scaleY: " << scaleY;
    return QPair<long, long>(yStrength, xStrength);
}

void HeavyTruckSlotGuard::updateSlotGuardEffects() {
    joyValues = devices->getJoystickValues2();
    //bool in_neutral = slotPattern->isInNeutral(joyValues);
    const Slot* nearest_slot = slotPattern->getNearestSlot(joyValues);

    if (last_nearest_slot != nearest_slot && slot_state != HeavyTruckSlotState::NEUTRAL) {
        // Refusing to update slot effects without passing through neutral
        qDebug() << "Refusing to update slot effects without passing through neutral";
        return;
    }
    last_nearest_slot = nearest_slot;

    /*
    if (in_neutral)
        passed_through_neutral = true;
    if (last_slot != slot && (!passed_through_neutral)) {
        // Refusing to update slot effects without passing through neutral
        qDebug() << "Refusing to update slot effects without passing through neutral";
        return;
    }
    else if (passed_through_neutral && slot != SLOT_NONE)
        passed_through_neutral = false;
    last_slot = slot;
    */

    if (nearest_slot->isEnabled() && slotPattern->isInCorner(*nearest_slot, joyValues)) {
        QPair<float, float> corner_strength = getCornerStrength(slotPattern->slotPositionAsJoystick(*nearest_slot));
        slotSpringConditions[0].lPositiveCoefficient = corner_strength.first;
        slotSpringConditions[0].lNegativeCoefficient = corner_strength.first;
        //slotSpringConditions[0].lOffset = slotParams->asFFBOffset(nearest_slot) + ((joystickPositionToFFBOffset(joyValues.lr) - slotParams->asFFBOffset(nearest_slot)) * -1.3);
        double slot_pos_ffb = slotPattern->slotPositionAsFFBOffset(*nearest_slot);
        slotSpringConditions[0].lOffset = slot_pos_ffb + ((joystickPositionToFFBOffset(joyValues.lr) - slot_pos_ffb) * -1.3);
        slotSpringConditions[1].lPositiveCoefficient = corner_strength.second;
        slotSpringConditions[1].lNegativeCoefficient = corner_strength.second;
        slotSpringConditions[1].lOffset = joystickPositionToFFBOffset(joyValues.fb) * -1.3;
        //qDebug() << "corner_strength: " << corner_strength;
    }
    //else if (slot != SLOT_NONE && slot_state != HeavyTruckSlotState::UNKNOWN && slot_state != HeavyTruckSlotState::NEUTRAL) {
    else if (nearest_slot->isEnabled() && slot_state == HeavyTruckSlotState::SLOTTED) {
        // Keep stick centered L/R
        slotSpringConditions[0] = keepLRCentered;
        //slotSpringConditions[0].lOffset = slotParams->asFFBOffset(nearest_slot); // +((joystickPositionToFFBOffset(joyValues.lr) - slotParams->asFFBOffset(nearest_slot)) * -1.3);
        double slot_pos_ffb = slotPattern->slotPositionAsFFBOffset(*nearest_slot);
        slotSpringConditions[0].lOffset = slot_pos_ffb + ((joystickPositionToFFBOffset(joyValues.lr) - slot_pos_ffb) * -1.3);
        slotSpringConditions[1] = noSpring;
    }
    else if (slot_state == HeavyTruckSlotState::NEUTRAL)
    //else if(in_neutral)
    {
        // Keep stick centered F/B
        slotSpringConditions[0] = noSpring;
        slotSpringConditions[1] = keepFBCentered;
        slotSpringConditions[1].lOffset = joystickPositionToFFBOffset(joyValues.fb) * -1.3;
        //qDebug() << "neutral lOffset: " << slotSpringConditions[1].lOffset;
    }
    else {
        qDebug() << "Bad state!";
    }

    // Set the wall effect strength
    /*
    if (slotPattern->hasSlotWall(SLOT_WALL_LEFT)) {
        const Slot* left_wall_slot = slotPattern->getWallSlot(SLOT_WALL_LEFT);
        Slot leftmost_slot = slotPattern->getLeftmostSlot();
        if (joyValues.lr < slotPattern->slotPositionAsJoystick(*left_wall_slot) && joyValues.lr >= slotPattern->slotPositionAsJoystick(leftmost_slot) + 5000 && (in_neutral || (slot != nullptr && slot->position_pct_nominal == left_wall_slot->position_pct_nominal))) {
            double left_wall_slot_pos_ffb = slotPattern->slotPositionAsFFBOffset(*left_wall_slot);
            slotSpringConditions[0] = keepLRCentered;
            slotSpringConditions[0].lOffset = left_wall_slot_pos_ffb + ((joystickPositionToFFBOffset(joyValues.lr) - left_wall_slot_pos_ffb) * -1.3);
            if (in_neutral && slot != SLOT_NONE)
                slotSpringConditions[1] = noSpring;
            if (in_neutral) {
                // Downscale wall effect when approaching the left slot
                double scalingDistance = 15000;
                double leftmost_slot_pos = slotPattern->slotPositionAsJoystick(leftmost_slot);
                slotSpringConditions[0].lNegativeCoefficient = slotSpringConditions[0].lNegativeCoefficient * scaleRangeValue(joyValues.lr, leftmost_slot_pos + 5000, leftmost_slot_pos + scalingDistance);
                slotSpringConditions[0].lPositiveCoefficient = slotSpringConditions[0].lPositiveCoefficient * scaleRangeValue(joyValues.lr, leftmost_slot_pos + 5000, leftmost_slot_pos + scalingDistance);
                //qDebug() << "slotSpringConditions[0].lNegativeCoefficient" << slotSpringConditions[0].lNegativeCoefficient;
            }
        }
    }*/

    // Prevent the stick from being pushed too far left at any point
    if (joyValues.lr < slotPattern->getPatternLeftMinimumAsJoystick()) {
        slotSpringConditions[0] = keepLRCentered;
        double leftmost_pos_ffb = joystickPositionToFFBOffset(slotPattern->getPatternLeftMinimumAsJoystick());
        slotSpringConditions[0].lOffset = leftmost_pos_ffb + ((joystickPositionToFFBOffset(joyValues.lr) - leftmost_pos_ffb) * -1.3);
        if (nearest_slot->isEnabled())
        {
            slotSpringConditions[1] = noSpring;
        }
    }
    // Prevent the stick from being pushed too far right at any point
    if(joyValues.lr > slotPattern->getPatternRightMaximumAsJoystick()) {
        slotSpringConditions[0] = keepLRCentered;
        double rightmost_pos_ffb = joystickPositionToFFBOffset(slotPattern->getPatternRightMaximumAsJoystick());
        slotSpringConditions[0].lOffset = rightmost_pos_ffb + ((joystickPositionToFFBOffset(joyValues.lr) - rightmost_pos_ffb) * -1.3);
        if (nearest_slot->isEnabled())
        {
            slotSpringConditions[1] = noSpring;
        }
    }
    // Prevent the stick from being pushed too far forward or back when the slot depth is less than 100%
    if (nearest_slot != SLOT_NONE && nearest_slot->isOrientationFwd() && joyValues.fb <= slotPattern->slotDepthAsJoystick(nearest_slot->orientation)) {
        slotSpringConditions[1] = keepFBCentered;
        double depth_pos_ffb = slotPattern->slotDepthAsFFBOffset(nearest_slot->orientation);
        int offset = depth_pos_ffb + (std::abs(joystickPositionToFFBOffset(joyValues.fb) - depth_pos_ffb) * 2.5);
        slotSpringConditions[1].lOffset = offset;
    }
    else if (nearest_slot != SLOT_NONE && nearest_slot->isOrientationBack() && joyValues.fb >= slotPattern->slotDepthAsJoystick(nearest_slot->orientation)) {
        slotSpringConditions[1] = keepFBCentered;
        double depth_pos_ffb = slotPattern->slotDepthAsFFBOffset(nearest_slot->orientation);
        int offset = depth_pos_ffb - (std::abs(joystickPositionToFFBOffset(joyValues.fb) - depth_pos_ffb) * 2.5);
        slotSpringConditions[1].lOffset = offset;
    }
    devices->joystick->updateEffect("slotSpring");

    // Set the gate latch friction
    /*
    bool inLatchGateZone = (joyValues.fb <= slot->grindPointDepthAsJoystickValueFwd() && joyValues.fb >= slot->grindPointDepthAsJoystickValueFwd() - latchDepth) || (joyValues.fb >= slot->grindPointDepthAsJoystickValueBack() && joyValues.fb <= slot->grindPointDepthAsJoystickValueBack() + latchDepth);
    if (inLatchGateZone && frictionCondition[0].lPositiveCoefficient != gateLatchFrictionStrength)
    {
        frictionCondition[0] = { 0, gateLatchFrictionStrength, gateLatchFrictionStrength };
        frictionCondition[1] = { 0, gateLatchFrictionStrength, gateLatchFrictionStrength };
        //qDebug() << "setting gateLatchFrictionStrength " << gateLatchFrictionStrength;
        device->updateEffect("friction");
    }
    else if (!inLatchGateZone && frictionCondition[0].lPositiveCoefficient != frictionStrength) {
        frictionCondition[0] = { 0, frictionStrength, frictionStrength };
        frictionCondition[1] = { 0, frictionStrength, frictionStrength };
        //qDebug() << "setting gateLatchFrictionStrength " << frictionStrength;
        device->updateEffect("friction");
    }
    */

    // Play the end-of-slot click effect
    if (!clickPlayed && nearest_slot != SLOT_NONE) {
        if (nearest_slot->isOrientationFwd() && joyValues.fb <= slotPattern->slotDepthAsJoystick(nearest_slot->orientation) + 1000) {
            HRESULT hr = devices->joystick->playEffect("clickPushBack");
            if (!SUCCEEDED(hr)) {
                qDebug() << "device->playEffect(\"clickPushBack\") failed";
            }
            clickPlayed = true;
        }
        else if (nearest_slot->isOrientationBack() && joyValues.fb >= slotPattern->slotDepthAsJoystick(nearest_slot->orientation) - 1000){
            devices->joystick->playEffect("clickPushForward");
            clickPlayed = true;
        }
        //qDebug() << "Playing click";
    }
    else if (clickPlayed && slot_state != HeavyTruckSlotState::SLOTTED) {
        clickPlayed = false;
        //qDebug() << "Resetting click";
    }
}