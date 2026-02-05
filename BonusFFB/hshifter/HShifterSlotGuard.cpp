/*
Copyright (C) 2024-2026 Ken Monteith.

This file is part of Bonus FFB.

Bonus FFB is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or any later version.

Bonus FFB is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Bonus FFB. If not, see <https://www.gnu.org/licenses/>.
*/

#include "HShifterSlotGuard.h"

#include <QDebug>

HRESULT HShifterSlotGuard::start(DeviceInfo* devPtr) {
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

    neutralSpringEff.dwSize = sizeof(DIEFFECT);
    neutralSpringEff.dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
    neutralSpringEff.dwDuration = INFINITE;
    neutralSpringEff.dwSamplePeriod = 0;
    neutralSpringEff.dwGain = DI_FFNOMINALMAX;
    neutralSpringEff.dwTriggerButton = DIEB_NOTRIGGER;
    neutralSpringEff.dwTriggerRepeatInterval = 0;
    neutralSpringEff.cAxes = 2;
    neutralSpringEff.rgdwAxes = AXES;
    neutralSpringEff.rglDirection = FORWARDBACK;
    neutralSpringEff.lpEnvelope = 0;
    neutralSpringEff.cbTypeSpecificParams = sizeof(DICONDITION) * 2;
    neutralSpringEff.lpvTypeSpecificParams = &neutralSpringConditions;
    neutralSpringEff.dwStartDelay = 0;
    device->addEffect("neutralSpring", { GUID_Spring, &neutralSpringEff });
    return DI_OK;
}
/*
void HShifterSlotGuard::updateSlotGuardEffects(QPair<int, int> joystickValues) {
    long lrValue = joystickValues.first;
    long fbValue = joystickValues.second;

    long scaledMagnitude = -1;
    if (fbValue > JOY_MIDPOINT) {
        if(lrValue < JOY_QUARTERPOINT || (lrValue > JOY_MIDPOINT && lrValue < JOY_THREEQUARTERPOINT)) 
        {
            scaledMagnitude = scaleRangeValue(fbValue, JOY_MIDPOINT + 500, JOY_THREEQUARTERPOINT - 5000) * 10000;
        }
    }
    else {
        if((lrValue > JOY_QUARTERPOINT && lrValue < JOY_MIDPOINT) || lrValue > JOY_THREEQUARTERPOINT)
        {
            scaledMagnitude = scaleRangeValue(fbValue, JOY_MIDPOINT - 500, JOY_QUARTERPOINT + 5000) * 10000;
        }
    }
    qDebug() << "scaledMagnitude: " << scaledMagnitude;


    if (lrValue <= JOY_QUARTERPOINT) {
        springConditions[0] = keepLeft;
        springConditions[1] = noSpring;
        if (fbValue > JOY_MIDPOINT) {
            springConditions[0].lNegativeCoefficient = scaledMagnitude;
        }
    }

    device->updateEffect("slotSpring");
    if (slot_state == SlotState::NEUTRAL_UNDER_SLOT) {
        // Disable the effect to prevent thrashing
        // We can scale the condition coefficients near the junctions instead, but good enough for now
        springConditions[0] = noSpring;
        springConditions[1] = noSpring;
        springConditions[1].lDeadBand = 500;
    }
    else if (slot_state == SlotState::NEUTRAL) {
        springConditions[0] = noSpring;
        springConditions[1] = keepFBCentered;
        springConditions[1].lDeadBand = 500;
    }
    else if (slot_state == SlotState::SLOT_LEFT_FWD || slot_state == SlotState::SLOT_LEFT_BACK) {
        springConditions[0] = keepLeft;
        springConditions[1] = noSpring;
    }
    else if (slot_state == SlotState::SLOT_MIDDLE_FWD || slot_state == SlotState::SLOT_MIDDLE_BACK) {
        springConditions[0] = keepLRCentered;
        springConditions[1] = noSpring;
    }
    else if (slot_state == SlotState::SLOT_RIGHT_FWD || slot_state == SlotState::SLOT_RIGHT_BACK) {
        springConditions[0] = keepRight;
        springConditions[1] = noSpring;
    }
    if (scaledMagnitude > -1)  
    {
        springConditions[0].lPositiveCoefficient = scaledMagnitude;
        springConditions[0].lNegativeCoefficient = scaledMagnitude;
        qDebug() << "scaledMagnitude: " << scaledMagnitude;
    }
    device->updateEffect("slotSpring");

    int neutralSpringStrength = 0;
    if (fbValue <= JOY_MIDPOINT) {
        neutralSpringStrength = neutral_spring_strength * scaleRangeValue(fbValue, 5000, 10000);
        //neutralSpringStrength = neutral_spring_strength * scaleRangeValue(fbValue, JOY_QUARTERPOINT - 5000, JOY_QUARTERPOINT + 5000);
    }
    else {
        neutralSpringStrength = neutral_spring_strength * scaleRangeValue(fbValue, JOY_MAXPOINT - 5000, JOY_MAXPOINT - 10000);
        //neutralSpringStrength = neutral_spring_strength * scaleRangeValue(fbValue, JOY_THREEQUARTERPOINT + 5000, JOY_THREEQUARTERPOINT - 5000);
    }
    neutralSpringConditions[0].lPositiveCoefficient = neutralSpringStrength;
    neutralSpringConditions[0].lNegativeCoefficient = neutralSpringStrength;
    neutralSpringConditions[1].lPositiveCoefficient = neutralSpringStrength;
    neutralSpringConditions[1].lNegativeCoefficient = neutralSpringStrength;
    device->updateEffect("neutralSpring");
}*/

