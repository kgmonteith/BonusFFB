/*
Copyright (C) 2024-2026 Ken Monteith.

This file is part of Bonus FFB.

Bonus FFB is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or any later version.

Bonus FFB is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Bonus FFB. If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include "BonusFFBApp.h"
#include "HShifterStateManager.h"
#include "HShifterSlotGuard.h"
#include "HShifterSynchroGuard.h"
#include "PedalsManager.h"

class HShifter : public BonusFFBApp
{
	Q_OBJECT;

public:
	QString getAppName(bool = false);
	void initialize();
	void saveSettings(QSettings*);
	void loadSettings(QSettings*);
	void initializeJoystickMap();
	HRESULT startMode();
	void gameLoop();

public slots:

	void redrawJoystickMap();
	void updateJoystickCircle(int, int);
	void updateGearText(int);

signals:
	void gearValuesChanged(QPair<int, int>);
	void engineRPMChanged(float);
	void resetClutchAxes();

private:
	QGraphicsScene* scene = nullptr;
	QGraphicsRectItem* neutralChannelRect;
	QGraphicsRectItem* centerSlotRect;
	QGraphicsRectItem* rightSlotRect;
	QGraphicsRectItem* leftSlotRect;
	QGraphicsEllipseItem* joystickCircle;

	// Stateful FFB effect managers
	HShifterStateManager stateManager;
	HShifterSlotGuard slotGuard;
	HShifterSynchroGuard synchroGuard;
	PedalsManager pedalsManager;

	QPair<int, int> lastGearValues = { 0, 0 };
	float lastEngineRPM = 0.0;
};

