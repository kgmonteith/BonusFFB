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
#include "HShifterStateManager.h"
#include "HShifterSlotGuard.h"
#include "HShifterSynchroGuard.h"

class HShifter : public BonusFFBApp
{
	Q_OBJECT;

public:
	void initialize();
	void saveSettings();
	void loadSettings();
	void initializeJoystickMap();
	HRESULT startGameLoop();
	void stopGameLoop();
	void gameLoop();

	QPair<int, int> getJoystickValues();
	QPair<int, int> getPedalValues();

	DeviceInfo* joystick = nullptr;
	QUuid joystickLRAxisGuid;
	QUuid joystickFBAxisGuid;

	DeviceInfo* pedals = nullptr;
	QUuid clutchAxisGuid;
	QUuid throttleAxisGuid;

public slots:
	void changeJoystickDevice(int);
	void changePedalsDevice(int);
	void changeJoystickLRAxis(int);
	void changeJoystickFBAxis(int);
	void changeClutchAxis(int);
	void changeThrottleAxis(int);

	void redrawJoystickMap();
	void updateJoystickCircle(int, int);
	void updateGearText(int);
	void showAxisProgressBars();
	void hideAxisProgressBars();

signals:
	void joystickValueChanged(int, int);
	void joystickLRValueChanged(int);
	void joystickFBValueChanged(int);
	void clutchValueChanged(int);
	void throttleValueChanged(int);
	void pedalValuesChanged(int, int);
	void gearValuesChanged(QPair<int, int>);
	void engineRPMChanged(float);
	void resetClutchAxes();

protected:
	QString deviceSettingsFile = QStandardPaths::standardLocations(QStandardPaths::AppDataLocation)[0] + "/hshifterDeviceSettings.ini";

private:
	QGraphicsScene* scene = nullptr;
	QGraphicsRectItem* neutralChannelRect;
	QGraphicsRectItem* centerSlotRect;
	QGraphicsRectItem* rightSlotRect;
	QGraphicsRectItem* leftSlotRect;
	QGraphicsEllipseItem* joystickCircle;

	// Stateful FFB effect managers
	HShifterStateManager stateManager;
	HShifterSlotGuard slotGuard;
	HShifterSynchroGuard synchroGuard;

	QPair<int, int> lastGearValues = { 0, 0 };
	QPair<int, int> lastPedalValues = { 0, 0 };
	float lastEngineRPM = 0.0;
};

