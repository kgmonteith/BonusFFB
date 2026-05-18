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

#define FLAG_DEVICES_REQUIRED	0x0001
#define FLAG_DEVICES_PEDALS		0x0010
#define FLAG_DEVICES_SHIFTLOCK	0x0100

#define DEVICES_NOT_AVAILABLE 0
#define DEVICES_NOT_CONFIGURED 1
#define DEVICES_OK 2

class DeviceConfiguration : public QObject
{
	Q_OBJECT;

protected:
	Ui_InputOutputSettingsDialog dialog;

public slots:
	void openConfigurationDialog();
	void updateJoystickAxisList(int);
	void updatePedalsAxisList(int);
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
	QPair<int, int> getPedalValues();

	QList<DeviceInfo> deviceList;
	HWND hwnd;

	DeviceInfo* joystick = nullptr;
	QUuid joystickLRAxisGuid;
	QUuid joystickFBAxisGuid;

	DeviceInfo* pedals = nullptr;
	QUuid clutchAxisGuid;
	bool invertClutchAxis = false;
	QUuid throttleAxisGuid;
	bool invertThrottleAxis = false;

	DeviceInfo* shiftLockDevice = nullptr;
	int shiftLockButton;

	vJoyFeeder vjoy = vJoyFeeder();
};