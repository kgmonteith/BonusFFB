/*
Copyright (C) 2024-2026 Ken Monteith.

This file is part of Bonus FFB.

Bonus FFB is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or any later version.

Bonus FFB is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Bonus FFB. If not, see <https://www.gnu.org/licenses/>.
*/


#pragma once
#include <QObject>
#include <QList>
#include <QMap>
#include <QGraphicsScene>
#include <QGraphicsRectItem>

#include "DeviceConfiguration.h"

#define SLOT_ORIENTATION_FORWARD true
#define SLOT_ORIENTATION_BACK false

#define SLOT_WALL_LEFT	0b01
#define SLOT_WALL_RIGHT 0b10

#define SLOT_NONE nullptr

#define R 0
#define X -1

enum class SlotState {
	UNKNOWN,
	NEUTRAL,
	NEUTRAL_UNDER_SLOT,
	SLOTTED
};

struct PatternDef {
	QString name;
	QList<int> slot_numbers;
	int wall_flags = 0;
	double neutral_spring_pos = 0.5;
};


static const QList<PatternDef> TruckPatterns = {
	{ "Eaton-Fuller 18/13", {1, 2, 3, 4, 5, 6}, SLOT_WALL_LEFT},
	{ "Eaton-Fuller 10", {1, 2, 3, 4, 5, 6} },
	{ "Scania 12", { 1, X, X, 4, 5, 6 }, SLOT_WALL_LEFT},
	{ "Scania 12+2", {1, 2, X, 4, 5, 6 }, SLOT_WALL_LEFT},
	{ "Volvo 12", {X, 2, 3, 4, 5, X}, SLOT_WALL_LEFT},
	{ "Volvo 12+2", {1, 2, 3, 4, 5, X}, SLOT_WALL_LEFT},
	{ "ZF 12", { X, 2, 3, X, 5, 6}, SLOT_WALL_LEFT},
	{ "ZF 16 (Modern)", { X, 2, 3, 4, 5, 6}, SLOT_WALL_LEFT},
	{ "ZF 16 (Double-H)", { X, 2, 3, 4, 5, 6, 3, 4, 5, 6}, SLOT_WALL_LEFT},
	{ "Generic R+6", { R, 1, 2, 3, 4, 5, 6, X }},
	{ "Generic dogleg R+6", { R, X, 1, 2, 3, 4, 5, 6 }, SLOT_WALL_LEFT}
};

static const QList<PatternDef> PresetPatterns = {
	{"R+4", { R, X, 1, 2, 3, 4}, SLOT_WALL_LEFT, 1},
	{"R+5", { R, X, 1, 2, 3, 4, 5, X}, SLOT_WALL_LEFT},
	{"R+6", { R, X, 1, 2, 3, 4, 5, 6}, SLOT_WALL_LEFT},
	{"R+7", { R, X, 1, 2, 3, 4, 5, 6, 7, X}, SLOT_WALL_LEFT},
	{"R+8", { R, X, 1, 2, 3, 4, 5, 6, 7, 8}, SLOT_WALL_LEFT},
	{"R over 1+4", { R, 1, 2, 3, 4, X} },
	{"R over 1+5", { R, 1, 2, 3, 4, 5 } },
	{"R over 1+6", { R, 1, 2, 3, 4, 5, 6, X } },
	{"R over 1+7", { R, 1, 2, 3, 4, 5, 6, 7 } },
	{"R over 1+8", { R, 1, 2, 3, 4, 5, 6 , 7, 8, X } },
	{"4+R", { 1, 2, 3, 4, X, R }, SLOT_WALL_RIGHT},
	{"5+R", { 1, 2, 3, 4, 5, R }},
	{"6+R", { 1, 2, 3, 4, 5, 6, X, R }, SLOT_WALL_RIGHT},
	{"7+R", { 1, 2, 3, 4, 5, 6, 7, R }},
	{"8+R", { 1, 2, 3, 4, 5, 6, 7, 8, X, R }, SLOT_WALL_RIGHT}
};

static const QList<PatternDef> AllPatterns = TruckPatterns + PresetPatterns;

class Slot {
public:
	bool isEnabled() const {
		if (number != X)
			return true;
		return false;
	}
	bool isOrientationFwd() const {
		return orientation == SLOT_ORIENTATION_FORWARD;
	}
	bool isOrientationBack() const {
		return orientation == SLOT_ORIENTATION_BACK;
	}
	int vJoyButton() const {
		return number + 1;
	}
	QString asText() const {
		if (number == R)
			return "R";
		else
			return QString::number(number);
	}

	int number = X;
	double position_pct_nominal = 0;
	bool orientation = SLOT_ORIENTATION_FORWARD;

	bool operator==(const Slot& other) const {
		return (this->position_pct_nominal == other.position_pct_nominal) && (this->orientation == other.orientation);
	}
};

class SlotPattern : public QObject {
	Q_OBJECT

public slots:
	void setWidthScale(int);
	void setDepthScale(int);
	void setLeftOffset(int);
	void setRoundingFactor(int value) {
		rounding_factor = value * 0.01;
	}
	//void setName(QString);
	void setPattern(QString);
	void setPatternFromText(QString);

	void setButtonZoneScale(int t) {
		button_zone_scale = double(t) * 0.01;
	}
	void setGrindZoneScale(int t) {
		grind_zone_scale = double(t) * 0.01;
	}

signals:
	void setRangeOverride(bool);
	void slotWallsChanged(int);

public:
	void setSlotWalls(int wall_flags);
	bool hasSlotWall(int wall_flag);
	const Slot* getWallSlot(int wall_flag);
	double getSlotPositionAbsolute(Slot);
	double slotPositionAsJoystick(Slot);
	double slotPositionAsFFBOffset(Slot);
	double slotDepthAsJoystick(bool);
	double slotDepthAsFFBOffset(bool);
	bool isInNeutral(JoystickValues);
	bool isInCorner(Slot, JoystickValues);
	bool isInButtonZone(Slot, JoystickValues);
	bool isInGrindZone(JoystickValues);
	bool isInDetentZone(JoystickValues);
	const Slot* isUnderSlot(JoystickValues);
	const Slot* getNearestSlot(JoystickValues);
	Slot getLeftmostSlot(bool);
	Slot getRightmostSlot(bool);
	double getPatternLeftMinimumAsJoystick();
	double getPatternRightMaximumAsJoystick();
	double getSlotSpacingAsJoystick();
	double getPositionPercentAsFFBOffset(double pos_pct);

	void setScene(QGraphicsScene*);
	void renderScene();

	QString name;
	QList<Slot> slot_list;
	int slot_wall_flags = 0;
	// The true default values for these vars are set by loadSettings
	double width_scale = 0.66;
	double depth_scale = 0.75;
	double left_offset = 0.0;
	//double top_offset = 0;
	double button_zone_scale = 0.35;
	double grind_zone_scale = 0.15;
	double detent_zone_scale = 0.20;

	double roundingFactorAsJoystick();
	double rounding_factor = 0.1;

	QGraphicsScene* scene = nullptr;
	QGraphicsRectItem neutralChannelRect = QGraphicsRectItem();
	QGraphicsRectItem slotRects[12];
};