/*
Copyright (C) 2024-2026
Ken Monteith.

This file is part of Bonus FFB.

Bonus FFB is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or any later version.

Bonus FFB is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Bonus FFB. If not, see <https://www.gnu.org/licenses/>.
*/

#include "HeavyTruckStateManager.h"
#include <QDebug>

void HeavyTruckStateManager::start(DeviceConfiguration* d, Telemetry* t, SlotPattern* spPtr) {
    devices = d;
    telemetry = t;
    //slotParams = sPtr;
    slotPattern = spPtr;
}

void HeavyTruckStateManager::setTelemetryState(TelemetrySource t) {
	telemetryState = t;
}

void HeavyTruckStateManager::update() {
    joystick = devices->getJoystickValues2();

    // Get new range and splitter values
    rangeSplitter = devices->getRangeSplitterValues();

    // Get telemetry values
    QPair<int, int> gearValues = { 0, 0 };
    if (telemetry->isConnected() != TelemetrySource::NONE) {
        gearValues = telemetry->getGearState();
        float engineRPM = telemetry->getEngineRPM();
        if (engineRPM != lastEngineRPM) {
            emit engineRPMChanged(engineRPM);
            lastEngineRPM = engineRPM;
        }
    }

    updateSlotState();
    updateButtonZoneState();
    updateHeavyTruckSynchroState(gearValues);
    updateHeavyTruckGrindingState();
}

/*
bool HeavyTruckStateManager::stateIsInGear(HeavyTruckSlotState state) {
    if (state == HeavyTruckSlotState::SLOT_LEFT_FWD || state == HeavyTruckSlotState::SLOT_LEFT_BACK || state == HeavyTruckSlotState::SLOT_MIDDLE_FWD || state == HeavyTruckSlotState::SLOT_MIDDLE_BACK || state == HeavyTruckSlotState::SLOT_RIGHT_FWD || state == HeavyTruckSlotState::SLOT_RIGHT_BACK)
        return true;
    return false;
}
*/

void HeavyTruckStateManager::updateSlotState() {
    HeavyTruckSlotState newState = HeavyTruckSlotState::UNKNOWN;
    //if (fbValue >= slot->grindPointDepthAsJoystickValueFwd() && fbValue <= slot->grindPointDepthAsJoystickValueBack()) {
    //int under_slot = slot->underSlot(joystick.lr);
    const Slot* newSlot = slotPattern->getNearestSlot(joystick);
    bool in_neutral = slotPattern->isInNeutral(joystick);
    //if (joystick.fb >= JOY_MIDPOINT - slotParams->neutral_channel_half_width && joystick.fb <= JOY_MIDPOINT + slotParams->neutral_channel_half_width) {
    if (in_neutral) {
        newState = HeavyTruckSlotState::NEUTRAL;
    } else if (newSlot != nullptr) {
        newState = HeavyTruckSlotState::SLOTTED;
    }
    // Do not allow state change if it's directly from one gear to another without passing through neutral
    bool disallowShift = (newState != slotState && newState == HeavyTruckSlotState::SLOTTED && slotState == HeavyTruckSlotState::SLOTTED && slot != newSlot);
    if (newState != HeavyTruckSlotState::UNKNOWN && newState != slotState && !disallowShift) {
        slotState = newState;
        slot = newSlot;
        emit slotStateChanged(slotState);
    }
    updateTargetGear();
}

void HeavyTruckStateManager::updateTargetGear() {
    int targetSlot = 0;
    if (slotState == HeavyTruckSlotState::SLOTTED && slot != nullptr)
        targetSlot = slot->button;
    targetGear = telemetry->getGearForSlot(targetSlot, &rangeSplitter);
    
    float engineRPM = telemetry->getEngineRPM();
    float transmissionRPM = telemetry->getTransmissionRPMForGear(targetGear);

    if (engineRPM > lastEngineRPM)
        rpmIncreasing = true;
    else
        rpmIncreasing = false;
    lastEngineRPM = engineRPM;

    rpmDelta = engineRPM - transmissionRPM;
    emit targetGearChanged(targetGear);
    emit rpmDeltaChanged(engineRPM - transmissionRPM);
}

