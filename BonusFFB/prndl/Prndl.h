/*
Copyright (C) 2024-2026 Ken Monteith.

This file is part of Bonus FFB.

Bonus FFB is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or any later version.

Bonus FFB is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Bonus FFB. If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include "BonusFFBApp.h"
#include "PrndlSlotGuard.h"
#include "PrndlStateManager.h"

#define SHIFTER_POSITION_MARKER_DIAMETER_PX 17.0

class Prndl : public BonusFFBApp
{
	Q_OBJECT;

public:
	QString getAppName();
	void initialize();
	void saveSettings(QSettings*);
	void loadSettings(QSettings*);
	void initializeJoystickMap();

	HRESULT startMode();
	void stopMode();
	void gameLoop();

	bool getShiftLockReleased();

public slots:
	void redrawJoystickMap();
	void changeSlotLabel(PrndlSlot slot);
	void updateJoystickCircle(int, int);


signals:
	void shiftLockStateChanged(bool);

private:
	QGraphicsScene* scene = nullptr;
	QGraphicsRectItem* centerSlotRect;
	QList<QGraphicsEllipseItem*> slotCircles;
	QGraphicsEllipseItem* joystickCircle;

	// Stateful FFB effect managers
	PrndlSlotGuard slotGuard;
	PrndlStateManager stateManager = PrndlStateManager();

	bool lastShiftLockReleased = false;
};

