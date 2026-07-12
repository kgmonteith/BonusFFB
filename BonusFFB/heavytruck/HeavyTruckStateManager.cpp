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
    updateTargetGear();
    updateHeavyTruckSynchroState(gearValues);
    updateHeavyTruckGrindingState();
}

void HeavyTruckStateManager::updateSlotState() {
    HeavyTruckSlotState newState = HeavyTruckSlotState::UNKNOWN;
    const Slot* newSlot = slotPattern->isUnderSlot(joystick);
    bool in_neutral = slotPattern->isInNeutral(joystick);
    if (in_neutral && newSlot != SLOT_NONE) {
        newState = HeavyTruckSlotState::NEUTRAL_UNDER_SLOT;
    } else if (in_neutral) {
        newState = HeavyTruckSlotState::NEUTRAL;
    } else if (newSlot != SLOT_NONE) {
        newState = HeavyTruckSlotState::SLOTTED;
    }
    // Do not allow state change if it's directly from one gear to another without passing through neutral
    bool disallowShift = (newState != slotState && newState == HeavyTruckSlotState::SLOTTED && slotState == HeavyTruckSlotState::SLOTTED && slot != newSlot);
    if (newState != HeavyTruckSlotState::UNKNOWN && (newState != slotState || slot != newSlot) && !disallowShift) {
        slotState = newState;
        slot = newSlot;
        emit slotStateChanged(slotState);
        if (slotState == HeavyTruckSlotState::SLOTTED)
            qDebug() << "HeavyTruckSlotState::SLOTTED";
        else if (slotState == HeavyTruckSlotState::NEUTRAL)
            qDebug() << "HeavyTruckSlotState::NEUTRAL";
        else if (slotState == HeavyTruckSlotState::NEUTRAL_UNDER_SLOT)
            qDebug() << "HeavyTruckSlotState::NEUTRAL_UNDER_SLOT";
        else if (slotState == HeavyTruckSlotState::UNKNOWN)
            qDebug() << "HeavyTruckSlotState::UNKNOWN";
    }
}

void HeavyTruckStateManager::updateTargetGear() {
    int targetSlot = 0;
    if ((slotState == HeavyTruckSlotState::SLOTTED || (slotState == HeavyTruckSlotState::NEUTRAL_UNDER_SLOT && slotPattern->isInGrindZone(joystick))) && slot != nullptr)
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
    qDebug() << "engineRPM: " << engineRPM << ", transmissionRPM: " << transmissionRPM;
    emit targetGearChanged(targetGear);
    emit rpmDeltaChanged(engineRPM - transmissionRPM);
}

void HeavyTruckStateManager::updateButtonZoneState() {
    int newState = 0;
    if (slot != nullptr) {
        if (slotPattern->isInButtonZone(*slot, joystick) || (synchroState == HeavyTruckSynchroState::IN_SYNCH && slotPattern->isInGrindZone(joystick))) {
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
        //qDebug() << "buttonZone changed: " << buttonZoneState;
        emit buttonZoneChanged(buttonZoneState);
    }
}

void HeavyTruckStateManager::updateHeavyTruckSynchroState(QPair<int, int> gearValues) {
    HeavyTruckSynchroState newState = HeavyTruckSynchroState::UNKNOWN;
    if (telemetryState != TelemetrySource::NONE && gearValues.first != 0 && gearValues.second == targetGear) {
        // Gears are synchronized from telemetry reading
        newState = HeavyTruckSynchroState::IN_SYNCH;
    }
    else if ((synchroState == HeavyTruckSynchroState::IN_SYNCH || synchroState == HeavyTruckSynchroState::EXITING_SYNCH) && slotPattern->isInGrindZone(joystick)) {
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
    if (synchroState == HeavyTruckSynchroState::ENTERING_SYNCH && slotState != HeavyTruckSlotState::NEUTRAL && slotPattern->isInGrindZone(joystick)) {
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