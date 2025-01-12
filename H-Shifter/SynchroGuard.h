/*
This file is part of Bonus FFB.

Bonus FFB is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or any later version.

Bonus FFB is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Bonus FFB. If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include "BonusFFB.h"

class SynchroGuard
{
public:
	HRESULT start(BonusFFB::DeviceInfo*);
	void update(long , long , long , long, bool);

private:
	DIEFFECT springEff = {};
	LPDIRECTINPUTEFFECT lpdiSpringEff = nullptr;

	DIEFFECT rumbleEff = {};
	LPDIRECTINPUTEFFECT lpdiRumbleEff = nullptr;

	DICONDITION noSpring = { 0, 0, 0 };
	//DICONDITION testCondition = { 15000, 0, -10000, 0, 0, 15000 };	// works, i guess... but why......
	DICONDITION testCondition = { 0, 0, -10000, 0, 0, 1000 }; // also works, but seems weaker. hmm.
	// ^^^ at 0 offset 1500 deadzone, 10k torque at y=48000
	//		-1000 offset 1500 deadzone, 10k torque at y=44500
	//		0 offset, 2500 deadzone, 10k torque at y=51000
	//		0 offset, 1000 deadzone, 10k torque at y=46000
	//		-2000 offset 1000 deadzone, 10k torque at y=40500. force is too high to actuate the neutral channel. or enter the slot, really.

	DIPERIODIC rumble = { 0, 0, 0, 140000 };
	bool rumbling = false;
};

