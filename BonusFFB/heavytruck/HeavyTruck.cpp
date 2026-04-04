/*
Copyright (C) 2024-2026 Ken Monteith.

This file is part of Bonus FFB.

Bonus FFB is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or any later version.

Bonus FFB is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Bonus FFB. If not, see <https://www.gnu.org/licenses/>.
*/


#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsRectItem>
#include <QGraphicsEllipseItem>
#include <QMessageBox>
#include <QSettings>
#include <QFile>
#include "HeavyTruck.h"

QString HeavyTruck::getAppName() {
    return "hshifter";
}

void HeavyTruck::initialize() {
    // Menu action connections
    connect(ui->actionSaveSettings, &QAction::triggered, this, &HeavyTruck::saveSettings);
    connect(ui->actionLoadSettings, &QAction::triggered, this, &HeavyTruck::loadSettings);
    // UI connections
    connect(ui->heavytruck_setPresetEatonFullerButton, &QPushButton::clicked, this, &HeavyTruck::setPresetPatternEatonFuller);
    connect(ui->heavytruck_setPresetFullRangeButton, &QPushButton::clicked, this, &HeavyTruck::setPresetPatternFullRange);
    connect(ui->heavytruck_buttonZoneDepthSpinbox, &QSpinBox::valueChanged, slot, &SlotParameters::setButtonZoneDepth);
    // Graphics connections
    connect(ui->heavytruckTabWidget, &QTabWidget::currentChanged, this, &HeavyTruck::redrawJoystickMap);
    // HeavyTruck joystick connections
    connect(ui->heavytruck_joystickDeviceComboBox, &QComboBox::currentIndexChanged, this, &HeavyTruck::changeJoystickDevice);
    connect(ui->heavytruck_joystickLRAxisComboBox, &QComboBox::currentIndexChanged, this, &HeavyTruck::changeJoystickLRAxis);
    connect(ui->heavytruck_joystickFBAxisComboBox, &QComboBox::currentIndexChanged, this, &HeavyTruck::changeJoystickFBAxis);
    connect(this, &HeavyTruck::joystickLRValueChanged, ui->heavytruck_ioTabJoystickLRProgressBar, &QProgressBar::setValue);
    connect(this, &HeavyTruck::joystickFBValueChanged, ui->heavytruck_ioTabJoystickFBProgressBar, &QProgressBar::setValue);
    connect(this, &HeavyTruck::joystickFBValueChanged, &synchroGuard, &HeavyTruckSynchroGuard::setJoystickFBValue);
    // HeavyTruck pedal connections
    connect(ui->heavytruck_pedalsDeviceComboBox, &QComboBox::currentIndexChanged, this, &HeavyTruck::changePedalsDevice);
    connect(ui->heavytruck_clutchAxisComboBox, &QComboBox::currentIndexChanged, this, &HeavyTruck::changeClutchAxis);
    connect(ui->heavytruck_throttleAxisComboBox, &QComboBox::currentIndexChanged, this, &HeavyTruck::changeThrottleAxis);
    connect(this, &HeavyTruck::clutchValueChanged, ui->heavytruck_ioTabClutchProgressBar, &QProgressBar::setValue);
    connect(this, &HeavyTruck::throttleValueChanged, ui->heavytruck_ioTabThrottleProgressBar, &QProgressBar::setValue);
    // Telemetry connections
    connect(telemetry, &Telemetry::telemetryChanged, &stateManager, &HeavyTruckStateManager::setTelemetryState);
    // Joystick connections
    connect(this, &HeavyTruck::joystickValueChanged, this, &HeavyTruck::updateJoystickCircle);
    // Pedal connections
    connect(this, &HeavyTruck::clutchValueChanged, ui->heavytruck_clutchProgressBar, &QProgressBar::setValue);
    connect(this, &HeavyTruck::throttleValueChanged, ui->heavytruck_throttleProgressBar, &QProgressBar::setValue);
    // vJoy connections
    connect(ui->heavytruck_vjoyDeviceComboBox, &QComboBox::currentIndexChanged, vjoy, &vJoyFeeder::setDeviceIndex);
    connect(&stateManager, &HeavyTruckStateManager::buttonZoneChanged, vjoy, &vJoyFeeder::updateButtons);
    connect(&stateManager, &HeavyTruckStateManager::buttonZoneChanged, this, &HeavyTruck::updateGearText);
    // FFB effect connections
    connect(&stateManager, &HeavyTruckStateManager::slotStateChanged, &slotGuard, &HeavyTruckSlotGuard::updateSlotGuardState);
    connect(&stateManager, &HeavyTruckStateManager::synchroStateChanged, &synchroGuard, &HeavyTruckSynchroGuard::synchroStateChanged);
    connect(&stateManager, &HeavyTruckStateManager::rpmDeltaChanged, &synchroGuard, &HeavyTruckSynchroGuard::updateGrindEffectRPM);
    connect(&stateManager, &HeavyTruckStateManager::rpmDeltaChanged, this, &HeavyTruck::updateRpmDeltaText);
    connect(&stateManager, &HeavyTruckStateManager::grindingStateChanged, &synchroGuard, &HeavyTruckSynchroGuard::grindingStateChanged);
    connect(ui->heavytruck_grindIntensitySlider, &QSlider::valueChanged, &synchroGuard, &HeavyTruckSynchroGuard::setGrindEffectIntensity);
    connect(ui->heavytruck_maxRevMatchRPMSlider, &QSlider::valueChanged, &synchroGuard, &HeavyTruckSynchroGuard::setMaxRevMatchRPM);
    //connect(ui->grindRPMSlider, &QSlider::valueChanged, &synchroGuard, &SynchroGuard::updateEngineRPM);
    connect(ui->heavytruck_grindEffectShapeComboBox, &QComboBox::currentIndexChanged, &synchroGuard, &HeavyTruckSynchroGuard::setGrindEffectShape);
    connect(ui->heavytruck_keepInGearIdleSlider, &QSlider::valueChanged, &synchroGuard, &HeavyTruckSynchroGuard::setKeepInGearIdleIntensity);
    connect(ui->heavytruck_slotDepthSlider, &QSlider::valueChanged, this, &HeavyTruck::slotParameterChanged);
    connect(ui->heavytruck_centerSlotPositionSlider, &QSlider::valueChanged, this, &HeavyTruck::slotParameterChanged);
    connect(ui->heavytruck_rightSlotPositionSlider, &QSlider::valueChanged, this, &HeavyTruck::slotParameterChanged);
    // Static FFB effect connections
    connect(ui->heavytruck_damperSlider, &QSlider::valueChanged, &slotGuard, &HeavyTruckSlotGuard::updateDamper);
    connect(ui->heavytruck_inertiaSlider, &QSlider::valueChanged, &slotGuard, &HeavyTruckSlotGuard::updateInertia);
    connect(ui->heavytruck_frictionSlider, &QSlider::valueChanged, &slotGuard, &HeavyTruckSlotGuard::updateFriction);

    // Populate the device lists
    for (const DeviceInfo& device : *deviceList)
    {
        ui->heavytruck_pedalsDeviceComboBox->addItem(device.name, device.instanceGuid);
        if (device.supportsFfb && device.productGuid.data1 != VJOY_PRODUCT_GUID) {
            ui->heavytruck_joystickDeviceComboBox->addItem(device.name, device.instanceGuid);
        }
    }

    // Populate vJoy combo boxes
    if (vJoyFeeder::isDriverEnabled()) {
        for (int i = 0; i < vJoyFeeder::deviceCount(); i++) {
            ui->heavytruck_vjoyDeviceComboBox->addItem(QString("vJoy Device ").append(QString(" %1").arg(i + 1)), i + 1);
        }
    }

    // Start with axis progress bars hidden
    hideAxisProgressBars();

    // For now, use our EA presets on launch
    // TODO: Remove this once config settings are saved to disk
    setPresetPatternEatonFuller();
}

