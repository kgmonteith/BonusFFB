/*
This file is part of Bonus FFB.

Bonus FFB is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or any later version.

Bonus FFB is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Bonus FFB. If not, see <https://www.gnu.org/licenses/>.
*/

#include "HShifter.h"
#include <QSettings>
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
    QObject::connect(ui.actionSaveSettings, &QAction::triggered, this, &HShifter::saveSettings);
    QObject::connect(ui.actionLoadSettings, &QAction::triggered, this, &HShifter::loadSettings);
    QObject::connect(ui.monitorTabWidget, &QTabWidget::currentChanged, this, &HShifter::rescaleShifterMap);
    QObject::connect(ui.actionUserGuide, &QAction::triggered, this, &BonusFFBApplication::openUserGuide);
    // Telemetry connections
    QObject::connect(&telemetry, &Telemetry::telemetryConnected,
        ui.telemetryLabel, &QLabel::setText);
    QObject::connect(&telemetry, &Telemetry::telemetryDisconnected,
        ui.telemetryLabel, &QLabel::setText);
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
    // Game loop connections
    QObject::connect(ui.toggleGameLoopButton, &QPushButton::toggled, this, &HShifter::toggleGameLoop);
    QObject::connect(&gameLoopTimer, &QTimer::timeout, this, &HShifter::gameLoop);
    
    // Initialize Direct Input, get the list of connected devices
    BonusFFB::initDirectInput(&deviceList);

    // Populate the device lists
    for (auto const device : deviceList)
    {
        ui.pedalsDeviceComboBox->addItem(device.name, device.instanceGuid);
        // Let's go mailroom, help a guy out
        //if (device.supportsFfb && device.productGuid.data1 != VJOY_PRODUCT_GUID) {
            ui.joystickDeviceComboBox->addItem(device.name, device.instanceGuid);
        //}
        if (device.productGuid.data1 == VJOY_PRODUCT_GUID) {
            ui.vjoyDeviceComboBox->addItem(device.name, device.instanceGuid);
        }
    }

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
    seethroughWhite.setAlphaF(0.85);
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

    // This one will have to change...
    joystickCircle->setPos(center - QPointF(joystickCircle->rect().width() / 2, joystickCircle->rect().height() / 2));
}

void HShifter::updateJoystickCircle(int LRValue, int FBValue) {
    long scaledLRValue = (LRValue * ui.graphicsView->viewport()->rect().width()) / 65535;
    long scaledFBValue = (FBValue * ui.graphicsView->viewport()->rect().height()) / 65535;

    ui.graphicsView->setUpdatesEnabled(false);
    joystickCircle->setPos(QPoint(scaledLRValue, scaledFBValue) - QPointF(joystickCircle->rect().width() / 2, joystickCircle->rect().height() / 2));
    ui.graphicsView->setUpdatesEnabled(true);
}

