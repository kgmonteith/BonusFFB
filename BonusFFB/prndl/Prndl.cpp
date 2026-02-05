/*
Copyright (C) 2024-2026 Ken Monteith.

This file is part of Bonus FFB.

Bonus FFB is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or any later version.

Bonus FFB is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Bonus FFB. If not, see <https://www.gnu.org/licenses/>.
*/

#include <QGraphicsRectItem>
#include <QMessageBox>
#include <QSettings>
#include <QFile>
#include "Prndl.h"

QString Prndl::getAppName() {
    return "prndl";
}

void Prndl::initialize() {
    // Menu action connections
    QObject::connect(ui->actionSaveSettings, &QAction::triggered, this, &Prndl::saveSettings);
    QObject::connect(ui->actionLoadSettings, &QAction::triggered, this, &Prndl::loadSettings);
    // Graphics connections
    QObject::connect(ui->prndlTabWidget, &QTabWidget::currentChanged, this, &Prndl::redrawJoystickMap);
    QObject::connect(&stateManager, &PrndlStateManager::slotChanged, this, &Prndl::changeSlotLabel);
    // Joystick connections
    QObject::connect(ui->prndl_joystickDeviceComboBox, &QComboBox::currentIndexChanged, this, &Prndl::changeJoystickDevice);
    QObject::connect(ui->prndl_joystickLRAxisComboBox, &QComboBox::currentIndexChanged, this, &Prndl::changeJoystickLRAxis);
    QObject::connect(ui->prndl_joystickFBAxisComboBox, &QComboBox::currentIndexChanged, this, &Prndl::changeJoystickFBAxis);
    QObject::connect(this, &Prndl::joystickValueChanged, this, &Prndl::updateJoystickCircle);
    QObject::connect(&stateManager, &PrndlStateManager::slotSpringChanged, &slotGuard, &PrndlSlotGuard::updateSlotSpringCenter);
    // vJoy connections
    QObject::connect(&stateManager, &PrndlStateManager::buttonZoneChanged, vjoy, &vJoyFeeder::updateButtons);
    QObject::connect(&stateManager, &PrndlStateManager::buttonShortPress, vjoy, &vJoyFeeder::shortPressButton);
    // Shift lock connections
    QObject::connect(ui->prndl_shiftLockDeviceComboBox, &QComboBox::currentIndexChanged, this, &Prndl::changeShiftLockDevice);
    QObject::connect(ui->prndl_shiftLockDeviceComboBox, &QComboBox::currentIndexChanged, &stateManager, &PrndlStateManager::toggleUsingShiftLock);
    QObject::connect(&stateManager, &PrndlStateManager::updateShiftLockEffectStrength, &slotGuard, &PrndlSlotGuard::updateShiftLockEffectStrength);
    QObject::connect(ui->prndl_shiftLockFromNeutralToReverseCheckBox, &QCheckBox::checkStateChanged, &stateManager, &PrndlStateManager::toggleLockShiftsFromNeutralToReverse);
    // Additional settings connections
    QObject::connect(ui->prndl_enableParkSlotCheckBox, &QCheckBox::checkStateChanged, &stateManager, &PrndlStateManager::toggleParkSlot);
    QObject::connect(ui->prndl_enableLowSlotCheckBox, &QCheckBox::checkStateChanged, &stateManager, &PrndlStateManager::toggleLastSlot);
    QObject::connect(ui->prndl_simulateParkUsingTelemetryCheckBox, &QCheckBox::checkStateChanged, &stateManager, &PrndlStateManager::toggleAtsTelemetryPark);
    

    // Populate the device lists
    for (const DeviceInfo& device : *deviceList)
    {
        if (device.supportsFfb && device.productGuid.data1 != VJOY_PRODUCT_GUID) {
            ui->prndl_joystickDeviceComboBox->addItem(device.name, device.instanceGuid);
        }
        if (device.buttonCount > 0) {
            ui->prndl_shiftLockDeviceComboBox->addItem(device.name, device.instanceGuid);
        }
    }

    // Populate vJoy combo boxes
    if (vJoyFeeder::isDriverEnabled()) {
        for (int i = 0; i < vJoyFeeder::deviceCount(); i++) {
            ui->prndl_vjoyDeviceComboBox->addItem(QString("vJoy Device ").append(QString(" %1").arg(i + 1)), i + 1);
        }
    }
}