void HeavyTruck::setPresetPatternEatonFuller() {
    ui->heavytruck_slotDepthSlider->setValue(66);
    ui->heavytruck_centerSlotPositionSlider->setValue(34);
    ui->heavytruck_rightSlotPositionSlider->setValue(66);
}


void HeavyTruck::setPresetPatternFullRange() {
    ui->heavytruck_slotDepthSlider->setValue(100);
    ui->heavytruck_centerSlotPositionSlider->setValue(50);
    ui->heavytruck_rightSlotPositionSlider->setValue(100);
}

void HeavyTruck::slotParameterChanged(int t) {
    // Either right or center slot position changed, so we need to update both
    slot->depth = (double)ui->heavytruck_slotDepthSlider->value() * .01;
    slot->pos_pct[1] = ui->heavytruck_centerSlotPositionSlider->value() * .01;
    slot->pos_pct[2] = ui->heavytruck_rightSlotPositionSlider->value() * .01;
}

void HeavyTruck::initializeJoystickMap() {
    scene = new QGraphicsScene();
    scene->setSceneRect(ui->heavytruck_graphicsView->viewport()->rect());

    long sceneWidth = ui->heavytruck_graphicsView->viewport()->rect().width();
    long sceneHeight = ui->heavytruck_graphicsView->viewport()->rect().height();
    long slotHeight = sceneHeight * slot->depth;
    long slotTop = (sceneHeight / 2) - (slotHeight / 2);
    QPointF center = scene->sceneRect().center();

    neutralChannelRect = new QGraphicsRectItem(0, 0, sceneWidth, SLOT_WIDTH_PX);
    neutralChannelRect->setBrush(QBrush(Qt::black));
    neutralChannelRect->setPen(Qt::NoPen);
    scene->addItem(neutralChannelRect);

    centerSlotRect = new QGraphicsRectItem(0, slotTop, SLOT_WIDTH_PX, slotHeight);
    centerSlotRect->setBrush(QBrush(Qt::black));
    centerSlotRect->setPen(Qt::NoPen);
    scene->addItem(centerSlotRect);

    rightSlotRect = new QGraphicsRectItem(0, slotTop, SLOT_WIDTH_PX, slotHeight);
    rightSlotRect->setBrush(QBrush(Qt::black));
    rightSlotRect->setPen(Qt::NoPen);
    scene->addItem(rightSlotRect);

    leftSlotRect = new QGraphicsRectItem(0, slotTop, SLOT_WIDTH_PX, slotHeight);
    leftSlotRect->setBrush(QBrush(Qt::black));
    leftSlotRect->setPen(Qt::NoPen);
    scene->addItem(leftSlotRect);

    joystickCircle = new QGraphicsEllipseItem(0, 0, JOYSTICK_MARKER_DIAMETER_PX, JOYSTICK_MARKER_DIAMETER_PX);
    QColor seethroughWhite = Qt::transparent;
    seethroughWhite.setAlphaF(float(0.15));
    joystickCircle->setBrush(QBrush(seethroughWhite));
    joystickCircle->setPen(QPen(QColor(1, 129, 231), 7));
    scene->addItem(joystickCircle);

    grindZoneRect = new QGraphicsRectItem(0, 0, 0, 0);
    grindZoneRect->setBrush(QBrush(Qt::NoBrush));
    grindZoneRect->setPen(QPen(Qt::red));
    scene->addItem(grindZoneRect);
    buttonZoneRect = new QGraphicsRectItem(0, 0, 0, 0);
    buttonZoneRect->setBrush(QBrush(Qt::NoBrush));
    buttonZoneRect->setPen(QPen(Qt::blue));
    scene->addItem(buttonZoneRect);

    ui->heavytruck_graphicsView->setScene(scene);
    ui->heavytruck_graphicsView->setRenderHints(QPainter::Antialiasing);
    ui->heavytruck_graphicsView->show();
}

