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
#include "SeqShifter.h"

QString SeqShifter::getAppName(bool readable) {
    if (readable)
        return "Sequential shifter";
    return "seqshifter";
}

void SeqShifter::initialize() {
    // Set flags for required and desired devices
    appDeviceFlags = FLAG_DEVICES_REQUIRED | FLAG_DEVICES_SHIFTLOCK;
    if (devices->throttle != nullptr)
        appDeviceFlags |= FLAG_DEVICES_THROTTLE;
    if (devices->brake != nullptr)
        appDeviceFlags |= FLAG_DEVICES_BRAKE;
    if (devices->clutch != nullptr)
        appDeviceFlags |= FLAG_DEVICES_CLUTCH;

    // Graphics connections
    connect(ui->seqshifterTabWidget, &QTabWidget::currentChanged, this, &SeqShifter::redrawJoystickMap);
    connect(this, &SeqShifter::shiftStateChanged, this, &SeqShifter::updateGearShiftText);
    // Shifter settings connections
    connect(ui->seqshifter_shifterThrowSlider, &QSlider::valueChanged, this, &SeqShifter::setShifterThrow);
    connect(ui->seqshifter_detentScaleSlider, &QSlider::valueChanged, this, &SeqShifter::setDetentScale);
    connect(ui->seqshifter_centeringSpringStrengthSlider, &QSlider::valueChanged, this, &SeqShifter::setCenteringSpringStrength);
    connect(ui->seqshifter_detentSpringStrengthSlider, &QSlider::valueChanged, this, &SeqShifter::setDetentSpringStrength);
    connect(ui->seqshifter_mechanicalResistanceStrengthSlider, &QSlider::valueChanged, this, &SeqShifter::setMechanicalResistanceStrength);
    // Joystick connections
    connect(devices, &DeviceConfiguration::joystickValueChanged, this, &SeqShifter::updateJoystickCircle);
    //connect(&stateManager, &SeqShifterStateManager::slotSpringChanged, &slotGuard, &SeqShifterSlotGuard::updateSlotSpringCenter);
    // vJoy connections
    connect(this, &SeqShifter::shiftStateChanged, this, &SeqShifter::updateButtons);
    // Additional settings connections
    //connect(ui->prndl_enableParkSlotCheckBox, &QCheckBox::checkStateChanged, &stateManager, &SeqShifterStateManager::toggleParkSlot);
    //connect(ui->prndl_enableLowSlotCheckBox, &QCheckBox::checkStateChanged, &stateManager, &SeqShifterStateManager::toggleLastSlot);
    //connect(ui->prndl_simulateParkUsingTelemetryCheckBox, &QCheckBox::checkStateChanged, &stateManager, &SeqShifterStateManager::toggleAtsTelemetryPark);
}

void SeqShifter::initializeJoystickMap() {
    scene = new QGraphicsScene();
    scene->setSceneRect(ui->seqshifter_graphicsView->viewport()->rect());

    long sceneWidth = ui->seqshifter_graphicsView->viewport()->rect().width();
    long sceneHeight = ui->seqshifter_graphicsView->viewport()->rect().height();

    centerSlotRect = new QGraphicsRectItem();
    centerSlotRect->setBrush(QBrush(Qt::black));
    centerSlotRect->setPen(Qt::NoPen);
    scene->addItem(centerSlotRect);

    joystickCircle = new QGraphicsEllipseItem(0, 0, JOYSTICK_MARKER_DIAMETER_PX, JOYSTICK_MARKER_DIAMETER_PX);
    QColor seethroughWhite = Qt::transparent;
    seethroughWhite.setAlphaF(float(0.15));
    joystickCircle->setBrush(QBrush(seethroughWhite));
    joystickCircle->setPen(QPen(QColor(1, 129, 231), 7));
    scene->addItem(joystickCircle);

    ui->seqshifter_graphicsView->setScene(scene);
    ui->seqshifter_graphicsView->setRenderHints(QPainter::Antialiasing);
    ui->seqshifter_graphicsView->show();

    redrawJoystickMap();
}

// Separate call because the event doesn't trigger if another tab is active
void SeqShifter::redrawJoystickMap() {
    if (scene == nullptr) {
        return;
    }
    ui->seqshifter_graphicsView->scene()->setSceneRect(ui->seqshifter_graphicsView->viewport()->rect());

    long sceneWidth = ui->seqshifter_graphicsView->viewport()->rect().width();
    long sceneHeight = ui->seqshifter_graphicsView->viewport()->rect().height();
    QPointF center = scene->sceneRect().center();

    centerSlotRect->setRect(0, 0, SLOT_WIDTH_PX, sceneHeight * depth_scale);
    centerSlotRect->setPos(center - QPointF(SLOT_WIDTH_PX / 2, (sceneHeight * depth_scale) / 2));

    joystickCircle->setPos(center - QPointF(joystickCircle->rect().width() / 2, joystickCircle->rect().height() / 2));
}

