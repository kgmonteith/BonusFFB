/*
Copyright (C) 2024-2026 Ken Monteith.

This file is part of Bonus FFB.

Bonus FFB is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or any later version.

Bonus FFB is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Bonus FFB. If not, see <https://www.gnu.org/licenses/>.
*/

#include "Pphc.h"
#include <QGraphicsRectItem>
#include <QLinearGradient>

QString Pphc::getAppName() {
    return "pphc";
}

void Pphc::initialize() {
    // Set flags for required and desired devices
    appDeviceFlags = FLAG_DEVICES_REQUIRED;

    // Graphics connections
    QObject::connect(ui->pphcTabWidget, &QTabWidget::currentChanged, this, &Pphc::redrawJoystickMap);
    // Joystick connections
    QObject::connect(devices, &DeviceConfiguration::joystickValueChanged, this, &Pphc::updateJoystickCircle);
    // Static FFB effect connections
    connect(ui->pphc_damperSlider, &QSlider::valueChanged, this, &Pphc::updateDamper);
    connect(ui->pphc_inertiaSlider, &QSlider::valueChanged, this, &Pphc::updateInertia);
    connect(ui->pphc_frictionSlider, &QSlider::valueChanged, this, &Pphc::updateFriction);
    // Additional settings connections
    connect(ui->pphc_brakeSpringScalingSlider, &QSlider::valueChanged, this, &Pphc::setBrakeSpringScaling);
    connect(ui->pphc_brakeAxisDeadzoneSlider, &QSlider::valueChanged, this, &Pphc::setBrakeAxisDeadzone);
    connect(ui->pphc_brakeAxisScalingSlider, &QSlider::valueChanged, this, &Pphc::setBrakeAxisScaling);
    connect(ui->pphc_throttleSlotDepthSlider, &QSlider::valueChanged, this, &Pphc::setThrottleSlotDepth);
    connect(ui->pphc_throttleSpringStrengthSlider, &QSlider::valueChanged, this, &Pphc::setThrottleSpringStrength);
    connect(ui->pphc_throttleAxisDeadzoneSlider, &QSlider::valueChanged, this, &Pphc::setThrottleAxisDeadzone);
}

void Pphc::initializeJoystickMap() {
    scene = new QGraphicsScene();
    scene->setSceneRect(ui->pphc_graphicsView->viewport()->rect());

    long sceneWidth = ui->pphc_graphicsView->viewport()->rect().width();
    long sceneHeight = ui->pphc_graphicsView->viewport()->rect().height();
    QPointF center = scene->sceneRect().center();

    QColor seethroughWhite = Qt::transparent;
    seethroughWhite.setAlphaF(float(0.15));

    QColor partWhite = Qt::white;
    partWhite.setAlphaF(float(0.8));

    centerSlotRect = new QGraphicsRectItem(0, 0, SLOT_WIDTH_PX, sceneHeight);
    centerSlotRect->setBrush(QBrush(Qt::black));
    centerSlotRect->setPen(Qt::NoPen);
    scene->addItem(centerSlotRect);

    deadzoneRect = new QGraphicsRectItem(0, 0, SLOT_WIDTH_PX, sceneHeight * (brakeDeadzone + throttleDeadzone));
    deadzoneRect->setBrush(QBrush(partWhite));
    deadzoneRect->setPen(Qt::NoPen);
    scene->addItem(deadzoneRect);

    joystickCircle = new QGraphicsEllipseItem(0, 0, JOYSTICK_MARKER_DIAMETER_PX, JOYSTICK_MARKER_DIAMETER_PX);
    joystickCircle->setBrush(QBrush(seethroughWhite));
    joystickCircle->setPen(QPen(QColor(1, 129, 231), 7));
    QPointF circlePos = center - QPointF(JOYSTICK_MARKER_DIAMETER_PX / 2.0, JOYSTICK_MARKER_DIAMETER_PX / 2.0);
    joystickCircle->setPos(circlePos);
    scene->addItem(joystickCircle);

    ui->pphc_graphicsView->setScene(scene);
    ui->pphc_graphicsView->setRenderHints(QPainter::Antialiasing);
    ui->pphc_graphicsView->show();
}

