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
#include <QFile>
#include "Handbrake.h"


QString Handbrake::getAppName() {
    return "handbrake";
}

void Handbrake::initialize() {
    // Menu action connections
    QObject::connect(ui->actionSaveSettings, &QAction::triggered, this, &Handbrake::saveSettings);
    QObject::connect(ui->actionLoadSettings, &QAction::triggered, this, &Handbrake::loadSettings);
    // Graphics connections
    QObject::connect(ui->handbrakeTabWidget, &QTabWidget::currentChanged, this, &Handbrake::redrawJoystickMap);
    // Joystick connections
    QObject::connect(ui->handbrake_joystickDeviceComboBox, &QComboBox::currentIndexChanged, this, &Handbrake::changeJoystickDevice);
    QObject::connect(ui->handbrake_joystickLRAxisComboBox, &QComboBox::currentIndexChanged, this, &Handbrake::changeJoystickLRAxis);
    QObject::connect(ui->handbrake_joystickFBAxisComboBox, &QComboBox::currentIndexChanged, this, &Handbrake::changeJoystickFBAxis);
    QObject::connect(this, &Handbrake::joystickValueChanged, this, &Handbrake::updateJoystickCircle);
    // Additional settings connections
    QObject::connect(ui->handbrakeSpringCenterSlider, &QSlider::valueChanged, this, &Handbrake::springCenterChanged);
    QObject::connect(ui->handbrakeSpringStrengthSlider, &QSlider::valueChanged, this, &Handbrake::springStrengthChanged);

    // Populate the device lists
    for (const DeviceInfo& device : *deviceList)
    {
        if (device.supportsFfb && device.productGuid.data1 != VJOY_PRODUCT_GUID) {
            ui->handbrake_joystickDeviceComboBox->addItem(device.name, device.instanceGuid);
        }
    }
}