void Prndl::initializeJoystickMap() {
    scene = new QGraphicsScene();
    scene->setSceneRect(ui->prndl_graphicsView->viewport()->rect());

    long sceneWidth = ui->prndl_graphicsView->viewport()->rect().width();
    long sceneHeight = ui->prndl_graphicsView->viewport()->rect().height();
    QPointF center = scene->sceneRect().center();

    centerSlotRect = new QGraphicsRectItem(0, 0, SLOT_WIDTH_PX, sceneHeight);
    centerSlotRect->setBrush(QBrush(Qt::black));
    centerSlotRect->setPen(Qt::NoPen);
    centerSlotRect->setPos(center - QPointF(SLOT_WIDTH_PX / 2, sceneHeight / 2));
    scene->addItem(centerSlotRect);

    int slotCenter = 0;
    int slotCircleStepSize = sceneHeight / (stateManager.getEnabledSlotCount() - 1);
    for (int i = 0; i < stateManager.getEnabledSlotCount(); i++) {
        QGraphicsEllipseItem* scPtr = new QGraphicsEllipseItem(0, 0, SHIFTER_POSITION_MARKER_DIAMETER_PX, SHIFTER_POSITION_MARKER_DIAMETER_PX);
        scPtr->setBrush(QBrush(Qt::black));
        scPtr->setPen(Qt::NoPen);
        scPtr->setPos(QPointF((sceneWidth / 2) - (SHIFTER_POSITION_MARKER_DIAMETER_PX / 2), (slotCircleStepSize * i) - SHIFTER_POSITION_MARKER_DIAMETER_PX / 2));
        scene->addItem(scPtr);
        slotCircles.push_back(scPtr);
    }

    joystickCircle = new QGraphicsEllipseItem(0, 0, JOYSTICK_MARKER_DIAMETER_PX, JOYSTICK_MARKER_DIAMETER_PX);
    QColor seethroughWhite = Qt::transparent;
    seethroughWhite.setAlphaF(float(0.15));
    joystickCircle->setBrush(QBrush(seethroughWhite));
    joystickCircle->setPen(QPen(QColor(1, 129, 231), 7));
    QPointF circlePos = center - QPointF(JOYSTICK_MARKER_DIAMETER_PX / 2.0, JOYSTICK_MARKER_DIAMETER_PX / 2.0);
    joystickCircle->setPos(circlePos);
    scene->addItem(joystickCircle);

    ui->prndl_graphicsView->setScene(scene);
    ui->prndl_graphicsView->setRenderHints(QPainter::Antialiasing);
    ui->prndl_graphicsView->show();
}

// Separate call because the event doesn't trigger if another tab is active
void Prndl::redrawJoystickMap() {
    if (scene == nullptr) {
        return;
    }
    ui->prndl_graphicsView->scene()->setSceneRect(ui->prndl_graphicsView->viewport()->rect());

    long sceneWidth = ui->prndl_graphicsView->viewport()->rect().width();
    long sceneHeight = ui->prndl_graphicsView->viewport()->rect().height();
    QPointF center = scene->sceneRect().center();

    centerSlotRect->setRect(0, 0, SLOT_WIDTH_PX, sceneHeight);
    centerSlotRect->setPos(center - QPointF(SLOT_WIDTH_PX / 2, sceneHeight / 2));

    int i = 0;
    int slotCircleStepSize = sceneHeight / (stateManager.getEnabledSlotCount() - 1);
    for (const auto scPtr : slotCircles) {
        scPtr->setPos(QPointF((sceneWidth / 2) - (SHIFTER_POSITION_MARKER_DIAMETER_PX / 2), (slotCircleStepSize * i) - SHIFTER_POSITION_MARKER_DIAMETER_PX / 2));
        i++;
    }

    joystickCircle->setPos(center - QPointF(joystickCircle->rect().width() / 2, joystickCircle->rect().height() / 2));
}

