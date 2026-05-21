/*
Copyright (C) 2024-
Ken Monteith.

This file is part of Bonus FFB.

Bonus FFB is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or any later version.

Bonus FFB is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Bonus FFB. If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once
#include <QObject>
#include <QSettings>
#include "ui_BonusFFB.h"
#include "DeviceConfiguration.h"
#include "Telemetry.h"

#define SLOT_WIDTH_PX 5.0
#define JOYSTICK_MARKER_DIAMETER_PX 21.0

class BonusFFBApp :
    public QObject
{
	Q_OBJECT;

public:
	long damperStrength = FFB_MAX;
	long inertiaStrength = FFB_MAX;
	long frictionStrength = FFB_MAX;
	DICONDITION damperCondition[2] = { {0, damperStrength, damperStrength}, {0, damperStrength, damperStrength} };
	DICONDITION inertiaCondition[2] = { {0, inertiaStrength, inertiaStrength}, {0, inertiaStrength, inertiaStrength} };
	DICONDITION frictionCondition[2] = { {0, frictionStrength, frictionStrength}, {0, frictionStrength, frictionStrength} };
	DIEFFECT damperEff = {};
	DIEFFECT inertiaEff = {};
	DIEFFECT frictionEff = {};

	Ui::BonusFFBClass* ui;
	DeviceConfiguration* devices;
	Telemetry* telemetry;
	int appDeviceFlags = FLAG_DEVICES_REQUIRED;

	void setPointers(Ui::BonusFFBClass*, DeviceConfiguration*, Telemetry*);
	HRESULT start();
	void stop();
	virtual void gameLoop() = 0;
	virtual void initialize() = 0;
	virtual void initializeJoystickMap() = 0;
	virtual void saveSettings(QSettings*);
	virtual void loadSettings(QSettings*);
	virtual QString getAppName() = 0;

protected:
	virtual HRESULT startMode() = 0;

public slots:
	virtual void redrawJoystickMap() = 0;
	void updateDamper(int);
	void updateInertia(int);
	void updateFriction(int);
};

