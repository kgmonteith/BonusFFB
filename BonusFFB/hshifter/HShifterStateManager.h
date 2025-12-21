/*
Copyright (C) 2024-2025 Ken Monteith.

This file is part of Bonus FFB.

Bonus FFB is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or any later version.

Bonus FFB is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Bonus FFB. If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once
#include <QObject>
#include "vJoyFeeder.h"
#include "Telemetry.h"
#include "DeviceInfo.h"


enum class SynchroState {
    UNKNOWN,
    ENTERING_SYNCH,
    IN_SYNCH,
    EXITING_SYNCH
};

enum class GrindingState {
    OFF,
    GRINDING_FWD,
    GRINDING_BACK
};

enum class SlotState {
    UNKNOWN,
    NEUTRAL,
    NEUTRAL_UNDER_SLOT,
    SLOT_LEFT_FWD,
    SLOT_LEFT_BACK,
    SLOT_MIDDLE_FWD,
    SLOT_MIDDLE_BACK,
    SLOT_RIGHT_FWD,
    SLOT_RIGHT_BACK
};

class HShifterStateManager: public QObject
{
    Q_OBJECT;

public:
    void update(QPair<int, int>, QPair<int, int>, QPair<int, int>);

public slots:
    void setTelemetryState(TelemetrySource);

signals:
    void slotStateChanged(SlotState);
    void buttonZoneChanged(int);
    void synchroStateChanged(SynchroState);
    void grindingStateChanged(GrindingState);

private:
    void updateSlotState(long, long);
    void updateButtonZoneState(long, long);
    void updateSynchroState(long, long, QPair<int, int>);
    void updateGrindingState(long, long);

    int buttonZoneState = 0;
    TelemetrySource telemetryState = TelemetrySource::NONE;
    SlotState slotState = SlotState::NEUTRAL;
    SynchroState synchroState = SynchroState::UNKNOWN;
    GrindingState grindingState = GrindingState::OFF;

    // AustinH18's settings
    /*
    int side_slot_width = 15000;
    int middle_slot_half_width = 15000;
    int neutral_channel_half_width = 5500;

    int button_zone_half_width = 2000;
    int button_zone_depth = 4000;
    int button_zone_depth_telemetry = JOY_MIDPOINT * 0.65;

    int in_synch_depth = JOY_MIDPOINT * 0.20;
    int finished_exiting_synch_depth = JOY_MIDPOINT * 0.30;
    int grind_point_depth = JOY_MIDPOINT * 0.65;
    */

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

