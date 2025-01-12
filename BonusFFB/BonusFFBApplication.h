/*
This file is part of Bonus FFB.

Bonus FFB is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or any later version.

Bonus FFB is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Bonus FFB. If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include "version.h"
#include <QtWidgets/QMainWindow>
#include <QTimer>
#include <QVersionNumber>

// This is a parent class implementing functions common to individual Bonus FFB apps
class BonusFFBApplication : public QMainWindow
{
	Q_OBJECT

public:
	BonusFFBApplication(QWidget* parent = nullptr);
	~BonusFFBApplication();

	QVersionNumber version = QVersionNumber(MAJOR_VERSION, MINOR_VERSION, PATCH_VERSION);

public slots:
	void openUserGuide();
	void openAbout();
};

