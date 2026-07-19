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
    
    neutralSpringCondition.lOffset = slotPattern->getPositionPercentAsFFBOffset(neutral_spring_pos_pct);
    neutralSpringEff.dwSize = sizeof(DIEFFECT);
    neutralSpringEff.dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
    neutralSpringEff.dwDuration = INFINITE;
    neutralSpringEff.dwSamplePeriod = 0;
    neutralSpringEff.dwGain = DI_FFNOMINALMAX;
    neutralSpringEff.dwTriggerButton = DIEB_NOTRIGGER;
    neutralSpringEff.dwTriggerRepeatInterval = 0;
    neutralSpringEff.cAxes = 1;
    neutralSpringEff.rgdwAxes = AXES;
    neutralSpringEff.rglDirection = FORWARDBACK;
    neutralSpringEff.lpEnvelope = 0;
    neutralSpringEff.cbTypeSpecificParams = sizeof(DICONDITION);
    neutralSpringEff.lpvTypeSpecificParams = &neutralSpringCondition;
    neutralSpringEff.dwStartDelay = 0;
    devices->joystick->addEffect("neutralSpring", { GUID_Spring, &neutralSpringEff });

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

    if (last_nearest_slot != nearest_slot && (slot_state != HeavyTruckSlotState::NEUTRAL && slot_state != HeavyTruckSlotState::NEUTRAL_UNDER_SLOT)) {
        // Refusing to update slot effects without passing through neutral
        qDebug() << "Refusing to update slot effects without passing through neutral";
        return;
    }
    last_nearest_slot = nearest_slot;

    if (nearest_slot->isEnabled() && slotPattern->isInCorner(*nearest_slot, joyValues)) {
        QPair<float, float> corner_strength = getCornerStrength(slotPattern->slotPositionAsJoystick(*nearest_slot));
        slotSpringConditions[0].lPositiveCoefficient = corner_strength.first;
        slotSpringConditions[0].lNegativeCoefficient = corner_strength.first;
        double slot_pos_ffb = slotPattern->slotPositionAsFFBOffset(*nearest_slot);
        slotSpringConditions[0].lOffset = slot_pos_ffb + ((joystickPositionToFFBOffset(joyValues.lr) - slot_pos_ffb) * -1.3);
        slotSpringConditions[1].lPositiveCoefficient = corner_strength.second;
        slotSpringConditions[1].lNegativeCoefficient = corner_strength.second;
        slotSpringConditions[1].lOffset = joystickPositionToFFBOffset(joyValues.fb) * -1.3;
        //qDebug() << "corner_strength: " << corner_strength;
    }
    else if (nearest_slot->isEnabled() && slot_state == HeavyTruckSlotState::SLOTTED) {
        // Keep stick centered L/R
        slotSpringConditions[0] = keepLRCentered;
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
    }
    else {
        //qDebug() << "Bad state!";
    }

    // Set the wall effect strength
    if (slotPattern->hasSlotWall(SLOT_WALL_LEFT)) {
        const Slot* left_wall_slot = slotPattern->getWallSlot(SLOT_WALL_LEFT);
        double left_pattern_limit = slotPattern->getPatternLeftMinimumAsJoystick();
        double wall_taper_range = slotPattern->getSlotSpacingAsJoystick() / 5.0;
        if (joyValues.lr < slotPattern->slotPositionAsJoystick(*left_wall_slot) && joyValues.lr >= left_pattern_limit + wall_taper_range && (slot_state == HeavyTruckSlotState::NEUTRAL || slot_state == HeavyTruckSlotState::NEUTRAL_UNDER_SLOT)) {
            double left_wall_slot_pos_ffb = slotPattern->slotPositionAsFFBOffset(*left_wall_slot);
            slotSpringConditions[0] = keepLRCentered;
            slotSpringConditions[0].lOffset = left_wall_slot_pos_ffb + ((joystickPositionToFFBOffset(joyValues.lr) - left_wall_slot_pos_ffb) * -1.3);
            //if (in_neutral && slot != SLOT_NONE)
            //    slotSpringConditions[1] = noSpring;
            if (slot_state == HeavyTruckSlotState::NEUTRAL || slot_state == HeavyTruckSlotState::NEUTRAL_UNDER_SLOT) {
                // Downscale wall effect when approaching the left slot
                double wall_scaling_range = slotPattern->getSlotSpacingAsJoystick() / 2.5;
                slotSpringConditions[0].lNegativeCoefficient = slotSpringConditions[0].lNegativeCoefficient * scaleRangeValue(joyValues.lr, left_pattern_limit + wall_taper_range, left_pattern_limit + wall_scaling_range);
                slotSpringConditions[0].lPositiveCoefficient = slotSpringConditions[0].lPositiveCoefficient * scaleRangeValue(joyValues.lr, left_pattern_limit + wall_taper_range, left_pattern_limit + wall_scaling_range);
                //qDebug() << "slotSpringConditions[0].lNegativeCoefficient" << slotSpringConditions[0].lNegativeCoefficient;
            }
        }
    }

    // Adjust neutral spring strength
    long prior_offset = neutralSpringCondition.lOffset;
    long prior_strength = neutralSpringCondition.lPositiveCoefficient;
    double neutral_spring_scale = 1 - scaleRangeValue(std::abs(joystickPositionToFFBOffset(joyValues.fb)), 0, FFB_MAX * slotPattern->depth_scale); // Consider using rounding factor, button zone depth, etc., if this isn't a good scaling point
    neutralSpringCondition.lPositiveCoefficient = neutral_spring_scale * neutral_spring_strength;
    neutralSpringCondition.lNegativeCoefficient = neutral_spring_scale * neutral_spring_strength;

    // Set the bump-through spring for the ZF-16 double-H
    if (slotPattern->truckPattern == TruckPattern::ZF_16_DOUBLEH) {
        double slot_pos_ffb = slotPattern->slotPositionAsFFBOffset(*nearest_slot);
        if ((nearest_slot == &slotPattern->slot_list[4] || nearest_slot == &slotPattern->slot_list[5]) && joyValues.lr > slotPattern->slotPositionAsJoystick(*nearest_slot)) {
            //qDebug() << "in third slot";
            slotSpringConditions[0] = keepLRCentered;
            slotSpringConditions[0].lOffset = slot_pos_ffb + ((joystickPositionToFFBOffset(joyValues.lr) - slot_pos_ffb) * -1.3);
            neutralSpringCondition.lOffset = slotPattern->slotPositionAsFFBOffset(slotPattern->slot_list[4]);
        }
        else if ((nearest_slot == &slotPattern->slot_list[6] || nearest_slot == &slotPattern->slot_list[7]) && joyValues.lr < slotPattern->slotPositionAsJoystick(*nearest_slot)) {
            //qDebug() << "in fourth slot";
            slotSpringConditions[0] = keepLRCentered;
            slotSpringConditions[0].lOffset = slot_pos_ffb + ((joystickPositionToFFBOffset(joyValues.lr) - slot_pos_ffb) * -1.3);
            neutralSpringCondition.lOffset = slotPattern->slotPositionAsFFBOffset(slotPattern->slot_list[6]);
        }
        // Override the range switch
        bool newRangeOverride = false;
        if (joyValues.lr > slotPattern->slotPositionAsJoystick(slotPattern->slot_list[6]) - (slotPattern->getSlotSpacingAsJoystick() * 0.5))
            newRangeOverride = true;
        if (newRangeOverride != rangeOverride) {
            rangeOverride = newRangeOverride;
            qDebug() << "rangeOverride changed: " << rangeOverride;
            emit forceRangeValue(rangeOverride);
        }
    }
    else {
        // For all other patterns, set the neutral spring offset from the UI setting
        neutralSpringCondition.lOffset = slotPattern->getPositionPercentAsFFBOffset(neutral_spring_pos_pct);
    }

    // Update neutral spring if needed
    if (neutralSpringCondition.lPositiveCoefficient != prior_strength || neutralSpringCondition.lOffset != prior_offset)
        devices->joystick->updateEffect("neutralSpring");

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