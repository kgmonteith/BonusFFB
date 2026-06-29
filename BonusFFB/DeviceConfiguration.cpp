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
    if (flags & FLAG_DEVICES_THROTTLE && (throttle == nullptr))
        return DEVICES_NOT_CONFIGURED;
    if (flags & FLAG_DEVICES_BRAKE && (brake == nullptr))
        return DEVICES_NOT_CONFIGURED;
    if (flags & FLAG_DEVICES_CLUTCH && (clutch == nullptr))
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
    if (flags & FLAG_DEVICES_THROTTLE && throttle == nullptr) {
        QMessageBox::critical(nullptr, "Error", "Throttle is not configured or is disconnected.");
        return E_FAIL;
    }
    if (flags & FLAG_DEVICES_BRAKE && brake == nullptr) {
        QMessageBox::critical(nullptr, "Error", "Brake is not configured or is disconnected.");
        return E_FAIL;
    }
    if (flags & FLAG_DEVICES_CLUTCH && clutch == nullptr) {
        QMessageBox::critical(nullptr, "Error", "Clutch is not configured or is disconnected.");
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

    if (flags & FLAG_DEVICES_THROTTLE) {
        qDebug() << "Acquiring throttle...";
        throttle->acquire(&hwnd);
        if (FAILED(hr)) {
            QMessageBox::critical(nullptr, "Error", "Could not acquire throttle device. Please ensure the throttle is connected and configured.");
            return hr;
        };
    }
    if (flags & FLAG_DEVICES_BRAKE) {
        qDebug() << "Acquiring brake...";
        brake->acquire(&hwnd);
        if (FAILED(hr)) {
            QMessageBox::critical(nullptr, "Error", "Could not acquire brake device. Please ensure the brake is connected and configured.");
            return hr;
        };
    }
    if (flags & FLAG_DEVICES_CLUTCH) {
        qDebug() << "Acquiring clutch...";
        clutch->acquire(&hwnd);
        if (FAILED(hr)) {
            QMessageBox::critical(nullptr, "Error", "Could not acquire clutch device. Please ensure the clutch is connected and configured.");
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
    if (throttle != nullptr && throttle->isAcquired) {
        qDebug() << "Releasing throttle...";
        throttle->release();
    }
    if (brake != nullptr && brake->isAcquired) {
        qDebug() << "Releasing brake...";
        brake->release();
    }
    if (clutch != nullptr && clutch->isAcquired) {
        qDebug() << "Releasing clutch...";
        clutch->release();
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

    if (throttle != nullptr) {
        config.beginGroup("throttle");
        config.setValue("device_guid", throttle->instanceGuid.toString());
        config.setValue("throttle_axis", throttleAxisGuid.toString());
        config.setValue("invert_throttle_axis", invertThrottleAxis);
        config.endGroup();
    }
    if (brake != nullptr) {
        config.beginGroup("brake");
        config.setValue("device_guid", brake->instanceGuid.toString());
        config.setValue("brake_axis", brakeAxisGuid.toString());
        config.setValue("invert_brake_axis", invertBrakeAxis);
        config.endGroup();
    }
    if (clutch != nullptr) {
        config.beginGroup("clutch");
        config.setValue("device_guid", clutch->instanceGuid.toString());
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

    if (config.childGroups().contains("throttle")) {
        config.beginGroup("throttle");
        throttle = getDeviceFromGuid(config.value("device_guid").toUuid());
        throttleAxisGuid = config.value("throttle_axis").toUuid();
        invertThrottleAxis = config.value("invert_throttle_axis").toBool();
        config.endGroup();

        if (throttle == nullptr) {
            QMessageBox::warning(nullptr, "Throttle not found", "Saved throttle device is not connected.\nReconnect the device or update the input/output device configuration.");
        }
    }
    if (config.childGroups().contains("brake")) {
        config.beginGroup("brake");
        brake = getDeviceFromGuid(config.value("device_guid").toUuid());
        brakeAxisGuid = config.value("brake_axis").toUuid();
        invertBrakeAxis = config.value("invert_brake_axis").toBool();
        config.endGroup();

        if (brake == nullptr) {
            QMessageBox::warning(nullptr, "Brake not found", "Saved brake device is not connected.\nReconnect the device or update the input/output device configuration.");
        }
    }
    if (config.childGroups().contains("clutch")) {
        config.beginGroup("clutch");
        clutch = getDeviceFromGuid(config.value("device_guid").toUuid());
        clutchAxisGuid = config.value("clutch_axis").toUuid();
        invertClutchAxis = config.value("invert_clutch_axis").toBool();
        config.endGroup();

        if (clutch == nullptr) {
            QMessageBox::warning(nullptr, "Clutch not found", "Saved clutch device is not connected.\nReconnect the device or update the input/output device configuration.");
        }
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
    connect(dialog.throttleDeviceComboBox, &QComboBox::currentIndexChanged, this, &DeviceConfiguration::updateThrottleAxisList);
    connect(dialog.throttleDeviceComboBox, &QComboBox::currentIndexChanged, this, &DeviceConfiguration::testEnableAcceptButton);
    connect(dialog.brakeDeviceComboBox, &QComboBox::currentIndexChanged, this, &DeviceConfiguration::updateBrakeAxisList);
    connect(dialog.brakeDeviceComboBox, &QComboBox::currentIndexChanged, this, &DeviceConfiguration::testEnableAcceptButton);
    connect(dialog.clutchDeviceComboBox, &QComboBox::currentIndexChanged, this, &DeviceConfiguration::updateClutchAxisList);
    connect(dialog.clutchDeviceComboBox, &QComboBox::currentIndexChanged, this, &DeviceConfiguration::testEnableAcceptButton);
    connect(dialog.shiftLockDeviceComboBox, &QComboBox::currentIndexChanged, this, &DeviceConfiguration::changeShiftLockDevice);
    connect(dialog.vjoyDeviceComboBox, &QComboBox::currentIndexChanged, this, &DeviceConfiguration::testEnableAcceptButton);
    // Connect bind axis buttons
    connect(dialog.bindJoystickLRButton, &QPushButton::clicked, this, [=]() {
        AxisBinding binding = bindAxis();
        if(joystick == nullptr || binding.deviceUuid == joystick->instanceGuid)
            updateDeviceComboBoxes(FLAG_DEVICES_JOYSTICK_LR, binding);
        else if (!binding.deviceUuid.isNull())
            QMessageBox::warning(nullptr, "Unable to bind axis", "Joystick axes must be bound to the same device.");
    });
    connect(dialog.bindJoystickFBButton, &QPushButton::clicked, this, [=]() {
        AxisBinding binding = bindAxis();
        if (joystick == nullptr || binding.deviceUuid == joystick->instanceGuid)
            updateDeviceComboBoxes(FLAG_DEVICES_JOYSTICK_FB, binding);
        else if (!binding.deviceUuid.isNull())
            QMessageBox::warning(nullptr, "Unable to bind axis", "Joystick axes must be bound to the same device.");
    });
    connect(dialog.bindThrottleButton, &QPushButton::clicked, this, [=]() {
        updateDeviceComboBoxes(FLAG_DEVICES_THROTTLE, bindAxis());
    });
    connect(dialog.bindBrakeButton, &QPushButton::clicked, this, [=]() {
        updateDeviceComboBoxes(FLAG_DEVICES_BRAKE, bindAxis());
    });
    connect(dialog.bindClutchButton, &QPushButton::clicked, this, [=]() {
        updateDeviceComboBoxes(FLAG_DEVICES_CLUTCH, bindAxis());
    });

    // Populate the device lists
    for (auto device : deviceList)
    {
        dialog.throttleDeviceComboBox->addItem(device.name, device.instanceGuid);
        dialog.brakeDeviceComboBox->addItem(device.name, device.instanceGuid);
        dialog.clutchDeviceComboBox->addItem(device.name, device.instanceGuid);
        if (device.supportsFfb && device.productGuid.data1 != VJOY_PRODUCT_GUID) {
            dialog.joystickDeviceComboBox->addItem(device.name, device.instanceGuid);
        }
        if (device.buttonCount > 0) {
            dialog.shiftLockDeviceComboBox->addItem(device.name, device.instanceGuid);
        }
        if (FAILED(device.acquire(&hwnd))) {
            qDebug() << "Failed to acquire " << device.name;
        }
    }

    // Populate vJoy combo boxes
    if (vJoyFeeder::isDriverEnabled()) {
        for (int i = 0; i < vJoyFeeder::deviceCount(); i++) {
            dialog.vjoyDeviceComboBox->addItem(QString("vJoy Device ").append(QString(" %1").arg(i + 1)), i + 1);
        }
    }

    // Set the combobox indices
    if(joystick != nullptr)
    {
        updateDeviceComboBoxes(FLAG_DEVICES_JOYSTICK_LR, { joystick->instanceGuid, joystickLRAxisGuid });
        updateDeviceComboBoxes(FLAG_DEVICES_JOYSTICK_FB, { joystick->instanceGuid, joystickFBAxisGuid });
    }
    if(throttle != nullptr)
        updateDeviceComboBoxes(FLAG_DEVICES_THROTTLE, {throttle->instanceGuid, throttleAxisGuid});
    if(brake != nullptr)
        updateDeviceComboBoxes(FLAG_DEVICES_BRAKE, { brake->instanceGuid, brakeAxisGuid });
    if(clutch != nullptr)
        updateDeviceComboBoxes(FLAG_DEVICES_CLUTCH, { clutch->instanceGuid, clutchAxisGuid });
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

        throttle = getDeviceFromGuid(dialog.throttleDeviceComboBox->currentData().toUuid());
        if(throttle != nullptr)
        {
            qDebug() << "New throttle: " << throttle->name;
            throttleAxisGuid = dialog.throttleAxisComboBox->currentData().toUuid();
            invertThrottleAxis = dialog.invertThrottleAxisBox->isChecked();
        }
        brake = getDeviceFromGuid(dialog.brakeDeviceComboBox->currentData().toUuid());
        if(brake != nullptr) 
        {
            brakeAxisGuid = dialog.brakeAxisComboBox->currentData().toUuid();
            invertBrakeAxis = dialog.invertBrakeAxisBox->isChecked();
            qDebug() << "New brake: " << brake->name;
        }
        clutch = getDeviceFromGuid(dialog.clutchDeviceComboBox->currentData().toUuid());
        if(clutch != nullptr)
        {
            clutchAxisGuid = dialog.clutchAxisComboBox->currentData().toUuid();
            invertClutchAxis = dialog.invertClutchAxisBox->isChecked();
            qDebug() << "New clutch: " << clutch->name;
        }

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

        for (auto device : deviceList)
            device.release();

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

void DeviceConfiguration::updateDeviceComboBoxes(int flag, AxisBinding binding) {
    if (binding.axisUuid.isNull() || binding.deviceUuid.isNull())
        return;
    if (flag & FLAG_DEVICES_JOYSTICK_LR) {
        dialog.joystickDeviceComboBox->setCurrentIndex(dialog.joystickDeviceComboBox->findData(binding.deviceUuid));
        dialog.joystickLRAxisComboBox->setCurrentIndex(dialog.joystickLRAxisComboBox->findData(binding.axisUuid));
    }
    else if (flag & FLAG_DEVICES_JOYSTICK_FB) {
        dialog.joystickDeviceComboBox->setCurrentIndex(dialog.joystickDeviceComboBox->findData(binding.deviceUuid));
        dialog.joystickFBAxisComboBox->setCurrentIndex(dialog.joystickFBAxisComboBox->findData(binding.axisUuid));
    }
    else if (flag & FLAG_DEVICES_THROTTLE) {
        dialog.throttleDeviceComboBox->setCurrentIndex(dialog.throttleDeviceComboBox->findData(binding.deviceUuid));
        dialog.throttleAxisComboBox->setCurrentIndex(dialog.throttleAxisComboBox->findData(binding.axisUuid));
        dialog.invertThrottleAxisBox->setChecked(invertThrottleAxis);
    }
    else if (flag & FLAG_DEVICES_BRAKE) {
        dialog.brakeDeviceComboBox->setCurrentIndex(dialog.brakeDeviceComboBox->findData(binding.deviceUuid));
        dialog.brakeAxisComboBox->setCurrentIndex(dialog.brakeAxisComboBox->findData(binding.axisUuid));
        dialog.invertBrakeAxisBox->setChecked(invertBrakeAxis);
    }
    else if (flag & FLAG_DEVICES_CLUTCH) {
        dialog.clutchDeviceComboBox->setCurrentIndex(dialog.clutchDeviceComboBox->findData(binding.deviceUuid));
        dialog.clutchAxisComboBox->setCurrentIndex(dialog.clutchAxisComboBox->findData(binding.axisUuid));
        dialog.invertClutchAxisBox->setChecked(invertClutchAxis);
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

void DeviceConfiguration::updateThrottleAxisList(int deviceIndex) {
    QUuid deviceGuid = dialog.throttleDeviceComboBox->currentData().toUuid();
    auto selectedPedals = getDeviceFromGuid(deviceGuid);

    dialog.throttleAxisComboBox->clear();
    QMap<QUuid, QString> axisMap = selectedPedals->getDeviceAxes();
    for (auto axis = axisMap.cbegin(), end = axisMap.cend(); axis != end; ++axis)
    {
        dialog.throttleAxisComboBox->addItem(axis.value(), axis.key());
    }
}


void DeviceConfiguration::updateBrakeAxisList(int deviceIndex) {
    QUuid deviceGuid = dialog.brakeDeviceComboBox->currentData().toUuid();
    auto selectedPedals = getDeviceFromGuid(deviceGuid);

    dialog.brakeAxisComboBox->clear();
    QMap<QUuid, QString> axisMap = selectedPedals->getDeviceAxes();
    for (auto axis = axisMap.cbegin(), end = axisMap.cend(); axis != end; ++axis)
    {
        dialog.brakeAxisComboBox->addItem(axis.value(), axis.key());
    }
}


void DeviceConfiguration::updateClutchAxisList(int deviceIndex) {
    QUuid deviceGuid = dialog.clutchDeviceComboBox->currentData().toUuid();
    auto selectedPedals = getDeviceFromGuid(deviceGuid);

    dialog.clutchAxisComboBox->clear();
    QMap<QUuid, QString> axisMap = selectedPedals->getDeviceAxes();
    for (auto axis = axisMap.cbegin(), end = axisMap.cend(); axis != end; ++axis)
    {
        dialog.clutchAxisComboBox->addItem(axis.value(), axis.key());
    }
}

PedalValues DeviceConfiguration::getPedalValues() {
    PedalValues values = { 0, 0, 0 };
    if (SUCCEEDED(throttle->updateState()))
    {
        values.throttle = throttle->getAxisReading(throttleAxisGuid);
        if (invertThrottleAxis) {
            values.throttle = abs(65535 - values.throttle);
        }
        emit throttleValueChanged(values.throttle);
    }
    if (SUCCEEDED(clutch->updateState()))
    {
        values.clutch = clutch->getAxisReading(clutchAxisGuid);
        if (invertClutchAxis) {
            values.clutch = abs(65535 - values.clutch);
        }
        emit clutchValueChanged(values.clutch);
    }

    if (SUCCEEDED(brake->updateState()))
    {
        values.brake = brake->getAxisReading(brakeAxisGuid);
        if (invertBrakeAxis) {
            values.brake = abs(65535 - values.brake);
        }
    }
    emit pedalValuesChanged(values.clutch, values.throttle);    // TODO: Deprecate this
    return values;
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

AxisBinding DeviceConfiguration::bindAxis() {
    BindAxisWindow popup(&deviceList);

    if (popup.exec() == QDialog::Accepted) {
        return popup.getSelectedAxis();
    }
    return { QUuid(), QUuid()};
}