/*
Copyright (C) 2024-2025 Ken Monteith.

This file is part of Bonus FFB.

Bonus FFB is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or any later version.

Bonus FFB is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Bonus FFB. If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include <QObject>
#include "DeviceInfo.h"
#include "vJoyFeeder.h"
#include "HShifterStateManager.h"

static LONG FORWARD[2] = { 0 , 0 };
static LONG BACK[2] = { 180 * DI_DEGREES , 0 };

enum class GrindEffectBehavior {
	MATCH_ENGINE_RPM,
	ADD_ENGINE_RPM,
	OVERRIDE_ENGINE_RPM
};

class HShifterSynchroGuard: public QObject
{
	Q_OBJECT

public:
	HRESULT start(DeviceInfo*);
	HRESULT createEffects();

public slots:
	void updatePedalEngagement(int, int);
	void synchroStateChanged(SynchroState);
	void grindingStateChanged(GrindingState);
	void updateEngineRPM(float);
	void updateGrindEffectRPM(float);
	void setGrindEffectIntensity(int);
	void setKeepInGearIdleIntensity(int);
	void setGrindEffectBehavior(int);

private:
	float computeGrindRPM();

	DeviceInfo* device = nullptr;

	SynchroState synchroState = SynchroState::ENTERING_SYNCH;
	GrindingState grindingState = GrindingState::OFF;
	GrindEffectBehavior grindEffectBehavior = GrindEffectBehavior::MATCH_ENGINE_RPM;

	int keepInGearSpringIdleCoefficient = 2200;
	int keepInGearSpringMaxCoefficient = 10000;
	float engineRPM = 0;
	float grindEffectRPM = 3000;
	int grindingIntensity = 1500;

	DIEFFECT unsynchronizedSpringEff = {};
	LPDIRECTINPUTEFFECT lpdiUnsynchronizedSpringEff = nullptr;

	DIEFFECT keepInGearSpringEff = {};

	DIEFFECT rumbleEff = {};

	DICONDITION noSpring = { 0, 0, 0, 0 , 0 };
	DICONDITION unsynchronizedSpring = { 0, 0, -10000, 0, 0, 1300 };
	DICONDITION keepInGearSpring = { 0 , 0, keepInGearSpringIdleCoefficient };
	DIPERIODIC rumble = { 0, 0, 0, DWORD(6e7 / engineRPM) };

	double clutchPercent = 0;
	double throttlePercent = 0;
};