/*
Copyright (C) 2024-2025 Ken Monteith.

This file is part of Bonus FFB.

Bonus FFB is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or any later version.

Bonus FFB is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Bonus FFB. If not, see <https://www.gnu.org/licenses/>.
*/

#include "BonusFFBApplication.h"
#include <QDate>
#include <QDesktopServices>
#include <QFile>
#include <QMessageBox>
#include <QSettings>
#include <QUrl>
#include <QComboBox>
#include <QProgressBar>

BonusFFBApplication::BonusFFBApplication(QWidget* parent)
    : QMainWindow(parent)
{
    qDebug("BonusFFBApplication constructor invoked");
    deviceSettings = new DeviceSettings(nullptr);
    qDebug() << deviceSettings;
    qDebug() << deviceSettings->children();

    // Joystick connections
    QObject::connect(deviceSettings->joystickDeviceComboBox, &QComboBox::currentIndexChanged, this, &BonusFFBApplication::changeJoystickDevice);
    QObject::connect(deviceSettings->joystickLRAxisComboBox, &QComboBox::currentIndexChanged, this, &BonusFFBApplication::changeJoystickLRAxis);
    QObject::connect(deviceSettings->joystickFBAxisComboBox, &QComboBox::currentIndexChanged, this, &BonusFFBApplication::changeJoystickFBAxis);
    QObject::connect(this, &BonusFFBApplication::joystickLRValueChanged, deviceSettings->findChild<QProgressBar*>("ioTabJoystickLRProgressBar"), &QProgressBar::setValue);
    QObject::connect(this, &BonusFFBApplication::joystickFBValueChanged, deviceSettings->findChild<QProgressBar*>("ioTabJoystickFBProgressBar"), &QProgressBar::setValue);
    // Pedals connections
    QObject::connect(deviceSettings->pedalsDeviceComboBox, &QComboBox::currentIndexChanged, this, &BonusFFBApplication::changePedalsDevice);
    QObject::connect(deviceSettings->clutchAxisComboBox, &QComboBox::currentIndexChanged, this, &BonusFFBApplication::changeClutchAxis);
    QObject::connect(deviceSettings->throttleAxisComboBox, &QComboBox::currentIndexChanged, this, &BonusFFBApplication::changeThrottleAxis);
    QObject::connect(this, &BonusFFBApplication::clutchValueChanged, deviceSettings->findChild<QProgressBar*>("ioTabClutchProgressBar"), &QProgressBar::setValue);
    QObject::connect(this, &BonusFFBApplication::throttleValueChanged, deviceSettings->findChild<QProgressBar*>("ioTabThrottleProgressBar"), &QProgressBar::setValue);
    // vJoy connections
    QObject::connect(deviceSettings->vjoyDeviceComboBox, &QComboBox::currentIndexChanged, &vjoy, &vJoyFeeder::setDeviceIndex);

    // Populate the device lists
    for (auto const device : deviceList)
    {
        deviceSettings->pedalsDeviceComboBox->addItem(device.name, device.instanceGuid);
        if (device.supportsFfb && device.productGuid.data1 != VJOY_PRODUCT_GUID) {
            deviceSettings->joystickDeviceComboBox->addItem(device.name, device.instanceGuid);
        }
    }
    for (int i = 0; i < vJoyFeeder::deviceCount(); i++) {
        deviceSettings->vjoyDeviceComboBox->addItem(QString("vJoy Device ").append(QString(" %1").arg(i + 1)), i + 1);
    }
}

BonusFFBApplication::~BonusFFBApplication()
{
}


