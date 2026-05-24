/*
Copyright (C) 2024-
Ken Monteith.

This file is part of Bonus FFB.

Bonus FFB is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or any later version.

Bonus FFB is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Bonus FFB. If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once
#define SAFE_RELEASE(p) { if(p) { (p)->Release(); (p)=nullptr; } }
#define DIRECTINPUT_VERSION 0x0800

#define GAMELOOP_INTERVAL_MS 1
#define QT_FATAL_WARNINGS

#include <QtWidgets/QMainWindow>
#include <QTimer>
#include <QVariant>
#include <QVersionNumber>
#include <QButtonGroup>
#include "ui_BonusFFB.h"
#include "Telemetry.h"
#include "version.h"
#include "DeviceConfiguration.h"

#include "hshifter/HShifter.h"
#include "heavytruck/HeavyTruck.h"
#include "prndl/Prndl.h"
#include "pphc/Pphc.h"
#include "handbrake/Handbrake.h"

class BonusFFB : public QMainWindow
{
    Q_OBJECT

public:
    BonusFFB(QWidget *parent = nullptr);
    ~BonusFFB();

    Ui::BonusFFBClass ui;

    DeviceConfiguration devices;

    QVersionNumber version = QVersionNumber(MAJOR_VERSION, MINOR_VERSION, PATCH_VERSION);
    
    QButtonGroup appSelectButtonGroup;
    QList<BonusFFBApp*> appList;
    BonusFFBApp* activeApp;

    HeavyTruck heavytruck;
    HShifter hshifter;
    Prndl prndl;
    Pphc pphc;
    Handbrake handbrake;

public slots:
    void changeApp(int);
    void openUserGuide();
    void openAbout();
    void displayTelemetryState(TelemetrySource);
    void start();
    void stop();
    void startButtonClicked();

    void saveNewProfile();
    void saveActiveProfile();
    void loadActiveProfile();
    void loadDefaultProfile();
    void loadProfileDialog();
    void openProfileFolder();
    void updateStartButton();
    void setStartupMode();
    void setStartOnLaunch(bool);
protected:
    void resizeEvent(QResizeEvent* event);

private:
    void saveSetting(QString, QVariant);
    QVariant loadSetting(QString);
    void loadProfile(QString);
    void saveProfile(QString);
    void setProfileDisplayName();
    void connectSlidersToSpinBoxes();

    QString active_profile_path;
    QString active_profile_name;

    QTimer gameLoopTimer;

    QTimer telemetryTimer;
    Telemetry telemetry;
};