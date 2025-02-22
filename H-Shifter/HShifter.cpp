/*
Copyright (C) 2024-2025 Ken Monteith.

This file is part of Bonus FFB.

Bonus FFB is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or any later version.

Bonus FFB is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Bonus FFB. If not, see <https://www.gnu.org/licenses/>.
*/

#include "HShifter.h"
#include <QSettings>
#include <QMessageBox>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsRectItem>
#include <QGraphicsEllipseItem>
#include <QDir>
#include <stdlib.h>

HShifter::HShifter(QWidget *parent)
    : BonusFFBApplication(parent)
{
    ui.setupUi(this);

    // Ensure the monitor is the default tab
    ui.monitorTabWidget->setCurrentIndex(0);
    ui.monitorTabWidget->insertTab(1, deviceSettings, DEVICESETTINGSTABNAME);

    // Show pedals settings
    deviceSettings->pedalsDeviceComboBox->parent()->setProperty("visible", true);
    deviceSettings->vjoyDeviceComboBox->parent()->setProperty("visible", true);

    // Menu action connections
    QObject::connect(ui.actionExit, &QAction::triggered, this, &HShifter::close);
    QObject::connect(ui.actionSaveSettings, &QAction::triggered, this, &HShifter::saveDeviceSettings);
    QObject::connect(ui.actionLoadSettings, &QAction::triggered, this, &HShifter::loadDeviceSettings);
    QObject::connect(ui.actionUserGuide, &QAction::triggered, this, &BonusFFBApplication::openUserGuide);
    QObject::connect(ui.actionAbout, &QAction::triggered, this, &BonusFFBApplication::openAbout);
    // Graphics connections
    QObject::connect(ui.monitorTabWidget, &QTabWidget::currentChanged, this, &HShifter::rescaleJoystickMap);
    // Telemetry connections
    QObject::connect(&telemetry, &Telemetry::telemetryChanged, this, &HShifter::displayTelemetryState);
    QObject::connect(&telemetry, &Telemetry::telemetryChanged, &stateManager, &StateManager::setTelemetryState);
    // Joystick connections
    QObject::connect(this, &HShifter::joystickValueChanged, this, &HShifter::updateJoystickCircle);
    // Pedal connections
    QObject::connect(this, &HShifter::clutchValueChanged, ui.clutchProgressBar, &QProgressBar::setValue);
    QObject::connect(this, &HShifter::throttleValueChanged, ui.throttleProgressBar, &QProgressBar::setValue);
    // vJoy connections
    QObject::connect(&stateManager, &StateManager::buttonZoneChanged, &vjoy, &vJoyFeeder::updateButtons);
    QObject::connect(&stateManager, &StateManager::buttonZoneChanged, this, &HShifter::updateGearText);
    // Game loop connections
    QObject::connect(ui.toggleGameLoopButton, &QPushButton::toggled, this, &HShifter::toggleGameLoop);
    QObject::connect(&gameLoopTimer, &QTimer::timeout, this, &HShifter::gameLoop);
    // FFB effect connections
    QObject::connect(&stateManager, &StateManager::slotStateChanged, &slotGuard, &SlotGuard::updateSlotGuardEffects);
    QObject::connect(this, &HShifter::pedalValuesChanged, &synchroGuard, &SynchroGuard::updatePedalEngagement);
    QObject::connect(&stateManager, &StateManager::synchroStateChanged, &synchroGuard, &SynchroGuard::synchroStateChanged);
    QObject::connect(this, &HShifter::engineRPMChanged, &synchroGuard, &SynchroGuard::updateEngineRPM);
    QObject::connect(&stateManager, &StateManager::grindingStateChanged, &synchroGuard, &SynchroGuard::grindingStateChanged);
    QObject::connect(ui.grindIntensitySlider, &QSlider::valueChanged, &synchroGuard, &SynchroGuard::setGrindEffectIntensity);
    QObject::connect(ui.grindRPMSlider, &QSlider::valueChanged, &synchroGuard, &SynchroGuard::updateEngineRPM);
    QObject::connect(ui.keepInGearIdleSlider, &QSlider::valueChanged, &synchroGuard, &SynchroGuard::setKeepInGearIdleIntensity);
    
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
    if (deviceSettings->joystickDeviceComboBox->count()) {
        ui.ffbDeviceFoundLabel->setText("🟢 FFB device detected");
    }

    // Start telemetry receiver
    telemetry.startConnectTimer();

    if (!deviceSettings->joystickDeviceComboBox->count() || !vJoyFeeder::isDriverEnabled()) {
        ui.toggleGameLoopButton->setDisabled(true);
        ui.toggleGameLoopButton->setText("🚫");
        ui.toggleGameLoopButton->setToolTip("Cannot start without FFB joystick and vJoy");
    }
}

