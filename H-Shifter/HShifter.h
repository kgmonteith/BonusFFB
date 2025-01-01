/*
This file is part of Bonus FFB.

Bonus FFB is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or any later version.

Bonus FFB is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Bonus FFB. If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include <QtWidgets/QMainWindow>
#include <QTimer>
#include "ui_HShifter.h"
#include "BonusFFB.h"
#include "BonusFFBApplication.h"
#include "Telemetry.h"

#define GAMELOOP_INTERVAL_MS 10
#define SLOT_WIDTH_PX 5.0
#define JOYSTICK_MARKER_DIAMETER_PX 21.0

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
    void saveSettings();
    void loadSettings();
    void startOnLaunch();

    void rescaleShifterMap();
    void updateJoystickCircle(int, int);

    void gameLoop();
    void toggleGameLoop(bool);

    void changeJoystickDevice(int);
    void changeJoystickLRAxis(int);
    void changeJoystickFBAxis(int);
    void changePedalsDevice(int);
    void changeClutchAxis(int);
    void changeThrottleAxis(int);

signals:
    void joystickValueChanged(int, int);
    void joystickLRValueChanged(int);
    void joystickFBValueChanged(int);
    void clutchValueChanged(int);
    void throttleValueChanged(int);
    void resetClutchAxes();

private:
    Ui::HShifterClass ui;
    QTimer telemetryTimer;
    Telemetry telemetry;

    QList<BonusFFB::DeviceInfo> deviceList;

    BonusFFB::DeviceInfo* joystick = nullptr;
    QUuid joystickLRAxisGuid;
    QUuid joystickFBAxisGuid;

    BonusFFB::DeviceInfo* pedals = nullptr;
    QUuid clutchAxisGuid;
    QUuid throttleAxisGuid;

    QTimer gameLoopTimer;

    QGraphicsScene* scene = nullptr;
    QGraphicsRectItem* neutralChannelRect;
    QGraphicsRectItem* centerSlotRect;
    QGraphicsRectItem* rightSlotRect;
    QGraphicsRectItem* leftSlotRect;
    QGraphicsEllipseItem* joystickCircle;
};
