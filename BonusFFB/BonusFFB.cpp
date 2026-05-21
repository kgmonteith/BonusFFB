/*
Copyright (C) 2024-2026 Ken Monteith.

This file is part of Bonus FFB.

Bonus FFB is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or any later version.

Bonus FFB is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Bonus FFB. If not, see <https://www.gnu.org/licenses/>.
*/

#include "BonusFFB.h"
#include <QMessageBox>
#include <QDir>
#include <QDesktopServices>
#include <QInputDialog>
#include <QFileDialog>

BonusFFB::BonusFFB(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);
    
    // Configure app selection button group
    appSelectButtonGroup.setExclusive(true);
    appSelectButtonGroup.addButton(ui.heavytruck_appSelectButton, 0);
    appSelectButtonGroup.addButton(ui.hshifter_appSelectButton, 1);
    appSelectButtonGroup.addButton(ui.prndl_appSelectButton, 2);
    appSelectButtonGroup.addButton(ui.pphc_appSelectButton, 3);
    appSelectButtonGroup.addButton(ui.handbrake_appSelectButton, 4);
    appList.append(&heavytruck);
    appList.append(&hshifter);
    appList.append(&prndl);
    appList.append(&pphc);
    appList.append(&handbrake);
    connect(&appSelectButtonGroup, &QButtonGroup::idClicked, this, &BonusFFB::changeApp);
    ui.appStackedWidget->setCurrentIndex(0);

    // Ensure the monitor is the default tab
    ui.heavytruckTabWidget->setCurrentIndex(0);
    ui.hshifterTabWidget->setCurrentIndex(0);
    ui.prndlTabWidget->setCurrentIndex(0);
    ui.pphcTabWidget->setCurrentIndex(0);
    ui.handbrakeTabWidget->setCurrentIndex(0);

    // Menu action connections
    connect(ui.actionSaveSettings, &QAction::triggered, this, &BonusFFB::saveActiveProfile);
    connect(ui.actionSaveSettingsNew, &QAction::triggered, this, &BonusFFB::saveNewProfile);
    connect(ui.actionOpen_profile_directory, &QAction::triggered, this, &BonusFFB::openProfileFolder);
    connect(ui.actionConfigure_input_output_devices, &QAction::triggered, this, &BonusFFB::stop);
    connect(ui.actionConfigure_input_output_devices, &QAction::triggered, &devices, &DeviceConfiguration::openConfigurationDialog);
    connect(ui.actionReset_profile_to_default_settings, &QAction::triggered, this, &BonusFFB::loadDefaultProfile);
    connect(ui.actionLoad_profile, &QAction::triggered, this, &BonusFFB::loadProfileDialog);
    connect(ui.actionExit, &QAction::triggered, this, &BonusFFB::close);
    connect(ui.actionUserGuide, &QAction::triggered, this, &BonusFFB::openUserGuide);
    connect(ui.actionAbout, &QAction::triggered, this, &BonusFFB::openAbout);
    connect(&devices, &DeviceConfiguration::deviceConfigurationChanged, this, &BonusFFB::updateStartButton);
    // Game loop connections
    connect(ui.startButton, &QPushButton::clicked, this, &BonusFFB::startButtonClicked);
    // Telemetry connections
    connect(&telemetry, &Telemetry::telemetryChanged, this, &BonusFFB::displayTelemetryState);
    // Connect sliders to spinboxes
    connectSlidersToSpinBoxes();

    // Initialize Direct Input, get the list of connected devices
    devices.initialize((HWND)(winId()));

    // Set FFB device detection label
    if (devices.isFFBDeviceInstalled()) {
        ui.ffbDeviceFoundLabel->setText("🟢 FFB-enabled device detected");
    }

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

    // Initialize application GUIs
    for (auto app : appList) {
        app->setPointers(&ui, &devices, &telemetry);
        app->initialize();
    }
    changeApp(0);

    // Start telemetry receiver
    telemetry.startConnectTimer();

    // Load active profile. Defaults will be loaded if the active profile is invalid.
    loadActiveProfile();

    // Disable start button if app isn't happy
    updateStartButton();

    qDebug("BonusFFBApplication constructor finished");
}

