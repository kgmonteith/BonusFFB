/*
Copyright (C) 2024-2025 Ken Monteith.

This file is part of Bonus FFB.

Bonus FFB is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or any later version.

Bonus FFB is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Bonus FFB. If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include "DeviceInfo.h"
#include "HShifterStateManager.h"

enum class QUADRANT {
	NW,
	SW,
	NE,
	SE
};

enum class NEUTRAL_SHAPE {
	SQUARE,
	ROUNDED,
	ANGLED
};

class HShifterSlotGuard: public QObject {
	Q_OBJECT

public:
	HRESULT start(DeviceInfo*);

public slots:
	void updateSlotGuardEffects(QPair<int, int>);
	void updateSlotGuardState(SlotState);
	void setNeutralSpringStrength(int);

private:
	DeviceInfo* device = nullptr;
	SlotState slot_state = SlotState::NEUTRAL_UNDER_SLOT;
	NEUTRAL_SHAPE neutralShape = NEUTRAL_SHAPE::SQUARE;

	DIEFFECT slotSpringEff = {};
	DIEFFECT neutralSpringEff = {};
	long neutral_spring_strength = 1500;
	//long neutral_spring_strength = 00;
	DICONDITION neutralSpring = { 0, neutral_spring_strength, neutral_spring_strength };
	DICONDITION neutralSpringConditions[2] = { neutralSpring, neutralSpring };

	DICONDITION leftPushoutSpring = { -5000, 0, -5000 };
	DICONDITION topPushoutSpring = { -5000, 0, -5000 };

	DICONDITION noSpring = { 0, 0, 0 };
	DICONDITION keepFBCentered = { 0, DI_FFNOMINALMAX, DI_FFNOMINALMAX };
	DICONDITION keepLeft = { -10000, DI_FFNOMINALMAX, DI_FFNOMINALMAX };
	DICONDITION keepLRCentered = { 0, DI_FFNOMINALMAX, DI_FFNOMINALMAX };
	DICONDITION keepRight = { 10000, DI_FFNOMINALMAX, DI_FFNOMINALMAX };

	DICONDITION springConditions[2] = { noSpring, noSpring };

	DIEFFECT lrSlotPushEff = {};
	DIEFFECT fbSlotPushEff = {};
	DICONSTANTFORCE lrcf;
	DICONSTANTFORCE fbcf;
	DICONSTANTFORCE slotPush[2] = { lrcf, fbcf };

	DIEFFECT safetyDamper = {};
	DIEFFECT safetyFriction = {};
	DICONDITION unsafe = { 0, 0, 0 };
	DICONDITION safe = { 0, 10000, 10000 };
	DICONDITION halfsafe = { 0, 4000, 4000 };
	DICONDITION safe2d[2] = { safe, halfsafe };
	DICONDITION unsafe2d[2] = { unsafe, unsafe };
};
