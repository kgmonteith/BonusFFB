/*
Copyright(C) 2024 - 2026 Ken Monteith.

This file is part of Bonus FFB.

Bonus FFB is free software : you can redistribute it and /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or any later version.

Bonus FFB is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Bonus FFB.If not, see < https://www.gnu.org/licenses/>.
*/

#include <QDir>
#include <QMessageBox>
#include <QSettings>
#include <QStandardPaths>
#include "DeviceConfiguration.h"

void DeviceConfiguration::initialize(HWND _hwnd) {
    hwnd = _hwnd;
    initDirectInput(&deviceList);
    loadDeviceConfiguration();
}

int DeviceConfiguration::ready(int flags = FLAG_DEVICES_REQUIRED) {
    if (!isFFBDeviceInstalled() || !vJoyFeeder::isDriverEnabled()) {
        return DEVICES_NOT_AVAILABLE;
    }
    if (flags & FLAG_DEVICES_REQUIRED && (joystick == nullptr))
        return DEVICES_NOT_CONFIGURED;
    if (flags & FLAG_DEVICES_PEDALS && (pedals == nullptr))
        return DEVICES_NOT_CONFIGURED;
    return DEVICES_OK;
}

HRESULT DeviceConfiguration::acquire(int flags) {
    if (joystick == nullptr) {
        QMessageBox::critical(nullptr, "Error", "Joystick is not configured or are disconnected.");
        return E_FAIL;
    }
    if (!vJoyFeeder::isDriverEnabled()) {
        QMessageBox::critical(nullptr, "Error", "vJoy driver is not detected.");
        return E_FAIL;
    }
    if (flags & FLAG_DEVICES_PEDALS && pedals == nullptr) {
        QMessageBox::critical(nullptr, "Error", "Pedals are not configured or are disconnected.");
        return E_FAIL;
    }

    // Acquire joystick
    qDebug() << "Acquiring joystick...";
    HRESULT hr = joystick->acquire(&hwnd, true);
    if (FAILED(hr)) {
        QMessageBox::critical(nullptr, "Error", "Could not acquire exclusive use of FFB joystick. Please close other games or applications and try again.");
        return hr;
    };

    // Acquire vJoy for feeding
    qDebug() << "Acquiring vJoy...";
    if (!vjoy.acquire()) {
        QMessageBox::critical(nullptr, "Error", "Could not acquire vJoy device. Only one program may feed each vJoy device, please close other games or applications and try again.");
        return E_FAIL;
    }

    if (flags & FLAG_DEVICES_PEDALS) {
        qDebug() << "Acquiring pedals...";
        pedals->acquire(&hwnd);
        if (FAILED(hr)) {
            QMessageBox::critical(nullptr, "Error", "Could not acquire pedals device. Please ensure the pedals are connected and configured.");
            return hr;
        };
    }

    if (flags & FLAG_DEVICES_SHIFTLOCK && shiftLockDevice != nullptr) {
        qDebug() << "Acquiring shift lock device...";
        shiftLockDevice->acquire(&hwnd);
        if (FAILED(hr)) {
            QMessageBox::critical(nullptr, "Error", "Could not acquire shift lock device. Please ensure the devices is connected and configured.");
            return hr;
        };
    }
    return S_OK;
}

void DeviceConfiguration::release() {
    qDebug() << "Releasing vJoy...";
    vjoy.release();
    qDebug() << "Releasing joystick...";
    joystick->release();
    if (pedals != nullptr && pedals->isAcquired) {
        qDebug() << "Releasing pedals...";
        pedals->release();
    }
    if (shiftLockDevice != nullptr && shiftLockDevice->isAcquired) {
        qDebug() << "Releasing shift lock device...";
        shiftLockDevice->release();
    }
}

