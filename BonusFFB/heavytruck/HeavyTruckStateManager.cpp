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

void HeavyTruckStateManager::update() {
    joystick = devices->getJoystickValues();

    // Get new range and splitter values
    rangeSplitter = devices->getShifterValues();

    // Get telemetry values
    QPair<int, int> gearValues = { 0, 0 };
    if (telemetry->isConnected() != TelemetrySource::NONE) {
        gearValues = telemetry->getGearState();
        float engineRPM = telemetry->getEngineRPM();
        emit engineRPMChanged(engineRPM);
    }

    updateSlotState();
    updateButtonZoneState(gearValues);
    updateDetentState();
    updateTargetGear();
    updateHeavyTruckSynchroState(gearValues);
    updateHeavyTruckGrindingState();
}

void HeavyTruckStateManager::updateSlotState() {
    SlotState newState = SlotState::UNKNOWN;
    const Slot* newSlot = slotPattern->isUnderSlot(joystick);
    bool in_neutral = slotPattern->isInNeutral(joystick);
    if (in_neutral && newSlot != SLOT_NONE) {
        newState = SlotState::NEUTRAL_UNDER_SLOT;
    } else if (in_neutral) {
        newState = SlotState::NEUTRAL;
    } else if (newSlot != SLOT_NONE) {
        newState = SlotState::SLOTTED;
    }
    // Do not allow state change if it's directly from one gear to another without passing through neutral
    bool disallowShift = (newState != slotState && newState == SlotState::SLOTTED && slotState == SlotState::SLOTTED && slot != newSlot);
    if (newState != SlotState::UNKNOWN && (newState != slotState || slot != newSlot) && !disallowShift) {
        slotState = newState;
        slot = newSlot;
        emit slotStateChanged(slotState);
        /*
        if (slotState == HeavyTruckSlotState::SLOTTED)
            qDebug() << "HeavyTruckSlotState::SLOTTED";
        else if (slotState == HeavyTruckSlotState::NEUTRAL)
            qDebug() << "HeavyTruckSlotState::NEUTRAL";
        else if (slotState == HeavyTruckSlotState::NEUTRAL_UNDER_SLOT)
            qDebug() << "HeavyTruckSlotState::NEUTRAL_UNDER_SLOT";
        else if (slotState == HeavyTruckSlotState::UNKNOWN)
            qDebug() << "HeavyTruckSlotState::UNKNOWN";
        */
    }
}

void HeavyTruckStateManager::updateTargetGear() {
    int targetSlot = 0;
    if ((slotState == SlotState::SLOTTED || (slotState == SlotState::NEUTRAL_UNDER_SLOT && slotPattern->isInGrindZone(joystick))) && slot != nullptr)
        targetSlot = slot->vJoyButton();
    targetGear = telemetry->getGearForSlot(targetSlot, &rangeSplitter);
    
    float engineRPM = telemetry->getEngineRPM();
    float transmissionRPM = telemetry->getTransmissionRPMForGear(targetGear);

    rpmDelta = engineRPM - transmissionRPM;

    
    if (engineRPM != lastEngineRPM) 
    {
        if (engineRPM > lastEngineRPM)
            rpmIncreasing = true;
        else
            rpmIncreasing = false;
        lastEngineRPM = engineRPM;
        //qDebug() << telemetry->getSpeed() << engineRPM << transmissionRPM << rpmDelta ;
    }

    emit targetGearChanged(targetGear);
    emit rpmDeltaChanged(engineRPM - transmissionRPM);
}

void HeavyTruckStateManager::updateDetentState() {
    if (slot != nullptr) {
        if (detentState == DetentState::ENTERING_DETENT && ((slot->orientation == SLOT_ORIENTATION_FORWARD && joystick.fb <= slotPattern->slotDepthAsJoystick(slot->orientation)) || (slot->orientation == SLOT_ORIENTATION_BACK && joystick.fb >= slotPattern->slotDepthAsJoystick(slot->orientation)))) {
            detentState = DetentState::DETENT_REACHED;
            //qDebug() << "DetentState::DETENT_REACHED";
        }
        else if (detentState == DetentState::DETENT_REACHED && !slotPattern->isInDetentZone(joystick)) {
            detentState = DetentState::EXITING_DETENT;
            //qDebug() << "DetentState::EXITING_DETENT";
        }
        else if (detentState == DetentState::EXITING_DETENT && !slotPattern->isInButtonZone(*slot, joystick)) {
            detentState = DetentState::ENTERING_DETENT;
            //qDebug() << "DetentState::ENTERING_DETENT";
        }
    }
}

void HeavyTruckStateManager::updateButtonZoneState(QPair<int, int> gearValues) {
    int newState = 0;
    if (slot != nullptr) {
        if (slotPattern->isInButtonZone(*slot, joystick) || (synchroState == HeavyTruckSynchroState::IN_SYNCH && slotPattern->isInGrindZone(joystick))) {
            newState = slot->vJoyButton();
        }
        if (detentState == DetentState::EXITING_DETENT) {
            newState = 0;
        }
    } 
    // Un-blip throttle if RPM is increasing, possible fix for truck sim's lack of support for throttle-on shifting
    if (rpmIncreasing && buttonZoneState && synchroState == HeavyTruckSynchroState::ENTERING_SYNCH && abs(rpmDelta) <= 35) {
        qDebug() << "rpmDelta: " << rpmDelta;
        emit unblipThrottle();
    }
    // Un-blip throttle if the stick is in neutral and telemetry says a gear is still engaged
    else if (!buttonZoneState && slotPattern->isInNeutral(joystick) && gearValues.second && devices->getPedalValues().throttle > 0) {
        //qDebug() << "Triggering neutral throttle blip";
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
    if ((telemetry->telemetrySource == TelemetrySource::SCS && gearValues.first != 0 && gearValues.second == targetGear) && (detentState != DetentState::EXITING_DETENT)) {
        // Gears are synchronized from telemetry reading
        newState = HeavyTruckSynchroState::IN_SYNCH;
    }
    else if ((synchroState == HeavyTruckSynchroState::IN_SYNCH || synchroState == HeavyTruckSynchroState::EXITING_SYNCH) && slotPattern->isInGrindZone(joystick) ) {
        // Gears were synched, but now we are exiting sync on our way back to neutral
        newState = HeavyTruckSynchroState::EXITING_SYNCH;
    }
    else {
        // We are out of sync completely and will need to reenter
        newState = HeavyTruckSynchroState::ENTERING_SYNCH;
    }
    if(synchroState != newState) 
    {
        /*
        if (newState == HeavyTruckSynchroState::IN_SYNCH)
            qDebug() << "HeavyTruckSynchroState::IN_SYNCH";
        else if (newState == HeavyTruckSynchroState::EXITING_SYNCH)
            qDebug() << "HeavyTruckSynchroState::EXITING_SYNCH";
        else if (newState == HeavyTruckSynchroState::ENTERING_SYNCH) {
            qDebug() << "HeavyTruckSynchroState::ENTERING_SYNCH";
        }
        */
        synchroState = newState;
        emit synchroStateChanged(synchroState);
    }
}

void HeavyTruckStateManager::updateHeavyTruckGrindingState() {
    HeavyTruckGrindingState newGrindingState = HeavyTruckGrindingState::OFF;
    if (synchroState == HeavyTruckSynchroState::ENTERING_SYNCH && slotState != SlotState::NEUTRAL && slotPattern->isInGrindZone(joystick)) {
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