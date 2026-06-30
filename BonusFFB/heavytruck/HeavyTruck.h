/*
Copyright (C) 2024-2026 Ken Monteith.

This file is part of Bonus FFB.

Bonus FFB is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or any later version.

Bonus FFB is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Bonus FFB. If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once
#include <QMap>

#include "BonusFFBApp.h"
#include "HeavyTruckStateManager.h"
#include "HeavyTruckSlotGuard.h"
#include "HeavyTruckSynchroGuard.h"
#include "PedalsManager.h"
#include "SlotPattern.h"

class HeavyTruck : public BonusFFBApp
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
	void setSlotPattern(QString);
	void slotParameterChanged(int);

	void redrawJoystickMap();
	void updateJoystickCircle(int, int);
	void updateGearText(int);
	void updateRpmDeltaText(float);
	void updateRangeText(bool);
	void updateSplitterText(bool);

signals:
	void slotPositionsChanged(int slotDepth, int rightSlot, int centerSlot);
	void engineRPMChanged(float);

private:
	QGraphicsScene* scene = nullptr;
	QGraphicsEllipseItem* joystickCircle;
	QGraphicsRectItem* grindZoneRect;
	QGraphicsRectItem* buttonZoneRect;

	// Stateful FFB effect managers
	SlotParameters* slot = new SlotParameters();
	SlotPattern slotPattern;
	HeavyTruckStateManager stateManager;
	HeavyTruckSlotGuard slotGuard;
	HeavyTruckSynchroGuard synchroGuard;
	PedalsManager pedalsManager;

	QPair<int, int> lastPedalValues = { 0, 0 };
	float lastSpeed = 0.0;
	float lastEngineRPM = 0.0;
};