void Prndl::updateJoystickCircle(int LRValue, int FBValue) {
    long scaledLRValue = (LRValue * ui->prndl_graphicsView->viewport()->rect().width()) / 65535;
    long scaledFBValue = (FBValue * ui->prndl_graphicsView->viewport()->rect().height()) / 65535;

    ui->prndl_graphicsView->setUpdatesEnabled(false);
    joystickCircle->setPos(QPoint(scaledLRValue, scaledFBValue) - QPointF(joystickCircle->rect().width() / 2, joystickCircle->rect().height() / 2));
    ui->prndl_graphicsView->setUpdatesEnabled(true);
}

void Prndl::changeSlotLabel(PrndlSlot slot) {
    if (slot == PrndlSlot::PARK) {
        ui->prndl_gearLabel->setText("P");
    }
    else if (slot == PrndlSlot::REVERSE) {
        ui->prndl_gearLabel->setText("R");
    }
    else if (slot == PrndlSlot::NEUTRAL) {
        ui->prndl_gearLabel->setText("N");
    }
    else if (slot == PrndlSlot::DRIVE) {
        ui->prndl_gearLabel->setText("D");
    }
    else if (slot == PrndlSlot::LOW) {
        ui->prndl_gearLabel->setText("L");
    }
}

void Prndl::changeJoystickDevice(int deviceIndex) {
    // Release previous joystick
    if (joystick != nullptr && joystick->isAcquired) {
        joystick->release();
    }
    QUuid deviceGuid = ui->prndl_joystickDeviceComboBox->currentData().toUuid();
    qDebug() << "Device UUID: " << deviceGuid;
    joystick = getDeviceFromGuid(deviceList, deviceGuid);
    qDebug() << "New joystick device: " << joystick->name;
    ui->prndl_joystickLRAxisComboBox->clear();
    ui->prndl_joystickFBAxisComboBox->clear();
    QMap<QUuid, QString> axisMap = joystick->getDeviceAxes();
    for (auto axis = axisMap.cbegin(), end = axisMap.cend(); axis != end; ++axis)
    {
        ui->prndl_joystickLRAxisComboBox->addItem(axis.value(), axis.key());
        ui->prndl_joystickFBAxisComboBox->addItem(axis.value(), axis.key());
    }
}

void Prndl::changeJoystickLRAxis(int axisIndex) {
    joystickLRAxisGuid = ui->prndl_joystickLRAxisComboBox->currentData().toUuid();
}

void Prndl::changeJoystickFBAxis(int axisIndex) {
    joystickFBAxisGuid = ui->prndl_joystickFBAxisComboBox->currentData().toUuid();
}

void Prndl::changeShiftLockDevice(int deviceIndex) {
    if (shiftLockDevice != nullptr && shiftLockDevice->isAcquired) {
        shiftLockDevice->release();
    }
    if (deviceIndex > 0) {
        QVariant data = ui->prndl_shiftLockDeviceComboBox->currentData();
        QUuid deviceGuid = ui->prndl_shiftLockDeviceComboBox->currentData().toUuid();
        shiftLockDevice = getDeviceFromGuid(deviceList, deviceGuid);
        shiftLockDevice->acquire(&hwnd);
        ui->prndl_shiftLockButtonComboBox->setEnabled(true);
        ui->prndl_shiftLockButtonComboBox->clear();
        ui->prndl_shiftLockButtonMonitorLabel->setText("⭕");
        for (int i = 1; i <= shiftLockDevice->buttonCount; i++) {
            ui->prndl_shiftLockButtonComboBox->addItem(QString::number(i));
        }
    }
    else {
        shiftLockDevice = nullptr;
        ui->prndl_shiftLockButtonComboBox->clear();
        ui->prndl_shiftLockButtonComboBox->setEnabled(false);
        ui->prndl_shiftLockButtonMonitorLabel->setText("✖️");
    }
}

