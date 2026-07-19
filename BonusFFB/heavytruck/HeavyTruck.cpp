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
    appDeviceFlags = FLAG_DEVICES_REQUIRED | FLAG_DEVICES_THROTTLE | FLAG_DEVICES_CLUTCH | FLAG_DEVICES_RANGE | FLAG_DEVICES_SPLITTER;
    if (devices->brake != nullptr)
        appDeviceFlags |= FLAG_DEVICES_BRAKE;

    // Set default slot pattern
    slotPattern.setTruckPattern(0);

    // Slot pattern connections
    connect(ui->heavytruck_slotPatternComboBox, &QComboBox::currentIndexChanged, &slotPattern, &SlotPattern::setTruckPattern);
    connect(ui->heavytruck_slotPatternLeftOffsetSlider, &QSlider::valueChanged, &slotPattern, &SlotPattern::setLeftOffset);
    connect(ui->heavytruck_slotPatternDepthScaleSlider, &QSlider::valueChanged, &slotPattern, &SlotPattern::setDepthScale);
    connect(ui->heavytruck_slotPatternWidthScaleSlider, &QSlider::valueChanged, &slotPattern, &SlotPattern::setWidthScale);
    connect(ui->heavytruck_grindZoneDepthSpinbox, &QSpinBox::valueChanged, &slotPattern, &SlotPattern::setGrindZoneScale);
    connect(ui->heavytruck_buttonZoneDepthSpinbox, &QSpinBox::valueChanged, &slotPattern, &SlotPattern::setButtonZoneScale);
    connect(ui->heavytruck_slotRoundingFactorSlider, &QSlider::valueChanged, &slotPattern, &SlotPattern::setRoundingFactor);
    connect(&slotPattern, &SlotPattern::setRangeOverride, devices, &DeviceConfiguration::setRangeOverride);
    connect(&slotGuard, &HeavyTruckSlotGuard::forceRangeValue, devices, &DeviceConfiguration::forceRange);
    connect(ui->heavytruck_neutralSpringStrengthSlider, &QSlider::valueChanged, &slotGuard, &HeavyTruckSlotGuard::setNeutralSpringStrength);
    connect(ui->heavytruck_neutralSpringPositionSlider, &QSlider::valueChanged, &slotGuard, &HeavyTruckSlotGuard::setNeutralSpringPosition);
    // UI connections
    connect(&stateManager, &HeavyTruckStateManager::targetGearChanged, this, &HeavyTruck::updateGearText);
    connect(devices, &DeviceConfiguration::rangeChanged, this, &HeavyTruck::updateRangeText);
    connect(devices, &DeviceConfiguration::splitterChanged, this, &HeavyTruck::updateSplitterText);
    // Graphics connections
    connect(ui->heavytruckTabWidget, &QTabWidget::currentChanged, this, &HeavyTruck::redrawJoystickMap);
    // Telemetry connections
    connect(telemetry, &Telemetry::telemetryChanged, &stateManager, &HeavyTruckStateManager::setTelemetryState);
    connect(&stateManager, &HeavyTruckStateManager::engineRPMChanged, &synchroGuard, &HeavyTruckSynchroGuard::updateEngineRPM);
    // Joystick connections
    connect(devices, &DeviceConfiguration::joystickValueChanged, this, &HeavyTruck::updateJoystickCircle);
    // Pedal connections
    connect(devices, &DeviceConfiguration::clutchValueChanged, ui->heavytruck_clutchProgressBar, &QProgressBar::setValue);
    connect(devices, &DeviceConfiguration::throttleValueChanged, ui->heavytruck_throttleProgressBar, &QProgressBar::setValue);
    // vJoy connections
    connect(&stateManager, &HeavyTruckStateManager::buttonZoneChanged, &devices->vjoy, &vJoyFeeder::updateButtons);
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
    //connect(ui->heavytruck_slotDepthSlider, &QSlider::valueChanged, this, &HeavyTruck::slotParameterChanged);
    //connect(ui->heavytruck_centerSlotPositionSlider, &QSlider::valueChanged, this, &HeavyTruck::slotParameterChanged);
    //connect(ui->heavytruck_rightSlotPositionSlider, &QSlider::valueChanged, this, &HeavyTruck::slotParameterChanged);
    connect(ui->heavytruck_throttleOnShiftingCheckbox, &QCheckBox::toggled, &pedalsManager, &PedalsManager::toggleVirtualPedals);
}

void HeavyTruck::initializeJoystickMap() {
    scene = new QGraphicsScene();
    scene->setSceneRect(ui->heavytruck_graphicsView->viewport()->rect());
    slotPattern.setScene(scene);

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

    slotPattern.renderScene();


    if (ui->heavytruck_displayZoneMarkers->isChecked()) {
        grindZoneRect->setRect(-2, (scene->height() / 2) - (scene->height() / 2 * slotPattern.grind_zone_scale), scene->width() + 4, scene->height() * slotPattern.grind_zone_scale);
        buttonZoneRect->setRect(-2, (scene->height() / 2) - (scene->height() / 2 * slotPattern.button_zone_scale), scene->width() + 4, scene->height() * slotPattern.button_zone_scale);
        grindZoneRect->show();
        buttonZoneRect->show();
    }
    else {
        grindZoneRect->hide();
        buttonZoneRect->hide();
    }

    joystickCircle->setPos(scene->sceneRect().center() - QPointF(joystickCircle->rect().width() / 2, joystickCircle->rect().height() / 2));
}