void DeviceConfiguration::saveDeviceConfiguration() {
    QDir appSettingsDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
    QSettings config = QSettings(appSettingsDir.filePath("device_configuration.ini"), QSettings::IniFormat);

    config.beginGroup("joystick");
    config.setValue("device_guid", joystick->instanceGuid.toString());
    config.setValue("lr_axis", joystickLRAxisGuid.toString());
    config.setValue("fb_axis", joystickFBAxisGuid.toString());
    config.endGroup();

    config.beginGroup("vjoy");
    config.setValue("vjoy_device", vjoy.getDeviceIndex());
    config.endGroup();

    if (pedals != nullptr) {
        config.beginGroup("pedals");
        config.setValue("device_guid", pedals->instanceGuid.toString());
        config.setValue("throttle_axis", throttleAxisGuid.toString());
        config.setValue("invert_throttle_axis", invertThrottleAxis);
        config.setValue("brake_axis", brakeAxisGuid.toString());
        config.setValue("invert_brake_axis", invertBrakeAxis);
        config.setValue("clutch_axis", clutchAxisGuid.toString());
        config.setValue("invert_clutch_axis", invertClutchAxis);
        config.endGroup();
    }

    config.beginGroup("shiftlockdevice");
    if (shiftLockDevice != nullptr) {
        config.setValue("device_guid", shiftLockDevice->instanceGuid.toString());
        config.setValue("device_button", shiftLockButton);
    }
    else {
        config.setValue("device_guid", "None");
    }
    config.endGroup();
}

void DeviceConfiguration::loadDeviceConfiguration() {
    QDir appSettingsDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
    if (!QFile::exists(appSettingsDir.filePath("device_configuration.ini"))) {
        return;
    }
    QSettings config = QSettings(appSettingsDir.filePath("device_configuration.ini"), QSettings::IniFormat);

    config.beginGroup("joystick");
    joystick = getDeviceFromGuid(config.value("device_guid").toUuid());
    joystickLRAxisGuid = config.value("lr_axis").toUuid();
    joystickFBAxisGuid = config.value("fb_axis").toUuid();
    if (joystick == nullptr) {
        QMessageBox::warning(nullptr, "Joystick not found", "Saved joystick device is not connected.\nReconnect the device or update the input/output device configuration and restart Bonus FFB.");
    }
    config.endGroup();

    config.beginGroup("vjoy");
    vjoy.setDeviceIndex(config.value("vjoy_device").toInt());
    config.endGroup();

    if (config.childGroups().contains("pedals")) {
        config.beginGroup("pedals");
        pedals = getDeviceFromGuid(config.value("device_guid").toUuid());
        throttleAxisGuid = config.value("throttle_axis").toUuid();
        invertThrottleAxis = config.value("invert_throttle_axis").toBool();
        brakeAxisGuid = config.value("brake_axis").toUuid();
        invertBrakeAxis = config.value("invert_brake_axis").toBool();
        clutchAxisGuid = config.value("clutch_axis").toUuid();
        invertClutchAxis = config.value("invert_clutch_axis").toBool();
        if (pedals == nullptr) {
            QMessageBox::warning(nullptr, "Pedals not found", "Saved pedals device is not connected.\nReconnect the device or update the input/output device configuration.");
        }
        config.endGroup();
    }

    config.beginGroup("shiftlockdevice");
    if (config.value("device_guid").toString() != "None") {
        shiftLockDevice = getDeviceFromGuid(config.value("device_guid").toUuid());
        shiftLockButton = config.value("device_button").toInt();
        if (shiftLockDevice == nullptr) {
            QMessageBox::warning(nullptr, "Shift lock device not found", "Saved shift lock device is not connected.\nReconnect the device or update the input/output config.");
        }
    }
    config.endGroup();
}

bool DeviceConfiguration::isFFBDeviceInstalled() {
    for (const DeviceInfo device : deviceList)
    {
        if (device.supportsFfb && device.productGuid.data1 != VJOY_PRODUCT_GUID) {
            return true;
        }
    }
    return false;
}

