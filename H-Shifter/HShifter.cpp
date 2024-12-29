#include "HShifter.h"
#include <QSettings>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsRectItem>
#include <QGraphicsEllipseItem>
#include <QDir>
#include <stdlib.h>

HShifter::HShifter(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);

    //QLabel* testLabel = new QLabel("testing", ui.shifterMapGroupBox);

    // Menu action connections
    QObject::connect(ui.actionExit, &QAction::triggered, this, &HShifter::close);
    QObject::connect(ui.actionSaveSettings, &QAction::triggered, this, &HShifter::saveSettings);
    QObject::connect(ui.actionLoadSettings, &QAction::triggered, this, &HShifter::loadSettings);
    // Telemetry connections
    QObject::connect(&telemetry, &Telemetry::telemetryConnected,
        ui.telemetryLabel, &QLabel::setText);
    QObject::connect(&telemetry, &Telemetry::telemetryDisconnected,
        ui.telemetryLabel, &QLabel::setText);
    // Pedal connections
    QObject::connect(this, &HShifter::clutchValueChanged, ui.clutchProgressBar, &QProgressBar::setValue);
    QObject::connect(this, &HShifter::throttleValueChanged, ui.throttleProgressBar, &QProgressBar::setValue);
    QObject::connect(ui.ffbJoystickDeviceComboBox, &QComboBox::currentIndexChanged, this, &HShifter::changeFFBJoystickDevice);
    QObject::connect(ui.clutchDeviceComboBox, &QComboBox::currentIndexChanged, this, &HShifter::changePedalsDevice);
    QObject::connect(ui.clutchAxisComboBox, &QComboBox::currentIndexChanged, this, &HShifter::changeClutchAxis);
    QObject::connect(ui.throttleAxisComboBox, &QComboBox::currentIndexChanged, this, &HShifter::changeThrottleAxis);
    QObject::connect(ui.toggleGameLoopButton, &QPushButton::toggled, this, &HShifter::toggleGameLoop);

    QObject::connect(&gameLoopTimer, &QTimer::timeout, this, &HShifter::gameLoop);
    
    // Initialize Direct Input, get the list of connected devices
    BonusFFB::initDirectInput(&deviceList);

    // Populate the device lists
    for (auto const device : deviceList)
    {
        ui.clutchDeviceComboBox->addItem(device.name, device.instanceGuid);
        if (device.supportsFfb && device.productGuid.data1 != VJOY_PRODUCT_GUID) {
            ui.ffbJoystickDeviceComboBox->addItem(device.name, device.instanceGuid);
        }
        if (device.productGuid.data1 == VJOY_PRODUCT_GUID) {
            ui.vjoyDeviceComboBox->addItem(device.name, device.instanceGuid);
        }
    }

    telemetry.startConnectTimer();
}

void HShifter::initializeGraphics() {
    scene = new QGraphicsScene();
    scene->setSceneRect(ui.graphicsView->viewport()->rect());
    qDebug() << "sceneRect " << scene->sceneRect();

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


void HShifter::saveSettings() {
    QSettings settings = QSettings(QDir::currentPath() + "/hshifter.ini", QSettings::IniFormat);
    settings.beginGroup("clutch");
    settings.setValue("device_guid", pedals->instanceGuid.toString());
    settings.setValue("axis", clutchAxisGuid.toString());
    settings.setValue("invert_axis", ui.invertClutchAxisBox->isChecked());
    settings.endGroup();
}

void HShifter::loadSettings() {
    qDebug() << "Loading settings";
    QSettings settings = QSettings(QDir::currentPath() + "/hshifter.ini", QSettings::IniFormat);
    settings.beginGroup("clutch");
    ui.clutchDeviceComboBox->setCurrentIndex(ui.clutchDeviceComboBox->findData(settings.value("device_guid").toUuid()));
    ui.clutchAxisComboBox->setCurrentIndex(ui.clutchAxisComboBox->findData(settings.value("axis").toUuid()));
    ui.invertClutchAxisBox->setChecked(settings.value("invert_axis").toBool());
    settings.endGroup();
}

void HShifter::startOnLaunch() {
    ui.toggleGameLoopButton->setChecked(true);
}

void HShifter::toggleGameLoop(bool newState) {
    qDebug() << "toggleGameLoop new state: " << newState;
    ui.toggleGameLoopButton->setText(newState ? "🛑" : "▶️");
    if (newState == true) {
        gameLoopTimer.start(GAMELOOP_INTERVAL_MS);
    }
    else
    {
        gameLoopTimer.stop();
    }
}

void HShifter::changeFFBJoystickDevice(int deviceIndex) {
    qDebug() << "New FFB joystick device: " << ui.ffbJoystickDeviceComboBox->currentData();
}

void HShifter::changePedalsDevice(int deviceIndex) {
    if (pedals != nullptr) {
        BonusFFB::release(pedals);
    }
    QUuid deviceGuid = ui.clutchDeviceComboBox->currentData().toUuid();
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

void HShifter::changeClutchAxis(int axisIndex) {
    clutchAxisGuid = ui.clutchAxisComboBox->currentData().toUuid();
}

void HShifter::changeThrottleAxis(int axisIndex) {
    throttleAxisGuid = ui.throttleAxisComboBox->currentData().toUuid();
}

// This is the main work function, called once per tick
void HShifter::gameLoop() {
    if (pedals == nullptr) {
        return;
    }

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
    qDebug() << throttleValue;
    emit throttleValueChanged(throttleValue);

}

HShifter::~HShifter()
{
    emit toggleGameLoop(false);
}