// Separate call because the event doesn't trigger if another tab is active
void Pphc::redrawJoystickMap() {
    if (scene == nullptr) {
        return;
    }
    ui->pphc_graphicsView->scene()->setSceneRect(ui->pphc_graphicsView->viewport()->rect());

    long sceneWidth = ui->pphc_graphicsView->viewport()->rect().width();
    long sceneHeight = ui->pphc_graphicsView->viewport()->rect().height();
    QPointF center = scene->sceneRect().center();


    centerSlotRect->setRect(0, 0, SLOT_WIDTH_PX, (sceneHeight / 2.0) + ((sceneHeight / 2.0) * throttleSlotDepth));
    centerSlotRect->setPos(sceneWidth / 2.0 - SLOT_WIDTH_PX / 2.0, 0);


    deadzoneRect->setRect(0, 0, SLOT_WIDTH_PX, sceneHeight * (brakeDeadzone + throttleDeadzone));
    deadzoneRect->setPos(center - QPointF(SLOT_WIDTH_PX / 2.0, sceneHeight * brakeDeadzone));
    QLinearGradient gradient(deadzoneRect->rect().topLeft(), deadzoneRect->rect().bottomLeft());

    // 3. Add color stops (position from 0.0 to 1.0)
    gradient.setColorAt(0.0, Qt::black);
    gradient.setColorAt(0.5, Qt::lightGray);
    gradient.setColorAt(1.0, Qt::lightGray);

    // 4. Apply the gradient to the item's brush
    deadzoneRect->setBrush(QBrush(gradient));

    joystickCircle->setPos(center - QPointF(joystickCircle->rect().width() / 2, joystickCircle->rect().height() / 2));
}

void Pphc::updateJoystickCircle(int LRValue, int FBValue) {
    long scaledLRValue = (LRValue * ui->pphc_graphicsView->viewport()->rect().width()) / 65535;
    long scaledFBValue = (FBValue * ui->pphc_graphicsView->viewport()->rect().height()) / 65535;

    ui->pphc_graphicsView->setUpdatesEnabled(false);
    joystickCircle->setPos(QPoint(scaledLRValue, scaledFBValue) - QPointF(joystickCircle->rect().width() / 2, joystickCircle->rect().height() / 2));
    ui->pphc_graphicsView->setUpdatesEnabled(true);
}

void Pphc::saveSettings(QSettings* settings) {
    settings->beginGroup(getAppName());

    settings->beginGroup("ffb_effect_settings");
    settings->setValue("damper", ui->pphc_damperSlider->value());
    settings->setValue("inertia", ui->pphc_inertiaSlider->value());
    settings->setValue("friction", ui->pphc_frictionSlider->value());
    settings->endGroup();

    settings->beginGroup("ffb_effect_settings");
    settings->setValue("brakeSpringScaling", ui->pphc_brakeSpringScalingSlider->value());
    settings->setValue("brakeAxisDeadzone", ui->pphc_brakeAxisDeadzoneSlider->value());
    settings->setValue("brakeAxisScaling", ui->pphc_brakeAxisScalingSlider->value());
    settings->setValue("throttleSpringStrength", ui->pphc_throttleSpringStrengthSlider->value());
    settings->setValue("throttleSlotDepth", ui->pphc_throttleSlotDepthSlider->value());
    settings->setValue("throttleAxisDeadzone", ui->pphc_throttleAxisDeadzoneSlider->value());
    settings->endGroup();

    settings->endGroup();
}

void Pphc::loadSettings(QSettings* settings) {
    settings->beginGroup(getAppName());

    settings->beginGroup("ffb_effect_settings");
    ui->pphc_damperSlider->setValue(settings->value("damper", 50).toInt());
    ui->pphc_inertiaSlider->setValue(settings->value("inertia", 0).toInt());
    ui->pphc_frictionSlider->setValue(settings->value("friction", 50).toInt());
    ui->pphc_brakeSpringScalingSlider->setValue(settings->value("brakeSpringScaling", 200).toInt());
    ui->pphc_brakeAxisDeadzoneSlider->setValue(settings->value("brakeAxisDeadzone", 10).toInt());
    ui->pphc_brakeAxisScalingSlider->setValue(settings->value("brakeAxisScaling", 200).toInt());
    ui->pphc_throttleSpringStrengthSlider->setValue(settings->value("throttleSpringStrength", 40).toInt());
    ui->pphc_throttleAxisDeadzoneSlider->setValue(settings->value("throttleAxisDeadzone", 50).toInt());
    ui->pphc_throttleSlotDepthSlider->setValue(settings->value("throttleSlotDepthSlider", 90).toInt());
    settings->endGroup();

    settings->endGroup();
    qDebug() << "Succesfully loaded" << getAppName() << "settings";
}

