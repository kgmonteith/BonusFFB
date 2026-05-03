/*
Copyright (C) 2024-2026 Ken Monteith.

This file is part of Bonus FFB.

Bonus FFB is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or any later version.

Bonus FFB is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Bonus FFB. If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include "DeviceInfo.h"
#include "MozaCompatibility.h"

class PrndlSlotGuard: public QObject {
	Q_OBJECT

public:
	HRESULT start(DeviceInfo*);
	void updateLRSpring(long);

public slots:
	void updateSlotSpringCenter(long);
	void updateShiftLockEffectStrength(double);

private:
	DeviceInfo* device = nullptr;

	DIEFFECT keepCenteredSpringEff = {};
	DICONDITION noSpring = { 0, 0, 0 };
	DICONDITION keepLRCentered = { 0, DI_FFNOMINALMAX, DI_FFNOMINALMAX };
	DICONDITION keepCenteredSpringConditions[2] = { noSpring, noSpring };

	DIEFFECT keepInGearSpringEff = {};
	int direction = MOZA_COMPATIBILITY;		// AB9 1.1.3.4 firmware force inversion
	DICONDITION keepInGearSpring = { 0 , DI_FFNOMINALMAX * direction, DI_FFNOMINALMAX * direction };

	DIEFFECT shiftLockEff = {};
	DICONSTANTFORCE shiftLockForce = { 0 };
	bool shiftLockEngaged = true;
	long lastShiftLockForce = 0;
};
