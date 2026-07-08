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

#define SLOT_ORIENTATION_FORWARD true
#define SLOT_ORIENTATION_BACK false

#define SLOT_WALL_LEFT	0b01
#define SLOT_WALL_RIGHT 0b10

enum class TruckPattern {
	EATON_18,
	EATON_10,
	SCANIA_12,
	SCANIA_12_2,
	VOLVO_12,
	VOLVO_12_2,
	ZF_12,
	ZF_16,
	ZF_16_DOUBLEH
};


class Slot {
public:
	bool isEnabled() {
		if (button != 0)
			return true;
		return false;
	}

	int button = 0;
	double position_pct_nominal = 0;
	bool orientation = true;
};

class SlotPattern : public QObject {
	Q_OBJECT

public slots:
	void setWidthScale(int);
	void setDepthScale(int);
	void setLeftOffset(int);
	//void setName(QString);
	void setTruckPattern(int index);

public:
	SlotPattern() {};
	void setSlotWalls(int wall_flags);
	double getSlotPositionAbsolute(Slot);

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

	QGraphicsScene* scene = nullptr;
	QGraphicsRectItem neutralChannelRect = QGraphicsRectItem();
	QGraphicsRectItem slotRects[12];
};