HRESULT Pphc::startGameLoop() {
    // Acquire joystick
    HRESULT hr = devices->acquire(appDeviceFlags);
    if (FAILED(hr)) {
        return hr;
    };

    damperEff.dwSize = sizeof(DIEFFECT);
    damperEff.dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
    damperEff.dwDuration = INFINITE;
    damperEff.dwSamplePeriod = 0;
    damperEff.dwGain = DI_FFNOMINALMAX;
    damperEff.dwTriggerButton = DIEB_NOTRIGGER;
    damperEff.dwTriggerRepeatInterval = 0;
    damperEff.cAxes = 2;
    damperEff.rgdwAxes = AXES;
    damperEff.rglDirection = FORWARDBACK;
    damperEff.lpEnvelope = 0;
    damperEff.cbTypeSpecificParams = sizeof(DICONDITION) * 2;
    damperEff.lpvTypeSpecificParams = &damperCondition;
    damperEff.dwStartDelay = 0;
    devices->joystick->addEffect("damper", { GUID_Damper, &damperEff });

    inertiaEff.dwSize = sizeof(DIEFFECT);
    inertiaEff.dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
    inertiaEff.dwDuration = INFINITE;
    inertiaEff.dwSamplePeriod = 0;
    inertiaEff.dwGain = DI_FFNOMINALMAX;
    inertiaEff.dwTriggerButton = DIEB_NOTRIGGER;
    inertiaEff.dwTriggerRepeatInterval = 0;
    inertiaEff.cAxes = 2;
    inertiaEff.rgdwAxes = AXES;
    inertiaEff.rglDirection = FORWARDBACK;
    inertiaEff.lpEnvelope = 0;
    inertiaEff.cbTypeSpecificParams = sizeof(DICONDITION) * 2;
    inertiaEff.lpvTypeSpecificParams = &inertiaCondition;
    inertiaEff.dwStartDelay = 0;
    devices->joystick->addEffect("inertia", { GUID_Inertia, &inertiaEff });

    frictionEff.dwSize = sizeof(DIEFFECT);
    frictionEff.dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
    frictionEff.dwDuration = INFINITE;
    frictionEff.dwSamplePeriod = 0;
    frictionEff.dwGain = DI_FFNOMINALMAX;
    frictionEff.dwTriggerButton = DIEB_NOTRIGGER;
    frictionEff.dwTriggerRepeatInterval = 0;
    frictionEff.cAxes = 2;
    frictionEff.rgdwAxes = AXES;
    frictionEff.rglDirection = FORWARDBACK;
    frictionEff.lpEnvelope = 0;
    frictionEff.cbTypeSpecificParams = sizeof(DICONDITION) * 2;
    frictionEff.lpvTypeSpecificParams = &frictionCondition;
    frictionEff.dwStartDelay = 0;
    devices->joystick->addEffect("friction", { GUID_Friction, &frictionEff });

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

    pphcSpringEff.dwSize = sizeof(DIEFFECT);
    pphcSpringEff.dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
    pphcSpringEff.dwDuration = INFINITE;
    pphcSpringEff.dwSamplePeriod = 0;
    pphcSpringEff.dwGain = DI_FFNOMINALMAX;
    pphcSpringEff.dwTriggerButton = DIEB_NOTRIGGER;
    pphcSpringEff.dwTriggerRepeatInterval = 0;
    pphcSpringEff.cAxes = 1;
    pphcSpringEff.rgdwAxes = &AXES[1];
    pphcSpringEff.rglDirection = &FORWARDBACK[0];
    pphcSpringEff.lpEnvelope = 0;
    pphcSpringEff.cbTypeSpecificParams = sizeof(pphcSpring);
    pphcSpringEff.lpvTypeSpecificParams = &pphcSpring;
    pphcSpringEff.dwStartDelay = 0;
    devices->joystick->addEffect("pphcSpring", { GUID_Spring, &pphcSpringEff });

    devices->joystick->startEffects();
    return S_OK;
}

void Pphc::stopGameLoop() {
    // Release devices
    devices->release();
    return;
}

void Pphc::gameLoop() {
    if (devices->joystick == nullptr || !devices->joystick->isAcquired) {
        return;
    }
    // Get new joystick values
    QPair<int, int> joystickValues = devices->getJoystickValues();

    updateSlotSpring(joystickValues);
    updateBrake(joystickValues.second);
    updateThrottle(joystickValues.second);
}

