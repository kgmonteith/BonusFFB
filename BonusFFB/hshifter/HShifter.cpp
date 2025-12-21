/*
Copyright (C) 2024-2025 Ken Monteith.

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
#include "HShifter.h"

void HShifter::initialize() {
    // Menu action connections
    QObject::connect(ui->actionSaveSettings, &QAction::triggered, this, &HShifter::saveSettings);
    QObject::connect(ui->actionLoadSettings, &QAction::triggered, this, &HShifter::loadSettings);
    // Graphics connections
    QObject::connect(ui->hshifterTabWidget, &QTabWidget::currentChanged, this, &HShifter::redrawJoystickMap);
    // HShifter joystick connections
    QObject::connect(ui->hshifter_joystickDeviceComboBox, &QComboBox::currentIndexChanged, this, &HShifter::changeJoystickDevice);
    QObject::connect(ui->hshifter_joystickLRAxisComboBox, &QComboBox::currentIndexChanged, this, &HShifter::changeJoystickLRAxis);
    QObject::connect(ui->hshifter_joystickFBAxisComboBox, &QComboBox::currentIndexChanged, this, &HShifter::changeJoystickFBAxis);
    QObject::connect(this, &HShifter::joystickLRValueChanged, ui->ioTabJoystickLRProgressBar, &QProgressBar::setValue);
    QObject::connect(this, &HShifter::joystickFBValueChanged, ui->ioTabJoystickFBProgressBar, &QProgressBar::setValue);
    // HShifter pedal connections
    QObject::connect(ui->hshifter_pedalsDeviceComboBox, &QComboBox::currentIndexChanged, this, &HShifter::changePedalsDevice);
    QObject::connect(ui->hshifter_clutchAxisComboBox, &QComboBox::currentIndexChanged, this, &HShifter::changeClutchAxis);
    QObject::connect(ui->hshifter_throttleAxisComboBox, &QComboBox::currentIndexChanged, this, &HShifter::changeThrottleAxis);
    QObject::connect(this, &HShifter::clutchValueChanged, ui->ioTabClutchProgressBar, &QProgressBar::setValue);
    QObject::connect(this, &HShifter::throttleValueChanged, ui->ioTabThrottleProgressBar, &QProgressBar::setValue);
    // Telemetry connections
    QObject::connect(telemetry, &Telemetry::telemetryChanged, &stateManager, &HShifterStateManager::setTelemetryState);
    // Joystick connections
    QObject::connect(this, &HShifter::joystickValueChanged, this, &HShifter::updateJoystickCircle);
    // Pedal connections
    QObject::connect(this, &HShifter::clutchValueChanged, ui->clutchProgressBar, &QProgressBar::setValue);
    QObject::connect(this, &HShifter::throttleValueChanged, ui->throttleProgressBar, &QProgressBar::setValue);
    // vJoy connections
    QObject::connect(ui->hshifter_vjoyDeviceComboBox, &QComboBox::currentIndexChanged, vjoy, &vJoyFeeder::setDeviceIndex);
    QObject::connect(&stateManager, &HShifterStateManager::buttonZoneChanged, vjoy, &vJoyFeeder::updateButtons);
    QObject::connect(&stateManager, &HShifterStateManager::buttonZoneChanged, this, &HShifter::updateGearText);
    // FFB effect connections
    QObject::connect(&stateManager, &HShifterStateManager::slotStateChanged, &slotGuard, &HShifterSlotGuard::updateSlotGuardEffects);
    QObject::connect(this, &HShifter::pedalValuesChanged, &synchroGuard, &HShifterSynchroGuard::updatePedalEngagement);
    QObject::connect(&stateManager, &HShifterStateManager::synchroStateChanged, &synchroGuard, &HShifterSynchroGuard::synchroStateChanged);
    QObject::connect(this, &HShifter::engineRPMChanged, &synchroGuard, &HShifterSynchroGuard::updateEngineRPM);
    QObject::connect(&stateManager, &HShifterStateManager::grindingStateChanged, &synchroGuard, &HShifterSynchroGuard::grindingStateChanged);
    QObject::connect(ui->grindIntensitySlider, &QSlider::valueChanged, &synchroGuard, &HShifterSynchroGuard::setGrindEffectIntensity);
    QObject::connect(ui->grindRPMSlider, &QSlider::valueChanged, &synchroGuard, &HShifterSynchroGuard::updateGrindEffectRPM);
    //QObject::connect(ui->grindRPMSlider, &QSlider::valueChanged, &synchroGuard, &SynchroGuard::updateEngineRPM);
    QObject::connect(ui->grindEffectBehaviorComboBox, &QComboBox::currentIndexChanged, &synchroGuard, &HShifterSynchroGuard::setGrindEffectBehavior);
    QObject::connect(ui->keepInGearIdleSlider, &QSlider::valueChanged, &synchroGuard, &HShifterSynchroGuard::setKeepInGearIdleIntensity);

    // Populate the device lists
    for (const DeviceInfo& device : *deviceList)
    {
        ui->hshifter_pedalsDeviceComboBox->addItem(device.name, device.instanceGuid);
        if (device.supportsFfb && device.productGuid.data1 != VJOY_PRODUCT_GUID) {
            ui->hshifter_joystickDeviceComboBox->addItem(device.name, device.instanceGuid);
        }
    }

    // Populate vJoy combo boxes
    if (vJoyFeeder::isDriverEnabled()) {
        for (int i = 0; i < vJoyFeeder::deviceCount(); i++) {
            ui->hshifter_vjoyDeviceComboBox->addItem(QString("vJoy Device ").append(QString(" %1").arg(i + 1)), i + 1);
        }
    }

    // Start with axis progress bars hidden
    hideAxisProgressBars();
}

void HShifter::initializeJoystickMap() {
    scene = new QGraphicsScene();
    scene->setSceneRect(ui->hshifter_graphicsView->viewport()->rect());

    long sceneWidth = ui->hshifter_graphicsView->viewport()->rect().width();
    long sceneHeight = ui->hshifter_graphicsView->viewport()->rect().height();
    QPointF center = scene->sceneRect().center();

    neutralChannelRect = new QGraphicsRectItem(0, 0, sceneWidth, SLOT_WIDTH_PX);
    neutralChannelRect->setBrush(QBrush(Qt::black));
    neutralChannelRect->setPen(Qt::NoPen);
    neutralChannelRect->setPos(center - QPointF(sceneWidth / 2, SLOT_WIDTH_PX / 2));
    scene->addItem(neutralChannelRect);

    centerSlotRect = new QGraphicsRectItem(0, 0, SLOT_WIDTH_PX, sceneHeight);
    centerSlotRect->setBrush(QBrush(Qt::black));
    centerSlotRect->setPen(Qt::NoPen);
    centerSlotRect->setPos(center - QPointF(SLOT_WIDTH_PX / 2, sceneHeight / 2));
    scene->addItem(centerSlotRect);

    rightSlotRect = new QGraphicsRectItem(0, 0, SLOT_WIDTH_PX, sceneHeight);
    rightSlotRect->setBrush(QBrush(Qt::black));
    rightSlotRect->setPen(Qt::NoPen);
    rightSlotRect->setPos(QPointF(sceneWidth - SLOT_WIDTH_PX, 0));
    scene->addItem(rightSlotRect);

    leftSlotRect = new QGraphicsRectItem(0, 0, SLOT_WIDTH_PX, sceneHeight);
    leftSlotRect->setBrush(QBrush(Qt::black));
    leftSlotRect->setPen(Qt::NoPen);
    leftSlotRect->setPos(QPointF(0, 0));
    scene->addItem(leftSlotRect);

    joystickCircle = new QGraphicsEllipseItem(0, 0, JOYSTICK_MARKER_DIAMETER_PX, JOYSTICK_MARKER_DIAMETER_PX);
    QColor seethroughWhite = Qt::transparent;
    seethroughWhite.setAlphaF(float(0.15));
    joystickCircle->setBrush(QBrush(seethroughWhite));
    joystickCircle->setPen(QPen(QColor(1, 129, 231), 7));
    QPointF circlePos = center - QPointF(JOYSTICK_MARKER_DIAMETER_PX / 2.0, JOYSTICK_MARKER_DIAMETER_PX / 2.0);
    joystickCircle->setPos(circlePos);
    scene->addItem(joystickCircle);

    ui->hshifter_graphicsView->setScene(scene);
    ui->hshifter_graphicsView->setRenderHints(QPainter::Antialiasing);
    ui->hshifter_graphicsView->show();
}


// Separate call because the event doesn't trigger if another tab is active
void HShifter::redrawJoystickMap() {
    if (scene == nullptr) {
        return;
    }
    ui->hshifter_graphicsView->scene()->setSceneRect(ui->hshifter_graphicsView->viewport()->rect());

    long sceneWidth = ui->hshifter_graphicsView->viewport()->rect().width();
    long sceneHeight = ui->hshifter_graphicsView->viewport()->rect().height();
    QPointF center = scene->sceneRect().center();

    neutralChannelRect->setRect(0, 0, sceneWidth, SLOT_WIDTH_PX);
    neutralChannelRect->setPos(center - QPointF(sceneWidth / 2, SLOT_WIDTH_PX / 2));

    centerSlotRect->setRect(0, 0, SLOT_WIDTH_PX, sceneHeight);
    centerSlotRect->setPos(center - QPointF(SLOT_WIDTH_PX / 2, sceneHeight / 2));

    rightSlotRect->setRect(0, 0, SLOT_WIDTH_PX, sceneHeight);
    rightSlotRect->setPos(QPointF(sceneWidth - SLOT_WIDTH_PX, 0));

    leftSlotRect->setRect(0, 0, SLOT_WIDTH_PX, sceneHeight);

    joystickCircle->setPos(center - QPointF(joystickCircle->rect().width() / 2, joystickCircle->rect().height() / 2));
}

void HShifter::updateJoystickCircle(int LRValue, int FBValue) {
    long scaledLRValue = (LRValue * ui->hshifter_graphicsView->viewport()->rect().width()) / 65535;
    long scaledFBValue = (FBValue * ui->hshifter_graphicsView->viewport()->rect().height()) / 65535;

    ui->hshifter_graphicsView->setUpdatesEnabled(false);
    joystickCircle->setPos(QPoint(scaledLRValue, scaledFBValue) - QPointF(joystickCircle->rect().width() / 2, joystickCircle->rect().height() / 2));
    ui->hshifter_graphicsView->setUpdatesEnabled(true);
}

void HShifter::updateGearText(int button) {
    if (button) {
        ui->gearLabel->setText(QString::number(button));
    }
    else {
        ui->gearLabel->setText("N");
    }
}

void HShifter::showAxisProgressBars() {
    ui->ioTabJoystickLRProgressBar->show();
    ui->ioTabJoystickFBProgressBar->show();
    ui->ioTabClutchProgressBar->show();
    ui->ioTabThrottleProgressBar->show();
}

void HShifter::hideAxisProgressBars() {
    ui->ioTabJoystickLRProgressBar->hide();
    ui->ioTabJoystickFBProgressBar->hide();
    ui->ioTabClutchProgressBar->hide();
    ui->ioTabThrottleProgressBar->hide();
}

void HShifter::saveSettings() {
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

void HShifter::loadSettings() {
    qDebug() << "Loading settings";
    if (!QFile(deviceSettingsFile).exists()) {
        qDebug() << "Settings file does not exist";
        return;
    }
    QSettings settings = QSettings(deviceSettingsFile, QSettings::IniFormat);

    settings.beginGroup("joystick");
    int joystick_index = ui->hshifter_joystickDeviceComboBox->findData(settings.value("device_guid").toUuid());
    if (joystick_index == -1 && !g_joystick_warned) {
        QMessageBox::warning(nullptr, "Joystick not found", "Saved H-shifter joystick device is not connected.\nReconnect the device or update the input/output settings.");
        g_joystick_warned = true;
    }
    else
    {
        ui->hshifter_joystickDeviceComboBox->setCurrentIndex(joystick_index);
        ui->hshifter_joystickLRAxisComboBox->setCurrentIndex(ui->hshifter_joystickLRAxisComboBox->findData(settings.value("lr_axis").toUuid()));
        //ui.invertJoystickLRAxisBox->setChecked(settings.value("invert_lr_axis").toBool());
        ui->hshifter_joystickFBAxisComboBox->setCurrentIndex(ui->hshifter_joystickFBAxisComboBox->findData(settings.value("fb_axis").toUuid()));
        //ui.invertJoystickFBAxisBox->setChecked(settings.value("invert_fb_axis").toBool());
    }
    settings.endGroup();

    settings.beginGroup("pedals");
    int pedals_index = ui->hshifter_pedalsDeviceComboBox->findData(settings.value("device_guid").toUuid());
    if (pedals_index == -1) {
        QMessageBox::warning(nullptr, "Pedals not found", "Saved pedals device is not connected.\nReconnect the device or update the input/output settings.");
    }
    else
    {
        ui->hshifter_pedalsDeviceComboBox->setCurrentIndex(pedals_index);
        ui->hshifter_clutchAxisComboBox->setCurrentIndex(ui->hshifter_clutchAxisComboBox->findData(settings.value("clutch_axis").toUuid()));
        ui->invertClutchAxisBox->setChecked(settings.value("invert_clutch_axis").toBool());
        ui->hshifter_throttleAxisComboBox->setCurrentIndex(ui->hshifter_throttleAxisComboBox->findData(settings.value("throttle_axis").toUuid()));
        ui->invertThrottleAxisBox->setChecked(settings.value("invert_throttle_axis").toBool());
    }
    settings.endGroup();

    settings.beginGroup("vjoy");
    ui->hshifter_vjoyDeviceComboBox->setCurrentIndex(settings.value("vjoy_device").toInt());
    settings.endGroup();
}

void HShifter::changeJoystickDevice(int deviceIndex) {
    // Release previous joystick
    if (joystick != nullptr && joystick->isAcquired) {
        joystick->release();
    }
    QUuid deviceGuid = ui->hshifter_joystickDeviceComboBox->currentData().toUuid();
    qDebug() << "Device UUID: " << deviceGuid;
    joystick = getDeviceFromGuid(deviceList, deviceGuid);
    qDebug() << "New joystick device: " << joystick->name;
    ui->hshifter_joystickLRAxisComboBox->clear();
    ui->hshifter_joystickFBAxisComboBox->clear();
    QMap<QUuid, QString> axisMap = joystick->getDeviceAxes();
    for (auto axis = axisMap.cbegin(), end = axisMap.cend(); axis != end; ++axis)
    {
        ui->hshifter_joystickLRAxisComboBox->addItem(axis.value(), axis.key());
        ui->hshifter_joystickFBAxisComboBox->addItem(axis.value(), axis.key());
    }
}

void HShifter::changePedalsDevice(int deviceIndex) {
    if (pedals != nullptr && pedals->isAcquired) {
        pedals->release();
    }
    QUuid deviceGuid = ui->hshifter_pedalsDeviceComboBox->currentData().toUuid();
    pedals = getDeviceFromGuid(deviceList, deviceGuid);
    pedals->acquire(&hwnd);
    qDebug() << "New pedals device acquired: " << pedals->name;

    ui->hshifter_clutchAxisComboBox->clear();
    ui->hshifter_throttleAxisComboBox->clear();
    QMap<QUuid, QString> axisMap = pedals->getDeviceAxes();
    for (auto axis = axisMap.cbegin(), end = axisMap.cend(); axis != end; ++axis)
    {
        ui->hshifter_clutchAxisComboBox->addItem(axis.value(), axis.key());
        ui->hshifter_throttleAxisComboBox->addItem(axis.value(), axis.key());
    }
}

void HShifter::changeJoystickLRAxis(int axisIndex) {
    joystickLRAxisGuid = ui->hshifter_joystickLRAxisComboBox->currentData().toUuid();
}

void HShifter::changeJoystickFBAxis(int axisIndex) {
    joystickFBAxisGuid = ui->hshifter_joystickFBAxisComboBox->currentData().toUuid();
}

void HShifter::changeClutchAxis(int axisIndex) {
    clutchAxisGuid = ui->hshifter_clutchAxisComboBox->currentData().toUuid();
}

void HShifter::changeThrottleAxis(int axisIndex) {
    throttleAxisGuid = ui->hshifter_throttleAxisComboBox->currentData().toUuid();
}

QPair<int, int> HShifter::getJoystickValues() {
    joystick->updateState();
    long joystickLRValue = joystick->getAxisReading(joystickLRAxisGuid);
    long joystickFBValue = joystick->getAxisReading(joystickFBAxisGuid);
    emit joystickLRValueChanged(joystickLRValue);
    emit joystickFBValueChanged(joystickFBValue);
    emit joystickValueChanged(joystickLRValue, joystickFBValue);
    return QPair<int, int>(joystickLRValue, joystickFBValue);
}

QPair<int, int> HShifter::getPedalValues() {
    pedals->updateState();
    long clutchValue = pedals->getAxisReading(clutchAxisGuid);
    if (ui->invertClutchAxisBox->isChecked()) {
        clutchValue = abs(65535 - clutchValue);
    }
    emit clutchValueChanged(clutchValue);
    long throttleValue = pedals->getAxisReading(throttleAxisGuid);
    if (ui->invertThrottleAxisBox->isChecked()) {
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

HRESULT HShifter::startGameLoop() {
    showAxisProgressBars();

    // Acquire joystick
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

    // Initialize FFB
    slotGuard.start(joystick);
    synchroGuard.start(joystick);
    joystick->startEffects();
    return S_OK;
}

void HShifter::stopGameLoop() {
    hideAxisProgressBars();
    // Release devices
    vjoy->release();
    joystick->release();
}

void HShifter::gameLoop() {
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
        float engineRPM = telemetry->getEngineRPM();
        if (engineRPM != lastEngineRPM) {
            emit engineRPMChanged(engineRPM);
            lastEngineRPM = engineRPM;
        }
    }

    // Update state
    stateManager.update(joystickValues, pedalValues, lastGearValues);
}