void Prndl::saveSettings() {
    QSettings settings = QSettings(this->deviceSettingsFile, QSettings::IniFormat);
    settings.beginGroup("joystick");
    settings.setValue("device_guid", joystick->instanceGuid.toString());
    settings.setValue("lr_axis", joystickLRAxisGuid.toString());
    settings.setValue("fb_axis", joystickFBAxisGuid.toString());
    settings.endGroup();

    settings.beginGroup("shiftlockdevice");
    if (shiftLockDevice != nullptr) {
        settings.setValue("device_guid", shiftLockDevice->instanceGuid.toString());
        settings.setValue("device_button", ui->prndl_shiftLockButtonComboBox->currentIndex());
    }
    else {
        settings.setValue("device_guid", "None");
    }
    settings.endGroup();

    settings.beginGroup("vjoy");
    settings.setValue("vjoy_device", vjoy->getDeviceIndex());
    settings.endGroup();

    settings.beginGroup("other_settings");
    settings.setValue("enable_park_slot", ui->prndl_enableParkSlotCheckBox->isChecked());
    settings.setValue("enable_low_slot", ui->prndl_enableLowSlotCheckBox->isChecked());
    settings.setValue("simulate_park_atsets2", ui->prndl_simulateParkUsingTelemetryCheckBox->isChecked());
    settings.setValue("shift_lock_neutral_reverse", ui->prndl_shiftLockFromNeutralToReverseCheckBox->isChecked());
    settings.endGroup();
}

void Prndl::loadSettings() {
    qDebug() << "Loading settings";
    if (!QFile(deviceSettingsFile).exists()) {
        qDebug() << "Settings file does not exist";
        return;
    }
    QSettings settings = QSettings(deviceSettingsFile, QSettings::IniFormat);

    settings.beginGroup("joystick");
    int joystick_index = ui->prndl_joystickDeviceComboBox->findData(settings.value("device_guid").toUuid());
    if (joystick_index == -1 && !g_joystick_warned) {
        QMessageBox::warning(nullptr, "Joystick not found", "Saved PRNDL joystick device is not connected.\nReconnect the device or update the input/output settings.");
        g_joystick_warned = true;
    }
    else
    {
        ui->prndl_joystickDeviceComboBox->setCurrentIndex(joystick_index);
        ui->prndl_joystickLRAxisComboBox->setCurrentIndex(ui->prndl_joystickLRAxisComboBox->findData(settings.value("lr_axis").toUuid()));
        ui->prndl_joystickFBAxisComboBox->setCurrentIndex(ui->prndl_joystickFBAxisComboBox->findData(settings.value("fb_axis").toUuid()));
    }
    settings.endGroup();

    settings.beginGroup("shiftlockdevice");
    if (settings.value("device_guid").toString() != "None") {
        int shiftlockdevice_index = ui->prndl_shiftLockDeviceComboBox->findData(settings.value("device_guid").toUuid());
        if (shiftlockdevice_index == -1) {
            QMessageBox::warning(nullptr, "Shift lock device not found", "Saved shift lock device is not connected.\nReconnect the device or update the input/output settings.");
        }
        else
        {
            ui->prndl_shiftLockDeviceComboBox->setCurrentIndex(shiftlockdevice_index);
            ui->prndl_shiftLockButtonComboBox->setCurrentIndex(settings.value("device_button").toInt());
        }
    }
    settings.endGroup();

    settings.beginGroup("vjoy");
    ui->prndl_vjoyDeviceComboBox->setCurrentIndex(settings.value("vjoy_device").toInt());
    settings.endGroup();

    settings.beginGroup("other_settings");
    ui->prndl_enableParkSlotCheckBox->setChecked(settings.value("enable_park_slot").toBool());
    ui->prndl_enableLowSlotCheckBox->setChecked(settings.value("enable_low_slot").toBool());
    ui->prndl_simulateParkUsingTelemetryCheckBox->setChecked(settings.value("simulate_park_atsets2").toBool());
    ui->prndl_shiftLockFromNeutralToReverseCheckBox->setChecked(settings.value("shift_lock_neutral_reverse").toBool());
    settings.endGroup();
    qDebug() << "Succesfully loaded PRNDL settings";
}

