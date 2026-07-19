/*
Copyright (C) 2024-2026 Ken Monteith.

This file is part of Bonus FFB.

Bonus FFB is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or any later version.

Bonus FFB is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Bonus FFB. If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once
#include "Telemetry.h"
#include "DeviceConfiguration.h"
#include "SlotPattern.h"

enum class HeavyTruckSynchroState {
    UNKNOWN,
    ENTERING_SYNCH,
    IN_SYNCH,
    EXITING_SYNCH
};

enum class HeavyTruckGrindingState {
    OFF,
    GRINDING_FWD,
    GRINDING_BACK
};

enum class HeavyTruckSlotState {
    UNKNOWN,
    NEUTRAL,
    NEUTRAL_UNDER_SLOT,
    SLOTTED
};

class HeavyTruckStateManager: public QObject
{
    Q_OBJECT;

public:
    void start(DeviceConfiguration*, Telemetry*, SlotPattern*);
    void update();

public slots:
    void setTelemetryState(TelemetrySource);

signals:
    void slotStateChanged(HeavyTruckSlotState);
    void buttonZoneChanged(int);
    void synchroStateChanged(HeavyTruckSynchroState);
    void grindingStateChanged(HeavyTruckGrindingState);
    void targetGearChanged(int);
    void rpmDeltaChanged(float);
    void unblipThrottle();
    void engineRPMChanged(float);

private:
    void updateSlotState();
    void updateButtonZoneState(QPair<int, int>);
    void updateHeavyTruckSynchroState(QPair<int, int>);
    void updateHeavyTruckGrindingState();
    void updateTargetGear();

    DeviceConfiguration* devices = nullptr;

    Telemetry* telemetry = nullptr;
    RangeSplitterValues rangeSplitter;
    JoystickValues joystick;

    int buttonZoneState = 0;
    float rpmDelta = 0;
    float rpmMaxForFloat = 120;

    float lastEngineRPM = 0;
    bool rpmIncreasing = false;

    SlotPattern* slotPattern = nullptr;
    const Slot* slot = nullptr;

    TelemetrySource telemetryState = TelemetrySource::NONE;
    HeavyTruckSlotState slotState = HeavyTruckSlotState::NEUTRAL;
    HeavyTruckSynchroState synchroState = HeavyTruckSynchroState::UNKNOWN;
    HeavyTruckGrindingState grindingState = HeavyTruckGrindingState::OFF;

    int targetGear = 0;
    
};
