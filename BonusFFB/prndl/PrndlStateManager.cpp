/*
Copyright (C) 2024-
Ken Monteith.

This file is part of Bonus FFB.

Bonus FFB is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or any later version.

Bonus FFB is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Bonus FFB. If not, see <https://www.gnu.org/licenses/>.
*/

#include "PrndlStateManager.h"
#include <QDebug>

PrndlStateManager::PrndlStateManager() {
    // Initialize slots
    enabledPrndlSlots.append(PrndlSlot::PARK);
    enabledPrndlSlots.append(PrndlSlot::REVERSE);
    enabledPrndlSlots.append(PrndlSlot::NEUTRAL);
    enabledPrndlSlots.append(PrndlSlot::DRIVE);
    enabledPrndlSlots.append(PrndlSlot::LOW);
}

void PrndlStateManager::setTelemetryState(TelemetrySource t) {
	telemetryState = t;
}

void PrndlStateManager::update(QPair<int, int> joystickValues, bool shiftLockButtonState, bool parkingBrakeState) {
    long lrValue = joystickValues.first;
    long fbValue = joystickValues.second;
    //updateShiftLockButtonState(shiftLockButtonState);
    //updateSlotState(lrValue, fbValue);
    updateButtonZoneState(fbValue, shiftLockButtonState, parkingBrakeState);
    updateSlotState(fbValue, shiftLockButtonState);
    updateShiftLockEffect(fbValue, shiftLockButtonState);
    //updateSynchroState(lrValue, fbValue, gearValues);
    //updateGrindingState(lrValue, fbValue);
}

int PrndlStateManager::getEnabledSlotCount() {
    return enabledPrndlSlots.length();
}

bool PrndlStateManager::isParkEnabled() {
    for (const auto slot : enabledPrndlSlots) {
        if (slot == PrndlSlot::PARK) {
            return true;
        }
    }
    return false;
}

void PrndlStateManager::toggleUsingShiftLock(int index) {
    if (index > 0) {
        using_shift_lock = true;
    }
    else {
        using_shift_lock = false;
    }
}

void PrndlStateManager::toggleParkSlot(bool parkEnabled) {
    if (parkEnabled) {
        enabledPrndlSlots.prepend(PrndlSlot::PARK);
    }
    else {
        enabledPrndlSlots.pop_front();
    }
    emit buttonZoneChanged(getButtonNumberForSlot(PrndlSlot::NONE));
}

void PrndlStateManager::toggleLastSlot(bool parkEnabled) {
    if (parkEnabled) {
        enabledPrndlSlots.append(PrndlSlot::LOW);
    }
    else {
        enabledPrndlSlots.pop_back();
    }
    emit buttonZoneChanged(getButtonNumberForSlot(PrndlSlot::NONE));
}

void PrndlStateManager::toggleAtsTelemetryPark(bool newState) {
    using_ats_telemetry_park = newState;
}

void PrndlStateManager::toggleLockShiftsFromNeutralToReverse(bool newState) {
    lock_shifts_from_neutral_to_reverse = newState;
}

int PrndlStateManager::getButtonNumberForSlot(PrndlSlot slot) {
    switch (slot) {
    case PrndlSlot::NONE:
        return 0;
    case PrndlSlot::PARK:
        return buttonOffset;
    case PrndlSlot::REVERSE:
        return buttonOffset + 1;
    case PrndlSlot::NEUTRAL:
        return buttonOffset + 2;
    case PrndlSlot::DRIVE:
        return buttonOffset + 3;
    case PrndlSlot::LOW:
        return buttonOffset + 4;
    }
    return 0;
}

double PrndlStateManager::getFBValueForSlot(PrndlSlot slot) {
    double slotCenterStep = 65536 / (enabledPrndlSlots.length() - 1);
    return slotCenterStep * enabledPrndlSlots.indexOf(slot);
}

