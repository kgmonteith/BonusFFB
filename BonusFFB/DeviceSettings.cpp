#include "DeviceSettings.h"
#include <QProgressBar>
#include <QSizePolicy>

DeviceSettings::DeviceSettings(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);

    // Hide I/O config sliders on startup
    QProgressBar* progressBar = this->findChild<QProgressBar*>("ioTabJoystickLRProgressBar");
    QSizePolicy sp_retain = progressBar->sizePolicy();
    sp_retain.setRetainSizeWhenHidden(true);
    progressBar->setSizePolicy(sp_retain);
    this->findChild<QProgressBar*>("ioTabJoystickFBProgressBar")->setSizePolicy(sp_retain);
    this->findChild<QProgressBar*>("ioTabClutchProgressBar")->setSizePolicy(sp_retain);
    this->findChild<QProgressBar*>("ioTabThrottleProgressBar")->setSizePolicy(sp_retain);
    hideAxisProgressBars();

    // Set up some helper pointers
    joystickDeviceComboBox = this->findChild<QComboBox*>("joystickDeviceComboBox");
    joystickLRAxisComboBox = this->findChild<QComboBox*>("joystickLRAxisComboBox");
    joystickFBAxisComboBox = this->findChild<QComboBox*>("joystickFBAxisComboBox");
    pedalsDeviceComboBox = this->findChild<QComboBox*>("pedalsDeviceComboBox");
    clutchAxisComboBox = this->findChild<QComboBox*>("clutchAxisComboBox");
    throttleAxisComboBox = this->findChild<QComboBox*>("throttleAxisComboBox");
    vjoyDeviceComboBox = this->findChild<QComboBox*>("vjoyDeviceComboBox");
}

DeviceSettings::~DeviceSettings()
{}

void DeviceSettings::showAxisProgressBars() {
    this->findChild<QProgressBar*>("ioTabJoystickLRProgressBar")->show();
    this->findChild<QProgressBar*>("ioTabJoystickFBProgressBar")->show();
    this->findChild<QProgressBar*>("ioTabClutchProgressBar")->show();
    this->findChild<QProgressBar*>("ioTabThrottleProgressBar")->show();
}

void DeviceSettings::hideAxisProgressBars() {
    this->findChild<QProgressBar*>("ioTabJoystickLRProgressBar")->hide();
    this->findChild<QProgressBar*>("ioTabJoystickFBProgressBar")->hide();
    this->findChild<QProgressBar*>("ioTabClutchProgressBar")->hide();
    this->findChild<QProgressBar*>("ioTabThrottleProgressBar")->hide();
}
