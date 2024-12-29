#pragma once

#include <QtWidgets/QMainWindow>
#include <QTimer>
#include "ui_HShifter.h"
#include "BonusFFB.h"
#include "Telemetry.h"

#define GAMELOOP_INTERVAL_MS 10
#define SLOT_WIDTH_PX 5.0
#define JOYSTICK_MARKER_DIAMETER_PX 21.0

class HShifter : public QMainWindow
{
    Q_OBJECT

public:
    HShifter(QWidget *parent = nullptr);
    ~HShifter();

protected:
    void resizeEvent(QResizeEvent* event);

public slots:
    void saveSettings();
    void loadSettings();
    void startOnLaunch();
    void initializeGraphics();

    void gameLoop();
    void toggleGameLoop(bool);

    void changeFFBJoystickDevice(int);
    void changePedalsDevice(int);
    void changeClutchAxis(int);
    void changeThrottleAxis(int);

signals:
    void clutchValueChanged(int);
    void throttleValueChanged(int);
    void resetClutchAxes();

private:
    Ui::HShifterClass ui;
    QTimer telemetryTimer;
    Telemetry telemetry;

    QList<BonusFFB::DeviceInfo> deviceList;

    BonusFFB::DeviceInfo* pedals = nullptr;
    QUuid clutchAxisGuid;
    QUuid throttleAxisGuid;

    BonusFFB::DeviceInfo ffbBase;

    QTimer gameLoopTimer;

    QGraphicsScene* scene = nullptr;
    QGraphicsRectItem* neutralChannelRect;
    QGraphicsRectItem* centerSlotRect;
    QGraphicsRectItem* rightSlotRect;
    QGraphicsRectItem* leftSlotRect;
    QGraphicsEllipseItem* joystickCircle;
};