BonusFFB::~BonusFFB()
{
    if (gameLoopTimer.isActive())
        stop();
}

void BonusFFB::changeApp(int appSelectButtonIndex) {
    ui.appStackedWidget->setCurrentIndex(appSelectButtonIndex);
    activeApp = appList[appSelectButtonIndex];
    activeApp->redrawJoystickMap();

    // Check if device configuration is suitable for the app
    updateStartButton();
}

void BonusFFB::connectSlidersToSpinBoxes() {
    QList<QSlider*> sliders = ui.centralWidget->findChildren<QSlider*>();
    for (QSlider* slider : sliders) {
        QString sliderName = slider->objectName();
        QString targetSpinBoxName = sliderName.replace("Slider", "Spinbox");
        QSpinBox* matchingSpinBox = ui.centralWidget->findChild<QSpinBox*>(targetSpinBoxName);
        if (matchingSpinBox) {
            QObject::connect(slider, &QSlider::valueChanged, matchingSpinBox, &QSpinBox::setValue);
            QObject::connect(matchingSpinBox, qOverload<int>(&QSpinBox::valueChanged), slider, &QSlider::setValue);
        }
        else {
            qDebug() << "No matching QSpinBox found for" << sliderName;
        }
    }
}

void BonusFFB::resizeEvent(QResizeEvent* e)
{
    activeApp->redrawJoystickMap();
}

void BonusFFB::saveNewProfile() {
    QInputDialog dialog(this);
    dialog.setWindowTitle("Save new profile");
    dialog.setLabelText("New profile name:");
    if (dialog.exec() == QDialog::Accepted) {
        saveProfile(dialog.textValue());
    }
}

void BonusFFB::saveProfile(QString profileName) {
    if (profileName.isEmpty()) {
        saveNewProfile();
        return;
    }
    QString safeName = profileName;
    safeName.remove(QRegularExpression("[\\\\/:*?\"<>|]"));
    QDir profileDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/profiles/");
    QSettings settings = QSettings(profileDir.filePath(safeName + ".ini"), QSettings::IniFormat);

    if (!settings.isWritable()) {
        QMessageBox::warning(this, "Failed to save profile", "Unable to write to profile configuration file.");
        return;
    }

    settings.setValue("profile_name", profileName);
    settings.setValue("app_version", QVersionNumber(MAJOR_VERSION, MINOR_VERSION, PATCH_VERSION).toString());

    for (auto app : appList) {
        app->saveSettings(&settings);
    }

    QDir appSettingsDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
    QSettings activeProfile = QSettings(appSettingsDir.filePath("active_profile.ini"), QSettings::IniFormat);
    activeProfile.setValue("profile_path", settings.fileName());
    active_profile_path = settings.fileName();
    active_profile_name = profileName;
    setProfileDisplayName();
}

void BonusFFB::loadActiveProfile() {
    QDir appSettingsDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
    QSettings settings = QSettings(appSettingsDir.filePath("active_profile.ini"), QSettings::IniFormat);
    loadProfile(settings.value("profile_path").toString());
}

void BonusFFB::loadDefaultProfile() {
    loadProfile("");
}

void BonusFFB::loadProfileDialog() {
    QString profileFolder = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/profiles";
    QDir dir;
    if (!dir.exists(profileFolder)) {
        dir.mkdir(profileFolder);
    }
    QString profilePath = QFileDialog::getOpenFileName(this,
        tr("Load profile"), profileFolder, tr("Profile configs (*.ini)"));
    if (!profilePath.isEmpty()) {
        loadProfile(profilePath);
    }
}

void BonusFFB::loadProfile(QString profilePath) {
    // If profilePath is empty, default values will be loaded
    qDebug() << "Loading profile: " << profilePath;
    QSettings activeProfile = QSettings(profilePath, QSettings::IniFormat);
    if (!profilePath.isEmpty()) {
        active_profile_name = activeProfile.value("profile_name").toString();
        active_profile_path = profilePath;
    }

    for (auto app : appList) {
        app->loadSettings(&activeProfile);
    }
    setProfileDisplayName();
}

