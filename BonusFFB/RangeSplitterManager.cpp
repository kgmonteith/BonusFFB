/*
Copyright (C) 2024-2026
Ken Monteith.

This file is part of Bonus FFB.

Bonus FFB is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or any later version.

Bonus FFB is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Bonus FFB. If not, see <https://www.gnu.org/licenses/>.
*/

#include "RangeSplitterManager.h"

void RangeSplitterManager::start(DeviceConfiguration* devPtr) {
	devices = devPtr;
}

void RangeSplitterManager::updateVirtualRangeSplitter() {
	RangeSplitterValues newValues = devices->getRangeSplitterValues();
	// Set range and splitter vJoy buttons
	if (newValues.range != lastValues.range) {
		if (newValues.range == true)
			devices->vjoy.pressButton(RANGE_BUTTON);
		else
			devices->vjoy.releaseButton(RANGE_BUTTON);
	}
	if (newValues.splitter != lastValues.splitter) {
		if (newValues.splitter == true)
			devices->vjoy.pressButton(SPLITTER_BUTTON);
		else
			devices->vjoy.releaseButton(SPLITTER_BUTTON);
	}
	lastValues = newValues;
}
