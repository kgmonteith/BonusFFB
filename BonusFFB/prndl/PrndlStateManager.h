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

enum class PrndlSlot {
    NONE,
    PARK,
    REVERSE,
    NEUTRAL,
    DRIVE,
    LOW
};

const int SHIFT_LOCK_OFFSET_LOW = 1500;
const int SHIFT_LOCK_OFFSET_HIGH = 7500;

class PrndlStateManager: public QObject
{
    Q_OBJECT;

public:
    PrndlStateManager();
    void update(QPair<int, int>, bool, bool);
    int getEnabledSlotCount();
    bool isParkEnabled();
    int getButtonNumberForSlot(PrndlSlot);

public slots:
    void setTelemetryState(TelemetrySource);
    void toggleUsingShiftLock(int);
    void toggleLockShiftsFromNeutralToReverse(bool);
    void toggleParkSlot(bool);
    void toggleLastSlot(bool);
    void toggleAtsTelemetryPark(bool);

signals:
    void enabledSlotCountChanged();
    void buttonZoneChanged(int);
    void buttonShortPress(int);
    void slotChanged(PrndlSlot);
    void slotSpringChanged(long);
    void updateShiftLockEffectStrength(double);

private:
    void updateButtonZoneState(long, bool, bool);
    void updateSlotState(long, bool);
    void updateShiftLockEffect(long, bool);
    double getFBValueForSlot(PrndlSlot);

    QList<PrndlSlot> enabledPrndlSlots;

    PrndlSlot buttonZoneSlot = PrndlSlot::NONE;
    PrndlSlot engagedSlotState = PrndlSlot::NONE;
    PrndlSlot lastEngagedSlotState = PrndlSlot::NONE;
    bool delay_shift_lock = false;
    TelemetrySource telemetryState = TelemetrySource::NONE;

    int buttonOffset = 8;
    int slot_half_depth = 4000;
    bool using_ats_telemetry_park = true;
    bool using_shift_lock = false;
    bool lock_shifts_from_neutral_to_reverse = true;
};