/*
Copyright (C) 2024-2025 Ken Monteith.

This file is part of Bonus FFB.

Bonus FFB is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or any later version.

Bonus FFB is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Bonus FFB. If not, see <https://www.gnu.org/licenses/>.
*/

#include "HShifter.h"
#include <QtWidgets/QApplication>
#include <QDebug>
#include <QFile>
#include <QCommandLineParser>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setApplicationName("Bonus FFB");
    a.setStyle("Fusion");
    a.setWindowIcon(QIcon("HShifter.ico"));

    QCommandLineParser parser;
    QCommandLineOption startOnLaunchOption(QStringList() << "s" << "start-on-launch", "Start FFB on launch");
    parser.addOption(startOnLaunchOption);
    QCommandLineOption settingsFileOption(QStringList() << "f" << "settings-file", "Settings file name", "settingsFile");
    parser.addOption(settingsFileOption);
    parser.process(a);

    bool startOnLaunch = parser.isSet(startOnLaunchOption);
    QString settingsFile = parser.value(settingsFileOption);

    qDebug() << "start-on-launch: " << startOnLaunch;

    HShifter w;
    w.show();
    w.loadDeviceSettings();
    if (startOnLaunch) {
        w.startOnLaunch();
    }
    w.initializeGraphics();

    return a.exec();
}