void SeqShifter::updateGearShiftText(SeqShifterState state) {
    if (state == SeqShifterState::SHIFT_DOWN) {
        ui->seqshifter_gearShiftLabel->setText("➖");
    }
    else if (state == SeqShifterState::SHIFT_UP) {
        ui->seqshifter_gearShiftLabel->setText("➕");
    }
    else {
        ui->seqshifter_gearShiftLabel->setText("");
    }
}

void SeqShifter::updateJoystickCircle(int LRValue, int FBValue) {
    long scaledLRValue = (LRValue * ui->seqshifter_graphicsView->viewport()->rect().width()) / 65535;
    long scaledFBValue = (FBValue * ui->seqshifter_graphicsView->viewport()->rect().height()) / 65535;

    ui->seqshifter_graphicsView->setUpdatesEnabled(false);
    joystickCircle->setPos(QPoint(scaledLRValue, scaledFBValue) - QPointF(joystickCircle->rect().width() / 2, joystickCircle->rect().height() / 2));
    ui->seqshifter_graphicsView->setUpdatesEnabled(true);
}

void SeqShifter::saveSettings(QSettings* settings) {
    BonusFFBApp::saveSettings(settings);

    settings->beginGroup(this->getAppName());

    settings->beginGroup("shifter_settings");
    settings->setValue("shifterThrow", ui->seqshifter_shifterThrowSlider->value());
    settings->setValue("detentSize", ui->seqshifter_detentScaleSlider->value());
    settings->setValue("centeringSpringStrength", ui->seqshifter_centeringSpringStrengthSlider->value());
    settings->setValue("detentSpringStrength", ui->seqshifter_detentSpringStrengthSlider->value());
    settings->setValue("mechanicalResistanceStrength", ui->seqshifter_mechanicalResistanceStrengthSlider->value());
    settings->endGroup();

    settings->endGroup();
}

void SeqShifter::loadSettings(QSettings* settings) {
    BonusFFBApp::loadSettings(settings);

    settings->beginGroup(this->getAppName());

    settings->beginGroup("shifter_settings");
    ui->seqshifter_shifterThrowSlider->setValue(settings->value("shifterThrow", 50).toInt());
    ui->seqshifter_detentScaleSlider->setValue(settings->value("detentSize", 20).toInt());
    ui->seqshifter_centeringSpringStrengthSlider->setValue(settings->value("centeringSpringStrength", 50).toInt());
    ui->seqshifter_detentSpringStrengthSlider->setValue(settings->value("detentSpringStrength", 50).toInt());
    ui->seqshifter_mechanicalResistanceStrengthSlider->setValue(settings->value("mechanicalResistanceStrength", 30).toInt());
    settings->endGroup();

    settings->endGroup();
}

HRESULT SeqShifter::startMode() {
    ZeroMemory(&slotSpringEff, sizeof(slotSpringEff));
    slotSpringEff.dwSize = sizeof(DIEFFECT);
    slotSpringEff.dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
    slotSpringEff.dwDuration = INFINITE;
    slotSpringEff.dwSamplePeriod = 0;
    slotSpringEff.dwGain = DI_FFNOMINALMAX;
    slotSpringEff.dwTriggerButton = DIEB_NOTRIGGER;
    slotSpringEff.dwTriggerRepeatInterval = 0;
    slotSpringEff.cAxes = 2;
    slotSpringEff.rgdwAxes = AXES;
    slotSpringEff.rglDirection = FORWARDBACK;
    slotSpringEff.lpEnvelope = 0;
    slotSpringEff.cbTypeSpecificParams = sizeof(slotSpringConditions);
    slotSpringEff.lpvTypeSpecificParams = &slotSpringConditions;
    slotSpringEff.dwStartDelay = 0;
    devices->joystick->addEffect("slotSpring", { GUID_Spring, &slotSpringEff });

    ZeroMemory(&centeringSpringEff, sizeof(centeringSpringEff));
    centeringSpringEff.dwSize = sizeof(DIEFFECT);
    centeringSpringEff.dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
    centeringSpringEff.dwDuration = INFINITE;
    centeringSpringEff.dwSamplePeriod = 0;
    centeringSpringEff.dwGain = DI_FFNOMINALMAX;
    centeringSpringEff.dwTriggerButton = DIEB_NOTRIGGER;
    centeringSpringEff.dwTriggerRepeatInterval = 0;
    centeringSpringEff.cAxes = 1;
    centeringSpringEff.rgdwAxes = &AXES[1];
    centeringSpringEff.rglDirection = &FORWARDBACK[0];
    centeringSpringEff.lpEnvelope = 0;
    centeringSpringEff.cbTypeSpecificParams = sizeof(centeringSpring);
    centeringSpringEff.lpvTypeSpecificParams = &centeringSpring;
    centeringSpringEff.dwStartDelay = 0;
    devices->joystick->addEffect("centeringSpring", { GUID_Spring, &centeringSpringEff });

    ZeroMemory(&detentSpringEff, sizeof(detentSpringEff));
    detentSpringCondition.lOffset = 0;
    detentSpringEff.dwSize = sizeof(DIEFFECT);
    detentSpringEff.dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
    detentSpringEff.dwDuration = INFINITE;
    detentSpringEff.dwSamplePeriod = 0;
    detentSpringEff.dwGain = DI_FFNOMINALMAX;
    detentSpringEff.dwTriggerButton = DIEB_NOTRIGGER;
    detentSpringEff.dwTriggerRepeatInterval = 0;
    detentSpringEff.cAxes = 1;
    detentSpringEff.rgdwAxes = &AXES[1];
    detentSpringEff.rglDirection = &FORWARDBACK[0];
    detentSpringEff.lpEnvelope = 0;
    detentSpringEff.cbTypeSpecificParams = sizeof(DICONDITION);
    detentSpringEff.lpvTypeSpecificParams = &detentSpringCondition;
    detentSpringEff.dwStartDelay = 0;
    devices->joystick->addEffect("detentSpring", { GUID_Spring, &detentSpringEff });
    return S_OK;
}

