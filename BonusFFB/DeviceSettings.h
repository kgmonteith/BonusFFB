#pragma once

#include <QWidget>
#include <QComboBox>
#include "ui_DeviceSettings.h"

class DeviceSettings : public QWidget
{
	Q_OBJECT

public:
	DeviceSettings(QWidget *parent = nullptr);
	~DeviceSettings();

	void showAxisProgressBars();
	void hideAxisProgressBars();

	QComboBox* joystickDeviceComboBox = nullptr;
	QComboBox* joystickLRAxisComboBox = nullptr;
	QComboBox* joystickFBAxisComboBox = nullptr;
	QComboBox* pedalsDeviceComboBox = nullptr;
	QComboBox* clutchAxisComboBox = nullptr;
	QComboBox* throttleAxisComboBox = nullptr;
	QComboBox* vjoyDeviceComboBox = nullptr;
private:
	Ui::DeviceSettingsClass ui;
};
