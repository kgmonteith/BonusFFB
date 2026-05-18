/*
Copyright (C) 2024-2026 Ken Monteith.

This file is part of Bonus FFB.

Bonus FFB is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or any later version.

Bonus FFB is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Bonus FFB. If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include <QObject>
#include <QStandardPaths>
#include "BonusFFBApp.h"

#define SHIFTER_POSITION_MARKER_DIAMETER_PX 17.0

class Handbrake : public BonusFFBApp
{
	Q_OBJECT;

public:
	QString getAppName();
	void initialize();
	void saveSettings(QSettings*);
	void loadSettings(QSettings*);
	void initializeJoystickMap();

	HRESULT startGameLoop();
	void stopGameLoop();
	void gameLoop();

public slots:
	void redrawJoystickMap();
	void updateJoystickCircle(int, int);
	void springCenterChanged(int);
	void springStrengthChanged(int);

private:
	QGraphicsScene* scene = nullptr;
	QGraphicsRectItem* centerSlotRect;
	QGraphicsEllipseItem* joystickCircle;

	int springStrength = DI_FFNOMINALMAX * -0.5;	// AB9 1.1.3.4 firmware force inversion
	int springCenter = -10000;

	DIEFFECT keepCenteredSpringEff = {};
	DICONDITION keepLRCentered = { 0, DI_FFNOMINALMAX, DI_FFNOMINALMAX };

	DIEFFECT handbrakeSpringEff = {};
	DICONDITION handbrakeSpring = { springCenter, springStrength, springStrength };
};