void HShifter::saveSettings() {
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

void HShifter::loadSettings() {
    qDebug() << "Loading settings";
    QString settingsFile = QDir::currentPath() + "/hshifter.ini";
    if (!QFile(settingsFile).exists()) {
        qDebug() << "Settings file does not exist";
        return;
    }
    QSettings settings = QSettings(settingsFile, QSettings::IniFormat);

    settings.beginGroup("joystick");
    ui.joystickDeviceComboBox->setCurrentIndex(ui.joystickDeviceComboBox->findData(settings.value("device_guid").toUuid()));
    ui.joystickLRAxisComboBox->setCurrentIndex(ui.joystickLRAxisComboBox->findData(settings.value("lr_axis").toUuid()));
    ui.invertJoystickLRAxisBox->setChecked(settings.value("invert_lr_axis").toBool());
    ui.joystickFBAxisComboBox->setCurrentIndex(ui.joystickFBAxisComboBox->findData(settings.value("fb_axis").toUuid()));
    ui.invertJoystickFBAxisBox->setChecked(settings.value("invert_fb_axis").toBool());
    settings.endGroup();

    settings.beginGroup("pedals");
    ui.pedalsDeviceComboBox->setCurrentIndex(ui.pedalsDeviceComboBox->findData(settings.value("device_guid").toUuid()));
    ui.clutchAxisComboBox->setCurrentIndex(ui.clutchAxisComboBox->findData(settings.value("clutch_axis").toUuid()));
    ui.invertClutchAxisBox->setChecked(settings.value("invert_clutch_axis").toBool());
    ui.throttleAxisComboBox->setCurrentIndex(ui.throttleAxisComboBox->findData(settings.value("throttle_axis").toUuid()));
    ui.invertThrottleAxisBox->setChecked(settings.value("invert_throttle_axis").toBool());
    settings.endGroup();
}

void HShifter::startOnLaunch() {
    ui.toggleGameLoopButton->setChecked(true);
}

void HShifter::changeJoystickDevice(int deviceIndex) {
    if (joystick != nullptr) {
        BonusFFB::release(joystick);
    }
    QUuid deviceGuid = ui.joystickDeviceComboBox->currentData().toUuid();
    joystick = BonusFFB::getDeviceFromGuid(&deviceList, deviceGuid);
    HWND hwnd = (HWND)(winId());
    BonusFFB::prepare(joystick, &hwnd);
    qDebug() << "New joystick device: " << joystick->name;

    ui.joystickLRAxisComboBox->clear();
    ui.joystickFBAxisComboBox->clear();
    QMap<QUuid, QString> axisMap = BonusFFB::getDeviceAxes(joystick);
    for (auto axis = axisMap.cbegin(), end = axisMap.cend(); axis != end; ++axis)
    {
        ui.joystickLRAxisComboBox->addItem(axis.value(), axis.key());
        ui.joystickFBAxisComboBox->addItem(axis.value(), axis.key());
    }
}

void HShifter::changePedalsDevice(int deviceIndex) {
    if (pedals != nullptr) {
        BonusFFB::release(pedals);
    }
    QUuid deviceGuid = ui.pedalsDeviceComboBox->currentData().toUuid();
    pedals = BonusFFB::getDeviceFromGuid(&deviceList, deviceGuid);
    HWND hwnd = (HWND)(winId());
    BonusFFB::prepare(pedals, &hwnd);
    qDebug() << "New clutch device acquired: " << pedals->name;

    ui.clutchAxisComboBox->clear();
    ui.throttleAxisComboBox->clear();
    QMap<QUuid, QString> axisMap = BonusFFB::getDeviceAxes(pedals);
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
    qDebug() << "toggleGameLoop new state: " << newState;
    ui.toggleGameLoopButton->setText(newState ? "🛑" : "▶️");
    if (newState == true) {
        gameLoopTimer.start(GAMELOOP_INTERVAL_MS);
        ui.ioTabJoystickLRProgressBar->show();
        ui.ioTabJoystickFBProgressBar->show();
        ui.ioTabClutchProgressBar->show();
        ui.ioTabThrottleProgressBar->show();
    }
    else
    {
        gameLoopTimer.stop();
        ui.ioTabJoystickLRProgressBar->hide();
        ui.ioTabJoystickFBProgressBar->hide();
        ui.ioTabClutchProgressBar->hide();
        ui.ioTabThrottleProgressBar->hide();
    }
}
// This is the main work function, called once per tick
void HShifter::gameLoop() {
    if (pedals == nullptr || joystick == nullptr) {
        return;
    }

    DIJOYSTATE2 joystickState;
    BonusFFB::updateState(joystick, &joystickState);
    long joystickLRValue = BonusFFB::getAxisReading(&joystickState, joystickLRAxisGuid);
    long joystickFBValue = BonusFFB::getAxisReading(&joystickState, joystickFBAxisGuid);
    if (ui.invertJoystickLRAxisBox->isChecked()) {
        joystickLRValue = abs(65535 - joystickLRValue);
    }
    if (ui.invertJoystickFBAxisBox->isChecked()) {
        joystickFBValue = abs(65535 - joystickFBValue);

    }
    emit joystickLRValueChanged(joystickLRValue);
    emit joystickFBValueChanged(joystickFBValue);
    emit joystickValueChanged(joystickLRValue, joystickFBValue);

    DIJOYSTATE2 pedalsState;
    BonusFFB::updateState(pedals, &pedalsState);
    long clutchValue = BonusFFB::getAxisReading(&pedalsState, clutchAxisGuid);
    if (ui.invertClutchAxisBox->isChecked()) {
        clutchValue = abs(65535 - clutchValue);
    }
    emit clutchValueChanged(clutchValue);
    long throttleValue = BonusFFB::getAxisReading(&pedalsState, throttleAxisGuid);
    if (ui.invertThrottleAxisBox->isChecked()) {
        throttleValue = abs(65535 - throttleValue);
    }
    emit throttleValueChanged(throttleValue);

}

HShifter::~HShifter()
{
    emit toggleGameLoop(false);
}
