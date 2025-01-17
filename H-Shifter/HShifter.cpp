/*
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

    // Hide I/O config sliders on startup
    QSizePolicy sp_retain = ui.ioTabJoystickLRProgressBar->sizePolicy();
    sp_retain.setRetainSizeWhenHidden(true);
    ui.ioTabJoystickLRProgressBar->setSizePolicy(sp_retain);
    ui.ioTabJoystickLRProgressBar->hide();
    ui.ioTabJoystickFBProgressBar->setSizePolicy(sp_retain);
    ui.ioTabJoystickFBProgressBar->hide();
    ui.ioTabClutchProgressBar->setSizePolicy(sp_retain);
    ui.ioTabClutchProgressBar->hide();
    ui.ioTabThrottleProgressBar->setSizePolicy(sp_retain);
    ui.ioTabThrottleProgressBar->hide();

    // Menu action connections
    QObject::connect(ui.actionExit, &QAction::triggered, this, &HShifter::close);
    QObject::connect(ui.actionSaveSettings, &QAction::triggered, this, &HShifter::saveDeviceSettings);
    QObject::connect(ui.actionLoadSettings, &QAction::triggered, this, &HShifter::loadDeviceSettings);
    QObject::connect(ui.monitorTabWidget, &QTabWidget::currentChanged, this, &HShifter::rescaleShifterMap);
    QObject::connect(ui.actionUserGuide, &QAction::triggered, this, &BonusFFBApplication::openUserGuide);
    QObject::connect(ui.actionAbout, &QAction::triggered, this, &BonusFFBApplication::openAbout);
    // Telemetry connections
    QObject::connect(&telemetry, &Telemetry::telemetryConnected, ui.telemetryLabel, &QLabel::setText);
    QObject::connect(&telemetry, &Telemetry::telemetryDisconnected, ui.telemetryLabel, &QLabel::setText);
    // Joystick connections
    QObject::connect(this, &HShifter::joystickLRValueChanged, ui.ioTabJoystickLRProgressBar, &QProgressBar::setValue);
    QObject::connect(this, &HShifter::joystickFBValueChanged, ui.ioTabJoystickFBProgressBar, &QProgressBar::setValue);
    QObject::connect(this, &HShifter::joystickValueChanged, this, &HShifter::updateJoystickCircle);
    QObject::connect(ui.joystickDeviceComboBox, &QComboBox::currentIndexChanged, this, &HShifter::changeJoystickDevice);
    QObject::connect(ui.joystickLRAxisComboBox, &QComboBox::currentIndexChanged, this, &HShifter::changeJoystickLRAxis);
    QObject::connect(ui.joystickFBAxisComboBox, &QComboBox::currentIndexChanged, this, &HShifter::changeJoystickFBAxis);
    // Pedal connections
    QObject::connect(this, &HShifter::clutchValueChanged, ui.clutchProgressBar, &QProgressBar::setValue);
    QObject::connect(this, &HShifter::clutchValueChanged, ui.ioTabClutchProgressBar, &QProgressBar::setValue);
    QObject::connect(this, &HShifter::throttleValueChanged, ui.throttleProgressBar, &QProgressBar::setValue);
    QObject::connect(this, &HShifter::throttleValueChanged, ui.ioTabThrottleProgressBar, &QProgressBar::setValue);
    QObject::connect(ui.pedalsDeviceComboBox, &QComboBox::currentIndexChanged, this, &HShifter::changePedalsDevice);
    QObject::connect(ui.clutchAxisComboBox, &QComboBox::currentIndexChanged, this, &HShifter::changeClutchAxis);
    QObject::connect(ui.throttleAxisComboBox, &QComboBox::currentIndexChanged, this, &HShifter::changeThrottleAxis);
    // vJoy connections
    QObject::connect(ui.vjoyDeviceComboBox, &QComboBox::currentIndexChanged, &vjoy, &vJoyFeeder::setDeviceIndex);
    QObject::connect(&stateManager, &StateManager::buttonZoneChanged, &vjoy, &vJoyFeeder::updateButtons);
    // Game loop connections
    QObject::connect(ui.toggleGameLoopButton, &QPushButton::toggled, this, &HShifter::toggleGameLoop);
    QObject::connect(&gameLoopTimer, &QTimer::timeout, this, &HShifter::gameLoop);
    // FFB effect connections
    QObject::connect(&stateManager, &StateManager::slotStateChanged, &slotGuard, &SlotGuard::updateSlotGuardEffects);
    QObject::connect(this, &HShifter::clutchValueChanged, &synchroGuard, &SynchroGuard::updateClutchEngagement);
    QObject::connect(&stateManager, &StateManager::synchroStateChanged, &synchroGuard, &SynchroGuard::synchroStateChanged);
    
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
    qDebug() << "Is vJoy enabled? " << vJoyFeeder::isDriverEnabled() << ", driver match: " << vJoyFeeder::checkVersionMatch();
    qDebug() << "vJoy device count: " << vJoyFeeder::deviceCount();
    

    // Initialize Direct Input, get the list of connected devices
    BonusFFB::initDirectInput(&deviceList);

    // Populate the device lists
    for (auto const device : deviceList)
    {
        ui.pedalsDeviceComboBox->addItem(device.name, device.instanceGuid);
        if (device.supportsFfb && device.productGuid.data1 != VJOY_PRODUCT_GUID) {
            ui.joystickDeviceComboBox->addItem(device.name, device.instanceGuid);
            ui.ffbDeviceFoundLabel->setText("🟢 FFB device detected");
        }
    }

    for (int i = 0; i < vJoyFeeder::deviceCount(); i++) {
        ui.vjoyDeviceComboBox->addItem(QString("vJoy Device ").append(QString(" %1").arg(i+1)), i+1);
    }
    // Start telemetry receiver
    telemetry.startConnectTimer();
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
    QColor seethroughWhite = Qt::white;
    seethroughWhite.setAlphaF(float(0.85));
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
    rescaleShifterMap();
}

// Separate call because the event doesn't trigger if another tab is active
void HShifter::rescaleShifterMap() {
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

void HShifter::saveDeviceSettings() {
    QSettings settings = QSettings(QDir::currentPath() + "/hshifter.ini", QSettings::IniFormat);
    settings.beginGroup("joystick");
    settings.setValue("device_guid", joystick->instanceGuid.toString());
    settings.setValue("lr_axis", joystickLRAxisGuid.toString());
    settings.setValue("invert_lr_axis", ui.invertJoystickLRAxisBox->isChecked());
    settings.setValue("fb_axis", joystickFBAxisGuid.toString());
    settings.setValue("invert_fb_axis", ui.invertJoystickFBAxisBox->isChecked());
    settings.endGroup();

    settings.beginGroup("pedals");
    settings.setValue("device_guid", pedals->instanceGuid.toString());
    settings.setValue("clutch_axis", clutchAxisGuid.toString());
    settings.setValue("invert_clutch_axis", ui.invertClutchAxisBox->isChecked());
    settings.setValue("throttle_axis", throttleAxisGuid.toString());
    settings.setValue("invert_throttle_axis", ui.invertThrottleAxisBox->isChecked());
    settings.endGroup();
}

void HShifter::loadDeviceSettings() {
    qDebug() << "Loading settings";
    QString settingsFile = QDir::currentPath() + "/hshifter.ini";
    if (!QFile(settingsFile).exists()) {
        qDebug() << "Settings file does not exist";
        return;
    }
    QSettings settings = QSettings(settingsFile, QSettings::IniFormat);

    settings.beginGroup("joystick");
    int joystick_index = ui.joystickDeviceComboBox->findData(settings.value("device_guid").toUuid());
    if (joystick_index == -1) {
        QMessageBox::warning(this, "Joystick not found", "Saved joystick device is not connected.\nReconnect the device or update the input/output settings.");
    }
    else
    {
        ui.joystickDeviceComboBox->setCurrentIndex(joystick_index);
        ui.joystickLRAxisComboBox->setCurrentIndex(ui.joystickLRAxisComboBox->findData(settings.value("lr_axis").toUuid()));
        ui.invertJoystickLRAxisBox->setChecked(settings.value("invert_lr_axis").toBool());
        ui.joystickFBAxisComboBox->setCurrentIndex(ui.joystickFBAxisComboBox->findData(settings.value("fb_axis").toUuid()));
        ui.invertJoystickFBAxisBox->setChecked(settings.value("invert_fb_axis").toBool());
    }
    settings.endGroup();

    settings.beginGroup("pedals");
    int pedals_index = ui.pedalsDeviceComboBox->findData(settings.value("device_guid").toUuid());
    if (pedals_index == -1) {
        QMessageBox::warning(this, "Pedals not found", "Saved pedals device is not connected.\nReconnect the device or update the input/output settings.");
    }
    else
    {
        ui.pedalsDeviceComboBox->setCurrentIndex(pedals_index);
        ui.clutchAxisComboBox->setCurrentIndex(ui.clutchAxisComboBox->findData(settings.value("clutch_axis").toUuid()));
        ui.invertClutchAxisBox->setChecked(settings.value("invert_clutch_axis").toBool());
        ui.throttleAxisComboBox->setCurrentIndex(ui.throttleAxisComboBox->findData(settings.value("throttle_axis").toUuid()));
        ui.invertThrottleAxisBox->setChecked(settings.value("invert_throttle_axis").toBool());
    }
    settings.endGroup();
}

void HShifter::startOnLaunch() {
    ui.toggleGameLoopButton->setChecked(true);
}

void HShifter::changeJoystickDevice(int deviceIndex) {
    // Release previous joystick
    if (joystick != nullptr) {
        joystick->release();
    }
    QUuid deviceGuid = ui.joystickDeviceComboBox->currentData().toUuid();
    joystick = BonusFFB::getDeviceFromGuid(&deviceList, deviceGuid);
    qDebug() << "New joystick device: " << joystick->name;

    ui.joystickLRAxisComboBox->clear();
    ui.joystickFBAxisComboBox->clear();
    QMap<QUuid, QString> axisMap = joystick->getDeviceAxes();
    for (auto axis = axisMap.cbegin(), end = axisMap.cend(); axis != end; ++axis)
    {
        ui.joystickLRAxisComboBox->addItem(axis.value(), axis.key());
        ui.joystickFBAxisComboBox->addItem(axis.value(), axis.key());
    }
}

void HShifter::changePedalsDevice(int deviceIndex) {
    if (pedals != nullptr) {
        pedals->release();
    }
    QUuid deviceGuid = ui.pedalsDeviceComboBox->currentData().toUuid();
    pedals = BonusFFB::getDeviceFromGuid(&deviceList, deviceGuid);
    HWND hwnd = (HWND)(winId());
    pedals->acquire(&hwnd);
    qDebug() << "New clutch device acquired: " << pedals->name;

    ui.clutchAxisComboBox->clear();
    ui.throttleAxisComboBox->clear();
    QMap<QUuid, QString> axisMap = pedals->getDeviceAxes();
    for (auto axis = axisMap.cbegin(), end = axisMap.cend(); axis != end; ++axis)
    {
        ui.clutchAxisComboBox->addItem(axis.value(), axis.key());
        ui.throttleAxisComboBox->addItem(axis.value(), axis.key());
    }
}

void HShifter::changeJoystickLRAxis(int axisIndex) {
    joystickLRAxisGuid = ui.joystickLRAxisComboBox->currentData().toUuid();
}

void HShifter::changeJoystickFBAxis(int axisIndex) {
    joystickFBAxisGuid = ui.joystickFBAxisComboBox->currentData().toUuid();
}

void HShifter::changeClutchAxis(int axisIndex) {
    clutchAxisGuid = ui.clutchAxisComboBox->currentData().toUuid();
}

void HShifter::changeThrottleAxis(int axisIndex) {
    throttleAxisGuid = ui.throttleAxisComboBox->currentData().toUuid();
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
    ui.ioTabJoystickLRProgressBar->show();
    ui.ioTabJoystickFBProgressBar->show();
    ui.ioTabClutchProgressBar->show();
    ui.ioTabThrottleProgressBar->show();

    // Acquire joystick
    HWND hwnd = (HWND)(winId());
    qDebug() << "Acquiring joystick...";
    if (FAILED(joystick->acquire(&hwnd))) {
        QMessageBox::critical(this, "Error", "Could not acquire exclusive use of FFB joystick. Please close other games or applications and try again.");
        emit toggleGameLoop(false);
        return;
    };

    // Acquire vJoy for feeding
    vjoy.acquire();

    // Initialize FFB
    HRESULT hr = slotGuard.start(joystick);
    hr = synchroGuard.start(joystick);
}

void HShifter::stopGameLoop() {
    gameLoopTimer.stop();
    ui.ioTabJoystickLRProgressBar->hide();
    ui.ioTabJoystickFBProgressBar->hide();
    ui.ioTabClutchProgressBar->hide();
    ui.ioTabThrottleProgressBar->hide();

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
    joystick->updateState();
    long joystickLRValue = joystick->getAxisReading(joystickLRAxisGuid);
    long joystickFBValue = joystick->getAxisReading(joystickFBAxisGuid);
    if (ui.invertJoystickLRAxisBox->isChecked()) {
        joystickLRValue = abs(65535 - joystickLRValue);
    }
    if (ui.invertJoystickFBAxisBox->isChecked()) {
        joystickFBValue = abs(65535 - joystickFBValue);

    }
    emit joystickLRValueChanged(joystickLRValue);
    emit joystickFBValueChanged(joystickFBValue);
    emit joystickValueChanged(joystickLRValue, joystickFBValue);

    // Get new pedal values
    pedals->updateState();
    long clutchValue = pedals->getAxisReading(clutchAxisGuid);
    if (ui.invertClutchAxisBox->isChecked()) {
        clutchValue = abs(65535 - clutchValue);
    }
    emit clutchValueChanged(clutchValue);
    long throttleValue = pedals->getAxisReading(throttleAxisGuid);
    if (ui.invertThrottleAxisBox->isChecked()) {
        throttleValue = abs(65535 - throttleValue);
    }
    emit throttleValueChanged(throttleValue);

    // State things...
    stateManager.update(joystickLRValue, joystickFBValue, clutchValue, throttleValue);
    //auto state = slotGuard.update(joystickLRValue, joystickFBValue);
    //synchroGuard.update(joystickLRValue, joystickFBValue, clutchValue, throttleValue, false);
}

HShifter::~HShifter()
{
    emit toggleGameLoop(false);
}