void BonusFFBApplication::saveDeviceSettings() {
    QSettings settings = QSettings(this->deviceSettingsFile, QSettings::IniFormat);
    settings.beginGroup("joystick");
    settings.setValue("device_guid", joystick->instanceGuid.toString());
    settings.setValue("lr_axis", joystickLRAxisGuid.toString());
    //settings.setValue("invert_lr_axis", ui.invertJoystickLRAxisBox->isChecked());
    settings.setValue("fb_axis", joystickFBAxisGuid.toString());
    //settings.setValue("invert_fb_axis", ui.invertJoystickFBAxisBox->isChecked());
    settings.endGroup();

    settings.beginGroup("pedals");
    settings.setValue("device_guid", pedals->instanceGuid.toString());
    settings.setValue("clutch_axis", clutchAxisGuid.toString());
    //settings.setValue("invert_clutch_axis", ui.invertClutchAxisBox->isChecked());
    settings.setValue("throttle_axis", throttleAxisGuid.toString());
    //settings.setValue("invert_throttle_axis", ui.invertThrottleAxisBox->isChecked());
    settings.endGroup();

    settings.beginGroup("vjoy");
    settings.setValue("vjoy_device", vjoy.getDeviceIndex());
    settings.endGroup();
}

void BonusFFBApplication::loadDeviceSettings() {
    qDebug() << "Loading settings";
    if (!QFile(deviceSettingsFile).exists()) {
        qDebug() << "Settings file does not exist";
        return;
    }
    QSettings settings = QSettings(this->deviceSettingsFile, QSettings::IniFormat);

    settings.beginGroup("joystick");
    int joystick_index = deviceSettings->joystickDeviceComboBox->findData(settings.value("device_guid").toUuid());
    if (joystick_index == -1) {
        QMessageBox::warning(this, "Joystick not found", "Saved joystick device is not connected.\nReconnect the device or update the input/output settings.");
    }
    else
    {
        deviceSettings->joystickDeviceComboBox->setCurrentIndex(joystick_index);
        deviceSettings->joystickLRAxisComboBox->setCurrentIndex(deviceSettings->joystickLRAxisComboBox->findData(settings.value("lr_axis").toUuid()));
        //ui.invertJoystickLRAxisBox->setChecked(settings.value("invert_lr_axis").toBool());
        deviceSettings->joystickFBAxisComboBox->setCurrentIndex(deviceSettings->joystickFBAxisComboBox->findData(settings.value("fb_axis").toUuid()));
        //ui.invertJoystickFBAxisBox->setChecked(settings.value("invert_fb_axis").toBool());
    }
    settings.endGroup();

    settings.beginGroup("pedals");
    int pedals_index = deviceSettings->pedalsDeviceComboBox->findData(settings.value("device_guid").toUuid());
    if (pedals_index == -1) {
        QMessageBox::warning(this, "Pedals not found", "Saved pedals device is not connected.\nReconnect the device or update the input/output settings.");
    }
    else
    {
        deviceSettings->pedalsDeviceComboBox->setCurrentIndex(pedals_index);
        deviceSettings->clutchAxisComboBox->setCurrentIndex(deviceSettings->clutchAxisComboBox->findData(settings.value("clutch_axis").toUuid()));
        //ui.invertClutchAxisBox->setChecked(settings.value("invert_clutch_axis").toBool());
        deviceSettings->throttleAxisComboBox->setCurrentIndex(deviceSettings->throttleAxisComboBox->findData(settings.value("throttle_axis").toUuid()));
        //ui.invertThrottleAxisBox->setChecked(settings.value("invert_throttle_axis").toBool());
    }
    settings.endGroup();

    settings.beginGroup("vjoy");
    deviceSettings->vjoyDeviceComboBox->setCurrentIndex(settings.value("vjoy_device").toInt());
    settings.endGroup();
}


void BonusFFBApplication::changeJoystickDevice(int deviceIndex) {
    // Release previous joystick
    if (joystick != nullptr) {
        joystick->release();
    }
    QUuid deviceGuid = deviceSettings->joystickDeviceComboBox->currentData().toUuid();
    joystick = BonusFFB::getDeviceFromGuid(&deviceList, deviceGuid);
    qDebug() << "New joystick device: " << joystick->name;

    deviceSettings->joystickLRAxisComboBox->clear();
    deviceSettings->joystickFBAxisComboBox->clear();
    QMap<QUuid, QString> axisMap = joystick->getDeviceAxes();
    for (auto axis = axisMap.cbegin(), end = axisMap.cend(); axis != end; ++axis)
    {
        deviceSettings->joystickLRAxisComboBox->addItem(axis.value(), axis.key());
        deviceSettings->joystickFBAxisComboBox->addItem(axis.value(), axis.key());
    }
}

