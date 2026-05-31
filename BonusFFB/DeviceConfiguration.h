/*
Copyright(C) 2024 - 2026 Ken Monteith.

This file is part of Bonus FFB.

Bonus FFB is free software : you can redistribute it and /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or any later version.

Bonus FFB is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Bonus FFB.If not, see < https://www.gnu.org/licenses/>.
*/

#pragma once
#include "ui_InputOutputSettingsDialog.h"
#include "DeviceInfo.h"
#include "vJoyFeeder.h"

#define FLAG_DEVICES_REQUIRED	0b0000001
#define FLAG_DEVICES_THROTTLE	0b0000010
#define FLAG_DEVICES_BRAKE		0b0000100
#define FLAG_DEVICES_CLUTCH		0b0001000
#define FLAG_DEVICES_SHIFTLOCK	0b0010000

#define DEVICES_NOT_AVAILABLE 0
#define DEVICES_NOT_CONFIGURED 1
#define DEVICES_OK 2

struct PedalValues {
	long throttle;
	long brake;
	long clutch;
};

class DeviceConfiguration : public QObject
{
	Q_OBJECT;

protected:
	Ui_InputOutputSettingsDialog dialog;

public slots:
	void openConfigurationDialog();
	void updateJoystickAxisList(int);
	void updateThrottleAxisList(int);
	void updateBrakeAxisList(int);
	void updateClutchAxisList(int);
	void changeShiftLockDevice(int);
	void testEnableAcceptButton();

signals:
	void joystickValueChanged(int, int);
	void joystickLRValueChanged(int);
	void joystickFBValueChanged(int);
	void clutchValueChanged(int);
	void throttleValueChanged(int);
	void pedalValuesChanged(int, int);
	void deviceConfigurationChanged();

public:
	void initialize(HWND);
	int ready(int);
	HRESULT acquire(int);
	void release();
	void saveDeviceConfiguration();
	void loadDeviceConfiguration();
	bool isFFBDeviceInstalled();
	DeviceInfo* getDeviceFromGuid(QUuid);

	QPair<int, int> getJoystickValues();
	PedalValues getPedalValues();

	QList<DeviceInfo> deviceList;
	HWND hwnd;

	DeviceInfo* joystick = nullptr;
	QUuid joystickLRAxisGuid;
	QUuid joystickFBAxisGuid;

	DeviceInfo* throttle = nullptr;
	QUuid throttleAxisGuid;
	bool invertThrottleAxis = false;
	DeviceInfo* brake = nullptr;
	QUuid brakeAxisGuid;
	bool invertBrakeAxis = false;
	DeviceInfo* clutch = nullptr;
	QUuid clutchAxisGuid;
	bool invertClutchAxis = false;

	DeviceInfo* shiftLockDevice = nullptr;
	int shiftLockButton;

	vJoyFeeder vjoy = vJoyFeeder();
};