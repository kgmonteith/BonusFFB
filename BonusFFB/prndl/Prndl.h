/*
Copyright (C) 2024-2025 Ken Monteith.

This file is part of Bonus FFB.

Bonus FFB is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or any later version.

Bonus FFB is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Bonus FFB. If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include <QObject>
#include <QStandardPaths>
#include "BonusFFBApp.h"
#include "PrndlSlotGuard.h"
#include "PrndlStateManager.h"

#define SHIFTER_POSITION_MARKER_DIAMETER_PX 17.0

class Prndl : public BonusFFBApp
{
	Q_OBJECT;

public:
	void initialize();
	void saveSettings();
	void loadSettings();
	void initializeJoystickMap();

	QPair<int, int> getJoystickValues();

	HRESULT startGameLoop();
	void stopGameLoop();
	void gameLoop();

	bool getShiftLockReleased();

	DeviceInfo* joystick = nullptr;
	QUuid joystickLRAxisGuid;
	QUuid joystickFBAxisGuid;

	DeviceInfo* shiftLockDevice = nullptr;
	QUuid shiftLockButtonGuid;

public slots:
	void redrawJoystickMap();
	void changeSlotLabel(PrndlSlot slot);
	void changeJoystickDevice(int);
	void changeJoystickLRAxis(int);
	void changeJoystickFBAxis(int);
	void changeShiftLockDevice(int);
	void updateJoystickCircle(int, int);


signals:
	void joystickValueChanged(int, int);
	void joystickLRValueChanged(int);
	void joystickFBValueChanged(int);
	void shiftLockStateChanged(bool);

protected:
	QString deviceSettingsFile = QStandardPaths::standardLocations(QStandardPaths::AppDataLocation)[0] + "/prndlDeviceSettings.ini";

private:
	QGraphicsScene* scene = nullptr;
	QGraphicsRectItem* centerSlotRect;
	QList<QGraphicsEllipseItem*> slotCircles;
	QGraphicsEllipseItem* joystickCircle;


	// Stateful FFB effect managers
	PrndlSlotGuard slotGuard;
	PrndlStateManager stateManager = PrndlStateManager();

	bool lastShiftLockReleased = false;
};

