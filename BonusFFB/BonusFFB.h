/*
Copyright (C) 2024-2025 Ken Monteith.

This file is part of Bonus FFB.

Bonus FFB is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or any later version.

Bonus FFB is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Bonus FFB. If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once
#define SAFE_RELEASE(p) { if(p) { (p)->Release(); (p)=nullptr; } }
#define DIRECTINPUT_VERSION 0x0800

#define GAMELOOP_INTERVAL_MS 1

#include <QtWidgets/QMainWindow>
#include <QTimer>
#include <QVersionNumber>
#include <QButtonGroup>
#include "ui_BonusFFB.h"
#include "Telemetry.h"
#include "version.h"
#include "vJoyFeeder.h"
#include "DeviceInfo.h"
#include "hshifter/HShifter.h"
#include "prndl/Prndl.h"
#include "handbrake/Handbrake.h"

class BonusFFB : public QMainWindow
{
    Q_OBJECT

public:
    BonusFFB(QWidget *parent = nullptr);
    ~BonusFFB();
    void initializeGraphics();
    DeviceInfo* getDeviceFromGuid(QUuid);

    Ui::BonusFFBClass ui;

    vJoyFeeder vjoy = vJoyFeeder();
    QList<DeviceInfo> deviceList;

    QVersionNumber version = QVersionNumber(MAJOR_VERSION, MINOR_VERSION, PATCH_VERSION);
    
    QButtonGroup appSelectButtonGroup;
    QList<BonusFFBApp*> appList;
    BonusFFBApp* activeApp;
    HShifter hshifter;
    Prndl prndl;
    Handbrake handbrake;

public slots:
    void changeApp(int);
    void openUserGuide();
    void openAbout();
    void displayTelemetryState(TelemetrySource);
    void toggleGameLoop(bool);

protected:
    void resizeEvent(QResizeEvent* event);

private:
    QTimer gameLoopTimer;

    QTimer telemetryTimer;
    Telemetry telemetry;
};
