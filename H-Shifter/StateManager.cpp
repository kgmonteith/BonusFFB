/*
This file is part of Bonus FFB.

Bonus FFB is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or any later version.

Bonus FFB is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Bonus FFB. If not, see <https://www.gnu.org/licenses/>.
*/

#include "StateManager.h"
#include <QDebug>

void StateManager::setTelemetryState(TelemetrySource t) {
	telemetryState = t;
}

void StateManager::update(long lrValue, long fbValue, long clutchValue, long throttleValue, QPair<int, int> gearValues) {
    updateSlotState(lrValue, fbValue);
    updateButtonZoneState(lrValue, fbValue);
    updateSynchroState(lrValue, fbValue, gearValues);
    updateGrindingState(lrValue, fbValue);
}

void StateManager::updateSlotState(long lrValue, long fbValue) {
    SlotGuard::SlotState newState = SlotGuard::SlotState::UNKNOWN;
    bool inNeutral = fbValue <= JOY_MIDPOINT + neutral_channel_half_width && fbValue >= JOY_MIDPOINT - neutral_channel_half_width;
    if (inNeutral && newState != SlotGuard::SlotState::UNKNOWN) {
        newState = SlotGuard::SlotState::NEUTRAL_UNDER_SLOT;
    }
    else if (inNeutral) {
        newState = SlotGuard::SlotState::NEUTRAL;
    } else if (lrValue <= JOY_MINPOINT + side_slot_width) {
        // In or under left channel
        if (fbValue <= JOY_MIDPOINT)
            newState = SlotGuard::SlotState::SLOT_LEFT_FWD;
        else
            newState = SlotGuard::SlotState::SLOT_LEFT_BACK;
    }
    else if (lrValue >= JOY_MIDPOINT - middle_slot_half_width && lrValue <= JOY_MIDPOINT + middle_slot_half_width)
    {
        // In or under center channel
        if (fbValue <= JOY_MIDPOINT)
            newState = SlotGuard::SlotState::SLOT_MIDDLE_FWD;
        else
            newState = SlotGuard::SlotState::SLOT_MIDDLE_BACK;
    }
    else if (lrValue >= JOY_MAXPOINT - side_slot_width) {
        // In neutral under right channel
        if (fbValue <= JOY_MIDPOINT)
            newState = SlotGuard::SlotState::SLOT_RIGHT_FWD;
        else
            newState = SlotGuard::SlotState::SLOT_RIGHT_BACK;
    }
    if (newState != slotState && newState != SlotGuard::SlotState::UNKNOWN) {
        slotState = newState;
        emit slotStateChanged(slotState);
    }
}

void StateManager::updateButtonZoneState(long lrValue, long fbValue) {
    int newState = 0;
    if (fbValue <= button_zone_depth || (fbValue <= button_zone_depth_telemetry && telemetryState != TelemetrySource::NONE)) {
        if (slotState == SlotGuard::SlotState::SLOT_LEFT_FWD)
            newState = 1;
        else if (slotState == SlotGuard::SlotState::SLOT_MIDDLE_FWD)
            newState = 3;
        else if (slotState == SlotGuard::SlotState::SLOT_RIGHT_FWD)
            newState = 5;
    }
    else if (fbValue >= JOY_MAXPOINT - button_zone_depth || (fbValue >= JOY_MAXPOINT - button_zone_depth_telemetry && telemetryState != TelemetrySource::NONE)) {
        if (slotState == SlotGuard::SlotState::SLOT_LEFT_BACK)
            newState = 2;
        else if (slotState == SlotGuard::SlotState::SLOT_MIDDLE_BACK)
            newState = 4;
        else if (slotState == SlotGuard::SlotState::SLOT_RIGHT_BACK)
            newState = 6;
    }
    if (buttonZoneState != newState) {
        buttonZoneState = newState;
        emit buttonZoneChanged(buttonZoneState);
    }
}

void StateManager::updateSynchroState(long lrValue, long fbValue, QPair<int, int> gearValues) {
    SynchroGuard::SynchroState newState = SynchroGuard::SynchroState::UNKNOWN;
    if (telemetryState != TelemetrySource::NONE && gearValues.first != 0 && gearValues.second != 0) {
        // Gears are synchronized from telemetry reading
        newState = SynchroGuard::SynchroState::IN_SYNCH;
    } else if (fbValue <= in_synch_depth || fbValue >= JOY_MAXPOINT - in_synch_depth) {
        // Gears are synchronized
        newState = SynchroGuard::SynchroState::IN_SYNCH;
    }
    else if ((synchroState == SynchroGuard::IN_SYNCH || synchroState == SynchroGuard::EXITING_SYNCH) && (fbValue <= finished_exiting_synch_depth || fbValue >= JOY_MAXPOINT - finished_exiting_synch_depth)) {
        // Gears were synched, but now we are exiting sync on our way back to neutral
        newState = SynchroGuard::SynchroState::EXITING_SYNCH;
    }
    else {
        // We are out of sync completely and will need to reenter
        newState = SynchroGuard::SynchroState::ENTERING_SYNCH;
    }
    if (newState != synchroState) {
        qDebug() << "New synchro state: " << newState << ", gearValues: " << gearValues;
        synchroState = newState;
        emit synchroStateChanged(synchroState);
    }
}

void StateManager::updateGrindingState(long lrValue, long fbValue) {
    bool newState = false;
    if (grindingState == false && synchroState == SynchroGuard::ENTERING_SYNCH && (fbValue <= grind_point_depth || fbValue >= JOY_MAXPOINT - grind_point_depth)) {
        newState = true;
    }
    if (newState != grindingState) {
        grindingState = newState;
        emit grindingStateChanged(grindingState);
    }
}