void DeviceConfiguration::openConfigurationDialog() {
    QDialog d;
    dialog.setupUi(&d);

    connect(dialog.joystickDeviceComboBox, &QComboBox::currentIndexChanged, this, &DeviceConfiguration::updateJoystickAxisList);
    connect(dialog.joystickDeviceComboBox, &QComboBox::currentIndexChanged, this, &DeviceConfiguration::testEnableAcceptButton);
    connect(dialog.pedalsDeviceComboBox, &QComboBox::currentIndexChanged, this, &DeviceConfiguration::updatePedalsAxisList);
    connect(dialog.pedalsDeviceComboBox, &QComboBox::currentIndexChanged, this, &DeviceConfiguration::testEnableAcceptButton);
    connect(dialog.shiftLockDeviceComboBox, &QComboBox::currentIndexChanged, this, &DeviceConfiguration::changeShiftLockDevice);
    connect(dialog.vjoyDeviceComboBox, &QComboBox::currentIndexChanged, this, &DeviceConfiguration::testEnableAcceptButton);

    // Populate the device lists
    for (auto device : deviceList)
    {
        dialog.pedalsDeviceComboBox->addItem(device.name, device.instanceGuid);
        if (device.supportsFfb && device.productGuid.data1 != VJOY_PRODUCT_GUID) {
            dialog.joystickDeviceComboBox->addItem(device.name, device.instanceGuid);
        }
        if (device.buttonCount > 0) {
            dialog.shiftLockDeviceComboBox->addItem(device.name, device.instanceGuid);
        }
    }

    // Populate vJoy combo boxes
    if (vJoyFeeder::isDriverEnabled()) {
        for (int i = 0; i < vJoyFeeder::deviceCount(); i++) {
            dialog.vjoyDeviceComboBox->addItem(QString("vJoy Device ").append(QString(" %1").arg(i + 1)), i + 1);
        }
    }

    // Set the combobox indices
    if (joystick != nullptr) {
        dialog.joystickDeviceComboBox->setCurrentIndex(dialog.joystickDeviceComboBox->findData(joystick->instanceGuid));
        dialog.joystickLRAxisComboBox->setCurrentIndex(dialog.joystickLRAxisComboBox->findData(joystickLRAxisGuid));
        dialog.joystickFBAxisComboBox->setCurrentIndex(dialog.joystickFBAxisComboBox->findData(joystickFBAxisGuid));
    }
    if(pedals != nullptr) {
        dialog.pedalsDeviceComboBox->setCurrentIndex(dialog.pedalsDeviceComboBox->findData(pedals->instanceGuid));
        dialog.throttleAxisComboBox->setCurrentIndex(dialog.throttleAxisComboBox->findData(throttleAxisGuid));
        dialog.brakeAxisComboBox->setCurrentIndex(dialog.brakeAxisComboBox->findData(brakeAxisGuid));
        dialog.clutchAxisComboBox->setCurrentIndex(dialog.clutchAxisComboBox->findData(clutchAxisGuid));
        dialog.invertThrottleAxisBox->setChecked(invertThrottleAxis);
        dialog.invertBrakeAxisBox->setChecked(invertBrakeAxis);
        dialog.invertClutchAxisBox->setChecked(invertClutchAxis);
    }
    dialog.vjoyDeviceComboBox->setCurrentIndex(vjoy.getDeviceIndex());
    if (shiftLockDevice != nullptr) {
        dialog.shiftLockDeviceComboBox->setCurrentIndex(dialog.shiftLockDeviceComboBox->findData(shiftLockDevice->instanceGuid));
        dialog.shiftLockButtonComboBox->setCurrentIndex(shiftLockButton);
    }

    // Disable accept button if devices need to be selected
    testEnableAcceptButton();

    // Apply button was pressed
    if (d.exec() == QDialog::Accepted) {
        joystick = getDeviceFromGuid(dialog.joystickDeviceComboBox->currentData().toUuid());
        joystickLRAxisGuid = dialog.joystickLRAxisComboBox->currentData().toUuid();
        joystickFBAxisGuid = dialog.joystickFBAxisComboBox->currentData().toUuid();
        qDebug() << "New joystick: " << joystick->name;

        pedals = getDeviceFromGuid(dialog.pedalsDeviceComboBox->currentData().toUuid());
        throttleAxisGuid = dialog.throttleAxisComboBox->currentData().toUuid();
        brakeAxisGuid = dialog.brakeAxisComboBox->currentData().toUuid();
        clutchAxisGuid = dialog.clutchAxisComboBox->currentData().toUuid();
        invertThrottleAxis = dialog.invertThrottleAxisBox->isChecked();
        invertBrakeAxis = dialog.invertBrakeAxisBox->isChecked();
        invertClutchAxis = dialog.invertClutchAxisBox->isChecked();
        qDebug() << "New pedals: " << joystick->name;

        vjoy.setDeviceIndex(dialog.vjoyDeviceComboBox->currentIndex());
        qDebug() << "New vJoy device: " << vjoy.getDeviceIndex();

        if (dialog.shiftLockDeviceComboBox->currentIndex() > 0) {
            shiftLockDevice = getDeviceFromGuid(dialog.shiftLockDeviceComboBox->currentData().toUuid());
            shiftLockButton = dialog.shiftLockButtonComboBox->currentIndex();
            qDebug() << "New shift lock device: " << shiftLockDevice->name;
        }
        else {
            shiftLockDevice = nullptr;
        }

        saveDeviceConfiguration();
    }

    emit deviceConfigurationChanged();
}

