/*
Copyright(C) 2024 - 2026 Ken Monteith.

This file is part of Bonus FFB.

Bonus FFB is free software : you can redistribute it and /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or any later version.

Bonus FFB is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Bonus FFB.If not, see < https://www.gnu.org/licenses/>.
*/

#pragma once
#include <QDialogButtonBox>
#include <QTimer>
#include "ui_InputOutputSettingsDialog.h"
#include "DeviceInfo.h"
#include "vJoyFeeder.h"

#define FLAG_DEVICES_JOYSTICK_LR	0b00000000001
#define FLAG_DEVICES_JOYSTICK_FB	0b00000000010
#define FLAG_DEVICES_VJOY			0b00000000100
#define FLAG_DEVICES_REQUIRED		0b00000000111
#define FLAG_DEVICES_THROTTLE		0b00000001000
#define FLAG_DEVICES_BRAKE			0b00000010000
#define FLAG_DEVICES_CLUTCH			0b00000100000
#define FLAG_DEVICES_SHIFTLOCK		0b00001000000

#define DEVICES_NOT_AVAILABLE 0
#define DEVICES_NOT_CONFIGURED 1
#define DEVICES_OK 2

struct PedalValues {
	long throttle;
	long brake;
	long clutch;
};

struct AxisBinding {
	QUuid deviceUuid;
	QUuid axisUuid;
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
	AxisBinding bindAxis();
	void updateDeviceComboBoxes(int, AxisBinding);

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

class BindAxisWindow : public QDialog {
	Q_OBJECT

public:
	QList<DeviceInfo>* deviceList;
	AxisBinding selectedAxis;
	QTimer detectAxisTimer;
	QMap<QUuid, QMap<QUuid, long>> axisValues;

	QLabel label;
	QDialogButtonBox buttonBox = QDialogButtonBox(
		QDialogButtonBox::Ok | QDialogButtonBox::Cancel
	);

	explicit BindAxisWindow(QList<DeviceInfo>* deviceListPtr, QWidget* parent = nullptr) : QDialog(parent), deviceList(deviceListPtr)  {

		setWindowTitle("Bind axis");

		QVBoxLayout* layout = new QVBoxLayout(this);
		label.setText("Move device to bind axis...");
		layout->addWidget(&label);

		buttonBox.button(QDialogButtonBox::Ok)->setText("Accept");
		buttonBox.button(QDialogButtonBox::Ok)->setDisabled(true);

		connect(&buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
		connect(&buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

		layout->addWidget(&buttonBox);

		for (auto device : *deviceList) {
			device.updateState();
			for (auto [axisUuid, name] : device.getDeviceAxes().asKeyValueRange()) {
				axisValues[device.instanceGuid][axisUuid] = device.getAxisReading(axisUuid);
			}
		}

		connect(&detectAxisTimer, &QTimer::timeout, this, &BindAxisWindow::detectAxis);
		detectAxisTimer.setInterval(50);
		detectAxisTimer.start();
	}

	AxisBinding getSelectedAxis() const {
		return selectedAxis;
	}

private slots:
	void detectAxis() {
		for (auto device : *deviceList) {
			device.updateState();
			for (auto [axisUuid, name] : device.getDeviceAxes().asKeyValueRange()) {
				long difference = std::abs(axisValues[device.instanceGuid][axisUuid] - device.getAxisReading(axisUuid));
				if (difference >= JOY_MAXPOINT * 0.25) {
					 label.setText(device.name + ", " + name);
					 //qDebug() << device.instanceGuid << axisUuid;
					 selectedAxis = { device.instanceGuid, axisUuid };
					 buttonBox.button(QDialogButtonBox::Ok)->setDisabled(false);
				}
			}
		}
	}
};
