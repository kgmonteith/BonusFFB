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
#include <QMessageBox>
#include <QSettings>
#include "HeavyTruck.h"

QString HeavyTruck::getAppName(bool readable) {
    if (readable)
        return "Heavy truck shifter";
    return "heavytruck";
}

void HeavyTruck::initialize() {
    // Set flags for required and desired devices
    appDeviceFlags = FLAG_DEVICES_REQUIRED | FLAG_DEVICES_PEDALS;

    ui->heavytruck_throttleOnShiftingLabel->setVisible(false);

    // UI connections
    connect(ui->heavytruck_setPresetEatonFullerButton, &QPushButton::clicked, this, &HeavyTruck::setPresetPatternEatonFuller);
    connect(ui->heavytruck_setPresetFullRangeButton, &QPushButton::clicked, this, &HeavyTruck::setPresetPatternFullRange);
    connect(ui->heavytruck_buttonZoneDepthSpinbox, &QSpinBox::valueChanged, slot, &SlotParameters::setButtonZoneDepth);
    // Graphics connections
    connect(ui->heavytruckTabWidget, &QTabWidget::currentChanged, this, &HeavyTruck::redrawJoystickMap);
    // Telemetry connections
    connect(telemetry, &Telemetry::telemetryChanged, &stateManager, &HeavyTruckStateManager::setTelemetryState);
    connect(this, &HeavyTruck::engineRPMChanged, &synchroGuard, &HeavyTruckSynchroGuard::updateEngineRPM);
    // Joystick connections
    connect(devices, &DeviceConfiguration::joystickFBValueChanged, &synchroGuard, &HeavyTruckSynchroGuard::setJoystickFBValue);
    connect(devices, &DeviceConfiguration::joystickValueChanged, this, &HeavyTruck::updateJoystickCircle);
    // Pedal connections
    connect(devices, &DeviceConfiguration::clutchValueChanged, ui->heavytruck_clutchProgressBar, &QProgressBar::setValue);
    connect(devices, &DeviceConfiguration::throttleValueChanged, ui->heavytruck_throttleProgressBar, &QProgressBar::setValue);
    // vJoy connections
    connect(&stateManager, &HeavyTruckStateManager::buttonZoneChanged, &devices->vjoy, &vJoyFeeder::updateButtons);
    connect(&stateManager, &HeavyTruckStateManager::buttonZoneChanged, this, &HeavyTruck::updateGearText);
    // FFB effect connections
    connect(&stateManager, &HeavyTruckStateManager::slotStateChanged, &slotGuard, &HeavyTruckSlotGuard::updateSlotGuardState);
    connect(&stateManager, &HeavyTruckStateManager::synchroStateChanged, &synchroGuard, &HeavyTruckSynchroGuard::synchroStateChanged);
    connect(&stateManager, &HeavyTruckStateManager::rpmDeltaChanged, &synchroGuard, &HeavyTruckSynchroGuard::updateGrindEffectRPM);
    connect(&stateManager, &HeavyTruckStateManager::rpmDeltaChanged, this, &HeavyTruck::updateRpmDeltaText);
    connect(&stateManager, &HeavyTruckStateManager::grindingStateChanged, &synchroGuard, &HeavyTruckSynchroGuard::grindingStateChanged);
    connect(&stateManager, &HeavyTruckStateManager::unblipThrottle, &pedalsManager, &PedalsManager::unblipThrottle);
    // FFB settings connections
    connect(ui->heavytruck_grindIntensitySlider, &QSlider::valueChanged, &synchroGuard, &HeavyTruckSynchroGuard::setGrindEffectIntensity);
    connect(ui->heavytruck_maxRevMatchRPMSlider, &QSlider::valueChanged, &synchroGuard, &HeavyTruckSynchroGuard::setMaxRevMatchRPM);
    //connect(ui->grindRPMSlider, &QSlider::valueChanged, &synchroGuard, &SynchroGuard::updateEngineRPM);
    connect(ui->heavytruck_grindEffectShapeComboBox, &QComboBox::currentIndexChanged, &synchroGuard, &HeavyTruckSynchroGuard::setGrindEffectShape);
    connect(ui->heavytruck_keepInGearIdleSlider, &QSlider::valueChanged, &synchroGuard, &HeavyTruckSynchroGuard::setKeepInGearIdleIntensity);
    connect(ui->heavytruck_torqueLoadStrengthSlider, &QSlider::valueChanged, &synchroGuard, &HeavyTruckSynchroGuard::setTorqueLoadStrength);
    connect(ui->heavytruck_engineVibrationStrengthSlider, &QSlider::valueChanged, &synchroGuard, &HeavyTruckSynchroGuard::setEngineVibrationIntensity);
    connect(ui->heavytruck_slotDepthSlider, &QSlider::valueChanged, this, &HeavyTruck::slotParameterChanged);
    connect(ui->heavytruck_centerSlotPositionSlider, &QSlider::valueChanged, this, &HeavyTruck::slotParameterChanged);
    connect(ui->heavytruck_rightSlotPositionSlider, &QSlider::valueChanged, this, &HeavyTruck::slotParameterChanged);
    connect(ui->heavytruck_slotRoundingFactorSlider, &QSlider::valueChanged, this, &HeavyTruck::slotParameterChanged);
    connect(ui->heavytruck_throttleOnShiftingCheckbox, &QCheckBox::toggled, &pedalsManager, &PedalsManager::toggleVirtualPedals);
}

