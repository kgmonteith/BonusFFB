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
#include "HShifterStateManager.h"
#include "SharedEnums.h"

class HShifterSynchroGuard: public QObject
{
	Q_OBJECT

public:
	HRESULT start(DeviceConfiguration*, SlotPattern*);

public slots:
	void updateTorqueLock();
	void synchroStateChanged(SynchroState);
	void grindingStateChanged(GrindingState);
	void updateGrindEffectRPM(float);
	void setGrindEffectStrength(int);
	void setRumbleRPM();
	void setEngineVibrationStrength(int);
	void setEngineRPM(int newRPM);

private:
	DeviceConfiguration* devices = nullptr;
	SlotPattern* slotPattern = nullptr;

	SynchroState synchroState = SynchroState::ENTERING_SYNCH;
	GrindingState grindingState = GrindingState::OFF;
	GrindEffectBehavior grindEffectBehavior = GrindEffectBehavior::MATCH_ENGINE_RPM;

	float grindRPM = 3000;
	int grind_strength = 1500;

	QTimer* rumbleUpdateTimer;
	int grindPushbackScalingRange = 5000;

	DIEFFECT rumbleEff = {};
	DIEFFECT rumblePushbackEff = {};

	DIPERIODIC rumble = { 0, 0, 0, (DWORD)grindRPM };
	DICONSTANTFORCE rumblePushback = { 0 };

	float engine_vibration_strength = 500;
	float engineRPM = 2000;

	DIEFFECT engineVibrationEff = {};
	DIPERIODIC engineVibration = { (DWORD)engine_vibration_strength, 0, 0, (DWORD)(6e7 / engineRPM) };

	DIEFFECT torqueLockSpringEff = {};
	DICONDITION torqueLockSpring = { 0, 0, 0 };
};