void BonusFFB::saveActiveProfile() {
    QSettings profile = QSettings(active_profile_path, QSettings::IniFormat);
    QString profileName = profile.value("profile_name").toString();
    saveProfile(profileName);
}

void BonusFFB::setProfileDisplayName() {
    QString saveProfileText = "Save profile";
    if (!active_profile_name.isEmpty()) {
        saveProfileText += " (" + active_profile_name + ")";
    }
    ui.actionSaveSettings->setText(saveProfileText);
}

void BonusFFB::openProfileFolder() {
    QString folderName = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/profiles";
    QDir dir;
    if (!dir.exists(folderName)) {
        dir.mkdir(folderName);
    }
    QDesktopServices::openUrl(QUrl("file:///" + folderName));
}

void BonusFFB::openUserGuide() {
    QDesktopServices::openUrl(QUrl("https://kgmonteith.github.io/BonusFFB/", QUrl::TolerantMode));
}

void BonusFFB::openAbout() {
    QString about = "Bonus FFB v" + version.toString() + "\n\nCopyright 2024-" + QString::number(QDate::currentDate().year()) + ", Ken Monteith. All rights reserved.\n\nThis program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.\n\nThis program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.\n\nYou should have received a copy of the GNU General Public License along with this program. If not, see < https://www.gnu.org/licenses/>.";
    QMessageBox::about(this, "About Bonus FFB", about);
}

void BonusFFB::displayTelemetryState(TelemetrySource newState) {
    if (newState == TelemetrySource::NONE) {
        ui.telemetryLabel->setText("⚠️ Telemetry disconnected");
    }
    else if (newState == TelemetrySource::SCS) {
        ui.telemetryLabel->setText("🟢 ATS/ETS2 telemetry connected");
    }
}

void BonusFFB::updateStartButton() {
    qDebug() << "updateStartButton invoked";
    int deviceStatus = devices.ready(activeApp->appDeviceFlags);
    if (deviceStatus == DEVICES_NOT_AVAILABLE) {
        ui.startButton->setDisabled(true);
        ui.startButton->setText("🚫");
        ui.startButton->setToolTip("Cannot start without FFB joystick and vJoy");
        ui.startButton->setCheckable(false);
    }
    else if (deviceStatus == DEVICES_NOT_CONFIGURED) {
        ui.startButton->setDisabled(false);
        ui.startButton->setText("🛠️");
        ui.startButton->setToolTip("Devices must be configured before " + activeApp->getAppName() + " can run");
        ui.startButton->setCheckable(false);
    }
    else {
        ui.startButton->setDisabled(false);
        ui.startButton->setText("▶️");
        ui.startButton->setToolTip("");
    }
}

void BonusFFB::startButtonClicked() {
    qDebug() << "startButtonClicked";
    if (ui.startButton->text() == "▶️") {
        start();
    }
    else if (ui.startButton->text() == "🛑") {
        stop();
    }
    else if (ui.startButton->text() == "🛠️") {
        devices.openConfigurationDialog();
    }
}

void BonusFFB::start() {
    if (!gameLoopTimer.isActive())
    {
        gameLoopTimer.start(GAMELOOP_INTERVAL_MS);
        for (QAbstractButton* button : appSelectButtonGroup.buttons()) {
            button->setEnabled(false);
        }
        qDebug() << "Active app: " << activeApp->getAppName();
        if (FAILED(activeApp->start())) {
            qDebug() << "Failed to start game loop, stopping";
            stop();
            return;
        }
        ui.startButton->setText("🛑");
        ui.startButton->setCheckable(true);
        ui.startButton->setChecked(true);
        connect(&gameLoopTimer, &QTimer::timeout, activeApp, &BonusFFBApp::gameLoop);
    }
}

void BonusFFB::stop() {
    if (gameLoopTimer.isActive())
    {
        gameLoopTimer.stop();
        QObject::disconnect(&gameLoopTimer, &QTimer::timeout, activeApp, &BonusFFBApp::gameLoop);
        activeApp->stop();
        for (QAbstractButton* button : appSelectButtonGroup.buttons()) {
            button->setEnabled(true);
        }
        ui.startButton->setText("▶️");
        ui.startButton->setCheckable(true);
        ui.startButton->setChecked(false);
    }
}
