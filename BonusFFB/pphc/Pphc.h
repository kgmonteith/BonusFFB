/*
Copyright (C) 2024-2026 Ken Monteith.

This file is part of Bonus FFB.

Bonus FFB is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or any later version.

Bonus FFB is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Bonus FFB. If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include "BonusFFBApp.h"

class Pphc : public BonusFFBApp
{
	Q_OBJECT;

public:
	QString getAppName();
	void initialize();
	void saveSettings(QSettings*);
	void loadSettings(QSettings*);
	void initializeJoystickMap();

	HRESULT startMode();
	void gameLoop();

public slots:
	void redrawJoystickMap();
	void updateJoystickCircle(int, int);
	void updateBrake(int);
	void updateThrottle(int);
	void updateSlotSpring(QPair<int, int>);

	void setBrakeSpringScaling(int);
	void setBrakeAxisDeadzone(int);
	void setBrakeAxisScaling(int);
	void setThrottleSlotDepth(int);
	void setThrottleSpringStrength(int);
	void setThrottleAxisDeadzone(int);

private:
	QGraphicsScene* scene = nullptr;
	QGraphicsRectItem* centerSlotRect;
	QGraphicsEllipseItem* joystickCircle;
	QGraphicsRectItem* deadzoneRect;

	DIEFFECT slotSpringEff = {};
	DICONDITION noSpring = { 0, 0, 0 };
	DICONDITION keepLRCentered = { 0, DI_FFNOMINALMAX, DI_FFNOMINALMAX };
	DICONDITION keepFBCentered = { 0, DI_FFNOMINALMAX, DI_FFNOMINALMAX };
	DICONDITION slotSpringConditions[2] = { keepLRCentered, noSpring };

	DIEFFECT pphcSpringEff = {};
	DICONDITION pphcSpring = { 0, -5000, -5000 };


	int throttleSpringStrength = -0.3 * FFB_MAX;
	double throttleDeadzone = 0.05;
	float throttleSlotDepth = 0.75;

	float brakeSpringScaling = 2;
	double brakeDeadzone = 0.05;
	int brakeAxisScaling = 2;
};