void HeavyTruck::updateJoystickCircle(int LRValue, int FBValue) {
    long scaledLRValue = (LRValue * ui->heavytruck_graphicsView->viewport()->rect().width()) / 65535;
    long scaledFBValue = (FBValue * ui->heavytruck_graphicsView->viewport()->rect().height()) / 65535;

    ui->heavytruck_graphicsView->setUpdatesEnabled(false);
    joystickCircle->setPos(QPoint(scaledLRValue, scaledFBValue) - QPointF(joystickCircle->rect().width() / 2, joystickCircle->rect().height() / 2));
    ui->heavytruck_graphicsView->setUpdatesEnabled(true);
}

void HeavyTruck::updateGearText(int gear) {
    if (gear) {
        QString gearString = QString::number(gear).replace("-", "R");
        ui->heavytruck_gearLabel->setText(gearString);
    }
    else {
        ui->heavytruck_gearLabel->setText("N");
    }
}

void HeavyTruck::updateRangeText(bool newState) {
    if (newState)   // High range
    {
        ui->heavytruck_rangeLabel->setText("HIGH");
    }
    else  // Low range
    {
        ui->heavytruck_rangeLabel->setText("LOW");
    }
}

void HeavyTruck::updateSplitterText(bool newState) {
    if (newState)
    {
        ui->heavytruck_splitterLabel->setText("⬆️");
    }
    else {
        ui->heavytruck_splitterLabel->setText("⬇️");
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
    settings->setValue("slotPattern", ui->heavytruck_slotPatternComboBox->currentText());
    settings->setValue("slotPatternLeftOffset", ui->heavytruck_slotPatternLeftOffsetSlider->value());
    settings->setValue("slotPatternDepthScale", ui->heavytruck_slotPatternDepthScaleSlider->value());
    settings->setValue("slotPatternWidthScale", ui->heavytruck_slotPatternWidthScaleSlider->value());
    settings->setValue("slotRoundingFactor", ui->heavytruck_slotRoundingFactorSlider ->value());
    settings->setValue("neutralSpringStrength", ui->heavytruck_neutralSpringStrengthSlider->value());
    settings->setValue("neutralSpringPosition", ui->heavytruck_neutralSpringPositionSlider->value());
    settings->setValue("grindZoneDepth", ui->heavytruck_grindZoneDepthSpinbox->value());
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
    settings->setValue("throttleOnShifting", ui->heavytruck_throttleOnShiftingCheckbox->isChecked());
    settings->endGroup();

    settings->endGroup();
}

void HeavyTruck::loadSettings(QSettings* settings) {
    BonusFFBApp::loadSettings(settings);

    settings->beginGroup(this->getAppName());

    settings->beginGroup("slot_pattern_settings");
    ui->heavytruck_slotPatternComboBox->setCurrentIndex(ui->heavytruck_slotPatternComboBox->findText(settings->value("slotPattern", "Eaton-Fuller 18/13").toString()));
    ui->heavytruck_slotPatternLeftOffsetSlider->setValue(settings->value("slotPatternLeftOffset", 0).toInt());
    ui->heavytruck_slotPatternDepthScaleSlider->setValue(settings->value("slotPatternDepthScale", 75).toInt());
    ui->heavytruck_slotPatternWidthScaleSlider->setValue(settings->value("slotPatternWidthScale", 67).toInt());
    ui->heavytruck_slotRoundingFactorSlider->setValue(settings->value("slotRoundingFactor", 10).toInt());
    ui->heavytruck_neutralSpringStrengthSlider->setValue(settings->value("neutralSpringStrength", 0).toInt());
    ui->heavytruck_neutralSpringPositionSlider->setValue(settings->value("neutralSpringPosition", 50).toInt());
    ui->heavytruck_grindZoneDepthSpinbox->setValue(settings->value("grindZoneDepth", 15).toInt());
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
    ui->heavytruck_throttleOnShiftingCheckbox->setChecked(settings->value("throttleOnShifting", false).toBool());
    settings->endGroup();

    settings->endGroup();
    redrawJoystickMap();
}

HRESULT HeavyTruck::startMode() {
    // Initialize FFB
    stateManager.start(devices, telemetry, &slotPattern);
    slotGuard.start(devices, &slotPattern);
    synchroGuard.start(devices, &slotPattern, telemetry);
    pedalsManager.start(devices);
    rangeSplitterManager.start(devices);

    return S_OK;
}

void HeavyTruck::gameLoop() {
    // Update state
    stateManager.update();
    slotGuard.updateSlotGuardEffects();
    synchroGuard.updateTorqueLock();
    pedalsManager.updateVirtualPedals();
    rangeSplitterManager.updateVirtualRangeSplitter();
}