void BonusFFBApplication::changePedalsDevice(int deviceIndex) {
    if (pedals != nullptr) {
        pedals->release();
    }
    QUuid deviceGuid = deviceSettings->pedalsDeviceComboBox->currentData().toUuid();
    pedals = BonusFFB::getDeviceFromGuid(&deviceList, deviceGuid);
    HWND hwnd = (HWND)(winId());
    pedals->acquire(&hwnd);
    qDebug() << "New clutch device acquired: " << pedals->name;

    deviceSettings->clutchAxisComboBox->clear();
    deviceSettings->throttleAxisComboBox->clear();
    QMap<QUuid, QString> axisMap = pedals->getDeviceAxes();
    for (auto axis = axisMap.cbegin(), end = axisMap.cend(); axis != end; ++axis)
    {
        deviceSettings->clutchAxisComboBox->addItem(axis.value(), axis.key());
        deviceSettings->throttleAxisComboBox->addItem(axis.value(), axis.key());
    }
}


void BonusFFBApplication::changeJoystickLRAxis(int axisIndex) {
    joystickLRAxisGuid = deviceSettings->joystickLRAxisComboBox->currentData().toUuid();
}

void BonusFFBApplication::changeJoystickFBAxis(int axisIndex) {
    joystickFBAxisGuid = deviceSettings->joystickFBAxisComboBox->currentData().toUuid();
}

void BonusFFBApplication::changeClutchAxis(int axisIndex) {
    clutchAxisGuid = deviceSettings->clutchAxisComboBox->currentData().toUuid();
}

void BonusFFBApplication::changeThrottleAxis(int axisIndex) {
    throttleAxisGuid = deviceSettings->throttleAxisComboBox->currentData().toUuid();
}


QPair<int, int> BonusFFBApplication::getJoystickValues() {
    joystick->updateState();
    long joystickLRValue = joystick->getAxisReading(joystickLRAxisGuid);
    long joystickFBValue = joystick->getAxisReading(joystickFBAxisGuid);
    /*if (ui.invertJoystickLRAxisBox->isChecked()) {
        joystickLRValue = abs(65535 - joystickLRValue);
    }
    if (ui.invertJoystickFBAxisBox->isChecked()) {
        joystickFBValue = abs(65535 - joystickFBValue);

    }*/
    emit joystickLRValueChanged(joystickLRValue);
    emit joystickFBValueChanged(joystickFBValue);
    emit joystickValueChanged(joystickLRValue, joystickFBValue);
    return QPair<int, int>(joystickLRValue, joystickFBValue);
}

QPair<int, int> BonusFFBApplication::getPedalValues() {
    pedals->updateState();
    long clutchValue = pedals->getAxisReading(clutchAxisGuid);
    /*if (ui.invertClutchAxisBox->isChecked()) {
        clutchValue = abs(65535 - clutchValue);
    }*/
    emit clutchValueChanged(clutchValue);
    long throttleValue = pedals->getAxisReading(throttleAxisGuid);
    /*if (ui.invertThrottleAxisBox->isChecked()) {
        throttleValue = abs(65535 - throttleValue);
    }*/
    emit throttleValueChanged(throttleValue);
    QPair<int, int> pedalValues = { clutchValue , throttleValue };
    if (lastPedalValues != pedalValues) {
        emit pedalValuesChanged(clutchValue, throttleValue);
        lastPedalValues = pedalValues;
    }
    return pedalValues;
}

void BonusFFBApplication::openUserGuide() {
    QDesktopServices::openUrl(QUrl("https://kgmonteith.github.io/BonusFFB/", QUrl::TolerantMode));
}

void BonusFFBApplication::openAbout() {
    QString about = "Bonus FFB v" + version.toString() + "\n\nCopyright 2024-" + QString::number(QDate::currentDate().year()) + ", Ken Monteith. All rights reserved.";
    QMessageBox::about(this, "About Bonus FFB", about);
}