/* Unfortunately far too strong and dangerous...
HRESULT HShifterSlotGuard::start(DeviceInfo* devPtr) {
    device = devPtr;

    neutralSpringEff.dwSize = sizeof(DIEFFECT);
    neutralSpringEff.dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
    neutralSpringEff.dwDuration = INFINITE;
    neutralSpringEff.dwSamplePeriod = 0;
    neutralSpringEff.dwGain = DI_FFNOMINALMAX;
    neutralSpringEff.dwTriggerButton = DIEB_NOTRIGGER;
    neutralSpringEff.dwTriggerRepeatInterval = 0;
    neutralSpringEff.cAxes = 2;
    neutralSpringEff.rgdwAxes = AXES;
    neutralSpringEff.rglDirection = FORWARDBACK;
    neutralSpringEff.lpEnvelope = 0;
    neutralSpringEff.cbTypeSpecificParams = sizeof(DICONDITION) * 2;
    neutralSpringEff.lpvTypeSpecificParams = &neutralSpringConditions;
    neutralSpringEff.dwStartDelay = 0;
    device->addEffect("neutralSpring", { GUID_Spring, &neutralSpringEff });

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
    //springConditions[0] = noSpring;
    //springConditions[1] = keepFBCentered;
    slotSpringEff.lpvTypeSpecificParams = &springConditions;
    slotSpringEff.dwStartDelay = 0;
    device->addEffect("slotSpring", { GUID_Spring, &slotSpringEff });

    lrSlotPushEff.dwSize = sizeof(lrSlotPushEff);
    lrSlotPushEff.dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
    lrSlotPushEff.dwDuration = INFINITE;
    lrSlotPushEff.dwSamplePeriod = 0;
    lrSlotPushEff.dwGain = DI_FFNOMINALMAX; // Max gain applied to the effect
    lrSlotPushEff.dwTriggerButton = DIEB_NOTRIGGER;
    lrSlotPushEff.dwTriggerRepeatInterval = 0;
    lrSlotPushEff.cAxes = 1;
    lrSlotPushEff.rgdwAxes = &AXES[0];
    lrSlotPushEff.rglDirection = &FORWARDBACK[0];
    lrSlotPushEff.cbTypeSpecificParams = sizeof(DICONSTANTFORCE);
    lrSlotPushEff.lpvTypeSpecificParams = &lrcf;
    lrSlotPushEff.dwStartDelay = 0;
    device->addEffect("lrSlotPush", { GUID_ConstantForce, &lrSlotPushEff });

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
    device->addEffect("fbSlotPush", { GUID_ConstantForce, &fbSlotPushEff });

    safetyDamper.dwSize = sizeof(safetyDamper);
    safetyDamper.dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
    safetyDamper.dwDuration = INFINITE;
    safetyDamper.dwSamplePeriod = 0;
    safetyDamper.dwGain = DI_FFNOMINALMAX; // Max gain applied to the effect
    safetyDamper.dwTriggerButton = DIEB_NOTRIGGER;
    safetyDamper.dwTriggerRepeatInterval = 0;
    safetyDamper.cAxes = 2;
    safetyDamper.rgdwAxes = AXES;
    safetyDamper.rglDirection = FORWARDBACK;
    safetyDamper.cbTypeSpecificParams = sizeof(DICONDITION) * 2;
    safetyDamper.lpvTypeSpecificParams = safe2d;
    safetyDamper.dwStartDelay = 0;
    device->addEffect("damper", { GUID_Damper, &safetyDamper });

    safetyFriction.dwSize = sizeof(safetyFriction);
    safetyFriction.dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
    safetyFriction.dwDuration = INFINITE;
    safetyFriction.dwSamplePeriod = 0;
    safetyFriction.dwGain = DI_FFNOMINALMAX; // Max gain applied to the effect
    safetyFriction.dwTriggerButton = DIEB_NOTRIGGER;
    safetyFriction.dwTriggerRepeatInterval = 0;
    safetyFriction.cAxes = 2;
    safetyFriction.rgdwAxes = AXES;
    safetyFriction.rglDirection = FORWARDBACK;
    safetyFriction.cbTypeSpecificParams = sizeof(DICONDITION) * 2;
    safetyFriction.lpvTypeSpecificParams = safe2d;
    safetyFriction.dwStartDelay = 0;
    device->addEffect("friction", { GUID_Friction, &safetyFriction });
    return DI_OK;
}

// Safety dampening and friction effects are required to curb this implementation from violently oscillating if the lever is released laterally at the end of a slot
void HShifterSlotGuard::updateSlotGuardEffects(QPair<int, int> joystickValues) {
    long lrValue = joystickValues.first;
    long fbValue = joystickValues.second;
    QUADRANT quadrant;

    long springDeadzone = 750;
    long springScalezone = 4500;

    long lrSpringStrength = 0;
    long lrSpringOffset = springConditions[0].lOffset;
    long lrConstantStrength = 0;
    long fbSpringOffset = springConditions[1].lOffset;
    long fbSpringStrength = 0;
    long fbConstantStrength = 0;
    int neutralSpringStrength = 0;

    if (lrValue < JOY_MIDPOINT - springDeadzone) {
        // Upper and lower channel segments, locks left-right movement with supplemental constant effects
        lrSpringOffset = -5000;
        if (lrValue < JOY_QUARTERPOINT) {
            lrSpringStrength = FFB_MAX * scaleRangeValue(lrValue, springDeadzone, springScalezone) * -1;
            lrConstantStrength = FFB_MAX * scaleRangeValue(lrValue, springScalezone * 1.25, springScalezone * 3);
        }
        else {
            lrSpringStrength = FFB_MAX * scaleRangeValue(lrValue, JOY_MIDPOINT - springDeadzone, JOY_MIDPOINT - springScalezone) * -1;
            lrConstantStrength = FFB_MAX * scaleRangeValue(lrValue, JOY_MIDPOINT - springScalezone * 1.25, JOY_MIDPOINT - (springScalezone * 3)) * -1;
        }
        if (fbValue < JOY_MIDPOINT) {
            quadrant = QUADRANT::NW;
        }
        else {
            quadrant = QUADRANT::SW;
        }
    }
    else if (lrValue > JOY_MIDPOINT + springDeadzone) {
        lrSpringOffset = 5000;
        if (lrValue < JOY_THREEQUARTERPOINT) {
            lrSpringStrength = FFB_MAX * scaleRangeValue(lrValue, JOY_MIDPOINT + springDeadzone, JOY_MIDPOINT + springScalezone) * -1;
            lrConstantStrength = FFB_MAX * scaleRangeValue(lrValue, JOY_MIDPOINT + springScalezone * 1.25, JOY_MIDPOINT + (springScalezone * 3));
        }
        else {
            lrSpringStrength = FFB_MAX * scaleRangeValue(lrValue, JOY_MAXPOINT - springDeadzone, JOY_MAXPOINT - springScalezone) * -1;
            lrConstantStrength = FFB_MAX * scaleRangeValue(lrValue, JOY_MAXPOINT - springScalezone * 1.25, JOY_MAXPOINT - (springScalezone * 3)) * -1;
        }
        if (fbValue < JOY_MIDPOINT) {
            quadrant = QUADRANT::NE;
        }
        else {
            quadrant = QUADRANT::SE;
        }
    }
    else {
        lrSpringStrength = 0;
    }
    if (fbValue > JOY_QUARTERPOINT && fbValue < JOY_THREEQUARTERPOINT) {
        // Neutral channel rounded spring effects
        if(neutralShape == NEUTRAL_SHAPE::ROUNDED)
        {
            double lrMidpoint;
            double fbMidpoint;
            if (quadrant == QUADRANT::NW) {
                lrSpringOffset = -5000;
                fbSpringOffset = -5000;
                lrMidpoint = JOY_QUARTERPOINT;
                fbMidpoint = JOY_QUARTERPOINT;
            }
            else if (quadrant == QUADRANT::SW) {
                lrSpringOffset = -5000;
                fbSpringOffset = 5000;
                lrMidpoint = JOY_QUARTERPOINT;
                fbMidpoint = JOY_THREEQUARTERPOINT;
            }
            else if (quadrant == QUADRANT::NE) {
                lrSpringOffset = 5000;
                fbSpringOffset = -5000;
                lrMidpoint = JOY_THREEQUARTERPOINT;
                fbMidpoint = JOY_QUARTERPOINT;
            }
            else if (quadrant == QUADRANT::SE) {
                lrSpringOffset = 5000;
                fbSpringOffset = 5000;
                lrMidpoint = JOY_THREEQUARTERPOINT;
                fbMidpoint = JOY_THREEQUARTERPOINT;
            }
            lrConstantStrength = 0;
            double dx = lrMidpoint - lrValue;
            double dy = fbMidpoint - fbValue;
            double distance = std::sqrt(dx * dx + dy * dy);
            lrSpringStrength = FFB_MAX * scaleRangeValue(distance, JOY_QUARTERPOINT - springDeadzone, JOY_QUARTERPOINT - springScalezone) * -1;
            fbSpringStrength = lrSpringStrength;
        }
        else if (neutralShape == NEUTRAL_SHAPE::SQUARE && (slot_state == SlotState::NEUTRAL_UNDER_SLOT || slot_state == SlotState::NEUTRAL)) {
            lrSpringStrength = 0;
            lrConstantStrength = 0;
            fbSpringOffset = 0;
            if (slot_state == SlotState::NEUTRAL_UNDER_SLOT) {
                fbSpringStrength = 0;
                fbConstantStrength = 0;
            }
            else if (slot_state == SlotState::NEUTRAL) {
                fbSpringStrength = FFB_MAX;
                if (fbValue <= JOY_MIDPOINT) {
                    fbConstantStrength = FFB_MAX * scaleRangeValue(fbValue, JOY_MIDPOINT - springScalezone * 1, JOY_MIDPOINT - springScalezone * 2) * -1;
                }
                else {
                    fbConstantStrength = FFB_MAX * scaleRangeValue(fbValue, JOY_MIDPOINT + springScalezone * 1, JOY_MIDPOINT + springScalezone * 2);
                }
            }
        }
    }
    if (fbValue <= JOY_MIDPOINT) {
        neutralSpringStrength = neutral_spring_strength * scaleRangeValue(fbValue, 5000, 10000);
        //neutralSpringStrength = neutral_spring_strength * scaleRangeValue(fbValue, JOY_QUARTERPOINT - 5000, JOY_QUARTERPOINT + 5000);
    }
    else {
        neutralSpringStrength = neutral_spring_strength * scaleRangeValue(fbValue, JOY_MAXPOINT - 5000, JOY_MAXPOINT - 10000);
        //neutralSpringStrength = neutral_spring_strength * scaleRangeValue(fbValue, JOY_THREEQUARTERPOINT + 5000, JOY_THREEQUARTERPOINT - 5000);
    }
    //qDebug() << "neutralSpringStrength: " << neutralSpringStrength;
    if (fbValue < JOY_MIDPOINT * .65 || fbValue >(JOY_MAXPOINT - JOY_MIDPOINT * .65)) {
        // Gear slotted, enable safety effects
        // Also doubles as a bit of a slot engagement effect in the Y axis
        safe2d[0].lNegativeCoefficient = 10000;
        safe2d[0].lPositiveCoefficient = 10000;
        safe2d[1].lNegativeCoefficient = 4000;
        safe2d[1].lPositiveCoefficient = 4000;
        //if(fbValue < JOY_MIDPOINT)
        //    neutralSpringStrength = neutral_spring_strength * scaleRangeValue(fbValue, 5000, 10000);
        //else
        //    neutralSpringStrength = neutral_spring_strength * scaleRangeValue(fbValue, JOY_MAXPOINT - 5000, JOY_MAXPOINT - 10000);
    } else {
        // Okay to disable safety friction and dampening effects when near neutral
        safe2d[0].lNegativeCoefficient = 0;
        safe2d[1].lNegativeCoefficient = 0;
        safe2d[0].lPositiveCoefficient = 0;
        safe2d[1].lPositiveCoefficient = 0;
    }
    
    springConditions[0].lOffset = lrSpringOffset;
    springConditions[0].lPositiveCoefficient = lrSpringStrength;
    springConditions[0].lNegativeCoefficient = lrSpringStrength;
    springConditions[1].lOffset = fbSpringOffset;
    springConditions[1].lPositiveCoefficient = fbSpringStrength;
    springConditions[1].lNegativeCoefficient = fbSpringStrength;
    device->updateEffect("slotSpring");
    lrcf.lMagnitude = lrConstantStrength;
    device->updateEffect("lrSlotPush");
    fbcf.lMagnitude = fbConstantStrength;
    device->updateEffect("fbSlotPush");
    neutralSpringConditions[0].lPositiveCoefficient = neutralSpringStrength;
    neutralSpringConditions[0].lNegativeCoefficient = neutralSpringStrength;
    neutralSpringConditions[1].lPositiveCoefficient = neutralSpringStrength;
    neutralSpringConditions[1].lNegativeCoefficient = neutralSpringStrength;
    device->updateEffect("neutralSpring");
    device->updateEffect("damper");
    device->updateEffect("friction");
}
*/

