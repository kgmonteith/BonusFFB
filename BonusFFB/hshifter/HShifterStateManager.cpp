/*
Copyright (C) 2024-
Ken Monteith.

This file is part of Bonus FFB.

Bonus FFB is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or any later version.

Bonus FFB is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Bonus FFB. If not, see <https://www.gnu.org/licenses/>.
*/

#include "HShifterStateManager.h"
#include <QDebug>

void HShifterStateManager::start(DeviceConfiguration* d, Telemetry* t, SlotPattern* spPtr) {
    devices = d;
    telemetry = t;
    slotPattern = spPtr;
}

void HShifterStateManager::setTelemetryState(TelemetrySource t) {
	telemetryState = t;
}

void HShifterStateManager::update() {
    joystick = devices->getJoystickValues();

    updateSlotState();
    updateButtonZoneState();
    updateSynchroState();
    updateGrindingState();
}

void HShifterStateManager::updateSlotState() {
    SlotState newState = SlotState::UNKNOWN;
    const Slot* newSlot = slotPattern->isUnderSlot(joystick);

    bool in_neutral = slotPattern->isInNeutral(joystick);
    if (in_neutral && newSlot != SLOT_NONE) {
        newState = SlotState::NEUTRAL_UNDER_SLOT;
    }
    else if (in_neutral) {
        newState = SlotState::NEUTRAL;
    }
    else if (newSlot != SLOT_NONE) {
        newState = SlotState::SLOTTED;
    }
    // Do not allow state change if it's directly from one gear to another without passing through neutral
    bool disallowShift = (newState != slotState && newState == SlotState::SLOTTED && slotState == SlotState::SLOTTED && slot != newSlot);
    if (newState != SlotState::UNKNOWN && (newState != slotState || slot != newSlot) && !disallowShift) {
        slotState = newState;
        slot = newSlot;
        emit slotStateChanged(slotState);
        /*
        if (slotState == HShifterSlotState::SLOTTED)
            qDebug() << "HShifterSlotState::SLOTTED";
        else if (slotState == HShifterSlotState::NEUTRAL)
            qDebug() << "HShifterSlotState::NEUTRAL";
        else if (slotState == HShifterSlotState::NEUTRAL_UNDER_SLOT)
            qDebug() << "HShifterSlotState::NEUTRAL_UNDER_SLOT";
        else if (slotState == HShifterSlotState::UNKNOWN)
            qDebug() << "HShifterSlotState::UNKNOWN";
        */
    }
}

void HShifterStateManager::updateButtonZoneState() {
    int newState = 0;
    if (slot != nullptr) {
        if (slotPattern->isInDetentZone(joystick)) {
            newState = slot->vJoyButton();
        }
    }
    // Either old or new state must be neutral, no direct change from gear to gear
    if (buttonZoneState != newState && (!buttonZoneState || !newState)) { 
        buttonZoneState = newState;
        //qDebug() << "buttonZone changed: " << buttonZoneState;
        emit buttonZoneChanged(buttonZoneState);
        if (newState)
            emit slotTextChanged(slot->asText());
        else
            emit slotTextChanged("N");
    }
}

void HShifterStateManager::updateSynchroState() {
    SynchroState newState = SynchroState::UNKNOWN;
    if (slotPattern->isInDetentZone(joystick)) {
        // Gears are synchronized
        newState = SynchroState::IN_SYNCH;
    }
    else if ((synchroState == SynchroState::IN_SYNCH || synchroState == SynchroState::EXITING_SYNCH) && slotPattern->isInGrindZone(joystick)) {
        // Gears were synched, but now we are exiting sync on our way back to neutral
        newState = SynchroState::EXITING_SYNCH;
    }
    else {
        // We are out of sync completely and will need to reenter
        newState = SynchroState::ENTERING_SYNCH;
    }
    if (synchroState != newState)
    {
        /*
        if (newState == SynchroState::IN_SYNCH)
            qDebug() << "SynchroState::IN_SYNCH";
        else if (newState == SynchroState::EXITING_SYNCH)
            qDebug() << "SynchroState::EXITING_SYNCH";
        else if (newState == SynchroState::ENTERING_SYNCH) {
            qDebug() << "SynchroState::ENTERING_SYNCH";
        }
        */
        synchroState = newState;
        emit synchroStateChanged(synchroState);
    }
}

void HShifterStateManager::updateGrindingState() {
    GrindingState newGrindingState = GrindingState::OFF;
    if (synchroState == SynchroState::ENTERING_SYNCH && slotState != SlotState::NEUTRAL && slotPattern->isInGrindZone(joystick)) {
        if (joystick.fb < JOY_MIDPOINT)
            newGrindingState = GrindingState::GRINDING_FWD;
        else
            newGrindingState = GrindingState::GRINDING_BACK;
    }
    if (newGrindingState != grindingState)
    {
        grindingState = newGrindingState;
        emit grindingStateChanged(grindingState);
    }
}