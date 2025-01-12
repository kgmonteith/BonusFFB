/*
This file is part of Bonus FFB.

Bonus FFB is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or any later version.

Bonus FFB is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Bonus FFB. If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include "BonusFFB.h"

#define MINPOINT 0
#define MIDPOINT 32768
#define MAXPOINT 65536
#define SIDE_SLOT_WIDTH 500
#define MIDDLE_SLOT_HALF_WIDTH 1200
#define NEUTRAL_HALF_WIDTH 1200

static DWORD AXES[2] = { DIJOFS_X, DIJOFS_Y };
static LONG FORWARDBACK[2] = { 1 , 0 };

class SlotGuard {
public:
	enum PositionState {
		NEUTRAL,
		SLOT_LEFT,
		SLOT_MIDDLE,
		SLOT_RIGHT,
		NEUTRAL_UNDER_SLOT
	};

	HRESULT start(BonusFFB::DeviceInfo*);
	PositionState update(long, long);
	static bool is_in_neutral(long);
	static int is_under_slot(long);

private:
	DIEFFECT eff = {};
	LPDIRECTINPUTEFFECT lpdiEff = nullptr;

	DICONDITION noSpring = { 0, 0, 0 };
	DICONDITION keepFBCentered = { 0, DI_FFNOMINALMAX, DI_FFNOMINALMAX };
	DICONDITION keepLeft = { -10000, DI_FFNOMINALMAX, DI_FFNOMINALMAX };
	DICONDITION keepLRCentered = { 0, DI_FFNOMINALMAX, DI_FFNOMINALMAX };
	DICONDITION keepRight = { 10000, DI_FFNOMINALMAX, DI_FFNOMINALMAX };

	DICONDITION conditions[2] = { noSpring, noSpring };

	PositionState state;
	BonusFFB::FFBEffect slotGuardSpring;
};
