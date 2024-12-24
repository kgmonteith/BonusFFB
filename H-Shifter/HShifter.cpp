#include "HShifter.h"

HShifter::HShifter(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);
    bffb = BonusFFB();
    QObject::connect(&telemetry, &Telemetry::telemetryConnected,
        ui.telemetryLabel, &QLabel::setText);
    QObject::connect(&telemetry, &Telemetry::telemetryDisconnected,
        ui.telemetryLabel, &QLabel::setText);

    telemetry.startConnectTimer();
}

void HShifter::initializeBonusFFB() {
    bffb.initDirectInput();

    // Populate the clutch device list
    BFFBDIDevice device;
    for (auto const& device : bffb.diDevices)
    {
        ui.clutchDeviceComboBox->addItem(device.instanceName);
    }
    qDebug() << "Is telemetry running? ";
}

void HShifter::setSlider()
{
    ui.horizontalSlider->setValue(50);
    qDebug() << "Progress bar step: " << ui.progressBar->value();
}

HShifter::~HShifter()
{}
