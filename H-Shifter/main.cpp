#include "HShifter.h"
#include <QtWidgets/QApplication>
#include <QDebug>
#include <QFile>
#include <QCommandLineParser>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setStyle("Fusion");

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
    w.loadSettings();
    if (startOnLaunch) {
        w.startOnLaunch();
    }
    w.initializeGraphics();

    return a.exec();
}
