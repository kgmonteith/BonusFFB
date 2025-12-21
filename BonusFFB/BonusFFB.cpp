/*
Copyright (C) 2024-2025 Ken Monteith.

This file is part of Bonus FFB.

Bonus FFB is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or any later version.

Bonus FFB is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Bonus FFB. If not, see <https://www.gnu.org/licenses/>.
*/

#include "BonusFFB.h"
#include <QSettings>
#include <QMessageBox>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsRectItem>
#include <QGraphicsEllipseItem>
#include <QDir>
#include <QDesktopServices>
#include <stdlib.h>

BonusFFB::BonusFFB(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);
    
    // Configure app selection button group
    appSelectButtonGroup.setExclusive(true);
    appSelectButtonGroup.addButton(ui.hshifter_appSelectButton, 0);
    appSelectButtonGroup.addButton(ui.prndl_appSelectButton, 1);
    appList.append(&hshifter);
    appList.append(&prndl);
    QObject::connect(&appSelectButtonGroup, &QButtonGroup::idClicked, this, &BonusFFB::changeApp);
    ui.appStackedWidget->setCurrentIndex(0);

    // Ensure the monitor is the default tab
    ui.hshifterTabWidget->setCurrentIndex(0);
    ui.prndlTabWidget->setCurrentIndex(0);

    // Menu action connections
    QObject::connect(ui.actionExit, &QAction::triggered, this, &BonusFFB::close);
    QObject::connect(ui.actionUserGuide, &QAction::triggered, this, &BonusFFB::openUserGuide);
    QObject::connect(ui.actionAbout, &QAction::triggered, this, &BonusFFB::openAbout);
    // Game loop connections
    QObject::connect(ui.toggleGameLoopButton, &QPushButton::toggled, this, &BonusFFB::toggleGameLoop);
    // Telemetry connections
    QObject::connect(&telemetry, &Telemetry::telemetryChanged, this, &BonusFFB::displayTelemetryState);

    // Initialize Direct Input, get the list of connected devices
    initDirectInput(&deviceList);

    // Set FFB device detection label
    bool ffbDeviceFound = false;
    for (const DeviceInfo device : deviceList)
    {
        if (device.supportsFfb && device.productGuid.data1 != VJOY_PRODUCT_GUID) {
            ui.ffbDeviceFoundLabel->setText("🟢 FFB-enabled device detected");
            ffbDeviceFound = true;
            break;
        }
    }

    // Initialize vJoyFeeder
    if (!vJoyFeeder::isDriverEnabled()) {
        ui.vjoyDeviceFoundLabel->setText("❌ vJoy not installed");
    }
    else if (!vJoyFeeder::checkVersionMatch()) {
        ui.vjoyDeviceFoundLabel->setText("❌ vJoy v2.1.8 or newer required");
    }
    else if (vJoyFeeder::deviceCount() <= 0) {
        ui.vjoyDeviceFoundLabel->setText("❌ vJoy device not configured");
    }
    else {
        ui.vjoyDeviceFoundLabel->setText("🟢 vJoy device found");
    }

    // Initialize application GUIs
    hshifter.setPointers(&ui, &deviceList, &vjoy, &telemetry, (HWND)(winId()));
    hshifter.initialize();
    prndl.setPointers(&ui, &deviceList, &vjoy, &telemetry, (HWND)(winId()));
    prndl.initialize();
    activeApp = &hshifter;

    // Start telemetry receiver
    telemetry.startConnectTimer();

    if (!ffbDeviceFound || !vJoyFeeder::isDriverEnabled()) {
        ui.toggleGameLoopButton->setDisabled(true);
        ui.toggleGameLoopButton->setText("🚫");
        ui.toggleGameLoopButton->setToolTip("Cannot start without FFB joystick and vJoy");
    }
    qDebug("BonusFFBApplication constructor finished");
}

BonusFFB::~BonusFFB()
{
    if (gameLoopTimer.isActive())
        emit toggleGameLoop(false);
}

void BonusFFB::changeApp(int appSelectButtonIndex) {
    ui.appStackedWidget->setCurrentIndex(appSelectButtonIndex);
    activeApp = appList[appSelectButtonIndex];
    activeApp->redrawJoystickMap();
}

void BonusFFB::resizeEvent(QResizeEvent* e)
{
    activeApp->redrawJoystickMap();
}

void BonusFFB::openUserGuide() {
    QDesktopServices::openUrl(QUrl("https://kgmonteith.github.io/BonusFFB/", QUrl::TolerantMode));
}

void BonusFFB::openAbout() {
    QString about = "Bonus FFB v" + version.toString() + "\n\nCopyright 2024-" + QString::number(QDate::currentDate().year()) + ", Ken Monteith. All rights reserved.";
    QMessageBox::about(this, "About Bonus FFB", about);
}

void BonusFFB::displayTelemetryState(TelemetrySource newState) {
    if (newState == TelemetrySource::NONE) {
        ui.telemetryLabel->setText("⚠️ Telemetry disconnected");
    }
    else if (newState == TelemetrySource::SCS) {
        ui.telemetryLabel->setText("🟢 ATS/ETS2 telemetry connected");
    }
}

void BonusFFB::toggleGameLoop(bool newState) {
    ui.toggleGameLoopButton->setText(newState ? "🛑" : "▶️");
    if (newState == true) {
        gameLoopTimer.start(GAMELOOP_INTERVAL_MS);
        for (QAbstractButton* button : appSelectButtonGroup.buttons()) {
            button->setEnabled(false);
        }
        if (FAILED(activeApp->startGameLoop())) {
            emit toggleGameLoop(false);
            return;
        }
        QObject::connect(&gameLoopTimer, &QTimer::timeout, activeApp, &BonusFFBApp::gameLoop);
    }
    else
    {
        QObject::disconnect(&gameLoopTimer, &QTimer::timeout, activeApp, &BonusFFBApp::gameLoop);
        activeApp->stopGameLoop();
        for (QAbstractButton* button : appSelectButtonGroup.buttons()) {
            button->setEnabled(true);
        }
    }
}

DeviceInfo* BonusFFB::getDeviceFromGuid(QUuid guid) {
    for (QList<DeviceInfo>::iterator it = deviceList.begin(); it != deviceList.end(); it++)
    {
        if (it->instanceGuid == guid) {
            return &*it;
        }
    }
    qDebug() << "Device not found: " << guid;
    return nullptr;
}