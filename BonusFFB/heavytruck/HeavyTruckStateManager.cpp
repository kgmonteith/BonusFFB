/*
Copyright (C) 2024-
Ken Monteith.

This file is part of Bonus FFB.

Bonus FFB is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or any later version.

Bonus FFB is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Bonus FFB. If not, see <https://www.gnu.org/licenses/>.
*/

#include "HeavyTruckStateManager.h"
#include <QDebug>

void HeavyTruckStateManager::start(Telemetry* t, SlotParameters * sPtr) {
    telemetry = t;
    slot = sPtr;
}

void HeavyTruckStateManager::setTelemetryState(TelemetrySource t) {
	telemetryState = t;
}

void HeavyTruckStateManager::update(QPair<int, int> joystickValues, QPair<int, int> pedalValues, QPair<int, int> gearValues) {
    long lrValue = joystickValues.first;
    long fbValue = joystickValues.second;
    updateSlotState(lrValue, fbValue);
    updateButtonZoneState(lrValue, fbValue);
    updateHeavyTruckSynchroState(lrValue, fbValue, gearValues);
    updateHeavyTruckGrindingState(lrValue, fbValue);
}

bool HeavyTruckStateManager::stateIsInGear(HeavyTruckSlotState state) {
    if (state == HeavyTruckSlotState::SLOT_LEFT_FWD || state == HeavyTruckSlotState::SLOT_LEFT_BACK || state == HeavyTruckSlotState::SLOT_MIDDLE_FWD || state == HeavyTruckSlotState::SLOT_MIDDLE_BACK || state == HeavyTruckSlotState::SLOT_RIGHT_FWD || state == HeavyTruckSlotState::SLOT_RIGHT_BACK)
        return true;
    return false;
}

void HeavyTruckStateManager::updateSlotState(long lrValue, long fbValue) {
    HeavyTruckSlotState newState = HeavyTruckSlotState::UNKNOWN;
    bool inNeutral = fbValue <= JOY_MIDPOINT + neutral_channel_half_width && fbValue >= JOY_MIDPOINT - neutral_channel_half_width;
    if (lrValue <= JOY_MINPOINT + side_slot_width) {
        // In or under left channel
        if (fbValue <= JOY_MIDPOINT - neutral_channel_half_width)
            newState = HeavyTruckSlotState::SLOT_LEFT_FWD;
        else if(fbValue >= JOY_MIDPOINT + neutral_channel_half_width)
            newState = HeavyTruckSlotState::SLOT_LEFT_BACK;
        else
            newState = HeavyTruckSlotState::NEUTRAL_UNDER_SLOT;
    }
    else if (lrValue >= slot->asJoystickValue(1) - slot->middle_slot_half_width && lrValue <= slot->asJoystickValue(1) + slot->middle_slot_half_width)
    {
        // In or under center channel
        if (fbValue <= JOY_MIDPOINT - neutral_channel_half_width)
            newState = HeavyTruckSlotState::SLOT_MIDDLE_FWD;
        else if (fbValue >= JOY_MIDPOINT + neutral_channel_half_width)
            newState = HeavyTruckSlotState::SLOT_MIDDLE_BACK;
        else
            newState = HeavyTruckSlotState::NEUTRAL_UNDER_SLOT;
    }
    else if (lrValue >= slot->asJoystickValue(2) - side_slot_width) {
        // In neutral under right channel
        if (fbValue <= JOY_MIDPOINT - neutral_channel_half_width)
            newState = HeavyTruckSlotState::SLOT_RIGHT_FWD;
        else if (fbValue >= JOY_MIDPOINT + neutral_channel_half_width)
            newState = HeavyTruckSlotState::SLOT_RIGHT_BACK;
        else
            newState = HeavyTruckSlotState::NEUTRAL_UNDER_SLOT;
    } else if (inNeutral) {
        newState = HeavyTruckSlotState::NEUTRAL;
    }
    // Do not allow state change if it's directly from one gear to another without passing through neutral
    bool disallowShift = (newState != slotState && stateIsInGear(newState) && stateIsInGear(slotState));
    if (newState != HeavyTruckSlotState::UNKNOWN && newState != slotState && !disallowShift) {
        slotState = newState;
        emit slotStateChanged(slotState);
    }
    updateTargetGear();
}

