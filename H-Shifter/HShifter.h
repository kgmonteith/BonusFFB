/*
Copyright (C) 2024-2025 Ken Monteith.

This file is part of Bonus FFB.

Bonus FFB is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or any later version.

Bonus FFB is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Bonus FFB. If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include <QtWidgets/QMainWindow>
#include <QTimer>
#include "ui_HShifter.h"
#include "BonusFFBApplication.h"
#include "StateManager.h"
#include "Telemetry.h"
#include "SlotGuard.h"
#include "SynchroGuard.h"

#define GAMELOOP_INTERVAL_MS 1

class HShifter : public BonusFFBApplication
{
    Q_OBJECT

public:
    HShifter(QWidget *parent = nullptr);
    ~HShifter();
    void initializeGraphics();

protected:
    void resizeEvent(QResizeEvent* event);

public slots:
    void startOnLaunch();

    void rescaleJoystickMap();
    void updateJoystickCircle(int, int);
    void displayTelemetryState(TelemetrySource);

    void toggleGameLoop(bool);

    void updateGearText(int);

signals:
    void gearValuesChanged(QPair<int, int>);
    void engineRPMChanged(float);
    void resetClutchAxes();

private:
    void startGameLoop();
    void stopGameLoop();
    void gameLoop();

    Ui::HShifterClass ui;

    QGraphicsScene* scene = nullptr;
    QGraphicsRectItem* neutralChannelRect;
    QGraphicsRectItem* centerSlotRect;
    QGraphicsRectItem* rightSlotRect;
    QGraphicsRectItem* leftSlotRect;
    QGraphicsEllipseItem* joystickCircle;

    QTimer gameLoopTimer;

    QTimer telemetryTimer;
    Telemetry telemetry;

    // Stateful FFB effect managers
    StateManager stateManager;
    SlotGuard slotGuard;
    SynchroGuard synchroGuard;

    QPair<int, int> lastGearValues = { 0, 0 };
    float lastEngineRPM = 0.0;
};