void HShifterSlotGuard::updateSlotGuardState(SlotState state) {
    slot_state = state;
}


void HShifterSlotGuard::updateSlotGuardEffects(QPair<int, int> joystickValues) {
    if (slot_state == SlotState::NEUTRAL_UNDER_SLOT) {
        // Disable the effect to prevent thrashing
        // We can scale the condition coefficients near the junctions instead, but good enough for now
        springConditions[0] = noSpring;
        springConditions[1] = noSpring;
        springConditions[1].lDeadBand = 500;
    } else if (slot_state == SlotState::NEUTRAL) {
        springConditions[0] = noSpring;
        springConditions[1] = keepFBCentered;
        springConditions[1].lDeadBand = 500;
    } else if (slot_state == SlotState::SLOT_LEFT_FWD || slot_state == SlotState::SLOT_LEFT_BACK) {
        springConditions[0] = keepLeft;
        springConditions[1] = noSpring;
    } else if (slot_state == SlotState::SLOT_MIDDLE_FWD || slot_state == SlotState::SLOT_MIDDLE_BACK) {
        springConditions[0] = keepLRCentered;
        springConditions[1] = noSpring;
    } else if (slot_state == SlotState::SLOT_RIGHT_FWD || slot_state == SlotState::SLOT_RIGHT_BACK) {
        springConditions[0] = keepRight;
        springConditions[1] = noSpring;
    }
    device->updateEffect("slotSpring");
    setNeutralSpringStrength(joystickValues.second);
}

void HShifterSlotGuard::setNeutralSpringStrength(int fbValue) {
    int neutralSpringStrength = 0;
    if (fbValue <= JOY_MIDPOINT) {
        neutralSpringStrength = neutral_spring_strength * scaleRangeValue(fbValue, 5000, 10000);
        //neutralSpringStrength = neutral_spring_strength * scaleRangeValue(fbValue, JOY_QUARTERPOINT - 5000, JOY_QUARTERPOINT + 5000);
    }
    else {
        neutralSpringStrength = neutral_spring_strength * scaleRangeValue(fbValue, JOY_MAXPOINT - 5000, JOY_MAXPOINT - 10000);
        //neutralSpringStrength = neutral_spring_strength * scaleRangeValue(fbValue, JOY_THREEQUARTERPOINT + 5000, JOY_THREEQUARTERPOINT - 5000);
    }
    neutralSpringConditions[0].lPositiveCoefficient = neutralSpringStrength;
    neutralSpringConditions[0].lNegativeCoefficient = neutralSpringStrength;
    neutralSpringConditions[1].lPositiveCoefficient = neutralSpringStrength;
    neutralSpringConditions[1].lNegativeCoefficient = neutralSpringStrength;
    device->updateEffect("neutralSpring");
}