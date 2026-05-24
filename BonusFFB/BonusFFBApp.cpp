/*
Copyright (C) 2024-2026 Ken Monteith.

This file is part of Bonus FFB.

Bonus FFB is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or any later version.

Bonus FFB is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Bonus FFB. If not, see <https://www.gnu.org/licenses/>.
*/


#include "BonusFFBApp.h"

bool g_joystick_warned = false;

void BonusFFBApp::setPointers(Ui::BonusFFBClass* uiPtr, DeviceConfiguration* devicesPtr, Telemetry* tPtr) {
    ui = uiPtr;
    devices = devicesPtr;
    telemetry = tPtr;

    // FFB effect settings
    connect(ui->centralWidget->findChild<QSlider*>(getAppName() + "_damperSlider"), &QSlider::valueChanged, this, &BonusFFBApp::updateDamper);
    connect(ui->centralWidget->findChild<QSlider*>(getAppName() + "_inertiaSlider"), &QSlider::valueChanged, this, &BonusFFBApp::updateInertia);
    connect(ui->centralWidget->findChild<QSlider*>(getAppName() + "_frictionSlider"), &QSlider::valueChanged, this, &BonusFFBApp::updateFriction);
}

HRESULT BonusFFBApp::start() {
    // Acquire devices
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

    qDebug() << "damperStrength: " << damperStrength;
    qDebug() << "inertiaStrength: " << inertiaStrength;
    qDebug() << "frictionStrength: " << frictionStrength;

    hr = startMode();

    devices->joystick->startEffects();
    return hr;
}

void BonusFFBApp::stop() {
    // Release devices
    devices->release();
}

void BonusFFBApp::saveSettings(QSettings* settings) {
    settings->beginGroup(this->getAppName());

    settings->beginGroup("ffb_effect_settings");
    settings->setValue("damper", ui->centralWidget->findChild<QSlider*>(getAppName() + "_damperSlider")->value());
    settings->setValue("inertia", ui->centralWidget->findChild<QSlider*>(getAppName() + "_inertiaSlider")->value());
    settings->setValue("friction", ui->centralWidget->findChild<QSlider*>(getAppName() + "_frictionSlider")->value());
    settings->endGroup();

    settings->endGroup();
}


void BonusFFBApp::loadSettings(QSettings* settings) {
    settings->beginGroup(this->getAppName());

    settings->beginGroup("ffb_effect_settings");
    if (int value = settings->value("damper").toInt()) {
        ui->centralWidget->findChild<QSlider*>(getAppName() + "_damperSlider")->setValue(value);
    }
    if (int value = settings->value("inertia").toInt()) {
        ui->centralWidget->findChild<QSlider*>(getAppName() + "_inertiaSlider")->setValue(value);
    }
    if (int value = settings->value("friction").toInt()) {
        ui->centralWidget->findChild<QSlider*>(getAppName() + "_frictionSlider")->setValue(value);
    }
    settings->endGroup();

    settings->endGroup();
}

void BonusFFBApp::updateDamper(int value) {
    damperStrength = FFB_MAX * value * 0.01;
    damperCondition[0] = { 0, damperStrength, damperStrength };
    damperCondition[1] = { 0, damperStrength, damperStrength };
    qDebug() << "damperStrength: " << damperStrength;
    if (devices->joystick != nullptr && devices->joystick->isAcquired) {
        devices->joystick->updateEffect("damper");
    }
}

void BonusFFBApp::updateInertia(int value) {
    inertiaStrength = FFB_MAX * value * 0.01;
    inertiaCondition[0] = { 0, inertiaStrength, inertiaStrength };
    inertiaCondition[1] = { 0, inertiaStrength, inertiaStrength };
    qDebug() << "inertiaStrength: " << inertiaStrength;
    if (devices->joystick != nullptr && devices->joystick->isAcquired) {
        devices->joystick->updateEffect("inertia");
    }
}

void BonusFFBApp::updateFriction(int value) {
    frictionStrength = FFB_MAX * value * 0.01;
    frictionCondition[0] = { 0, frictionStrength, frictionStrength };
    frictionCondition[1] = { 0, frictionStrength, frictionStrength };
    qDebug() << "frictionStrength: " << frictionStrength;
    if (devices->joystick != nullptr && devices->joystick->isAcquired) {
        devices->joystick->updateEffect("friction");
    }
}