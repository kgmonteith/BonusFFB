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
#include "Telemetry.h"

class StateManager: public QObject
{
    Q_OBJECT;

public:
    void update(long, long, long, long, QPair<int, int>);

public slots:
    void setTelemetryState(TelemetrySource);

signals:
    void slotStateChanged(SlotGuard::SlotState);
    void buttonZoneChanged(int);
    void synchroStateChanged(SynchroGuard::SynchroState);
    void grindingStateChanged(bool);

private:
    void updateSlotState(long, long);
    void updateButtonZoneState(long, long);
    void updateSynchroState(long, long, QPair<int, int>);
    void updateGrindingState(long, long);

    int buttonZoneState = 0;
    TelemetrySource telemetryState = TelemetrySource::NONE;
    SlotGuard::SlotState slotState = SlotGuard::SlotState::NEUTRAL;
    SynchroGuard::SynchroState synchroState = SynchroGuard::SynchroState::ENTERING_SYNCH;
    bool grindingState = false;

    int side_slot_width = 500;
    int middle_slot_half_width = 1200;
    int neutral_channel_half_width = 1400;

    int button_zone_half_width = 2000;
    int button_zone_depth = 4000;
    int button_zone_depth_telemetry = JOY_MIDPOINT * 0.65;

    int in_synch_depth = JOY_MIDPOINT * 0.20;
    int finished_exiting_synch_depth = JOY_MIDPOINT * 0.55;
    int grind_point_depth = JOY_MIDPOINT * 0.65;
};