void HeavyTruck::setPresetPatternEatonFuller() {
    ui->heavytruck_slotDepthSlider->setValue(75);
    ui->heavytruck_centerSlotPositionSlider->setValue(34);
    ui->heavytruck_rightSlotPositionSlider->setValue(66);
    ui->heavytruck_slotRoundingFactorSlider->setValue(10);
}


void HeavyTruck::setPresetPatternFullRange() {
    ui->heavytruck_slotDepthSlider->setValue(100);
    ui->heavytruck_centerSlotPositionSlider->setValue(50);
    ui->heavytruck_rightSlotPositionSlider->setValue(100);
    ui->heavytruck_slotRoundingFactorSlider->setValue(25);
}

void HeavyTruck::slotParameterChanged(int t) {
    // Either right or center slot position changed, so we need to update both
    slot->depth = (double)ui->heavytruck_slotDepthSlider->value() * .01;
    slot->pos_pct[1] = ui->heavytruck_centerSlotPositionSlider->value() * .01;
    slot->pos_pct[2] = ui->heavytruck_rightSlotPositionSlider->value() * .01;
    slot->rounding_factor = JOY_MAXPOINT * ui->heavytruck_slotRoundingFactorSlider->value() * 0.01;
}

void HeavyTruck::initializeJoystickMap() {
    scene = new QGraphicsScene();
    scene->setSceneRect(ui->heavytruck_graphicsView->viewport()->rect());

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

    redrawJoystickMap();
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
        grindZoneRect->show();
        buttonZoneRect->show();
    }
    else {
        grindZoneRect->hide();
        buttonZoneRect->hide();
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

void HeavyTruck::saveSettings(QSettings* settings) {
    BonusFFBApp::saveSettings(settings);

    settings->beginGroup(this->getAppName());

    settings->beginGroup("slot_pattern_settings");
    settings->setValue("slotDepth", ui->heavytruck_slotDepthSlider->value());
    settings->setValue("centerSlotPosition", ui->heavytruck_centerSlotPositionSlider->value());
    settings->setValue("rightSlotPosition", ui->heavytruck_rightSlotPositionSlider->value());
    settings->setValue("slotRoundingFactor", ui->heavytruck_slotRoundingFactorSlider ->value());
    settings->setValue("buttonZoneDepth", ui->heavytruck_buttonZoneDepthSpinbox->value());
    settings->setValue("displayZoneMarkers", ui->heavytruck_displayZoneMarkers->isChecked());
    settings->endGroup();

    settings->beginGroup("ffb_effect_settings");
    settings->setValue("grindIntensity", ui->heavytruck_grindIntensitySlider->value());
    settings->setValue("grindEffectShape", ui->heavytruck_grindEffectShapeComboBox->currentIndex());
    settings->setValue("keepInGearIdle", ui->heavytruck_keepInGearIdleSlider->value());
    settings->setValue("torqueLoadStrength", ui->heavytruck_torqueLoadStrengthSlider->value());
    settings->setValue("maxRevMatchRPM", ui->heavytruck_maxRevMatchRPMSlider->value());
    settings->setValue("engineVibrationStrength", ui->heavytruck_engineVibrationStrengthSlider->value());
    settings->endGroup();

    settings->endGroup();
}

void HeavyTruck::loadSettings(QSettings* settings) {
    settings->beginGroup(this->getAppName());

    settings->beginGroup("slot_pattern_settings");
    ui->heavytruck_slotDepthSlider->setValue(settings->value("slotDepth", 75).toInt());
    ui->heavytruck_centerSlotPositionSlider->setValue(settings->value("centerSlotPosition", 34).toInt());
    ui->heavytruck_rightSlotPositionSlider->setValue(settings->value("rightSlotPosition", 66).toInt());
    ui->heavytruck_slotRoundingFactorSlider->setValue(settings->value("slotRoundingFactor", 10).toInt());
    ui->heavytruck_buttonZoneDepthSpinbox->setValue(settings->value("buttonZoneDepth", 35).toInt());
    ui->heavytruck_displayZoneMarkers->setChecked(settings->value("displayZoneMarkers", false).toBool());
    settings->endGroup();

    settings->beginGroup("ffb_effect_settings");
    ui->heavytruck_grindIntensitySlider->setValue(settings->value("grindIntensity", 15).toInt());
    ui->heavytruck_grindEffectShapeComboBox->setCurrentIndex(settings->value("grindEffectShape", 0).toInt());
    ui->heavytruck_keepInGearIdleSlider->setValue(settings->value("keepInGearIdle", 30).toInt());
    ui->heavytruck_torqueLoadStrengthSlider->setValue(settings->value("torqueLoadStrength", 30).toInt());
    ui->heavytruck_engineVibrationStrengthSlider->setValue(settings->value("engineVibrationStrength", 0).toInt());
    ui->heavytruck_maxRevMatchRPMSlider->setValue(settings->value("maxRevMatchRPM", 90).toInt());
    settings->endGroup();

    settings->endGroup();
    redrawJoystickMap();
}

HRESULT HeavyTruck::startMode() {
    // Initialize FFB
    stateManager.start(telemetry, slot);
    slotGuard.start(devices->joystick, slot);
    synchroGuard.start(devices->joystick, slot, telemetry);
    pedalsManager.start(devices);

    return S_OK;
}

void HeavyTruck::gameLoop() {
    if (devices->pedals == nullptr || devices->joystick == nullptr || !devices->joystick->isAcquired || !devices->pedals->isAcquired) {
        return;
    }

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
    synchroGuard.updateTorqueLock(pedalValues, joystickValues);
    stateManager.update(joystickValues, lastGearValues);
    pedalsManager.updateVirtualPedals();
}