void Pphc::updateBrake(int fbValue) {
    float brakeSpringOffset = joystickPositionToFFBOffset(fbValue) * brakeSpringScaling * -1;
    int brakeAxisOutput = VJOY_AXIS_MAX_VALUE * (scaleRangeValue(brakeSpringOffset, 10000 * brakeDeadzone, 10000.0 / brakeAxisScaling));
    devices->vjoy.setAxisValue(brakeAxisOutput, HID_USAGE_RY);

    if (fbValue <= JOY_MIDPOINT) {
        ui->pphc_brakeLabel->setText(QString::number(brakeAxisOutput));
        ui->pphc_brakeProgressBar->setValue(brakeAxisOutput);
        pphcSpring.lOffset = brakeSpringOffset;
        pphcSpring.lNegativeCoefficient = FFB_MAX * -1;
        pphcSpring.lPositiveCoefficient = FFB_MAX * -1;
        devices->joystick->updateEffect("pphcSpring");
    }
}

void Pphc::updateThrottle(int fbValue) {
    int throttleAxisOutput = VJOY_AXIS_MAX_VALUE * scaleRangeValue(fbValue, JOY_MIDPOINT + (JOY_MAXPOINT * throttleDeadzone), JOY_MIDPOINT + (JOY_MIDPOINT * throttleSlotDepth));
    devices->vjoy.setAxisValue(throttleAxisOutput, HID_USAGE_RX);

    if (fbValue > JOY_MIDPOINT) {
        ui->pphc_throttleLabel->setText(QString::number(throttleAxisOutput));
        ui->pphc_throttleProgressBar->setValue(throttleAxisOutput);
        if (pphcSpring.lPositiveCoefficient != throttleSpringStrength || pphcSpring.lOffset != 0) {
            pphcSpring.lPositiveCoefficient = throttleSpringStrength;
            pphcSpring.lNegativeCoefficient = throttleSpringStrength;
            pphcSpring.lOffset = 0;
            devices->joystick->updateEffect("pphcSpring");
        }
    }
}

void Pphc::updateSlotSpring(QPair<int, int> joystickValues) {
    slotSpringConditions[0].lOffset = joystickPositionToFFBOffset(joystickValues.first) * -1;
    float slotDepthAsFFBOffsetBack = (throttleSlotDepth * FFB_MAXPOINT);
    if (joystickValues.second >= JOY_MIDPOINT + (throttleSlotDepth * JOY_MIDPOINT)) {
        slotSpringConditions[1] = keepFBCentered;
        int offset = slotDepthAsFFBOffsetBack - (std::abs(joystickPositionToFFBOffset(joystickValues.second) - slotDepthAsFFBOffsetBack) * 2.5);
        slotSpringConditions[1].lOffset = offset;
    }
    else {
        slotSpringConditions[1] = noSpring;
    }
    devices->joystick->updateEffect("slotSpring");
}


void Pphc::updateDamper(int value) {
    damperStrength = FFB_MAX * value * 0.01;
    damperCondition[0] = { 0, damperStrength, damperStrength };
    damperCondition[1] = { 0, damperStrength, damperStrength };
    if (devices->joystick != nullptr && devices->joystick->isAcquired) {
        devices->joystick->updateEffect("damper");
        qDebug() << "damperStrength: " << damperStrength;
    }
}

void Pphc::updateInertia(int value) {
    inertiaStrength = FFB_MAX * value * 0.01;
    inertiaCondition[0] = { 0, inertiaStrength, inertiaStrength };
    inertiaCondition[1] = { 0, inertiaStrength, inertiaStrength };
    if (devices->joystick != nullptr && devices->joystick->isAcquired) {
        devices->joystick->updateEffect("inertia");
        qDebug() << "inertiaStrength: " << inertiaStrength;
    }
}

void Pphc::updateFriction(int value) {
    frictionStrength = FFB_MAX * value * 0.01;
    frictionCondition[0] = { 0, frictionStrength, frictionStrength };
    frictionCondition[1] = { 0, frictionStrength, frictionStrength };
    if (devices->joystick != nullptr && devices->joystick->isAcquired) {
        devices->joystick->updateEffect("friction");
        qDebug() << "frictionStrength: " << frictionStrength;
    }
}

void Pphc::setThrottleSlotDepth(int value) {
    throttleSlotDepth = value * 0.01;
    qDebug() << "throttleSlotDepth: " << throttleSlotDepth;
}


void Pphc::setThrottleSpringStrength(int value) {
    throttleSpringStrength = FFB_MAX * float(value) * -0.01;
}

void Pphc::setThrottleAxisDeadzone(int value) {
    throttleDeadzone = float(value) * 0.001;
}

void Pphc::setBrakeSpringScaling(int value) {
    brakeSpringScaling = float(value) * 0.01;
}

void Pphc::setBrakeAxisDeadzone(int value) {
    brakeDeadzone = float(value) * 0.001;
}

void Pphc::setBrakeAxisScaling(int value) {
    brakeAxisScaling = float(value) * 0.01;
}