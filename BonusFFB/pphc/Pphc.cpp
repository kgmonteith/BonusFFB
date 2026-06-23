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

QString Pphc::getAppName(bool readable) {
    if (readable)
        return "Push/pull hand control";
    return "pphc";
}

void Pphc::initialize() {
    // Set flags for required and desired devices
    appDeviceFlags = FLAG_DEVICES_REQUIRED;

    // Graphics connections
    QObject::connect(ui->pphcTabWidget, &QTabWidget::currentChanged, this, &Pphc::redrawJoystickMap);
    // Joystick connections
    QObject::connect(devices, &DeviceConfiguration::joystickValueChanged, this, &Pphc::updateJoystickCircle);
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

    centerSlotRect = new QGraphicsRectItem();
    centerSlotRect->setBrush(QBrush(Qt::black));
    centerSlotRect->setPen(Qt::NoPen);
    scene->addItem(centerSlotRect);

    deadzoneRect = new QGraphicsRectItem();
    deadzoneRect->setPen(Qt::NoPen);
    scene->addItem(deadzoneRect);

    QColor seethroughWhite = Qt::transparent;
    seethroughWhite.setAlphaF(float(0.15));

    joystickCircle = new QGraphicsEllipseItem(0, 0, JOYSTICK_MARKER_DIAMETER_PX, JOYSTICK_MARKER_DIAMETER_PX);
    joystickCircle->setBrush(QBrush(seethroughWhite));
    joystickCircle->setPen(QPen(QColor(1, 129, 231), 7));
    scene->addItem(joystickCircle);

    ui->pphc_graphicsView->setScene(scene);
    ui->pphc_graphicsView->setRenderHints(QPainter::Antialiasing);
    ui->pphc_graphicsView->show();

    redrawJoystickMap();
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
    gradient.setColorAt(0.0, Qt::black);
    gradient.setColorAt(0.5, Qt::lightGray);
    gradient.setColorAt(1.0, Qt::lightGray);
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
    BonusFFBApp::saveSettings(settings);

    settings->beginGroup(getAppName());

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

HRESULT Pphc::startMode() {
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

    return S_OK;
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
    devices->vjoy.setAxisValue(brakeAxisOutput, HID_USAGE_Y);
    ui->pphc_brakeLabel->setText(QString::number(brakeAxisOutput));
    ui->pphc_brakeProgressBar->setValue(brakeAxisOutput);

    if (fbValue <= JOY_MIDPOINT) {
        pphcSpring.lOffset = brakeSpringOffset;
        pphcSpring.lNegativeCoefficient = FFB_MAX * -1;
        pphcSpring.lPositiveCoefficient = FFB_MAX * -1;
        devices->joystick->updateEffect("pphcSpring");
    }
}

void Pphc::updateThrottle(int fbValue) {
    int throttleAxisOutput = VJOY_AXIS_MAX_VALUE * scaleRangeValue(fbValue, JOY_MIDPOINT + (JOY_MAXPOINT * throttleDeadzone), JOY_MIDPOINT + (JOY_MIDPOINT * throttleSlotDepth));
    devices->vjoy.setAxisValue(throttleAxisOutput, HID_USAGE_X);
    ui->pphc_throttleLabel->setText(QString::number(throttleAxisOutput));
    ui->pphc_throttleProgressBar->setValue(throttleAxisOutput);

    if (fbValue > JOY_MIDPOINT) {
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

void Pphc::setThrottleSlotDepth(int value) {
    throttleSlotDepth = value * 0.01;
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