void PrndlStateManager::updateButtonZoneState(long fbValue, bool shiftLockButtonState, bool parkingBrakeState) {
    PrndlSlot newState = PrndlSlot::NONE;
    for (const auto prndlSlot : enabledPrndlSlots) {
        double slotCenter = getFBValueForSlot(prndlSlot);
        if (fbValue >= slotCenter - slot_half_depth && fbValue <= slotCenter + slot_half_depth) {
            newState = prndlSlot;
            break;
        }
    }
    if (buttonZoneSlot != newState) {
        // If we're using shift lock and trying to leave park, ensure the shift lock is depressed
        if (using_shift_lock && buttonZoneSlot == PrndlSlot::PARK && newState != PrndlSlot::PARK && !shiftLockButtonState) {
            // Return without leaving park
            return;
        }
        // Same, but for moving from Neutral to Reverse
        else if (using_shift_lock && lock_shifts_from_neutral_to_reverse && buttonZoneSlot == PrndlSlot::NEUTRAL && fbValue < getFBValueForSlot(PrndlSlot::NEUTRAL) && !shiftLockButtonState) {
            return;
        }
        // There's no park slot in ATS/ETS2, so simulate park using the parking brake telemetry
        if (using_ats_telemetry_park) {
            if (newState == PrndlSlot::PARK) {
                emit buttonZoneChanged(getButtonNumberForSlot(PrndlSlot::NEUTRAL));
                if (!parkingBrakeState) {
                    emit buttonShortPress(getButtonNumberForSlot(PrndlSlot::PARK));
                }
            }
            else if (newState != PrndlSlot::PARK) {
                emit buttonZoneChanged(getButtonNumberForSlot(newState));
                if (parkingBrakeState) {
                    emit buttonShortPress(getButtonNumberForSlot(PrndlSlot::PARK));
                }
            }
            else {
                emit buttonZoneChanged(getButtonNumberForSlot(newState));
            }
        }
        else {
            emit buttonZoneChanged(getButtonNumberForSlot(newState));
        }
        buttonZoneSlot = newState;
        emit slotChanged(buttonZoneSlot);
    }
}

// This is different from the button state, since the spring effect covers a wider range. But they can probably be combined if it works.
void PrndlStateManager::updateSlotState(long fbValue, bool shiftLockButtonState) {
    PrndlSlot newSlot;
    float slotCenterStep = 65536 / (enabledPrndlSlots.length() - 1);
    float slotDepth = slotCenterStep / 2.0;
    float testedSlotCenter = 0;
    float scaledFFBCenter = -10000;
    float scaledFFBCenterStep = 20000 / (enabledPrndlSlots.length() - 1);
    for (const auto prndlSlot : enabledPrndlSlots) {
        if (fbValue >= testedSlotCenter - slotDepth && fbValue <= testedSlotCenter + slotDepth) {
            newSlot = prndlSlot;
            break;
        }
        testedSlotCenter += slotCenterStep;
        scaledFFBCenter += scaledFFBCenterStep;
    }
    if (newSlot != engagedSlotState && newSlot != PrndlSlot::NONE) {
        if (using_shift_lock && engagedSlotState == PrndlSlot::PARK && !shiftLockButtonState) {
            // Using shift lock but button isn't pressed, so no state change
            return;
        }
        else if (using_shift_lock && lock_shifts_from_neutral_to_reverse && engagedSlotState == PrndlSlot::NEUTRAL && (newSlot == PrndlSlot::REVERSE || newSlot == PrndlSlot::PARK) && !shiftLockButtonState) {
            // Same, but for N to R
            return;
        }
        else if (newSlot == PrndlSlot::REVERSE) {
            delay_shift_lock = true;
        }
        lastEngagedSlotState = engagedSlotState;
        engagedSlotState = newSlot;
        emit slotSpringChanged((long)scaledFFBCenter);
    }
}

void PrndlStateManager::updateShiftLockEffect(long fbValue, bool shiftLockButtonState) {
    double scaledEffectStrength = 0;
    double nFBValue = getFBValueForSlot(PrndlSlot::NEUTRAL);
    if (engagedSlotState == PrndlSlot::PARK) {
        scaledEffectStrength = FFB_MAX * scaleRangeValue(fbValue, SHIFT_LOCK_OFFSET_LOW, SHIFT_LOCK_OFFSET_HIGH);
    }
    else if (engagedSlotState == PrndlSlot::NEUTRAL) {
        scaledEffectStrength = FFB_MAX * scaleRangeValue(fbValue, nFBValue - SHIFT_LOCK_OFFSET_LOW, nFBValue - SHIFT_LOCK_OFFSET_HIGH) * -1;
    }
    // Delay the shift lock effect if we just left reverse
    // This will prevent the shift lock from bumping the lever past gears
    if (delay_shift_lock && (fbValue <= SHIFT_LOCK_OFFSET_LOW || fbValue >= nFBValue - SHIFT_LOCK_OFFSET_LOW)) {
        delay_shift_lock = false;
    }
    if (delay_shift_lock || shiftLockButtonState) {
        scaledEffectStrength = 0;
    }
    if (using_shift_lock && ((engagedSlotState == PrndlSlot::PARK) || (lock_shifts_from_neutral_to_reverse && engagedSlotState == PrndlSlot::NEUTRAL))) {
        emit updateShiftLockEffectStrength(scaledEffectStrength);
    }
}
