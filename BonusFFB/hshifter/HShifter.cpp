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
#include <QSettings>
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

    // Hide UI elements that start hidden
    ui->hshifter_slotPatternCustomWarningLabel->hide();

    // Add slot patterns
    for (auto pattern : PresetPatterns) {
        ui->hshifter_slotPatternPresetComboBox->addItem(pattern.name);
    }

    // Slot pattern connections
    connect(ui->hshifter_slotPatternPresetComboBox, &QComboBox::currentTextChanged, &slotPattern, &SlotPattern::setPattern);
    connect(ui->hshifter_slotPatternCustomLineEdit, &QLineEdit::textChanged, &slotPattern, &SlotPattern::setPatternFromText);
    connect(ui->hshifter_slotPatternLeftOffsetSlider, &QSlider::valueChanged, &slotPattern, &SlotPattern::setLeftOffset);
    connect(ui->hshifter_slotPatternDepthScaleSlider, &QSlider::valueChanged, &slotPattern, &SlotPattern::setDepthScale);
    connect(ui->hshifter_slotPatternWidthScaleSlider, &QSlider::valueChanged, &slotPattern, &SlotPattern::setWidthScale);
    connect(ui->hshifter_grindZoneDepthSpinbox, &QSpinBox::valueChanged, &slotPattern, &SlotPattern::setGrindZoneScale);
    connect(ui->hshifter_buttonZoneDepthSpinbox, &QSpinBox::valueChanged, &slotPattern, &SlotPattern::setButtonZoneScale);
    //connect(ui->hshifter_slotRoundingFactorSlider, &QSlider::valueChanged, &slotPattern, &SlotPattern::setRoundingFactor);
    connect(&slotPattern, &SlotPattern::setRangeOverride, devices, &DeviceConfiguration::setRangeOverride);
    connect(ui->hshifter_neutralSpringStrengthSlider, &QSlider::valueChanged, &slotGuard, &HeavyTruckSlotGuard::setNeutralSpringStrength);
    connect(ui->hshifter_neutralSpringPositionSlider, &QSlider::valueChanged, &slotGuard, &HeavyTruckSlotGuard::setNeutralSpringPosition);
    connect(ui->hshifter_detentSpringStrengthSlider, &QSlider::valueChanged, &slotGuard, &HeavyTruckSlotGuard::setDetentSpringStrength);
    connect(ui->hshifter_shiftRailRampStrengthSlider, &QSlider::valueChanged, &slotGuard, &HeavyTruckSlotGuard::setShiftRailResistance);
    // Graphics connections
    connect(ui->hshifterTabWidget, &QTabWidget::currentChanged, this, &HShifter::redrawJoystickMap);
    // Telemetry connections
    connect(telemetry, &Telemetry::telemetryChanged, &stateManager, &HShifterStateManager::setTelemetryState);
    // Joystick connections
    connect(devices, &DeviceConfiguration::joystickValueChanged, this, &HShifter::updateJoystickCircle);
    // Pedal connections
    connect(devices, &DeviceConfiguration::clutchValueChanged, ui->clutchProgressBar, &QProgressBar::setValue);
    connect(devices, &DeviceConfiguration::throttleValueChanged, ui->throttleProgressBar, &QProgressBar::setValue);
    // vJoy connections
    connect(&stateManager, &HShifterStateManager::buttonZoneChanged, &devices->vjoy, &vJoyFeeder::updateButtons);
    connect(&stateManager, &HShifterStateManager::slotTextChanged, ui->gearLabel, &QLabel::setText);
    // FFB effect connections
    connect(&stateManager, &HShifterStateManager::slotStateChanged, &slotGuard, &HeavyTruckSlotGuard::updateSlotGuardState);
    connect(&stateManager, &HShifterStateManager::synchroStateChanged, &synchroGuard, &HShifterSynchroGuard::synchroStateChanged);
    connect(this, &HShifter::engineRPMChanged, &synchroGuard, &HShifterSynchroGuard::updateEngineRPM);
    connect(&stateManager, &HShifterStateManager::grindingStateChanged, &synchroGuard, &HShifterSynchroGuard::grindingStateChanged);
    connect(ui->grindIntensitySlider, &QSlider::valueChanged, &synchroGuard, &HShifterSynchroGuard::setGrindEffectIntensity);
    connect(ui->grindRPMSlider, &QSlider::valueChanged, &synchroGuard, &HShifterSynchroGuard::updateGrindEffectRPM);
    //connect(ui->grindRPMSlider, &QSlider::valueChanged, &synchroGuard, &SynchroGuard::updateEngineRPM);
    connect(ui->grindEffectBehaviorComboBox, &QComboBox::currentIndexChanged, &synchroGuard, &HShifterSynchroGuard::setGrindEffectBehavior);
    connect(ui->keepInGearIdleSlider, &QSlider::valueChanged, &synchroGuard, &HShifterSynchroGuard::setKeepInGearIdleIntensity);

    // Set default slot pattern
    ui->hshifter_slotPatternPresetComboBox->setCurrentIndex(2);
}

