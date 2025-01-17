/*
This file is part of Bonus FFB.

Bonus FFB is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or any later version.

Bonus FFB is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Bonus FFB. If not, see <https://www.gnu.org/licenses/>.
*/

#include "StateManager.h"
#include <QDebug>

void StateManager::setTelemetryState(TelemetryState t) {
	telemetryState = t;
}

void StateManager::update(long lrValue, long fbValue, long clutchValue, long throttleValue) {
    updateSlotState(lrValue, fbValue);
    updateButtonZoneState(lrValue, fbValue);
    updateSynchroState(lrValue, fbValue);
}

void StateManager::updateSlotState(long lrValue, long fbValue) {
    SlotGuard::SlotState newState = SlotGuard::SlotState::UNKNOWN;
    bool inNeutral = fbValue <= JOY_MIDPOINT + neutral_channel_half_width && fbValue >= JOY_MIDPOINT - neutral_channel_half_width;
    if (inNeutral && newState != SlotGuard::SlotState::UNKNOWN) {
        newState = SlotGuard::SlotState::NEUTRAL_UNDER_SLOT;
    }
    else if (inNeutral) {
        newState = SlotGuard::SlotState::NEUTRAL;
    }
    if (lrValue <= JOY_MINPOINT + side_slot_width) {
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
    vJoyFeeder::ButtonPressState newState = vJoyFeeder::ButtonPressState::NONE;
    if (fbValue <= button_zone_depth || (fbValue <= button_zone_depth_telemetry && telemetryState == TelemetryState::ENABLED)) {
        if (slotState == SlotGuard::SlotState::SLOT_LEFT_FWD)
            newState = vJoyFeeder::ButtonPressState::ONE;
        else if (slotState == SlotGuard::SlotState::SLOT_MIDDLE_FWD)
            newState = vJoyFeeder::ButtonPressState::THREE;
        else if (slotState == SlotGuard::SlotState::SLOT_RIGHT_FWD)
            newState = vJoyFeeder::ButtonPressState::FIVE;
    }
    else if (fbValue >= JOY_MAXPOINT - button_zone_depth || (fbValue >= JOY_MAXPOINT - button_zone_depth && telemetryState == TelemetryState::ENABLED)) {
        if (slotState == SlotGuard::SlotState::SLOT_LEFT_BACK)
            newState = vJoyFeeder::ButtonPressState::TWO;
        else if (slotState == SlotGuard::SlotState::SLOT_MIDDLE_BACK)
            newState = vJoyFeeder::ButtonPressState::FOUR;
        else if (slotState == SlotGuard::SlotState::SLOT_RIGHT_BACK)
            newState = vJoyFeeder::ButtonPressState::SIX;
    }
    if (buttonZoneState != newState) {
        buttonZoneState = newState;
        emit buttonZoneChanged(buttonZoneState);
    }
}

void StateManager::updateSynchroState(long lrValue, long fbValue) {
    SynchroGuard::SynchroState newState = SynchroGuard::SynchroState::UNKNOWN;
    if (fbValue <= in_synch_depth || fbValue >= JOY_MAXPOINT - in_synch_depth) {
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
        synchroState = newState;
        emit synchroStateChanged(synchroState);
    }
}