// Separate call because the event doesn't trigger if another tab is active
void HeavyTruck::redrawJoystickMap() {
    if (scene == nullptr) {
        return;
    }

    ui->heavytruck_graphicsView->scene()->setSceneRect(ui->heavytruck_graphicsView->viewport()->rect());

    long sceneWidth = ui->heavytruck_graphicsView->viewport()->rect().width();
    long sceneHeight = ui->heavytruck_graphicsView->viewport()->rect().height();
    long slotHeight = sceneHeight * slot->depth;
    long slotTop = (sceneHeight / 2) - (slotHeight / 2);
    long centerSlotPos = sceneWidth * slot->pos_pct[1] - SLOT_WIDTH_PX / 2;
    long rightSlotPos = sceneWidth * slot->pos_pct[2] - SLOT_WIDTH_PX / 2;
    if (rightSlotPos > sceneWidth - SLOT_WIDTH_PX) {
        rightSlotPos = sceneWidth - SLOT_WIDTH_PX;
    }
    
    QPointF center = scene->sceneRect().center();

    neutralChannelRect->setRect(0, 0, rightSlotPos, SLOT_WIDTH_PX);
    neutralChannelRect->setPos(0, sceneHeight / 2  - SLOT_WIDTH_PX / 2);

    centerSlotRect->setRect(0, slotTop, SLOT_WIDTH_PX, slotHeight);
    centerSlotRect->setPos(centerSlotPos, 0);

    rightSlotRect->setRect(0, slotTop, SLOT_WIDTH_PX, slotHeight);
    rightSlotRect->setPos(rightSlotPos, 0);

    leftSlotRect->setRect(0, slotTop, SLOT_WIDTH_PX, slotHeight);

    if (ui->heavytruck_displayZoneMarkers->isChecked()) {
        grindZoneRect->setRect(-2, (sceneHeight / 2) - (sceneHeight / 2 * slot->grind_point_depth), sceneWidth + 4, sceneHeight * slot->grind_point_depth);
        buttonZoneRect->setRect(-2, (sceneHeight / 2) - (sceneHeight / 2 * slot->button_zone_depth_telemetry), sceneWidth + 4, sceneHeight * slot->button_zone_depth_telemetry);
    }

    joystickCircle->setPos(center - QPointF(joystickCircle->rect().width() / 2, joystickCircle->rect().height() / 2));
}