void HeavyTruckStateManager::updateButtonZoneState() {
    int newState = 0;
    /*
    if (joystick.fb <= (slotParams->depthAsJoystickValueFwd() + (button_zone_depth * slotParams->depth)) || (joystick.fb <= slotParams->buttonZoneDepthAsJoystickValueFwd() && telemetryState != TelemetrySource::NONE) || (synchroState == HeavyTruckSynchroState::IN_SYNCH && joystick.fb <= slotParams->grindPointDepthAsJoystickValueFwd())) {
        if (slotState == HeavyTruckSlotState::SLOT_LEFT_FWD)
            newState = 1;
        else if (slotState == HeavyTruckSlotState::SLOT_MIDDLE_FWD)
            newState = 3;
        else if (slotState == HeavyTruckSlotState::SLOT_RIGHT_FWD)
            newState = 5;
    }
    else if (joystick.fb >= slotParams->depthAsJoystickValueBack() - (button_zone_depth * slotParams->depth) || (joystick.fb >= slotParams->buttonZoneDepthAsJoystickValueBack() && telemetryState != TelemetrySource::NONE) || (synchroState == HeavyTruckSynchroState::IN_SYNCH && joystick.fb >= slotParams->grindPointDepthAsJoystickValueBack())) {
        if (slotState == HeavyTruckSlotState::SLOT_LEFT_BACK)
            newState = 2;
        else if (slotState == HeavyTruckSlotState::SLOT_MIDDLE_BACK)
            newState = 4;
        else if (slotState == HeavyTruckSlotState::SLOT_RIGHT_BACK)
            newState = 6;
    }
    */
    if (slot != nullptr) {
        if ((slotPattern->isInButtonZone(*slot, joystick) && telemetryState != TelemetrySource::NONE) || (synchroState == HeavyTruckSynchroState::IN_SYNCH && slotPattern->isInGrindZone(joystick))) {
            newState = slot->button;
        }
    } 
    // Blip throttle if RPM is increasing, possible fix for truck sim's lack of support for throttle-on shifting
    if (rpmIncreasing && buttonZoneState && synchroState == HeavyTruckSynchroState::ENTERING_SYNCH) {
        //qDebug() << "Triggering throttle blip";
        emit unblipThrottle();
    }
    if (buttonZoneState != newState) {
        buttonZoneState = newState;
        qDebug() << "buttonZone changed: " << buttonZoneState;
        emit buttonZoneChanged(buttonZoneState);
    }
}

void HeavyTruckStateManager::updateHeavyTruckSynchroState(QPair<int, int> gearValues) {
    HeavyTruckSynchroState newState = HeavyTruckSynchroState::UNKNOWN;
    if (telemetryState != TelemetrySource::NONE && gearValues.first != 0 && gearValues.second == targetGear) {
        // Gears are synchronized from telemetry reading
        newState = HeavyTruckSynchroState::IN_SYNCH;
    /* } else if (fbValue <= in_synch_depth || fbValue >= JOY_MAXPOINT - in_synch_depth) {
        // Gears are synchronized
        newState = HeavyTruckSynchroState::IN_SYNCH;
        */
        //qDebug() << "HeavyTruckSynchroState::IN_SYNCH";
    }
    else if ((synchroState == HeavyTruckSynchroState::IN_SYNCH || synchroState == HeavyTruckSynchroState::EXITING_SYNCH) && (joystick.fb <= finished_exiting_synch_depth || joystick.fb >= JOY_MAXPOINT - finished_exiting_synch_depth)) {
        // Gears were synched, but now we are exiting sync on our way back to neutral
        newState = HeavyTruckSynchroState::EXITING_SYNCH;
    }
    else {
        // We are out of sync completely and will need to reenter
        newState = HeavyTruckSynchroState::ENTERING_SYNCH;
    }
    if(synchroState != newState) 
    {
        qDebug() << "new synchro state";
        if (newState == HeavyTruckSynchroState::IN_SYNCH)
            qDebug() << "HeavyTruckSynchroState::IN_SYNCH";
        else if (newState == HeavyTruckSynchroState::EXITING_SYNCH)
            qDebug() << "HeavyTruckSynchroState::EXITING_SYNCH";
        else if (newState == HeavyTruckSynchroState::ENTERING_SYNCH) {
            qDebug() << "HeavyTruckSynchroState::ENTERING_SYNCH";
        }
        synchroState = newState;
        emit synchroStateChanged(synchroState);
    }
}

void HeavyTruckStateManager::updateHeavyTruckGrindingState() {
    HeavyTruckGrindingState newGrindingState = HeavyTruckGrindingState::OFF;
    //if (synchroState == HeavyTruckSynchroState::ENTERING_SYNCH && slotState != HeavyTruckSlotState::NEUTRAL && (joystick.fb <= slotParams->grindPointDepthAsJoystickValueFwd() || joystick.fb >= slotParams->grindPointDepthAsJoystickValueBack())) {
    if (synchroState == HeavyTruckSynchroState::ENTERING_SYNCH && slotState == HeavyTruckSlotState::SLOTTED && slotPattern->isInGrindZone(joystick)) {
        if (joystick.fb < JOY_MIDPOINT)
            newGrindingState = HeavyTruckGrindingState::GRINDING_FWD;
        else
            newGrindingState = HeavyTruckGrindingState::GRINDING_BACK;
    }
    if (newGrindingState != grindingState)
    {
        grindingState = newGrindingState;
        emit grindingStateChanged(grindingState);
    }
}