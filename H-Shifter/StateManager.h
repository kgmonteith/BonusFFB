/*
This file is part of Bonus FFB.

Bonus FFB is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or any later version.

Bonus FFB is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Bonus FFB. If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once
#include <QObject>
#include "vJoyFeeder.h"
#include "SlotGuard.h"
#include "SynchroGuard.h"

class StateManager: public QObject
{
    Q_OBJECT;

public:
    // Tracks whether telemetry is running
    enum TelemetryState {
        DISABLED,
        ENABLED
    };
    Q_ENUM(TelemetryState)

public:
    void update(long, long, long, long);

public slots:
    void setTelemetryState(TelemetryState);

signals:
    void slotStateChanged(SlotGuard::SlotState);
    void buttonZoneChanged(int);
    void synchroStateChanged(SynchroGuard::SynchroState);

private:
    void updateSlotState(long, long);
    void updateButtonZoneState(long, long);
    void updateSynchroState(long, long);

    TelemetryState telemetryState = TelemetryState::DISABLED;
    SlotGuard::SlotState slotState = SlotGuard::SlotState::NEUTRAL;
    int buttonZoneState = 0;
    SynchroGuard::SynchroState synchroState = SynchroGuard::SynchroState::ENTERING_SYNCH;

    int side_slot_width = 500;
    int middle_slot_half_width = 1200;
    int neutral_channel_half_width = 1400;

    int button_zone_half_width = 2000;
    int button_zone_depth = 4000;
    int button_zone_depth_telemetry = 10000;

    int in_synch_depth = JOY_MIDPOINT * 0.20;
    int finished_exiting_synch_depth = JOY_MIDPOINT * 0.55;
};

