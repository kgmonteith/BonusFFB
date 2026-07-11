/*
Copyright (C) 2024-2026 Ken Monteith.

This file is part of Bonus FFB.

Bonus FFB is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or any later version.

Bonus FFB is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Bonus FFB. If not, see <https://www.gnu.org/licenses/>.
*/

#include <QDebug>
#include "SlotPattern.h"

/*
void SlotPattern::setName(QString t_name) {
	qDebug() << "Slot pattern name: " << t_name;
	name = t_name;
}
*/

void SlotPattern::setTruckPattern(int index) {
	TruckPattern newPattern = static_cast<TruckPattern>(index);
	setSlotWalls(SLOT_WALL_LEFT);
	QList<int> slot_buttons;
	if (newPattern == TruckPattern::EATON_18) {
		slot_buttons = {1, 2, 3, 4, 5, 6};
	}
	else if (newPattern == TruckPattern::EATON_10) {
		slot_buttons = {1, 2, 3, 4, 5, 6};
		setSlotWalls(0);
	}
	else if (newPattern == TruckPattern::SCANIA_12) {
		slot_buttons = { 1, 0, 0, 4, 5, 6 };
	}
	else if (newPattern == TruckPattern::SCANIA_12_2) {
		slot_buttons = {1, 2, 0, 4, 5, 6 };
	}
	else if (newPattern == TruckPattern::VOLVO_12) {
		slot_buttons = {0, 2, 3, 4, 5, 0};
	}
	else if (newPattern == TruckPattern::VOLVO_12_2) {
		slot_buttons = {1, 2, 3, 4, 5, 0};
	}
	else if (newPattern == TruckPattern::ZF_12) {
		slot_buttons = { 0, 2, 3, 0, 5, 6};
	}
	else if (newPattern == TruckPattern::ZF_16) {
		slot_buttons = { 0, 2, 3, 4, 5, 6};
	}
	else if (newPattern == TruckPattern::ZF_16_DOUBLEH) {
		slot_buttons = { 0, 2, 3, 4, 5, 6, 3, 4, 5, 6};
	}

	slot_list.clear();
	int full_slot_ct = slot_buttons.length() / 2;
	bool orientation = SLOT_ORIENTATION_FORWARD;
	for (int i = 0; i < slot_buttons.length(); i++) {
		double position_pct = double(i / 2) / (full_slot_ct - 1);
		slot_list.append({slot_buttons[i], position_pct, orientation});
		orientation = !orientation;
	}
	renderScene();
}

void SlotPattern::setSlotWalls(int t_flags) {
	slot_wall_flags = t_flags;
}

bool SlotPattern::hasSlotWall(int flag) {
	return slot_wall_flags & flag;
}

const Slot* SlotPattern::getWallSlot(int flag) {
	// Returns the slot where a wall effect applies. Only used for positional computation, so safe to return a disabled slot.
	if (slot_wall_flags & SLOT_WALL_LEFT && flag == SLOT_WALL_LEFT)
		return &slot_list.at(2);
	else if (slot_wall_flags & SLOT_WALL_RIGHT && flag == SLOT_WALL_RIGHT)
		return &slot_list.at(slot_list.size() - 2);
	return nullptr;
}

void SlotPattern::setWidthScale(int t_scale) {
	// t_scale is 0 to 100, convert to 0.0 to 1.0
	width_scale = double(t_scale) * 0.01;
	renderScene();
}

void SlotPattern::setDepthScale(int t_scale) {
	// t_scale is 0 to 100, convert to 0.0 to 1.0
	depth_scale = double(t_scale) * 0.01;
	renderScene();
}

void SlotPattern::setLeftOffset(int t_offset) {
	left_offset = t_offset * 0.01;
	renderScene();
}

double SlotPattern::getSlotPositionAbsolute(Slot slot) {
	// Still provided as a value from 0 to 1, justified by alignment
	float offset = (1 - width_scale) * left_offset;
	return offset + (slot.position_pct_nominal * width_scale);
}

double SlotPattern::slotPositionAsJoystick(Slot slot) {
	return getSlotPositionAbsolute(slot) * JOY_MAXPOINT;
}

double SlotPattern::slotPositionAsFFBOffset(Slot slot) {
	return (2 * FFB_MAX * getSlotPositionAbsolute(slot)) - FFB_MAX;
}

double SlotPattern::slotDepthAsJoystick(bool orientation) {
	if (orientation == SLOT_ORIENTATION_FORWARD)
		return JOY_MIDPOINT - (depth_scale * JOY_MIDPOINT);
	else
		return JOY_MIDPOINT + (depth_scale * JOY_MIDPOINT);
}

double SlotPattern::slotDepthAsFFBOffset(bool orientation) {
	if (orientation == SLOT_ORIENTATION_FORWARD)
		return depth_scale * FFB_MINPOINT;
	else
		return depth_scale * FFB_MAXPOINT;
}

double SlotPattern::roundingFactorAsJoystick() {
	return rounding_factor * JOY_MAXPOINT;
}