void HeavyTruck::updateJoystickCircle(int LRValue, int FBValue) {
    long scaledLRValue = (LRValue * ui->heavytruck_graphicsView->viewport()->rect().width()) / 65535;
    long scaledFBValue = (FBValue * ui->heavytruck_graphicsView->viewport()->rect().height()) / 65535;

    ui->heavytruck_graphicsView->setUpdatesEnabled(false);
    joystickCircle->setPos(QPoint(scaledLRValue, scaledFBValue) - QPointF(joystickCircle->rect().width() / 2, joystickCircle->rect().height() / 2));
    ui->heavytruck_graphicsView->setUpdatesEnabled(true);
}

void HeavyTruck::updateGearText(int button) {
    if (button) {
        ui->heavytruck_gearLabel->setText(QString::number(button));
    }
    else {
        ui->heavytruck_gearLabel->setText("N");
    }
}

void HeavyTruck::updateRpmDeltaText(float rpm) {
    ui->heavytruck_rpmDeltaLabel->setText(QString::number(int(rpm)) + " RPM");
    if (std::abs(rpm) <= ui->heavytruck_maxRevMatchRPMSlider->value()) {
        ui->heavytruck_revMatchIndicatorLabel->setText("🟢");
    }
    else {
        ui->heavytruck_revMatchIndicatorLabel->setText("🔴");
    }
}

void HeavyTruck::showAxisProgressBars() {
    ui->ioTabJoystickLRProgressBar->show();
    ui->ioTabJoystickFBProgressBar->show();
    ui->ioTabClutchProgressBar->show();
    ui->ioTabThrottleProgressBar->show();
}

void HeavyTruck::hideAxisProgressBars() {
    ui->ioTabJoystickLRProgressBar->hide();
    ui->ioTabJoystickFBProgressBar->hide();
    ui->ioTabClutchProgressBar->hide();
    ui->ioTabThrottleProgressBar->hide();
}

void HeavyTruck::saveSettings() {
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
    settings.setValue("invert_clutch_axis", ui->invertClutchAxisBox->isChecked());
    settings.setValue("throttle_axis", throttleAxisGuid.toString());
    settings.setValue("invert_throttle_axis", ui->invertThrottleAxisBox->isChecked());
    settings.endGroup();

    settings.beginGroup("vjoy");
    settings.setValue("vjoy_device", vjoy->getDeviceIndex());
    settings.endGroup();
}