void SeqShifter::gameLoop() {
    if (devices->joystick == nullptr || !devices->joystick->isAcquired ) {
        return;
    }
    devices->updateState();

    // Get new joystick values
    joyValues = devices->getJoystickValues();
    
    updateState();
    updateSlotSpring();
    updateDetent();
}

void SeqShifter::updateState() {
    SeqShifterState newState = SeqShifterState::UNKNOWN;
    double neutral_zone_joystick = neutral_zone_scale * JOY_MIDPOINT;
    double slotDepthAsJoystickFwd = JOY_MIDPOINT - (depth_scale * JOY_MIDPOINT);
    double slotDepthAsJoystickBack = JOY_MIDPOINT + (depth_scale * JOY_MIDPOINT);
    double detent_zone_joystick = JOY_MIDPOINT * detent_zone_scale * depth_scale;

    if ((joyValues.fb >= JOY_MIDPOINT - neutral_zone_joystick) && (joyValues.fb <= JOY_MIDPOINT + neutral_zone_joystick)) {
        newState = SeqShifterState::NEUTRAL;
        if (min_gear && current_gear <= min_gear)
            newState = SeqShifterState::SHIFT_DOWN_BLOCKED;
        else if (max_gear && current_gear >= max_gear)
            newState = SeqShifterState::SHIFT_UP_BLOCKED;
    }
    else if (state != SeqShifterState::SHIFT_DOWN_BLOCKED && joyValues.fb <= (slotDepthAsJoystickFwd + detent_zone_joystick)) {
        newState = SeqShifterState::SHIFT_DOWN;
    }
    else if (state != SeqShifterState::SHIFT_UP_BLOCKED && joyValues.fb >= (slotDepthAsJoystickBack - detent_zone_joystick)) {
        newState = SeqShifterState::SHIFT_UP;
    }
    else if (joyValues.fb >= (slotDepthAsJoystickFwd + detent_zone_joystick) && joyValues.fb <= (slotDepthAsJoystickBack - detent_zone_joystick)) {
        if (state == SeqShifterState::SHIFT_DOWN || state == SeqShifterState::SHIFT_UP)
            newState = SeqShifterState::EXITING_SHIFT;
    }
    if (newState != state && newState != SeqShifterState::UNKNOWN) {
        state = newState;
        if (state == SeqShifterState::NEUTRAL)
            qDebug() << "SeqShifterState::NEUTRAL";
        else if (state == SeqShifterState::SHIFT_DOWN_BLOCKED)
            qDebug() << "SeqShifterState::SHIFT_DOWN_BLOCKED";
        else if (state == SeqShifterState::SHIFT_UP_BLOCKED)
            qDebug() << "SeqShifterState::SHIFT_UP_BLOCKED";
        else if (state == SeqShifterState::SHIFT_DOWN)
            qDebug() << "SeqShifterState::SHIFT_DOWN";
        else if (state == SeqShifterState::SHIFT_UP)
            qDebug() << "SeqShifterState::SHIFT_UP";
        else if (state == SeqShifterState::EXITING_SHIFT)
            qDebug() << "SeqShifterState::EXITING_SHIFT";
        else if (state == SeqShifterState::UNKNOWN)
            qDebug() << "SeqShifterState::UNKNOWN";
        emit shiftStateChanged(state);
    }
}

