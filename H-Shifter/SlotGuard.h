/*
This file is part of Bonus FFB.

Bonus FFB is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or any later version.

Bonus FFB is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Bonus FFB. If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include "BonusFFB.h"

static DWORD AXES[2] = { DIJOFS_X, DIJOFS_Y };
static LONG FORWARDBACK[2] = { 1 , 0 };

class SlotGuard: public QObject {
	Q_OBJECT

public:
	enum SlotState {
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

	HRESULT start(BonusFFB::DeviceInfo*);

public slots:
	void updateSlotGuardEffects(SlotState);

private:
	DIEFFECT eff = {};
	LPDIRECTINPUTEFFECT lpdiEff = nullptr;

	DICONDITION noSpring = { 0, 0, 0 };
	DICONDITION keepFBCentered = { 0, DI_FFNOMINALMAX, DI_FFNOMINALMAX };
	DICONDITION keepLeft = { -10000, DI_FFNOMINALMAX, DI_FFNOMINALMAX };
	DICONDITION keepLRCentered = { 0, DI_FFNOMINALMAX, DI_FFNOMINALMAX };
	DICONDITION keepRight = { 10000, DI_FFNOMINALMAX, DI_FFNOMINALMAX };

	DICONDITION conditions[2] = { noSpring, noSpring };

	BonusFFB::FFBEffect slotGuardSpring;
};
