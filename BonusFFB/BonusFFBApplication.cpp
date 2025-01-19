/*
Copyright (C) 2024-2025 Ken Monteith.

This file is part of Bonus FFB.

Bonus FFB is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or any later version.

Bonus FFB is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Bonus FFB. If not, see <https://www.gnu.org/licenses/>.
*/

#include "BonusFFBApplication.h"
#include <QDate>
#include <QDesktopServices>
#include <QFile>
#include <QMessageBox>
#include <QSettings>
#include <QUrl>

BonusFFBApplication::BonusFFBApplication(QWidget* parent)
    : QMainWindow(parent)
{
}

BonusFFBApplication::~BonusFFBApplication()
{
}


void BonusFFBApplication::saveDeviceSettings() {
    QSettings settings = QSettings(this->deviceSettingsFile, QSettings::IniFormat);
    settings.beginGroup("joystick");
    settings.setValue("device_guid", joystick->instanceGuid.toString());
    settings.setValue("lr_axis", joystickLRAxisGuid.toString());
    //settings.setValue("invert_lr_axis", ui.invertJoystickLRAxisBox->isChecked());
    settings.setValue("fb_axis", joystickFBAxisGuid.toString());
    //settings.setValue("invert_fb_axis", ui.invertJoystickFBAxisBox->isChecked());
    settings.endGroup();

    settings.beginGroup("pedals");
    settings.setValue("device_guid", pedals->instanceGuid.toString());
    settings.setValue("clutch_axis", clutchAxisGuid.toString());
    //settings.setValue("invert_clutch_axis", ui.invertClutchAxisBox->isChecked());
    settings.setValue("throttle_axis", throttleAxisGuid.toString());
    //settings.setValue("invert_throttle_axis", ui.invertThrottleAxisBox->isChecked());
    settings.endGroup();

    settings.beginGroup("vjoy");
    settings.setValue("vjoy_device", vjoy.getDeviceIndex());
    settings.endGroup();
}

void BonusFFBApplication::openUserGuide() {
    QDesktopServices::openUrl(QUrl("https://kgmonteith.github.io/BonusFFB/", QUrl::TolerantMode));
}

void BonusFFBApplication::openAbout() {
    QString about = "Bonus FFB v" + version.toString() + "\n\nCopyright 2024-" + QString::number(QDate::currentDate().year()) + ", Ken Monteith. All rights reserved.";
    QMessageBox::about(this, "About Bonus FFB", about);
}
