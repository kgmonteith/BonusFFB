/*
This file is part of Bonus FFB.

Bonus FFB is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or any later version.

Bonus FFB is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Bonus FFB. If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include <QObject>
#include "BonusFFB.h"
#include "vJoyFeeder.h"

static LONG FORWARD[2] = { 0 , 0 };
static LONG BACK[2] = { 180 * DI_DEGREES , 0 };

class SynchroGuard: public QObject
{
	Q_OBJECT

public:
	enum SynchroState {
		UNKNOWN,
		ENTERING_SYNCH,
		IN_SYNCH,
		EXITING_SYNCH
	};

	HRESULT start(BonusFFB::DeviceInfo*);
public slots:
	void updatePedalEngagement(int, int);
	//void updateUnsynchRumble(int, int);
	void synchroStateChanged(SynchroState newState);
	void grindingStateChanged(bool);
	void updateEngineRPM(float);

private:
	int scaledUnsyncCoeff();
	int RPMtoMicrosendsPerCycle(float);

	SynchroState synchroState = SynchroState::ENTERING_SYNCH;

	DIEFFECT unsynchronizedSpringEff = {};
	LPDIRECTINPUTEFFECT lpdiUnsynchronizedSpringEff = nullptr;

	DIEFFECT unsynchronizedConstantEff = {};
	LPDIRECTINPUTEFFECT lpdiUnsynchronizedConstantEff = nullptr;

	DIEFFECT keepInGearSpringEff = {};
	LPDIRECTINPUTEFFECT lpdiKeepInGearSpringEff = nullptr;

	DIEFFECT rumbleEff = {};
	LPDIRECTINPUTEFFECT lpdiRumbleEff = nullptr;

	int keepInGearSpringIdleCoefficient = 2200;
	int keepInGearSpringMaxCoefficient = 10000;

	DICONDITION noSpring = { 0, 0, 0, 0 , 0 };
	//DICONDITION testCondition = { 15000, 0, -10000, 0, 0, 15000 };	// works, i guess... but why......
	DICONDITION unsynchronizedSpring = { 0, 0, -10000, 0, 0, 1500 }; // also works, but seems weaker. hmm.
	DICONDITION keepInGearSpring = { 0 , 0, keepInGearSpringMaxCoefficient };
	DICONSTANTFORCE unsynchronizedConstant = { 10000 }; // also works, but seems weaker. hmm.
	// ^^^ at 0 offset 1500 deadzone, 10k torque at y=48000
	//		-1000 offset 1500 deadzone, 10k torque at y=44500
	//		0 offset, 2500 deadzone, 10k torque at y=51000
	//		0 offset, 1000 deadzone, 10k torque at y=46000
	//		-2000 offset 1000 deadzone, 10k torque at y=40500. force is too high to actuate the neutral channel. or enter the slot, really.

	float engineRPM = 500;
	int rumbleIntensity = 1000;
	DIPERIODIC rumble = { 0, 0, 0, 140000 };

	double clutchPercent = 0;
	double throttlePercent = 0;

	bool grinding = false;
};

