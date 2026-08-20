/*
Copyright (C) 2024-2026 Ken Monteith.

This file is part of Bonus FFB.

Bonus FFB is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or any later version.

Bonus FFB is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Bonus FFB. If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once
#include <QObject>
#include "DeviceConfiguration.h"
#include "Telemetry.h"
#include "SlotPattern.h"


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

class HShifterStateManager: public QObject
{
    Q_OBJECT;

public:
    void start(DeviceConfiguration*, Telemetry*, SlotPattern*);
    void update();

public slots:
    void setTelemetryState(TelemetrySource);

signals:
    void slotStateChanged(SlotState);
    void buttonZoneChanged(int);
    void slotTextChanged(QString);
    void synchroStateChanged(SynchroState);
    void grindingStateChanged(GrindingState);

private:
    void updateSlotState();
    void updateButtonZoneState();
    void updateSynchroState();
    void updateGrindingState();

    DeviceConfiguration* devices = nullptr;

    Telemetry* telemetry = nullptr;
    JoystickValues joystick;

    int buttonZoneState = 0;

    SlotPattern* slotPattern = nullptr;
    const Slot* slot = nullptr;

    TelemetrySource telemetryState = TelemetrySource::NONE;
    SlotState slotState = SlotState::NEUTRAL;
    SynchroState synchroState = SynchroState::UNKNOWN;
    GrindingState grindingState = GrindingState::OFF;    
};

