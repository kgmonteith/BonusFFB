/*
Copyright (C) 2024-2025 Ken Monteith.

This file is part of Bonus FFB.

Bonus FFB is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or any later version.

Bonus FFB is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Bonus FFB. If not, see <https://www.gnu.org/licenses/>.
*/

#include "Handbrake.h"
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsRectItem>
#include <QGraphicsEllipseItem>

Handbrake::Handbrake(QWidget *parent)
    : BonusFFBApplication(parent)
{
    ui.setupUi(this);

    // Ensure the monitor is the default tab
    ui.monitorTabWidget->setCurrentIndex(0);
    ui.monitorTabWidget->insertTab(1, deviceSettings, DEVICESETTINGSTABNAME);

    QObject::connect(ui.toggleGameLoopButton, &QPushButton::toggled, this, &Handbrake::toggleGameLoop);
    // Graphics connections
    QObject::connect(ui.monitorTabWidget, &QTabWidget::currentChanged, this, &Handbrake::rescaleJoystickMap);
    // Joystick connections
    QObject::connect(this, &Handbrake::joystickValueChanged, this, &Handbrake::updateJoystickCircle);

    // Initialize Direct Input, get the list of connected devices
    BonusFFB::initDirectInput(&deviceList);

    if (deviceSettings->joystickDeviceComboBox->count()) {
        ui.ffbDeviceFoundLabel->setText("🟢 FFB device detected");
    }

    if (!deviceSettings->joystickDeviceComboBox->count()) {
        ui.toggleGameLoopButton->setDisabled(true);
        ui.toggleGameLoopButton->setText("🚫");
        ui.toggleGameLoopButton->setToolTip("Cannot start without FFB joystick");
    }
}

Handbrake::~Handbrake()
{}

void Handbrake::initializeGraphics() {
    scene = new QGraphicsScene();
    scene->setSceneRect(ui.graphicsView->viewport()->rect());

    long sceneWidth = ui.graphicsView->viewport()->rect().width();
    long sceneHeight = ui.graphicsView->viewport()->rect().height();
    QPointF center = scene->sceneRect().center();

    channelRect = new QGraphicsRectItem(0, 0, SLOT_WIDTH_PX, sceneHeight);
    channelRect->setBrush(QBrush(Qt::black));
    channelRect->setPen(Qt::NoPen);
    channelRect->setPos(center - QPointF(SLOT_WIDTH_PX / 2, sceneHeight / 2));
    scene->addItem(channelRect);

    joystickCircle = new QGraphicsEllipseItem(0, 0, JOYSTICK_MARKER_DIAMETER_PX, JOYSTICK_MARKER_DIAMETER_PX);
    QColor seethroughWhite = Qt::transparent;
    seethroughWhite.setAlphaF(float(0.15));
    joystickCircle->setBrush(QBrush(seethroughWhite));
    joystickCircle->setPen(QPen(QColor(1, 129, 231), 7));
    QPointF circlePos = center - QPointF(JOYSTICK_MARKER_DIAMETER_PX / 2.0, JOYSTICK_MARKER_DIAMETER_PX / 2.0);
    joystickCircle->setPos(circlePos);
    scene->addItem(joystickCircle);

    ui.graphicsView->setScene(scene);
    ui.graphicsView->setRenderHints(QPainter::Antialiasing);
    ui.graphicsView->show();
}


void Handbrake::resizeEvent(QResizeEvent* e)
{
    rescaleJoystickMap();
}

// Separate call because the event doesn't trigger if another tab is active
void Handbrake::rescaleJoystickMap() {
    if (scene == nullptr) {
        return;
    }
    ui.graphicsView->scene()->setSceneRect(ui.graphicsView->viewport()->rect());

    long sceneWidth = ui.graphicsView->viewport()->rect().width();
    long sceneHeight = ui.graphicsView->viewport()->rect().height();
    QPointF center = scene->sceneRect().center();

    channelRect->setRect(0, 0, SLOT_WIDTH_PX, sceneHeight);
    channelRect->setPos(center - QPointF(SLOT_WIDTH_PX / 2, sceneHeight / 2));

    joystickCircle->setPos(center - QPointF(joystickCircle->rect().width() / 2, joystickCircle->rect().height() / 2));
}

void Handbrake::updateJoystickCircle(int LRValue, int FBValue) {
    long scaledLRValue = (LRValue * ui.graphicsView->viewport()->rect().width()) / 65535;
    long scaledFBValue = (FBValue * ui.graphicsView->viewport()->rect().height()) / 65535;

    ui.graphicsView->setUpdatesEnabled(false);
    joystickCircle->setPos(QPoint(scaledLRValue, scaledFBValue) - QPointF(joystickCircle->rect().width() / 2, joystickCircle->rect().height() / 2));
    ui.graphicsView->setUpdatesEnabled(true);
}

void Handbrake::toggleGameLoop(bool newState) {
    ui.toggleGameLoopButton->setText(newState ? "🛑" : "▶️");
    if (newState == true) {
        //startGameLoop();
    }
    else
    {
        //stopGameLoop();
    }
}