bool SlotPattern::isInNeutral(JoystickValues joyValues) {
	// Returns true if the stick is near the neutral channel, plus or minus the rounding factor, regardless of whether the stick is under a slot.
	double neutral_depth_joystick = neutral_depth_scale * JOY_MAXPOINT * depth_scale;
	//if (joyValues.fb >= JOY_MIDPOINT - roundingFactorAsJoystick() && joyValues.fb <= JOY_MIDPOINT + roundingFactorAsJoystick())
	if (joyValues.fb >= JOY_MIDPOINT - neutral_depth_joystick && joyValues.fb <= JOY_MIDPOINT + neutral_depth_joystick)
		return true;
	return false;
}

bool SlotPattern::isInCorner(Slot slot, JoystickValues joyValues) {
	// If the stick is within the depth of the rounding factor, it's in a corner and should be rounded
	if (slot.isOrientationFwd() && joyValues.fb >= JOY_MIDPOINT - roundingFactorAsJoystick() && joyValues.fb <= JOY_MIDPOINT)
		return true;
	else if (slot.isOrientationBack() && joyValues.fb >= JOY_MIDPOINT && joyValues.fb <= JOY_MIDPOINT + roundingFactorAsJoystick())
		return true;
	return false;
}

bool SlotPattern::isInButtonZone(Slot slot, JoystickValues joyValues) {
	double button_zone = JOY_MIDPOINT * button_zone_scale;
	if (slot.isOrientationFwd() && joyValues.fb <= JOY_MIDPOINT - button_zone) {
		return true;
	}
	else if (slot.isOrientationBack() && joyValues.fb >= JOY_MIDPOINT + button_zone) {
		return true;
	}
	return false;
}

bool SlotPattern::isInGrindZone(JoystickValues joyValues) {
	double grind_zone = JOY_MIDPOINT * grind_zone_scale;
	if (joyValues.fb < JOY_MIDPOINT && joyValues.fb <= JOY_MIDPOINT - grind_zone) {
		return true;
	}
	else if (joyValues.fb > JOY_MIDPOINT && joyValues.fb >= JOY_MIDPOINT + grind_zone) {
		return true;
	}
	return false;
}

const Slot* SlotPattern::getNearestSlot(JoystickValues joyValues, bool narrow_tolerance) {
	double tolerance = roundingFactorAsJoystick();
	if (narrow_tolerance)
		tolerance = slot_tolerance_narrow_scale * JOY_MAXPOINT;
	// Returns a pointer to the slot if the stick is in or under a slot, within a tolerance of +/- the slot rounding factor
	for (const auto& slot : slot_list) {
		if (!slot.isEnabled())	// Don't return disabled slots
			continue;
		double slot_position = slotPositionAsJoystick(slot);
		if (joyValues.lr >= slot_position - tolerance && joyValues.lr <= slot_position + tolerance) {
			if ((joyValues.fb <= JOY_MIDPOINT && slot.isOrientationFwd()) || (joyValues.fb >= JOY_MIDPOINT && slot.isOrientationBack())) {
				return &slot;
			}
		}

	}
	return SLOT_NONE;
}

Slot SlotPattern::getLeftmostSlot() {
	// Returns the first enabled slot
	if (slot_list.first().isEnabled())
		return slot_list.first();
	return slot_list.at(1);
}

Slot SlotPattern::getRightmostSlot() {
	// Returns the last enabled slot
	if (slot_list.last().isEnabled())
		return slot_list.last();
	return slot_list.at(slot_list.length() - 2);
}

void SlotPattern::setScene(QGraphicsScene* s) {
	scene = s;

	neutralChannelRect.setBrush(QBrush(Qt::black));
	neutralChannelRect.setPen(Qt::NoPen);
	scene->addItem(&neutralChannelRect);

	for (auto& slotRect : slotRects) {
		slotRect.setBrush(QBrush(Qt::black));
		slotRect.setPen(Qt::NoPen);
		slotRect.setVisible(false);
		scene->addItem(&slotRect);
	}
}

void SlotPattern::renderScene() {
	if (scene == nullptr || slot_list.isEmpty())
		return;

	neutralChannelRect.setRect(0, 0, width_scale * scene->width(), 5.0);
	neutralChannelRect.setPos(getSlotPositionAbsolute(slot_list.first()) * scene->width(), scene->height() / 2 - 2.5);

	for (int i = 0; i < std::size(slotRects); i++) {
		auto& slotRect = slotRects[i];
		if (i < slot_list.length()) {
			// Display this slot
			auto slot = slot_list.at(i);

			if (slot.isEnabled()) 
			{
				slotRect.setRect(0, 0, 5.0, (scene->height() / 2) * depth_scale);
				if (slot.orientation == SLOT_ORIENTATION_FORWARD) {
					// Draw from top of scene
					slotRect.setPos((scene->width() - 5) * getSlotPositionAbsolute(slot), (scene->height() / 2) * (1.0 - depth_scale));
				}
				else {
					// Draw from the middle of scene
					slotRect.setPos((scene->width() - 5) * getSlotPositionAbsolute(slot), scene->height() / 2);
				}
				slotRect.setVisible(true);
			}
			else {
				slotRect.setVisible(false);
			}
		}
		else {
			// Hide this slot
			slotRect.setVisible(false);
		}
	}
}