void HeavyTruck::loadSettings() {
    qDebug() << "Loading settings";
    if (!QFile(deviceSettingsFile).exists()) {
        qDebug() << "Settings file does not exist";
        return;
    }
    QSettings settings = QSettings(deviceSettingsFile, QSettings::IniFormat);

    settings.beginGroup("joystick");
    int joystick_index = ui->heavytruck_joystickDeviceComboBox->findData(settings.value("device_guid").toUuid());
    if (joystick_index == -1 && !g_joystick_warned) {
        QMessageBox::warning(nullptr, "Joystick not found", "Saved heavy truck joystick device is not connected.\nReconnect the device or update the input/output settings.");
        g_joystick_warned = true;
    }
    else
    {
        ui->heavytruck_joystickDeviceComboBox->setCurrentIndex(joystick_index);
        ui->heavytruck_joystickLRAxisComboBox->setCurrentIndex(ui->heavytruck_joystickLRAxisComboBox->findData(settings.value("lr_axis").toUuid()));
        //ui.invertJoystickLRAxisBox->setChecked(settings.value("invert_lr_axis").toBool());
        ui->heavytruck_joystickFBAxisComboBox->setCurrentIndex(ui->heavytruck_joystickFBAxisComboBox->findData(settings.value("fb_axis").toUuid()));
        //ui.invertJoystickFBAxisBox->setChecked(settings.value("invert_fb_axis").toBool());
    }
    settings.endGroup();

    settings.beginGroup("pedals");
    int pedals_index = ui->heavytruck_pedalsDeviceComboBox->findData(settings.value("device_guid").toUuid());
    if (pedals_index == -1) {
        QMessageBox::warning(nullptr, "Pedals not found", "Saved pedals device is not connected.\nReconnect the device or update the input/output settings.");
    }
    else
    {
        ui->heavytruck_pedalsDeviceComboBox->setCurrentIndex(pedals_index);
        ui->heavytruck_clutchAxisComboBox->setCurrentIndex(ui->heavytruck_clutchAxisComboBox->findData(settings.value("clutch_axis").toUuid()));
        ui->invertClutchAxisBox->setChecked(settings.value("invert_clutch_axis").toBool());
        ui->heavytruck_throttleAxisComboBox->setCurrentIndex(ui->heavytruck_throttleAxisComboBox->findData(settings.value("throttle_axis").toUuid()));
        ui->invertThrottleAxisBox->setChecked(settings.value("invert_throttle_axis").toBool());
    }
    settings.endGroup();

    settings.beginGroup("vjoy");
    ui->heavytruck_vjoyDeviceComboBox->setCurrentIndex(settings.value("vjoy_device").toInt());
    settings.endGroup();
}

void HeavyTruck::changeJoystickDevice(int deviceIndex) {
    // Release previous joystick
    if (joystick != nullptr && joystick->isAcquired) {
        joystick->release();
    }
    QUuid deviceGuid = ui->heavytruck_joystickDeviceComboBox->currentData().toUuid();
    qDebug() << "Device UUID: " << deviceGuid;
    joystick = getDeviceFromGuid(deviceList, deviceGuid);
    qDebug() << "New joystick device: " << joystick->name;
    ui->heavytruck_joystickLRAxisComboBox->clear();
    ui->heavytruck_joystickFBAxisComboBox->clear();
    QMap<QUuid, QString> axisMap = joystick->getDeviceAxes();
    for (auto axis = axisMap.cbegin(), end = axisMap.cend(); axis != end; ++axis)
    {
        ui->heavytruck_joystickLRAxisComboBox->addItem(axis.value(), axis.key());
        ui->heavytruck_joystickFBAxisComboBox->addItem(axis.value(), axis.key());
    }
}