void DeviceConfiguration::testEnableAcceptButton() {
    if (dialog.joystickDeviceComboBox->currentIndex() >= 0 && dialog.vjoyDeviceComboBox->currentIndex() >= 0)
        dialog.okButton->setEnabled(true);
    else
        dialog.okButton->setEnabled(false);
}

DeviceInfo* DeviceConfiguration::getDeviceFromGuid(QUuid guid) {
    for (QList<DeviceInfo>::iterator it = deviceList.begin(); it != deviceList.end(); it++)
    {
        if (it->instanceGuid == guid) {
            return &*it;
        }
    }
    qDebug() << "Device not found: " << guid;
    return nullptr;
}


void DeviceConfiguration::updateJoystickAxisList(int deviceIndex) {
    QUuid deviceGuid = dialog.joystickDeviceComboBox->currentData().toUuid();
    auto selectedJoystick = getDeviceFromGuid(deviceGuid);
    dialog.joystickLRAxisComboBox->clear();
    dialog.joystickFBAxisComboBox->clear();
    QMap<QUuid, QString> axisMap = selectedJoystick->getDeviceAxes();
    for (auto axis = axisMap.cbegin(), end = axisMap.cend(); axis != end; ++axis)
    {
        dialog.joystickLRAxisComboBox->addItem(axis.value(), axis.key());
        dialog.joystickFBAxisComboBox->addItem(axis.value(), axis.key());
    }
}

QPair<int, int> DeviceConfiguration::getJoystickValues() {
    joystick->updateState();
    long joystickLRValue = joystick->getAxisReading(joystickLRAxisGuid);
    long joystickFBValue = joystick->getAxisReading(joystickFBAxisGuid);
    emit joystickLRValueChanged(joystickLRValue);
    emit joystickFBValueChanged(joystickFBValue);
    emit joystickValueChanged(joystickLRValue, joystickFBValue);
    return QPair<int, int>(joystickLRValue, joystickFBValue);
}

void DeviceConfiguration::updatePedalsAxisList(int deviceIndex) {
    QUuid deviceGuid = dialog.pedalsDeviceComboBox->currentData().toUuid();
    auto selectedPedals = getDeviceFromGuid(deviceGuid);

    dialog.throttleAxisComboBox->clear();
    dialog.brakeAxisComboBox->clear();
    dialog.clutchAxisComboBox->clear();
    QMap<QUuid, QString> axisMap = selectedPedals->getDeviceAxes();
    for (auto axis = axisMap.cbegin(), end = axisMap.cend(); axis != end; ++axis)
    {
        dialog.throttleAxisComboBox->addItem(axis.value(), axis.key());
        dialog.brakeAxisComboBox->addItem(axis.value(), axis.key());
        dialog.clutchAxisComboBox->addItem(axis.value(), axis.key());
    }
}

PedalValues DeviceConfiguration::getPedalValues() {
    pedals->updateState();
    long clutchValue = pedals->getAxisReading(clutchAxisGuid);
    if (invertClutchAxis) {
        clutchValue = abs(65535 - clutchValue);
    }
    emit clutchValueChanged(clutchValue);
    long throttleValue = pedals->getAxisReading(throttleAxisGuid);
    if (invertThrottleAxis) {
        throttleValue = abs(65535 - throttleValue);
    }
    emit throttleValueChanged(throttleValue);
    emit pedalValuesChanged(clutchValue, throttleValue);

    long brakeValue = pedals->getAxisReading(brakeAxisGuid);
    return { throttleValue, brakeValue, clutchValue };
}

void DeviceConfiguration::changeShiftLockDevice(int deviceIndex) {
    if (deviceIndex > 0) {
        QVariant data = dialog.shiftLockDeviceComboBox->currentData();
        QUuid deviceGuid = dialog.shiftLockDeviceComboBox->currentData().toUuid();
        shiftLockDevice = getDeviceFromGuid(deviceGuid);
        dialog.shiftLockButtonComboBox->setEnabled(true);
        dialog.shiftLockButtonComboBox->clear();
        //ui->prndl_shiftLockButtonMonitorLabel->setText("⭕");
        for (int i = 1; i <= shiftLockDevice->buttonCount; i++) {
            dialog.shiftLockButtonComboBox->addItem(QString::number(i));
        }
    }
    else {
        shiftLockDevice = nullptr;
        dialog.shiftLockButtonComboBox->clear();
        dialog.shiftLockButtonComboBox->setEnabled(false);
        //ui->prndl_shiftLockButtonMonitorLabel->setText("✖️");
    }
}