void HeavyTruckStateManager::updateTargetGear() {
    int targetSlot = 0;
    if (slotState == HeavyTruckSlotState::SLOT_LEFT_FWD)
        targetSlot = 1;
    else if (slotState == HeavyTruckSlotState::SLOT_MIDDLE_FWD)
        targetSlot = 3;
    else if (slotState == HeavyTruckSlotState::SLOT_RIGHT_FWD)
        targetSlot = 5;
    else if (slotState == HeavyTruckSlotState::SLOT_LEFT_BACK)
        targetSlot = 2;
    else if (slotState == HeavyTruckSlotState::SLOT_MIDDLE_BACK)
        targetSlot = 4;
    else if (slotState == HeavyTruckSlotState::SLOT_RIGHT_BACK)
        targetSlot = 6;
    int targetGear = telemetry->getGearForSlot(targetSlot);
    
    float engineRPM = telemetry->getEngineRPM();
    float transmissionRPM = telemetry->getTransmissionRPMForGear(targetGear);

    rpmDelta = engineRPM - transmissionRPM;
    emit targetGearChanged(targetGear);
    emit rpmDeltaChanged(engineRPM - transmissionRPM);
}

void HeavyTruckStateManager::updateButtonZoneState(long lrValue, long fbValue) {
    int newState = 0;
    if (fbValue <= (slot->depthAsJoystickValueFwd() + (button_zone_depth * slot->depth)) || (fbValue <= slot->buttonZoneDepthAsJoystickValueFwd() && telemetryState != TelemetrySource::NONE) || (buttonZoneState && fbValue <= slot->grindPointDepthAsJoystickValueFwd())) {
        if (slotState == HeavyTruckSlotState::SLOT_LEFT_FWD)
            newState = 1;
        else if (slotState == HeavyTruckSlotState::SLOT_MIDDLE_FWD)
            newState = 3;
        else if (slotState == HeavyTruckSlotState::SLOT_RIGHT_FWD)
            newState = 5;
    }
    else if (fbValue >= slot->depthAsJoystickValueBack() - (button_zone_depth * slot->depth) || (fbValue >= slot->buttonZoneDepthAsJoystickValueBack() && telemetryState != TelemetrySource::NONE) || (buttonZoneState && fbValue >= slot->grindPointDepthAsJoystickValueBack())) {
        if (slotState == HeavyTruckSlotState::SLOT_LEFT_BACK)
            newState = 2;
        else if (slotState == HeavyTruckSlotState::SLOT_MIDDLE_BACK)
            newState = 4;
        else if (slotState == HeavyTruckSlotState::SLOT_RIGHT_BACK)
            newState = 6;
    }
    if (buttonZoneState != newState) {
        buttonZoneState = newState;
        emit buttonZoneChanged(buttonZoneState);
    }
}

void HeavyTruckStateManager::updateHeavyTruckSynchroState(long lrValue, long fbValue, QPair<int, int> gearValues) {
    HeavyTruckSynchroState newState = HeavyTruckSynchroState::UNKNOWN;
    if (telemetryState != TelemetrySource::NONE && gearValues.first != 0 && gearValues.second != 0) {
        // Gears are synchronized from telemetry reading
        newState = HeavyTruckSynchroState::IN_SYNCH;
    /* } else if (fbValue <= in_synch_depth || fbValue >= JOY_MAXPOINT - in_synch_depth) {
        // Gears are synchronized
        newState = HeavyTruckSynchroState::IN_SYNCH;
        */
    }
    else if ((synchroState == HeavyTruckSynchroState::IN_SYNCH || synchroState == HeavyTruckSynchroState::EXITING_SYNCH) && (fbValue <= finished_exiting_synch_depth || fbValue >= JOY_MAXPOINT - finished_exiting_synch_depth)) {
        // Gears were synched, but now we are exiting sync on our way back to neutral
        newState = HeavyTruckSynchroState::EXITING_SYNCH;
    }
    else {
        // We are out of sync completely and will need to reenter
        newState = HeavyTruckSynchroState::ENTERING_SYNCH;
    }
    synchroState = newState;
    emit synchroStateChanged(synchroState, fbValue);
}

void HeavyTruckStateManager::updateHeavyTruckGrindingState(long lrValue, long fbValue) {
    HeavyTruckGrindingState newGrindingState = HeavyTruckGrindingState::OFF;
    if (synchroState == HeavyTruckSynchroState::ENTERING_SYNCH && slotState != HeavyTruckSlotState::NEUTRAL && (fbValue <= slot->grindPointDepthAsJoystickValueFwd() || fbValue >= slot->grindPointDepthAsJoystickValueBack())) {
        if (fbValue < JOY_MIDPOINT)
            newGrindingState = HeavyTruckGrindingState::GRINDING_FWD;
        else
            newGrindingState = HeavyTruckGrindingState::GRINDING_BACK;
    }
    if (newGrindingState != grindingState)
    {
        grindingState = newGrindingState;
        emit grindingStateChanged(grindingState, fbValue);
    }
}