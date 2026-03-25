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
	void setJoystickFBValue(long);
	void synchroStateChanged(HeavyTruckSynchroState, int);
	void grindingStateChanged(HeavyTruckGrindingState);
	void setGrindEffectShape(int);
	void updateGrindEffectRPM(float);
	void setGrindEffectIntensity(int);
	void setKeepInGearIdleIntensity(int);
	void setRumbleRPM();
	void setMaxRevMatchRPM(int);

private:
	DeviceInfo* device = nullptr;
	long fbValue = 0;

	HeavyTruckSlotState slot_state = HeavyTruckSlotState::NEUTRAL_UNDER_SLOT;
	HeavyTruckSynchroState synchroState = HeavyTruckSynchroState::ENTERING_SYNCH;
	HeavyTruckGrindingState grindingState = HeavyTruckGrindingState::OFF;
	GrindEffectBehavior grindEffectBehavior = GrindEffectBehavior::MATCH_ENGINE_RPM;
	GUID grindEffectShape = GUID_Triangle;

	int keepInGearSpringIdleCoefficient = 2200;
	int keepInGearSpringMaxCoefficient = 10000;
	float engineRPM = 0;
	float grindEffectRPM = 300;
	int grindingIntensity = 1500;
	int maxRevMatchRPM = 120;

	QTimer* rumbleUpdateTimer;
	long rumblePhase = 0;
	long rumblePeriod = 10000;

	int grindPushbackScalingRange = 5000;

	DIEFFECT keepInGearSpringEff = {};
	DIEFFECT keepInGearEff = {};
	DIEFFECT rumbleEff = {};
	DIEFFECT rumblePushbackEff = {};

	DICONDITION noSpring = { 0, 0, 0, 0 , 0 };
	DICONDITION keepInGearSpring = { 0 , 0, 0 };
	DICONSTANTFORCE keepInGearForce = { 0 };
	DIPERIODIC rumble = { 0, 0, 0, 10000 };
	DICONSTANTFORCE rumblePushback = { 0 };

	double clutchPercent = 0;
	double throttlePercent = 0;

};