void Handbrake::initializeJoystickMap() {
    scene = new QGraphicsScene();
    scene->setSceneRect(ui->handbrake_graphicsView->viewport()->rect());

    long sceneWidth = ui->handbrake_graphicsView->viewport()->rect().width();
    long sceneHeight = ui->handbrake_graphicsView->viewport()->rect().height();
    QPointF center = scene->sceneRect().center();

    centerSlotRect = new QGraphicsRectItem(0, 0, SLOT_WIDTH_PX, sceneHeight);
    centerSlotRect->setBrush(QBrush(Qt::black));
    centerSlotRect->setPen(Qt::NoPen);
    centerSlotRect->setPos(center - QPointF(SLOT_WIDTH_PX / 2, sceneHeight / 2));
    scene->addItem(centerSlotRect);

    joystickCircle = new QGraphicsEllipseItem(0, 0, JOYSTICK_MARKER_DIAMETER_PX, JOYSTICK_MARKER_DIAMETER_PX);
    QColor seethroughWhite = Qt::transparent;
    seethroughWhite.setAlphaF(float(0.15));
    joystickCircle->setBrush(QBrush(seethroughWhite));
    joystickCircle->setPen(QPen(QColor(1, 129, 231), 7));
    QPointF circlePos = center - QPointF(JOYSTICK_MARKER_DIAMETER_PX / 2.0, JOYSTICK_MARKER_DIAMETER_PX / 2.0);
    joystickCircle->setPos(circlePos);
    scene->addItem(joystickCircle);

    ui->handbrake_graphicsView->setScene(scene);
    ui->handbrake_graphicsView->setRenderHints(QPainter::Antialiasing);
    ui->handbrake_graphicsView->show();
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

void Handbrake::changeJoystickDevice(int deviceIndex) {
    // Release previous joystick
    if (joystick != nullptr && joystick->isAcquired) {
        joystick->release();
    }
    QUuid deviceGuid = ui->handbrake_joystickDeviceComboBox->currentData().toUuid();
    qDebug() << "Device UUID: " << deviceGuid;
    joystick = getDeviceFromGuid(deviceList, deviceGuid);
    qDebug() << "New joystick device: " << joystick->name;
    ui->handbrake_joystickLRAxisComboBox->clear();
    ui->handbrake_joystickFBAxisComboBox->clear();
    QMap<QUuid, QString> axisMap = joystick->getDeviceAxes();
    for (auto axis = axisMap.cbegin(), end = axisMap.cend(); axis != end; ++axis)
    {
        ui->handbrake_joystickLRAxisComboBox->addItem(axis.value(), axis.key());
        ui->handbrake_joystickFBAxisComboBox->addItem(axis.value(), axis.key());
    }
}

void Handbrake::changeJoystickLRAxis(int axisIndex) {
    joystickLRAxisGuid = ui->handbrake_joystickLRAxisComboBox->currentData().toUuid();
}

void Handbrake::changeJoystickFBAxis(int axisIndex) {
    joystickFBAxisGuid = ui->handbrake_joystickFBAxisComboBox->currentData().toUuid();
}

void Handbrake::saveSettings() {
    QSettings settings = QSettings(this->deviceSettingsFile, QSettings::IniFormat);
    settings.beginGroup("joystick");
    settings.setValue("device_guid", joystick->instanceGuid.toString());
    settings.setValue("lr_axis", joystickLRAxisGuid.toString());
    settings.setValue("fb_axis", joystickFBAxisGuid.toString());
    settings.endGroup();

    settings.beginGroup("handbrake");
    settings.setValue("spring_strength", ui->handbrakeSpringStrengthSlider->value());
    settings.setValue("spring_center", ui->handbrakeSpringCenterSlider->value());
    settings.endGroup();
}

void Handbrake::loadSettings() {
    qDebug() << "Loading settings";
    if (!QFile(deviceSettingsFile).exists()) {
        qDebug() << "Settings file does not exist";
        return;
    }
    QSettings settings = QSettings(deviceSettingsFile, QSettings::IniFormat);

    settings.beginGroup("joystick");
    int joystick_index = ui->handbrake_joystickDeviceComboBox->findData(settings.value("device_guid").toUuid());
    if (joystick_index == -1 && !g_joystick_warned) {
        QMessageBox::warning(nullptr, "Joystick not found", "Saved handbrake joystick device is not connected.\nReconnect the device or update the input/output settings.");
        g_joystick_warned = true;
    }
    else
    {
        ui->handbrake_joystickDeviceComboBox->setCurrentIndex(joystick_index);
        ui->handbrake_joystickLRAxisComboBox->setCurrentIndex(ui->handbrake_joystickLRAxisComboBox->findData(settings.value("lr_axis").toUuid()));
        ui->handbrake_joystickFBAxisComboBox->setCurrentIndex(ui->handbrake_joystickFBAxisComboBox->findData(settings.value("fb_axis").toUuid()));
    }
    settings.endGroup();

    settings.beginGroup("handbrake");
    int t_int = settings.value("spring_strength").toInt();
    if(t_int)
        ui->handbrakeSpringStrengthSlider->setValue(t_int);
    t_int = settings.value("spring_center").toInt();
    if (t_int)
        ui->handbrakeSpringCenterSlider->setValue(t_int);
    settings.endGroup();

    qDebug() << "Succesfully loaded Handbrake settings";
}

void Handbrake::springCenterChanged(int value) {
    springCenter = -10000 + (value * 200);
    handbrakeSpring.lOffset = springCenter;
    joystick->updateEffect("handbrakeSpring");
}

void Handbrake::springStrengthChanged(int value) {
    springStrength = 100 * value;
    handbrakeSpring.lPositiveCoefficient = springStrength;
    handbrakeSpring.lNegativeCoefficient = springStrength;
    joystick->updateEffect("handbrakeSpring");
}

QPair<int, int> Handbrake::getJoystickValues() {
    HRESULT hr = joystick->updateState();
    if (hr != DI_OK) {
        qDebug() << "updateState failed, reacquiring joystick. Return code " << unsigned long(hr);
        joystick->reacquire();
    }
    long joystickLRValue = joystick->getAxisReading(joystickLRAxisGuid);
    long joystickFBValue = joystick->getAxisReading(joystickFBAxisGuid);
    emit joystickLRValueChanged(joystickLRValue);
    emit joystickFBValueChanged(joystickFBValue);
    emit joystickValueChanged(joystickLRValue, joystickFBValue);
    return QPair<int, int>(joystickLRValue, joystickFBValue);
}

HRESULT Handbrake::startGameLoop() {    // Acquire joystick
    qDebug() << "Acquiring joystick...";
    HRESULT hr = joystick->acquire(&hwnd, true);
    if (FAILED(hr)) {
        QMessageBox::critical(nullptr, "Error", "Could not acquire exclusive use of FFB joystick. Please close other games or applications and try again.");
        return hr;
    };

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
    joystick->addEffect("handbrakeKeepCenteredSpring", { GUID_Spring, &keepCenteredSpringEff });

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
    joystick->addEffect("handbrakeSpring", { GUID_Spring, &handbrakeSpringEff });

    joystick->startEffects();
    return S_OK;
}

void Handbrake::stopGameLoop() {
    // Release devices
    joystick->release();
    return;
}

void Handbrake::gameLoop() {
    if (joystick == nullptr || !joystick->isAcquired ) {
        return;
    }
    // Get new joystick values
    QPair<int, int> joystickValues = getJoystickValues();

    // Update the effect, just to ensure the device gets reacquired. Should be harmless, right?
    joystick->updateEffect("handbrakeSpring");
}