void SeqShifter::updateSlotSpring() {
    float slotDepthAsFFBOffset = (joyValues.fb <= JOY_MIDPOINT) ? (depth_scale * FFB_MINPOINT) : (depth_scale * FFB_MAXPOINT);
    double neutral_zone_ffb_fwd = neutral_zone_scale * FFB_MINPOINT;
    double neutral_zone_ffb_back = neutral_zone_scale * FFB_MAXPOINT;

    slotSpringConditions[0].lOffset = joystickPositionToFFBOffset(joyValues.lr) * -1;
    if (state == SeqShifterState::SHIFT_DOWN_BLOCKED && (joystickPositionToFFBOffset(joyValues.fb) <= neutral_zone_ffb_fwd)) {
        slotSpringConditions[1] = keepFBCentered;
        slotSpringConditions[1].lOffset = neutral_zone_ffb_fwd + (std::abs(joystickPositionToFFBOffset(joyValues.fb) - neutral_zone_ffb_fwd) * 2.5);
    } else if (state == SeqShifterState::SHIFT_UP_BLOCKED && (joystickPositionToFFBOffset(joyValues.fb) >= neutral_zone_ffb_back)) {
        slotSpringConditions[1] = keepFBCentered;
        slotSpringConditions[1].lOffset = neutral_zone_ffb_back - (std::abs(joystickPositionToFFBOffset(joyValues.fb) - neutral_zone_ffb_back) * 2.5);
    } else if (joyValues.fb <= JOY_MIDPOINT - (depth_scale * JOY_MIDPOINT)) {
        // Push stick back if it moved too far forward
        slotSpringConditions[1] = keepFBCentered;
        int offset = slotDepthAsFFBOffset + (std::abs(joystickPositionToFFBOffset(joyValues.fb) - slotDepthAsFFBOffset) * 2.5);
        slotSpringConditions[1].lOffset = offset;
    }
    else if (joyValues.fb >= JOY_MIDPOINT + (depth_scale * JOY_MIDPOINT)) {
        // Push stick forward if it moved too far back
        slotSpringConditions[1] = keepFBCentered;
        int offset = slotDepthAsFFBOffset - (std::abs(joystickPositionToFFBOffset(joyValues.fb) - slotDepthAsFFBOffset) * 2.5);
        slotSpringConditions[1].lOffset = offset;
    }
    else {
        slotSpringConditions[1] = noSpring;
    }
    devices->joystick->updateEffect("slotSpring");

    // Check for changes to centering spring
    if (centering_spring_strength != centeringSpring.lPositiveCoefficient) {
        centeringSpring.lPositiveCoefficient = centering_spring_strength * -1;
        centeringSpring.lNegativeCoefficient = centering_spring_strength * -1;
        devices->joystick->updateEffect("centeringSpring");
    }
}

void SeqShifter::updateDetent() {
    // Play the end-of-slot detent and shift rail resistance effects
    float slotDepthAsFFBOffset = (joyValues.fb <= JOY_MIDPOINT) ? (depth_scale * FFB_MINPOINT) : (depth_scale * FFB_MAXPOINT);

    if (state == SeqShifterState::SHIFT_DOWN || state == SeqShifterState::SHIFT_UP) {
        detentSpringCondition.lOffset = slotDepthAsFFBOffset;
        detentSpringCondition.lPositiveCoefficient = detent_spring_strength * -1;   // I have no idea why this spring strength needs to be inverted, but it does
        detentSpringCondition.lNegativeCoefficient = detent_spring_strength * -1;
    }
    else {
        // Set strength to 0 if mechanical resistance is disabled. Solves setting 0 saturation.
        detentSpringCondition.lPositiveCoefficient = (mechanical_resistance_strength) ? -10000 : 0;
        detentSpringCondition.lNegativeCoefficient = (mechanical_resistance_strength) ? -10000 : 0;
        detentSpringCondition.lOffset = joystickPositionToFFBOffset(joyValues.fb) * -1;
        detentSpringCondition.dwNegativeSaturation = mechanical_resistance_strength;
        detentSpringCondition.dwPositiveSaturation = mechanical_resistance_strength;
    }
    devices->joystick->updateEffect("detentSpring");
    /*
    if (newState != state) {
        if (min_gear && max_gear) 
        {
            QString gearText = (current_gear) ? QString::number(current_gear).replace("-", "R") : "N";
            ui->seqshifter_currentGearLabel->setText(gearText);
        }
    }*/
}

void SeqShifter::updateButtons(SeqShifterState _state) {
    if (_state == SeqShifterState::SHIFT_DOWN) {
        devices->vjoy.updateButtons(BUTTON_SHIFT_DOWN);
    }
    else if (_state == SeqShifterState::SHIFT_UP) {
        devices->vjoy.updateButtons(BUTTON_SHIFT_UP);
    }
    else {
        devices->vjoy.updateButtons(0);
    }
}