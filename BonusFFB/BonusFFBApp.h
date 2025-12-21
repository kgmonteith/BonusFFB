/*
Copyright (C) 2024-2025 Ken Monteith.

This file is part of Bonus FFB.

Bonus FFB is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or any later version.

Bonus FFB is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Bonus FFB. If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once
#include <QObject>
#include "ui_BonusFFB.h"
#include "DeviceInfo.h"
#include "vJoyFeeder.h"
#include "Telemetry.h"

#define SLOT_WIDTH_PX 5.0
#define JOYSTICK_MARKER_DIAMETER_PX 21.0

static bool g_joystick_warned = false;

class BonusFFBApp :
    public QObject
{
	Q_OBJECT;

public:
	Ui::BonusFFBClass* ui;
	QList<DeviceInfo>* deviceList;
	vJoyFeeder* vjoy;
	HWND hwnd;
	Telemetry* telemetry;

	void setPointers(Ui::BonusFFBClass*, QList<DeviceInfo>*, vJoyFeeder*, Telemetry*, HWND);
	virtual HRESULT startGameLoop() = 0;
	virtual void stopGameLoop() = 0;
	virtual void gameLoop() = 0;
	virtual void initialize() = 0;
	virtual void initializeJoystickMap() = 0;
	virtual void saveSettings() = 0;
	virtual void loadSettings() = 0;

public slots:
	virtual void redrawJoystickMap() = 0;
};