void HShifter::initializeJoystickMap() {
    scene = new QGraphicsScene();
    scene->setSceneRect(ui->hshifter_graphicsView->viewport()->rect());
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

    slotPattern.renderScene();

    if (ui->hshifter_displayZoneMarkers->isChecked()) {
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

void HShifter::updateJoystickCircle(int LRValue, int FBValue) {
    long scaledLRValue = (LRValue * ui->hshifter_graphicsView->viewport()->rect().width()) / 65535;
    long scaledFBValue = (FBValue * ui->hshifter_graphicsView->viewport()->rect().height()) / 65535;

    ui->hshifter_graphicsView->setUpdatesEnabled(false);
    joystickCircle->setPos(QPoint(scaledLRValue, scaledFBValue) - QPointF(joystickCircle->rect().width() / 2, joystickCircle->rect().height() / 2));
    ui->hshifter_graphicsView->setUpdatesEnabled(true);
}

void HShifter::updateGearText(int button) {
    if (button) {
        (QString::number(button));
    }
    else {
        ui->gearLabel->setText("N");
    }
}

void HShifter::saveSettings(QSettings* settings) {
    BonusFFBApp::saveSettings(settings);

    settings->beginGroup(this->getAppName());

    settings->beginGroup("slot_pattern_settings");
    settings->setValue("slotPattern", ui->hshifter_slotPatternPresetComboBox->currentText());
    settings->setValue("slotPatternLeftOffset", ui->hshifter_slotPatternLeftOffsetSlider->value());
    settings->setValue("slotPatternDepthScale", ui->hshifter_slotPatternDepthScaleSlider->value());
    settings->setValue("slotPatternWidthScale", ui->hshifter_slotPatternWidthScaleSlider->value());
    settings->setValue("neutralSpringStrength", ui->hshifter_neutralSpringStrengthSlider->value());
    settings->setValue("neutralSpringPosition", ui->hshifter_neutralSpringPositionSlider->value());
    settings->setValue("detentSpringStrength", ui->hshifter_detentSpringStrengthSlider->value());
    settings->setValue("shiftRailRampStrength", ui->hshifter_shiftRailRampStrengthSlider->value());
    settings->setValue("grindZoneDepth", ui->hshifter_grindZoneDepthSpinbox->value());
    settings->setValue("buttonZoneDepth", ui->hshifter_buttonZoneDepthSpinbox->value());
    settings->setValue("displayZoneMarkers", ui->hshifter_displayZoneMarkers->isChecked());
    settings->endGroup();

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

    settings->beginGroup("slot_pattern_settings");
    ui->hshifter_slotPatternPresetComboBox->setCurrentIndex(ui->hshifter_slotPatternPresetComboBox->findText(settings->value("slotPattern", "R+6").toString()));
    ui->hshifter_slotPatternLeftOffsetSlider->setValue(settings->value("slotPatternLeftOffset", 0).toInt());
    ui->hshifter_slotPatternDepthScaleSlider->setValue(settings->value("slotPatternDepthScale", 100).toInt());
    ui->hshifter_slotPatternWidthScaleSlider->setValue(settings->value("slotPatternWidthScale", 100).toInt());
    //ui->hshifter_slotRoundingFactorSlider->setValue(settings->value("slotRoundingFactor", 10).toInt());
    ui->hshifter_neutralSpringStrengthSlider->setValue(settings->value("neutralSpringStrength", 25).toInt());
    ui->hshifter_neutralSpringPositionSlider->setValue(settings->value("neutralSpringPosition", 67).toInt());
    ui->hshifter_detentSpringStrengthSlider->setValue(settings->value("detentSpringStrength", 60).toInt());
    ui->hshifter_shiftRailRampStrengthSlider->setValue(settings->value("shiftRailRampStrength", 30).toInt());
    ui->hshifter_grindZoneDepthSpinbox->setValue(settings->value("grindZoneDepth", 15).toInt());
    ui->hshifter_buttonZoneDepthSpinbox->setValue(settings->value("buttonZoneDepth", 85).toInt());
    ui->hshifter_displayZoneMarkers->setChecked(settings->value("displayZoneMarkers", false).toBool());
    settings->endGroup();

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
    //oldSlotGuard.start(devices->joystick);
    stateManager.start(devices, telemetry, &slotPattern);
    slotGuard.start(devices, &slotPattern);
    synchroGuard.start(devices);
    pedalsManager.start(devices);

    return S_OK;
}

void HShifter::gameLoop() {
    devices->updateState();

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
    //oldSlotGuard.updateSlotGuardEffects(joystickValues);
    slotGuard.updateSlotGuardEffects();
    synchroGuard.updatePedalEngagement(pedalValues, joystickValues);
    stateManager.update();
    pedalsManager.updateVirtualPedals();
}