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
#define FLAG_DEVICES_RANGE			0b00010000000
#define FLAG_DEVICES_SPLITTER		0b00100000000

#define DEVICES_NOT_AVAILABLE 0
#define DEVICES_NOT_CONFIGURED 1
#define DEVICES_OK 2

struct JoystickValues {
	long lr;
	long fb;
};

struct PedalValues {
	long throttle;
	long brake;
	long clutch;
};

struct RangeSplitterValues {
	bool range;
	bool splitter;
};

struct AxisBinding {
	QUuid deviceUuid;
	QUuid axisUuid;
};

struct ButtonBinding {
	QUuid deviceUuid;
	int button;
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
	void changeRangeDevice(int);
	void changeSplitterDevice(int);
	void changeShiftLockDevice(int);
	void testEnableAcceptButton();

signals:
	void joystickValueChanged(int, int);
	void joystickLRValueChanged(int);
	void joystickFBValueChanged(int);
	void clutchValueChanged(int);
	void throttleValueChanged(int);
	void rangeChanged(bool);
	void splitterChanged(bool);
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
	ButtonBinding bindButton();
	void updateAxisComboBoxes(int, AxisBinding);
	void updateButtonComboBoxes(int, ButtonBinding);

	QPair<int, int> getJoystickValues();
	JoystickValues getJoystickValues2();
	PedalValues getPedalValues();
	RangeSplitterValues getRangeSplitterValues();

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

	DeviceInfo* range = nullptr;
	int rangeButton;
	DeviceInfo* splitter = nullptr;
	int splitterButton;

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


class BindButtonWindow : public QDialog {
	Q_OBJECT

public:
	QList<DeviceInfo>* deviceList;
	ButtonBinding selectedButton;

	QTimer detectButtonTimer;
	QMap<QUuid, QMap<int, bool>> buttonValues;

	QLabel label;
	QDialogButtonBox buttonBox = QDialogButtonBox(
		QDialogButtonBox::Ok | QDialogButtonBox::Cancel
	);

	explicit BindButtonWindow(QList<DeviceInfo>* deviceListPtr, QWidget* parent = nullptr) : QDialog(parent), deviceList(deviceListPtr) {

		setWindowTitle("Bind button");

		QVBoxLayout* layout = new QVBoxLayout(this);
		label.setText("Press a button to bind it...");
		layout->addWidget(&label);

		buttonBox.button(QDialogButtonBox::Ok)->setText("Accept");
		buttonBox.button(QDialogButtonBox::Ok)->setDisabled(true);

		connect(&buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
		connect(&buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

		layout->addWidget(&buttonBox);

		for (auto device : *deviceList) {
			device.updateState();
			for (int button = 0; button < device.buttonCount; button++) {
				buttonValues[device.instanceGuid][button] = device.isButtonPressed(button);
			}
		}

		connect(&detectButtonTimer, &QTimer::timeout, this, &BindButtonWindow::detectButton);
		detectButtonTimer.setInterval(10);
		detectButtonTimer.start();
	}

	ButtonBinding getSelectedButton() const {
		return selectedButton;
	}

private slots:
	void detectButton() {
		for (auto device : *deviceList) {
			device.updateState();
			for (int button = 0; button < device.buttonCount; button++) {
				if (buttonValues[device.instanceGuid][button] != device.isButtonPressed(button)) {
					label.setText(device.name + ", button " + QString::number(button+1));
					//qDebug() << device.instanceGuid << axisUuid;
					selectedButton = { device.instanceGuid, button };
					buttonBox.button(QDialogButtonBox::Ok)->setDisabled(false);
				}
			}
		}
	}
};