HShifter::~HShifter()
{
    if (gameLoopTimer.isActive())
        emit toggleGameLoop(false);
}

void HShifter::initializeGraphics() {
    scene = new QGraphicsScene();
    scene->setSceneRect(ui.graphicsView->viewport()->rect());

    long sceneWidth = ui.graphicsView->viewport()->rect().width();
    long sceneHeight = ui.graphicsView->viewport()->rect().height();
    QPointF center = scene->sceneRect().center();

    neutralChannelRect = new QGraphicsRectItem(0, 0, sceneWidth, SLOT_WIDTH_PX);
    neutralChannelRect->setBrush(QBrush(Qt::black));
    neutralChannelRect->setPen(Qt::NoPen);
    neutralChannelRect->setPos(center - QPointF(sceneWidth / 2, SLOT_WIDTH_PX / 2));
    scene->addItem(neutralChannelRect);

    centerSlotRect = new QGraphicsRectItem(0, 0, SLOT_WIDTH_PX, sceneHeight);
    centerSlotRect->setBrush(QBrush(Qt::black));
    centerSlotRect->setPen(Qt::NoPen);
    centerSlotRect->setPos(center - QPointF(SLOT_WIDTH_PX / 2, sceneHeight / 2));
    scene->addItem(centerSlotRect);

    rightSlotRect = new QGraphicsRectItem(0, 0, SLOT_WIDTH_PX, sceneHeight);
    rightSlotRect->setBrush(QBrush(Qt::black));
    rightSlotRect->setPen(Qt::NoPen);
    rightSlotRect->setPos(QPointF(sceneWidth - SLOT_WIDTH_PX, 0));
    scene->addItem(rightSlotRect);

    leftSlotRect = new QGraphicsRectItem(0, 0, SLOT_WIDTH_PX, sceneHeight);
    leftSlotRect->setBrush(QBrush(Qt::black));
    leftSlotRect->setPen(Qt::NoPen);
    leftSlotRect->setPos(QPointF(0, 0));
    scene->addItem(leftSlotRect);

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

void HShifter::resizeEvent(QResizeEvent* e)
{
    rescaleJoystickMap();
}

// Separate call because the event doesn't trigger if another tab is active
void HShifter::rescaleJoystickMap() {
    if (scene == nullptr) {
        return;
    }
    ui.graphicsView->scene()->setSceneRect(ui.graphicsView->viewport()->rect());

    long sceneWidth = ui.graphicsView->viewport()->rect().width();
    long sceneHeight = ui.graphicsView->viewport()->rect().height();
    QPointF center = scene->sceneRect().center();
   
    neutralChannelRect->setRect(0, 0, sceneWidth, SLOT_WIDTH_PX);
    neutralChannelRect->setPos(center - QPointF(sceneWidth / 2, SLOT_WIDTH_PX / 2));

    centerSlotRect->setRect(0, 0, SLOT_WIDTH_PX, sceneHeight);
    centerSlotRect->setPos(center - QPointF(SLOT_WIDTH_PX / 2, sceneHeight / 2));

    rightSlotRect->setRect(0, 0, SLOT_WIDTH_PX, sceneHeight);
    rightSlotRect->setPos(QPointF(sceneWidth - SLOT_WIDTH_PX, 0));

    leftSlotRect->setRect(0, 0, SLOT_WIDTH_PX, sceneHeight);

    joystickCircle->setPos(center - QPointF(joystickCircle->rect().width() / 2, joystickCircle->rect().height() / 2));
}

void HShifter::updateJoystickCircle(int LRValue, int FBValue) {
    long scaledLRValue = (LRValue * ui.graphicsView->viewport()->rect().width()) / 65535;
    long scaledFBValue = (FBValue * ui.graphicsView->viewport()->rect().height()) / 65535;

    ui.graphicsView->setUpdatesEnabled(false);
    joystickCircle->setPos(QPoint(scaledLRValue, scaledFBValue) - QPointF(joystickCircle->rect().width() / 2, joystickCircle->rect().height() / 2));
    ui.graphicsView->setUpdatesEnabled(true);
}

void HShifter::displayTelemetryState(TelemetrySource newState) {
    if (newState == TelemetrySource::NONE) {
        ui.telemetryLabel->setText("⚠️ Telemetry disconnected");
    }
    else if (newState == TelemetrySource::SCS) {
        ui.telemetryLabel->setText("🟢 ATS/ETS2 telemetry connected");
    }
}

void HShifter::startOnLaunch() {
    ui.toggleGameLoopButton->setChecked(true);
}

void HShifter::updateGearText(int button) {
    if (button) {
        ui.gearLabel->setText(QString::number(button));
    }
    else {
        ui.gearLabel->setText("N");
    }
}

void HShifter::toggleGameLoop(bool newState) {
    ui.toggleGameLoopButton->setText(newState ? "🛑" : "▶️");
    if (newState == true) {
        startGameLoop();
    }
    else
    {
        stopGameLoop();
    }
}

void HShifter::startGameLoop() {
    gameLoopTimer.start(GAMELOOP_INTERVAL_MS);
    deviceSettings->showAxisProgressBars();

    // Acquire joystick
    HWND hwnd = (HWND)(winId());
    qDebug() << "Acquiring joystick...";
    if (FAILED(joystick->acquire(&hwnd))) {
        QMessageBox::critical(this, "Error", "Could not acquire exclusive use of FFB joystick. Please close other games or applications and try again.");
        emit toggleGameLoop(false);
        return;
    };

    // Acquire vJoy for feeding
    if (!vjoy.acquire()) {
        QMessageBox::critical(this, "Error", "Could not acquire vJoy device. Only one program may feed each vJoy device, please close other games or applications and try again.");
        emit toggleGameLoop(false);
        return;
    }

    // Initialize FFB
    HRESULT hr = slotGuard.start(joystick);
    hr = synchroGuard.start(joystick);
}

void HShifter::stopGameLoop() {
    gameLoopTimer.stop();
    deviceSettings->hideAxisProgressBars();

    // Release devices
    vjoy.release();
    joystick->release();
}

// This is the main work function, called once per tick
void HShifter::gameLoop() {
    if (pedals == nullptr || joystick == nullptr) {
        return;
    }

    // Get new joystick values
    QPair<int, int> joystickValues = getJoystickValues();
    
    // Get new pedal values
    QPair<int, int> pedalValues = getPedalValues();

    // Get telemetry values
    if (telemetry.isConnected() != TelemetrySource::NONE) {
        QPair<int, int> gearValues = telemetry.getGearState();
        if (gearValues != lastGearValues) {
            emit gearValuesChanged(gearValues);
            lastGearValues = gearValues;
        }
        float engineRPM = telemetry.getEngineRPM();
        if (engineRPM != lastEngineRPM) {
            emit engineRPMChanged(engineRPM);
            lastEngineRPM = engineRPM;
        }
    }

    // Update state
    stateManager.update(joystickValues, pedalValues, lastGearValues);
}
