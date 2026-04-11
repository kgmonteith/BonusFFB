/*
Copyright (C) 2024-2026 Ken Monteith.

This file is part of Bonus FFB.

Bonus FFB is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or any later version.

Bonus FFB is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Bonus FFB. If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include "DeviceInfo.h"
#include "HeavyTruckStateManager.h"
#include "SharedEnums.h"

class HeavyTruckSlotGuard: public QObject {
	Q_OBJECT

public:
	HRESULT start(DeviceInfo*, SlotParameters*);

public slots:
	void updateSlotGuardEffects(QPair<int, int>);
	void updateSlotGuardState(HeavyTruckSlotState);
	void updateDamper(int);
	void updateInertia(int);
	void updateFriction(int);
	void updateLeftSlotResistance(int);

private:
	bool isInCorner(int slot_num, QPair<int, int> joystickValues);
	QPair<long, long> getCornerStrength(int slot_num, QPair<int, int> joystickValues);
	int last_nearest_slot = 1;

	DeviceInfo* device = nullptr;
	SlotParameters* slot = nullptr;
	HeavyTruckSlotState slot_state = HeavyTruckSlotState::NEUTRAL;

	DIEFFECT slotSpringEff = {};
	DIEFFECT leftSlotResistanceEff = {};

	DICONDITION noSpring = { 0, 0, 0 };
	DICONDITION keepFBCentered = { 0, DI_FFNOMINALMAX, DI_FFNOMINALMAX };
	DICONDITION keepLRCentered = { 0, DI_FFNOMINALMAX, DI_FFNOMINALMAX };
	DICONDITION slotSpringConditions[2] = { noSpring, noSpring };
	DICONDITION leftSlotResistance = { -10000, -5000, -5000 };
	DICONDITION leftSlotResistanceCondition = noSpring;

	long damperStrength = 3000;
	long inertiaStrength = 1000;
	long frictionStrength = 1000;
	DICONDITION damperCondition[2] = {{0, damperStrength, damperStrength}, {0, damperStrength, damperStrength}};
	DICONDITION inertiaCondition[2] = { {0, inertiaStrength, inertiaStrength}, {0, inertiaStrength, inertiaStrength} };
	DICONDITION frictionCondition[2] = { {0, frictionStrength, frictionStrength}, {0, frictionStrength, frictionStrength} };
	DIEFFECT damperEff = {};
	DIEFFECT inertiaEff = {};
	DIEFFECT frictionEff = {};
};
