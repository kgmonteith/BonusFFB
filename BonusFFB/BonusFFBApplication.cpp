/*
This file is part of Bonus FFB.

Bonus FFB is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or any later version.

Bonus FFB is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Bonus FFB. If not, see <https://www.gnu.org/licenses/>.
*/

#include "BonusFFBApplication.h"
#include <QDate>
#include <QDesktopServices>
#include <QMessageBox>
#include <QUrl>

BonusFFBApplication::BonusFFBApplication(QWidget* parent)
    : QMainWindow(parent)
{
}

BonusFFBApplication::~BonusFFBApplication()
{
}

void BonusFFBApplication::openUserGuide() {
    QDesktopServices::openUrl(QUrl("http://www.kgmonteith.com", QUrl::TolerantMode));
}

void BonusFFBApplication::openAbout() {
    QString about = "Bonus FFB v" + version.toString() + "\n\nCopyright 2024-" + QString::number(QDate::currentDate().year()) + ", Ken Monteith. All rights reserved.";
    QMessageBox::about(this, "About Bonus FFB", about);
}
