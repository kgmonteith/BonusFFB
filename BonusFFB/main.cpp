/*
Copyright (C) 2024-2026 Ken Monteith.

This file is part of Bonus FFB.

Bonus FFB is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or any later version.

Bonus FFB is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Bonus FFB. If not, see <https://www.gnu.org/licenses/>.
*/

#include "BonusFFB.h"
#include <QtWidgets/QApplication>
#include <QFile>
#include <QDir>

QFile logFile;

void fileLogMessageHandler(const QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    if (logFile.isOpen()) {
        logFile.write(qUtf8Printable(qFormatLogMessage(type, context, msg) + "\n"));
        logFile.flush();
    }
#ifdef Q_OS_WIN
    QString vsText = qFormatLogMessage(type, context, msg) + "\r\n";
    OutputDebugStringW(reinterpret_cast<const wchar_t*>(vsText.utf16()));
#endif
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setStyle("Fusion");
    app.setWindowIcon(QIcon("BonusFFB.ico"));

    // Enable debug logging to file
    QString logDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/logs/");
    QDir dir;
    if (!dir.exists(logDir)) {
        dir.mkdir(logDir);
    }
    dir.setPath(logDir);
    logFile.setFileName(dir.absoluteFilePath("%1.log").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss")));
    logFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append);
    qInstallMessageHandler(fileLogMessageHandler);
    qSetMessagePattern("%{time yyyy-MM-dd hh:mm:ss,zzz} [%{type}]: %{message}");

    BonusFFB window;
    window.show();
    for (const auto app : window.appList) {
        app->initializeJoystickMap();
        //app->loadSettings();
    }
    qDebug() << "Finished launching";
    return app.exec();
}