void HeavyTruck::changePedalsDevice(int deviceIndex) {
    if (pedals != nullptr && pedals->isAcquired) {
        pedals->release();
    }
    QUuid deviceGuid = ui->heavytruck_pedalsDeviceComboBox->currentData().toUuid();
    pedals = getDeviceFromGuid(deviceList, deviceGuid);
    pedals->acquire(&hwnd);
    qDebug() << "New pedals device acquired: " << pedals->name;

    ui->heavytruck_clutchAxisComboBox->clear();
    ui->heavytruck_throttleAxisComboBox->clear();
    QMap<QUuid, QString> axisMap = pedals->getDeviceAxes();
    for (auto axis = axisMap.cbegin(), end = axisMap.cend(); axis != end; ++axis)
    {
        ui->heavytruck_clutchAxisComboBox->addItem(axis.value(), axis.key());
        ui->heavytruck_throttleAxisComboBox->addItem(axis.value(), axis.key());
    }
}

void HeavyTruck::changeJoystickLRAxis(int axisIndex) {
    joystickLRAxisGuid = ui->heavytruck_joystickLRAxisComboBox->currentData().toUuid();
}

void HeavyTruck::changeJoystickFBAxis(int axisIndex) {
    joystickFBAxisGuid = ui->heavytruck_joystickFBAxisComboBox->currentData().toUuid();
}

void HeavyTruck::changeClutchAxis(int axisIndex) {
    clutchAxisGuid = ui->heavytruck_clutchAxisComboBox->currentData().toUuid();
}

void HeavyTruck::changeThrottleAxis(int axisIndex) {
    throttleAxisGuid = ui->heavytruck_throttleAxisComboBox->currentData().toUuid();
}

QPair<int, int> HeavyTruck::getJoystickValues() {
    joystick->updateState();
    long joystickLRValue = joystick->getAxisReading(joystickLRAxisGuid);
    long joystickFBValue = joystick->getAxisReading(joystickFBAxisGuid);
    emit joystickLRValueChanged(joystickLRValue);
    emit joystickFBValueChanged(joystickFBValue);
    emit joystickValueChanged(joystickLRValue, joystickFBValue);
    return QPair<int, int>(joystickLRValue, joystickFBValue);
}

QPair<int, int> HeavyTruck::getPedalValues() {
    pedals->updateState();
    long clutchValue = pedals->getAxisReading(clutchAxisGuid);
    if (ui->heavytruck_invertClutchAxisBox->isChecked()) {
        clutchValue = abs(65535 - clutchValue);
    }
    emit clutchValueChanged(clutchValue);
    long throttleValue = pedals->getAxisReading(throttleAxisGuid);
    if (ui->heavytruck_invertThrottleAxisBox->isChecked()) {
        throttleValue = abs(65535 - throttleValue);
    }
    emit throttleValueChanged(throttleValue);
    QPair<int, int> pedalValues = { clutchValue , throttleValue };
    if (lastPedalValues != pedalValues) {
        emit pedalValuesChanged(clutchValue, throttleValue);
        lastPedalValues = pedalValues;
    }
    return pedalValues;
}

HRESULT HeavyTruck::startGameLoop() {
    showAxisProgressBars();

    // Acquire joystick
    qDebug() << "Acquiring joystick for hshifter...";
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

    // Initialize FFB
    stateManager.start(telemetry, slot);
    slotGuard.start(joystick, slot);
    synchroGuard.start(joystick, slot, telemetry);
    joystick->startEffects();
    return S_OK;
}

void HeavyTruck::stopGameLoop() {
    hideAxisProgressBars();
    // Release devices
    vjoy->release();
    joystick->release();
}

void HeavyTruck::gameLoop() {
    if (pedals == nullptr || joystick == nullptr || !joystick->isAcquired || !pedals->isAcquired) {
        return;
    }

    // Get new joystick values
    QPair<int, int> joystickValues = getJoystickValues();

    // Get new pedal values
    QPair<int, int> pedalValues = getPedalValues();

    // Get telemetry values
    if (telemetry->isConnected() != TelemetrySource::NONE) {
        
        QPair<int, int> gearValues = telemetry->getGearState();
        if (gearValues != lastGearValues) {
            emit gearValuesChanged(gearValues);
            lastGearValues = gearValues;
        }
    }

    // Update state
    slotGuard.updateSlotGuardEffects(joystickValues);
    synchroGuard.updateTorqueLock(pedalValues, joystickValues);
    stateManager.update(joystickValues, pedalValues, lastGearValues);
}