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
#include "Handbrake.h"

QString Handbrake::getAppName(bool readable) {
    if (readable) {
        return "Handbrake";
    }
    return "handbrake";
}

void Handbrake::initialize() {
    // Set flags for required and desired devices
    appDeviceFlags = FLAG_DEVICES_REQUIRED;

    // Graphics connections
    connect(ui->handbrakeTabWidget, &QTabWidget::currentChanged, this, &Handbrake::redrawJoystickMap);
    // Joystick connections
    connect(devices, &DeviceConfiguration::joystickValueChanged, this, &Handbrake::updateJoystickCircle);
    // Additional settings connections
    connect(ui->handbrakeSpringCenterSlider, &QSlider::valueChanged, this, &Handbrake::springCenterChanged);
    connect(ui->handbrakeSpringStrengthSlider, &QSlider::valueChanged, this, &Handbrake::springStrengthChanged);
}

void Handbrake::initializeJoystickMap() {
    scene = new QGraphicsScene();
    scene->setSceneRect(ui->handbrake_graphicsView->viewport()->rect());

    long sceneWidth = ui->handbrake_graphicsView->viewport()->rect().width();
    long sceneHeight = ui->handbrake_graphicsView->viewport()->rect().height();

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

    ui->handbrake_graphicsView->setScene(scene);
    ui->handbrake_graphicsView->setRenderHints(QPainter::Antialiasing);
    ui->handbrake_graphicsView->show();

    redrawJoystickMap();
}

// Separate call because the event doesn't trigger if another tab is active
void Handbrake::redrawJoystickMap() {
    if (scene == nullptr) {
        return;
    }
    ui->handbrake_graphicsView->scene()->setSceneRect(ui->handbrake_graphicsView->viewport()->rect());

    long sceneWidth = ui->handbrake_graphicsView->viewport()->rect().width();
    long sceneHeight = ui->handbrake_graphicsView->viewport()->rect().height();
    QPointF center = scene->sceneRect().center();

    centerSlotRect->setRect(0, 0, SLOT_WIDTH_PX, sceneHeight);
    centerSlotRect->setPos(center - QPointF(SLOT_WIDTH_PX / 2, sceneHeight / 2));

    joystickCircle->setPos(center - QPointF(joystickCircle->rect().width() / 2, joystickCircle->rect().height() / 2));
}

void Handbrake::updateJoystickCircle(int LRValue, int FBValue) {
    long scaledLRValue = (LRValue * ui->handbrake_graphicsView->viewport()->rect().width()) / 65535;
    long scaledFBValue = (FBValue * ui->handbrake_graphicsView->viewport()->rect().height()) / 65535;

    ui->handbrake_graphicsView->setUpdatesEnabled(false);
    joystickCircle->setPos(QPoint(scaledLRValue, scaledFBValue) - QPointF(joystickCircle->rect().width() / 2, joystickCircle->rect().height() / 2));
    ui->handbrake_graphicsView->setUpdatesEnabled(true);
}

void Handbrake::saveSettings(QSettings* settings) {
    BonusFFBApp::saveSettings(settings);

    settings->beginGroup(this->getAppName());

    settings->beginGroup("ffb_effect_settings");
    settings->setValue("spring_strength", ui->handbrakeSpringStrengthSlider->value());
    settings->setValue("spring_center", ui->handbrakeSpringCenterSlider->value());
    settings->endGroup();

    settings->endGroup();
}

void Handbrake::loadSettings(QSettings* settings) {
    BonusFFBApp::loadSettings(settings);

    settings->beginGroup(this->getAppName());

    settings->beginGroup("ffb_effect_settings");
    ui->handbrakeSpringStrengthSlider->setValue(settings->value("spring_strength", 75).toInt());
    ui->handbrakeSpringCenterSlider->setValue(settings->value("spring_center", 0).toInt());
    settings->endGroup();

    settings->endGroup();

    qDebug() << "Succesfully loaded " << this->getAppName() << " settings";
}

void Handbrake::springCenterChanged(int value) {
    springCenter = -10000 + (value * 200);
    handbrakeSpring.lOffset = springCenter;
    if (devices->joystick != nullptr && devices->joystick->isAcquired) {
        devices->joystick->updateEffect("handbrakeSpring");
    }
}

void Handbrake::springStrengthChanged(int value) {
    springStrength = -100 * value;   // AB9 1.1.3.4 firmware force inversion
    handbrakeSpring.lPositiveCoefficient = springStrength;
    handbrakeSpring.lNegativeCoefficient = springStrength;
    if (devices->joystick != nullptr && devices->joystick->isAcquired) {
        devices->joystick->updateEffect("handbrakeSpring");
    }
}

HRESULT Handbrake::startMode() {
    keepCenteredSpringEff.dwSize = sizeof(DIEFFECT);
    keepCenteredSpringEff.dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
    keepCenteredSpringEff.dwDuration = INFINITE;
    keepCenteredSpringEff.dwSamplePeriod = 0;
    keepCenteredSpringEff.dwGain = DI_FFNOMINALMAX;
    keepCenteredSpringEff.dwTriggerButton = DIEB_NOTRIGGER;
    keepCenteredSpringEff.dwTriggerRepeatInterval = 0;
    keepCenteredSpringEff.cAxes = 1;
    keepCenteredSpringEff.rgdwAxes = &AXES[0];
    keepCenteredSpringEff.rglDirection = &FORWARDBACK[0];
    keepCenteredSpringEff.lpEnvelope = 0;
    keepCenteredSpringEff.cbTypeSpecificParams = sizeof(DICONDITION);
    keepCenteredSpringEff.lpvTypeSpecificParams = &keepLRCentered;
    keepCenteredSpringEff.dwStartDelay = 0;
    devices->joystick->addEffect("handbrakeKeepCenteredSpring", { GUID_Spring, &keepCenteredSpringEff });

    handbrakeSpringEff.dwSize = sizeof(DIEFFECT);
    handbrakeSpringEff.dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
    handbrakeSpringEff.dwDuration = INFINITE;
    handbrakeSpringEff.dwSamplePeriod = 0;
    handbrakeSpringEff.dwGain = DI_FFNOMINALMAX;
    handbrakeSpringEff.dwTriggerButton = DIEB_NOTRIGGER;
    handbrakeSpringEff.dwTriggerRepeatInterval = 0;
    handbrakeSpringEff.cAxes = 1;
    handbrakeSpringEff.rgdwAxes = &AXES[1];
    handbrakeSpringEff.rglDirection = &FORWARDBACK[0];
    handbrakeSpringEff.lpEnvelope = 0;
    handbrakeSpringEff.cbTypeSpecificParams = sizeof(DICONDITION);
    handbrakeSpringEff.lpvTypeSpecificParams = &handbrakeSpring;
    handbrakeSpringEff.dwStartDelay = 0;
    qDebug() << "Adding handbrakeSpring effect...";
    devices->joystick->addEffect("handbrakeSpring", { GUID_Spring, &handbrakeSpringEff });

    return S_OK;
}

void Handbrake::gameLoop() {
    if (devices->joystick == nullptr || !devices->joystick->isAcquired ) {
        return;
    }
    // Get new joystick values
    QPair<int, int> joystickValues = devices->getJoystickValues();

    // Update the effect, just to ensure the device gets reacquired. Should be harmless, right?
    devices->joystick->updateEffect("handbrakeSpring");
}