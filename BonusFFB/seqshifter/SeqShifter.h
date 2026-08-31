/*
Copyright (C) 2024-2026 Ken Monteith.

This file is part of Bonus FFB.

Bonus FFB is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or any later version.

Bonus FFB is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Bonus FFB. If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include "BonusFFBApp.h"
//#include "SeqShifterSlotGuard.h"
//#include "SeqShifterStateManager.h"
#include "SlotPattern.h"

#define SHIFTER_POSITION_MARKER_DIAMETER_PX 17.0

#define BUTTON_SHIFT_DOWN 15
#define BUTTON_SHIFT_UP 16

enum class SeqShifterState {
	UNKNOWN,
	NEUTRAL,
	SHIFT_DOWN,	// Shift down = stick forward
	SHIFT_UP,	// Shift up = stick back
	EXITING_SHIFT,
	SHIFT_DOWN_BLOCKED,
	SHIFT_UP_BLOCKED
};

class SeqShifter : public BonusFFBApp
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
	void updateState();
	void updateSlotSpring();
	void updateDetent();
	void updateGearShiftText(SeqShifterState);
	void updateButtons(SeqShifterState);

signals:
	void shiftStateChanged(SeqShifterState);

private:
	QGraphicsScene* scene = nullptr;
	QGraphicsRectItem* centerSlotRect;
	QGraphicsEllipseItem* joystickCircle;

	// Stateful FFB effect managers
	//SeqShifterSlotGuard slotGuard;
	//SeqShifterStateManager stateManager = SeqShifterStateManager();

	DIEFFECT slotSpringEff = {};
	DICONDITION noSpring = { 0, 0, 0 };
	DICONDITION keepLRCentered = { 0, DI_FFNOMINALMAX, DI_FFNOMINALMAX };
	DICONDITION keepFBCentered = { 0, DI_FFNOMINALMAX, DI_FFNOMINALMAX };
	DICONDITION slotSpringConditions[2] = { keepLRCentered, noSpring };

	DIEFFECT centeringSpringEff = {};
	DICONDITION centeringSpring = { 0, -5000, -5000 };

	long detent_spring_strength = 5000;
	long mechanical_resistance_strength = 3000;
	DIEFFECT detentSpringEff = {};
	DICONDITION detentSpringCondition = { 0, 0, 0 };

	JoystickValues joyValues;

	double depth_scale = 0.4;
	double detent_zone_scale = 0.2;
	double neutral_zone_scale = 0.04;

	int max_gear = 4;
	int min_gear = -1;
	int current_gear = 0;

	SlotPattern slotPattern;
	SeqShifterState state = SeqShifterState::NEUTRAL;
};