bool Prndl::getShiftLockReleased() {
    if (shiftLockDevice == nullptr || !shiftLockDevice->isAcquired || !ui->prndl_shiftLockButtonComboBox->isEnabled()) {
        return false;
    }
    shiftLockDevice->updateState();
    bool shiftLockReleased = shiftLockDevice->isButtonPressed(ui->prndl_shiftLockButtonComboBox->currentIndex());
    if (shiftLockReleased != lastShiftLockReleased) {
        if (shiftLockReleased) {
            ui->prndl_shiftLockButtonMonitorLabel->setText("🟢");
        }
        else
        {
            ui->prndl_shiftLockButtonMonitorLabel->setText("⭕");
        }
        emit shiftLockStateChanged(shiftLockReleased);
    }
    lastShiftLockReleased = shiftLockReleased;
    return shiftLockReleased;
}

QPair<int, int> Prndl::getJoystickValues() {
    HRESULT hr = joystick->updateState();
    if (hr != DI_OK) {
        qDebug() << "updateState failed, reacquiring joystick. Return code " << unsigned long(hr);
        joystick->reacquire();
        slotGuard.start(joystick);
    }
    long joystickLRValue = joystick->getAxisReading(joystickLRAxisGuid);
    long joystickFBValue = joystick->getAxisReading(joystickFBAxisGuid);
    emit joystickLRValueChanged(joystickLRValue);
    emit joystickFBValueChanged(joystickFBValue);
    emit joystickValueChanged(joystickLRValue, joystickFBValue);
    return QPair<int, int>(joystickLRValue, joystickFBValue);
}

HRESULT Prndl::startGameLoop() {    // Acquire joystick
    qDebug() << "Acquiring joystick...";
    HRESULT hr = joystick->acquire(&hwnd, true);
    if (FAILED(hr)) {
        QMessageBox::critical(nullptr, "Error", "Could not acquire exclusive use of FFB joystick. Please close other games or applications and try again.");
        return hr;
    };

    // Acquire vJoy for feeding
    if (!vjoy->acquire()) {
        QMessageBox::critical(nullptr, "Error", "Could not acquire vJoy device. Only one program may feed each vJoy device, please close other games or applications and try again.");
        return E_FAIL;
    }

    // TODO: Initialize FFB
    if (FAILED(slotGuard.start(joystick))) {
        qDebug() << "Failed to start slotGuard effects";
    }
    joystick->startEffects();
    return S_OK;
}

void Prndl::stopGameLoop() {
    // Release devices
    vjoy->release();
    joystick->release();
    return;
}

void Prndl::gameLoop() {
    if (joystick == nullptr || !joystick->isAcquired ) {
        return;
    }
    // Get new joystick values
    QPair<int, int> joystickValues = getJoystickValues();
    bool isShiftLockRelased = getShiftLockReleased();

    // Get telemetry values
    bool isParkingBrakeSet = false;
    if (telemetry->isConnected() != TelemetrySource::NONE) {
        isParkingBrakeSet = telemetry->getParkingBrakeState();
    }

    stateManager.update(joystickValues, isShiftLockRelased, isParkingBrakeSet);
}