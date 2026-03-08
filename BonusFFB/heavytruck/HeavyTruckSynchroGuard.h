/*
Copyright (C) 2024-2026 Ken Monteith.

This file is part of Bonus FFB.

Bonus FFB is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or any later version.

Bonus FFB is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Bonus FFB. If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include <QObject>
#include "DeviceInfo.h"
#include "vJoyFeeder.h"
#include "HeavyTruckStateManager.h"
#include "SharedEnums.h"

class HeavyTruckSynchroGuard: public QObject
{
	Q_OBJECT

public:
	HRESULT start(DeviceInfo*);

public slots:
	void updatePedalEngagement(QPair<int, int>, QPair<int, int>);
	void synchroStateChanged(HeavyTruckSynchroState, int);
	void grindingStateChanged(HeavyTruckGrindingState, int);
	void updateEngineRPM(float);
	void updateGrindEffectRPM(float);
	void setGrindEffectIntensity(int);
	void setKeepInGearIdleIntensity(int);
	void setGrindEffectBehavior(int);

private:
	float computeGrindRPM();

	DeviceInfo* device = nullptr;

	HeavyTruckSynchroState synchroState = HeavyTruckSynchroState::ENTERING_SYNCH;
	HeavyTruckGrindingState grindingState = HeavyTruckGrindingState::OFF;
	GrindEffectBehavior grindEffectBehavior = GrindEffectBehavior::MATCH_ENGINE_RPM;

	int keepInGearSpringIdleCoefficient = 2200;
	int keepInGearSpringMaxCoefficient = 10000;
	float engineRPM = 0;
	float grindEffectRPM = 3000;
	int grindingIntensity = 1500;

	DIEFFECT keepInGearSpringEff = {};
	DIEFFECT keepInGearEff = {};
	DIEFFECT rumbleEff = {};
	DIEFFECT rumblePushbackEff = {};

	DICONDITION noSpring = { 0, 0, 0, 0 , 0 };
	DICONDITION keepInGearSpring = { 0 , 0, 0 };
	DICONSTANTFORCE keepInGearForce = { 0 };
	DIPERIODIC rumble = { 0, 0, 0, (DWORD)grindEffectRPM };
	DICONSTANTFORCE rumblePushback = { 0 };

	double clutchPercent = 0;
	double throttlePercent = 0;
};