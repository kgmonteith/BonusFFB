/*
Copyright (C) 2024-2026 Ken Monteith.

This file is part of Bonus FFB.

Bonus FFB is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or any later version.

Bonus FFB is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Bonus FFB. If not, see <https://www.gnu.org/licenses/>.
*/

#include <QDebug>
#include "SlotPattern.h"

void SlotPattern::setName(QString t_name) {
	qDebug() << "Slot pattern name: " << t_name;
	name = t_name;
}

void SlotPattern::setPattern(const QList<int>& slot_buttons) {
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

void SlotPattern::setWidthScale(int t_scale) {
	// t_scale is 0 to 100, convert to 0.0 to 1.0
	width_scale = double(t_scale) / 100.0;
	renderScene();
}

void SlotPattern::setDepthScale(int t_scale) {
	// t_scale is 0 to 100, convert to 0.0 to 1.0
	depth_scale = double(t_scale) / 100.0;
	renderScene();
}

void SlotPattern::setAlignment(int new_alignment) {
	alignment = new_alignment;
	renderScene();
}

double SlotPattern::getSlotPositionAbsolute(Slot slot) {
	// Still provided as a value from 0 to 1, justified by alignment
	float offset = 0;	// Valid for LHD
	if (alignment == SLOT_ALIGNMENT_CENTERED) {
		offset = (1 - width_scale) * 0.5;
	}
	else if (alignment == SLOT_ALIGNMENT_RHD) {
		offset = 1 - width_scale;
	}
	return offset + (slot.position_pct_nominal * width_scale);
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