/*
Copyright (C) 2024-2025 Ken Monteith.

This file is part of Bonus FFB.

Bonus FFB is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or any later version.

Bonus FFB is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Bonus FFB. If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include "version.h"
#include <QtWidgets/QMainWindow>
#include <QTimer>
#include <QVersionNumber>
#include <QStandardPaths>
#include "BonusFFB.h"
#include "vJoyFeeder.h"
#include "DeviceSettings.h"

// This parent class implements objects and functions common to individual Bonus FFB apps
class BonusFFBApplication : public QMainWindow
{
	Q_OBJECT

public:
	BonusFFBApplication(QWidget* parent = nullptr);
	~BonusFFBApplication();
	QPair<int, int> getJoystickValues();
	QPair<int, int> getPedalValues();

	DeviceSettings* deviceSettings;

	vJoyFeeder vjoy = vJoyFeeder();
	QList<BonusFFB::DeviceInfo> deviceList;

	BonusFFB::DeviceInfo* joystick = nullptr;
	QUuid joystickLRAxisGuid;
	QUuid joystickFBAxisGuid;

	BonusFFB::DeviceInfo* pedals = nullptr;
	QUuid clutchAxisGuid;
	QUuid throttleAxisGuid;

	QVersionNumber version = QVersionNumber(MAJOR_VERSION, MINOR_VERSION, PATCH_VERSION);

public slots:
	void openUserGuide();
	void openAbout();
	void saveDeviceSettings();
	void loadDeviceSettings();
	void changeJoystickDevice(int);
	void changePedalsDevice(int);
	void changeJoystickLRAxis(int);
	void changeJoystickFBAxis(int);
	void changeClutchAxis(int);
	void changeThrottleAxis(int);

signals:
	void joystickValueChanged(int, int);
	void joystickLRValueChanged(int);
	void joystickFBValueChanged(int);
	void clutchValueChanged(int);
	void throttleValueChanged(int);
	void pedalValuesChanged(int, int);

protected:
	QString deviceSettingsFile = QStandardPaths::standardLocations(QStandardPaths::AppDataLocation)[0] + "/deviceSettings.ini";

private:
	QPair<int, int> lastPedalValues = { 0, 0 };
};

