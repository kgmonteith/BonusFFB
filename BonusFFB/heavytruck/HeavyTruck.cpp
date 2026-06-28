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

    ui->heavytruck_throttleOnShiftingLabel->setVisible(false);

    // Set default slot pattern
    setSlotPattern(ui->heavytruck_slotPatternComboBox->currentText());

    // UI connections
    connect(ui->heavytruck_slotPatternComboBox, &QComboBox::currentTextChanged, this, &HeavyTruck::setSlotPattern);
    connect(ui->heavytruck_slotPatternLeftOffsetSlider, &QSlider::valueChanged, &slotPattern, &SlotPattern::setLeftOffset);
    connect(ui->heavytruck_slotPatternDepthScaleSlider, &QSlider::valueChanged, &slotPattern, &SlotPattern::setDepthScale);
    connect(ui->heavytruck_slotPatternWidthScaleSlider, &QSlider::valueChanged, &slotPattern, &SlotPattern::setWidthScale);
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
    //connect(ui->heavytruck_slotDepthSlider, &QSlider::valueChanged, this, &HeavyTruck::slotParameterChanged);
    //connect(ui->heavytruck_centerSlotPositionSlider, &QSlider::valueChanged, this, &HeavyTruck::slotParameterChanged);
    //connect(ui->heavytruck_rightSlotPositionSlider, &QSlider::valueChanged, this, &HeavyTruck::slotParameterChanged);
    connect(ui->heavytruck_slotRoundingFactorSlider, &QSlider::valueChanged, this, &HeavyTruck::slotParameterChanged);
    connect(ui->heavytruck_throttleOnShiftingCheckbox, &QCheckBox::toggled, &pedalsManager, &PedalsManager::toggleVirtualPedals);
}

void HeavyTruck::setSlotPattern(QString newPattern) {
    slotPattern.setName(newPattern);
    slotPattern.setSlotWalls(SLOT_WALL_LEFT);
    if (newPattern == "Eaton-Fuller 18/13") {
        slotPattern.setPattern(QList<int>{1, 2, 3, 4, 5, -1});
    }
    else if (newPattern == "Eaton-Fuller 10") {
        slotPattern.setPattern(QList<int>{1, 2, 3, 4, 5, -1});
        slotPattern.setSlotWalls(0);
    }
    else if (newPattern == "Scania 12") {
        slotPattern.setPattern(QList<int>{1, 0, 0, 4, 5, -1});
    }
    else if (newPattern == "Scania 12+2") {
        slotPattern.setPattern(QList<int>{1, 2, 0, 4, 5, -1});
    }
    else if (newPattern == "Volvo 12") {
        slotPattern.setPattern(QList<int>{0, 2, 3, 4, 5, 0});
    }
    else if (newPattern == "Volvo 12+2") {
        slotPattern.setPattern(QList<int>{1, 2, 3, 4, 5, 0});
    }
    else if (newPattern == "ZF 12") {
        slotPattern.setPattern(QList<int>{0, 2, 3, 0, 5, -1});
    }
    else if (newPattern == "ZF 16 (Modern)") {
        slotPattern.setPattern(QList<int>{0, 2, 3, 4, 5, -1});
    }
    else if (newPattern == "ZF 16 (Double-H)") {
        slotPattern.setPattern(QList<int>{0, 2, 3, 4, 5, -1, 3, 4, 5, -1});
    }
}

void HeavyTruck::slotParameterChanged(int t) {
    // Either right or center slot position changed, so we need to update both
    //slot->depth = (double)ui->heavytruck_slotDepthSlider->value() * .01;
    //slot->pos_pct[1] = ui->heavytruck_centerSlotPositionSlider->value() * .01;
    //slot->pos_pct[2] = ui->heavytruck_rightSlotPositionSlider->value() * .01;
    slot->rounding_factor = JOY_MAXPOINT * ui->heavytruck_slotRoundingFactorSlider->value() * 0.01;
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
        grindZoneRect->setRect(-2, (scene->height() / 2) - (scene->height() / 2 * slot->grind_point_depth), scene->width() + 4, scene->height() * slot->grind_point_depth);
        buttonZoneRect->setRect(-2, (scene->height() / 2) - (scene->height() / 2 * slot->button_zone_depth_telemetry), scene->width() + 4, scene->height() * slot->button_zone_depth_telemetry);
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
    settings->setValue("slotPattern", ui->heavytruck_slotPatternComboBox->currentText());
    settings->setValue("slotPatternLeftOffset", ui->heavytruck_slotPatternLeftOffsetSlider->value());
    settings->setValue("slotPatternDepthScale", ui->heavytruck_slotPatternDepthScaleSlider->value());
    settings->setValue("slotPatternWidthScale", ui->heavytruck_slotPatternWidthScaleSlider->value());
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
    settings->setValue("throttleOnShifting", ui->heavytruck_throttleOnShiftingCheckbox->isChecked());
    settings->endGroup();

    settings->endGroup();
}

void HeavyTruck::loadSettings(QSettings* settings) {
    settings->beginGroup(this->getAppName());

    settings->beginGroup("slot_pattern_settings");
    ui->heavytruck_slotPatternComboBox->setCurrentIndex(ui->heavytruck_slotPatternComboBox->findText(settings->value("slotPattern", "Eaton-Fuller 18/13").toString()));
    ui->heavytruck_slotPatternLeftOffsetSlider->setValue(settings->value("slotPatternLeftOffset", 75).toInt());
    ui->heavytruck_slotPatternDepthScaleSlider->setValue(settings->value("slotPatternDepthScale", 75).toInt());
    ui->heavytruck_slotPatternWidthScaleSlider->setValue(settings->value("slotPatternWidthScale", 66).toInt());
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
    ui->heavytruck_throttleOnShiftingCheckbox->setChecked(settings->value("throttleOnShifting", false).toBool());
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