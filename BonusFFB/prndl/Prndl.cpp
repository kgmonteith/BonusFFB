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
#include "Prndl.h"

QString Prndl::getAppName(bool readable) {
    if (readable)
        return "PRNDL shifter";
    return "prndl";
}

void Prndl::initialize() {
    // Set flags for required and desired devices
    appDeviceFlags = FLAG_DEVICES_REQUIRED | FLAG_DEVICES_SHIFTLOCK;
    if (devices->throttle != nullptr)
        appDeviceFlags |= FLAG_DEVICES_THROTTLE;
    if (devices->brake != nullptr)
        appDeviceFlags |= FLAG_DEVICES_BRAKE;
    if (devices->clutch != nullptr)
        appDeviceFlags |= FLAG_DEVICES_CLUTCH;

    // Graphics connections
    QObject::connect(ui->prndlTabWidget, &QTabWidget::currentChanged, this, &Prndl::redrawJoystickMap);
    QObject::connect(&stateManager, &PrndlStateManager::slotChanged, this, &Prndl::changeSlotLabel);
    // Joystick connections
    QObject::connect(devices, &DeviceConfiguration::joystickValueChanged, this, &Prndl::updateJoystickCircle);
    QObject::connect(&stateManager, &PrndlStateManager::slotSpringChanged, &slotGuard, &PrndlSlotGuard::updateSlotSpringCenter);
    // vJoy connections
    QObject::connect(&stateManager, &PrndlStateManager::buttonZoneChanged, &devices->vjoy, &vJoyFeeder::updateButtons);
    QObject::connect(&stateManager, &PrndlStateManager::buttonShortPress, &devices->vjoy, &vJoyFeeder::shortPressButton);
    // Shift lock connections
    QObject::connect(&stateManager, &PrndlStateManager::updateShiftLockEffectStrength, &slotGuard, &PrndlSlotGuard::updateShiftLockEffectStrength);
    QObject::connect(ui->prndl_shiftLockFromNeutralToReverseCheckBox, &QCheckBox::checkStateChanged, &stateManager, &PrndlStateManager::toggleLockShiftsFromNeutralToReverse);
    // Additional settings connections
    QObject::connect(ui->prndl_enableParkSlotCheckBox, &QCheckBox::checkStateChanged, &stateManager, &PrndlStateManager::toggleParkSlot);
    QObject::connect(ui->prndl_enableLowSlotCheckBox, &QCheckBox::checkStateChanged, &stateManager, &PrndlStateManager::toggleLastSlot);
    QObject::connect(ui->prndl_simulateParkUsingTelemetryCheckBox, &QCheckBox::checkStateChanged, &stateManager, &PrndlStateManager::toggleAtsTelemetryPark);

    if (devices->shiftLockDevice != nullptr) {
        ui->prndl_shiftLockButtonMonitorLabel->setText("⭕");
        stateManager.toggleUsingShiftLock(true);
    }
}

void Prndl::initializeJoystickMap() {
    scene = new QGraphicsScene();
    scene->setSceneRect(ui->prndl_graphicsView->viewport()->rect());

    long sceneWidth = ui->prndl_graphicsView->viewport()->rect().width();
    long sceneHeight = ui->prndl_graphicsView->viewport()->rect().height();

    centerSlotRect = new QGraphicsRectItem();
    centerSlotRect->setBrush(QBrush(Qt::black));
    centerSlotRect->setPen(Qt::NoPen);
    scene->addItem(centerSlotRect);

    int slotCenter = 0;
    int slotCircleStepSize = sceneHeight / (stateManager.getEnabledSlotCount() - 1);
    for (int i = 0; i < stateManager.getEnabledSlotCount(); i++) {
        QGraphicsEllipseItem* scPtr = new QGraphicsEllipseItem(0, 0, SHIFTER_POSITION_MARKER_DIAMETER_PX, SHIFTER_POSITION_MARKER_DIAMETER_PX);
        scPtr->setBrush(QBrush(Qt::black));
        scPtr->setPen(Qt::NoPen);
        scene->addItem(scPtr);
        slotCircles.push_back(scPtr);
    }

    joystickCircle = new QGraphicsEllipseItem(0, 0, JOYSTICK_MARKER_DIAMETER_PX, JOYSTICK_MARKER_DIAMETER_PX);
    QColor seethroughWhite = Qt::transparent;
    seethroughWhite.setAlphaF(float(0.15));
    joystickCircle->setBrush(QBrush(seethroughWhite));
    joystickCircle->setPen(QPen(QColor(1, 129, 231), 7));
    scene->addItem(joystickCircle);

    ui->prndl_graphicsView->setScene(scene);
    ui->prndl_graphicsView->setRenderHints(QPainter::Antialiasing);
    ui->prndl_graphicsView->show();

    redrawJoystickMap();
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

void Prndl::saveSettings(QSettings* settings) {
    BonusFFBApp::saveSettings(settings);

    settings->beginGroup(this->getAppName());

    settings->beginGroup("other_settings");
    settings->setValue("enable_park_slot", ui->prndl_enableParkSlotCheckBox->isChecked());
    settings->setValue("enable_low_slot", ui->prndl_enableLowSlotCheckBox->isChecked());
    settings->setValue("simulate_park_atsets2", ui->prndl_simulateParkUsingTelemetryCheckBox->isChecked());
    settings->setValue("shift_lock_neutral_reverse", ui->prndl_shiftLockFromNeutralToReverseCheckBox->isChecked());
    settings->endGroup();

    settings->endGroup();
}

void Prndl::loadSettings(QSettings* settings) {
    settings->beginGroup(this->getAppName());

    settings->beginGroup("other_settings");
    ui->prndl_enableParkSlotCheckBox->setChecked(settings->value("enable_park_slot", true).toBool());
    ui->prndl_enableLowSlotCheckBox->setChecked(settings->value("enable_low_slot", true).toBool());
    ui->prndl_simulateParkUsingTelemetryCheckBox->setChecked(settings->value("simulate_park_atsets2", true).toBool());
    ui->prndl_shiftLockFromNeutralToReverseCheckBox->setChecked(settings->value("shift_lock_neutral_reverse", true).toBool());
    settings->endGroup();

    settings->endGroup();
    qDebug() << "Succesfully loaded PRNDL settings";
}

bool Prndl::getShiftLockReleased() {
    if (devices->shiftLockDevice == nullptr || !devices->shiftLockDevice->isAcquired) {
        return false;
    }
    devices->shiftLockDevice->updateState();
    bool shiftLockReleased = devices->shiftLockDevice->isButtonPressed(devices->shiftLockButton);
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

HRESULT Prndl::startMode() {
    if (FAILED(slotGuard.start(devices->joystick))) {
        qDebug() << "Failed to start slotGuard effects";
    }
    pedalsManager.start(devices);
    return S_OK;
}

void Prndl::gameLoop() {
    if (devices->joystick == nullptr || !devices->joystick->isAcquired ) {
        return;
    }
    // Get new joystick values
    QPair<int, int> joystickValues = devices->getJoystickValues();
    bool isShiftLockRelased = getShiftLockReleased();

    // Get telemetry values
    bool isParkingBrakeSet = false;
    if (telemetry->isConnected() != TelemetrySource::NONE) {
        isParkingBrakeSet = telemetry->getParkingBrakeState();
    }

    stateManager.update(joystickValues, isShiftLockRelased, isParkingBrakeSet);
    slotGuard.updateLRSpring(joystickValues.first);
    pedalsManager.updateVirtualPedals();
}