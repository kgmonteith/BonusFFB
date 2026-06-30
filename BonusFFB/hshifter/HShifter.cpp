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
#include "HShifter.h"

QString HShifter::getAppName(bool readable) {
    if (readable)
        return "H-pattern shifter";
    return "hshifter";
}

void HShifter::initialize() {
    // Set flags for required and desired devices
    appDeviceFlags = FLAG_DEVICES_REQUIRED | FLAG_DEVICES_THROTTLE | FLAG_DEVICES_CLUTCH;
    if (devices->brake != nullptr)
        appDeviceFlags |= FLAG_DEVICES_BRAKE;

    // Graphics connections
    QObject::connect(ui->hshifterTabWidget, &QTabWidget::currentChanged, this, &HShifter::redrawJoystickMap);
    // Telemetry connections
    QObject::connect(telemetry, &Telemetry::telemetryChanged, &stateManager, &HShifterStateManager::setTelemetryState);
    // Joystick connections
    QObject::connect(devices, &DeviceConfiguration::joystickValueChanged, this, &HShifter::updateJoystickCircle);
    // Pedal connections
    QObject::connect(devices, &DeviceConfiguration::clutchValueChanged, ui->clutchProgressBar, &QProgressBar::setValue);
    QObject::connect(devices, &DeviceConfiguration::throttleValueChanged, ui->throttleProgressBar, &QProgressBar::setValue);
    // vJoy connections
    QObject::connect(&stateManager, &HShifterStateManager::buttonZoneChanged, &devices->vjoy, &vJoyFeeder::updateButtons);
    QObject::connect(&stateManager, &HShifterStateManager::buttonZoneChanged, this, &HShifter::updateGearText);
    // FFB effect connections
    QObject::connect(&stateManager, &HShifterStateManager::slotStateChanged, &slotGuard, &HShifterSlotGuard::updateSlotGuardState);
    QObject::connect(&stateManager, &HShifterStateManager::synchroStateChanged, &synchroGuard, &HShifterSynchroGuard::synchroStateChanged);
    QObject::connect(this, &HShifter::engineRPMChanged, &synchroGuard, &HShifterSynchroGuard::updateEngineRPM);
    QObject::connect(&stateManager, &HShifterStateManager::grindingStateChanged, &synchroGuard, &HShifterSynchroGuard::grindingStateChanged);
    QObject::connect(ui->grindIntensitySlider, &QSlider::valueChanged, &synchroGuard, &HShifterSynchroGuard::setGrindEffectIntensity);
    QObject::connect(ui->grindRPMSlider, &QSlider::valueChanged, &synchroGuard, &HShifterSynchroGuard::updateGrindEffectRPM);
    //QObject::connect(ui->grindRPMSlider, &QSlider::valueChanged, &synchroGuard, &SynchroGuard::updateEngineRPM);
    QObject::connect(ui->grindEffectBehaviorComboBox, &QComboBox::currentIndexChanged, &synchroGuard, &HShifterSynchroGuard::setGrindEffectBehavior);
    QObject::connect(ui->keepInGearIdleSlider, &QSlider::valueChanged, &synchroGuard, &HShifterSynchroGuard::setKeepInGearIdleIntensity);
}

void HShifter::initializeJoystickMap() {
    scene = new QGraphicsScene();
    scene->setSceneRect(ui->hshifter_graphicsView->viewport()->rect());

    long sceneWidth = ui->hshifter_graphicsView->viewport()->rect().width();
    long sceneHeight = ui->hshifter_graphicsView->viewport()->rect().height();
    QPointF center = scene->sceneRect().center();

    neutralChannelRect = new QGraphicsRectItem();
    neutralChannelRect->setBrush(QBrush(Qt::black));
    neutralChannelRect->setPen(Qt::NoPen);
    scene->addItem(neutralChannelRect);

    centerSlotRect = new QGraphicsRectItem();
    centerSlotRect->setBrush(QBrush(Qt::black));
    centerSlotRect->setPen(Qt::NoPen);
    scene->addItem(centerSlotRect);

    rightSlotRect = new QGraphicsRectItem();
    rightSlotRect->setBrush(QBrush(Qt::black));
    rightSlotRect->setPen(Qt::NoPen);
    scene->addItem(rightSlotRect);

    leftSlotRect = new QGraphicsRectItem();
    leftSlotRect->setBrush(QBrush(Qt::black));
    leftSlotRect->setPen(Qt::NoPen);
    scene->addItem(leftSlotRect);

    joystickCircle = new QGraphicsEllipseItem(0, 0, JOYSTICK_MARKER_DIAMETER_PX, JOYSTICK_MARKER_DIAMETER_PX);
    QColor seethroughWhite = Qt::transparent;
    seethroughWhite.setAlphaF(float(0.15));
    joystickCircle->setBrush(QBrush(seethroughWhite));
    joystickCircle->setPen(QPen(QColor(1, 129, 231), 7));
    scene->addItem(joystickCircle);

    ui->hshifter_graphicsView->setScene(scene);
    ui->hshifter_graphicsView->setRenderHints(QPainter::Antialiasing);
    ui->hshifter_graphicsView->show();

    redrawJoystickMap();
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

void HShifter::saveSettings(QSettings* settings) {
    BonusFFBApp::saveSettings(settings);

    settings->beginGroup(this->getAppName());

    settings->beginGroup("ffb_effect_settings");
    settings->setValue("grindIntensity", ui->grindIntensitySlider->value());
    settings->setValue("grindEffectBehavior", ui->grindEffectBehaviorComboBox->currentIndex());
    settings->setValue("grindEffectRPM", ui->grindRPMSlider->value());
    settings->setValue("idleLockIntensity", ui->keepInGearIdleSlider->value());
    settings->endGroup();

    settings->endGroup();
}

void HShifter::loadSettings(QSettings* settings) {
    BonusFFBApp::loadSettings(settings);

    settings->beginGroup(this->getAppName());

    settings->beginGroup("ffb_effect_settings");
    ui->grindIntensitySlider->setValue(settings->value("grindIntensity", 15).toInt());
    ui->grindEffectBehaviorComboBox->setCurrentIndex(settings->value("grindEffectBehavior", 0).toInt());
    ui->grindRPMSlider->setValue(settings->value("grindEffectRPM", 3000).toInt());
    ui->keepInGearIdleSlider->setValue(settings->value("idleLockIntensity", 25).toInt());
    settings->endGroup();

    settings->endGroup();
}

HRESULT HShifter::startMode() {
    // Initialize FFB
    slotGuard.start(devices->joystick);
    synchroGuard.start(devices->joystick);
    pedalsManager.start(devices);

    return S_OK;
}

void HShifter::gameLoop() {
    // Get new joystick values
    QPair<int, int> joystickValues = devices->getJoystickValues();

    // Get new pedal values
    PedalValues pedalValues = devices->getPedalValues();

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
    slotGuard.updateSlotGuardEffects(joystickValues);
    synchroGuard.updatePedalEngagement(pedalValues, joystickValues);
    stateManager.update(joystickValues, pedalValues, lastGearValues);
    pedalsManager.updateVirtualPedals();
}