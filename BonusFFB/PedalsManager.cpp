/*
Copyright (C) 2024-2026
Ken Monteith.

This file is part of Bonus FFB.

Bonus FFB is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or any later version.

Bonus FFB is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Bonus FFB. If not, see <https://www.gnu.org/licenses/>.
*/

#include "PedalsManager.h"

void PedalsManager::start(DeviceConfiguration* devPtr) {
	devices = devPtr;
	unblipTimer.setInterval(10);
	unblipTimer.setSingleShot(true);
}

void PedalsManager::toggleVirtualPedals(bool state) {
	enabled = state;
}

void PedalsManager::updateVirtualPedals() {
	PedalValues pedalValues = devices->getPedalValues();
	// Set throttle if not unblipping
	if (!unblipTimer.isActive()) {
		devices->vjoy.setAxisValue(VJOY_AXIS_MAX_VALUE * scaleRangeValue(pedalValues.throttle, 0, JOY_MAXPOINT), HID_USAGE_X);
	}
	// Set brake and clutch
	devices->vjoy.setAxisValue(VJOY_AXIS_MAX_VALUE * scaleRangeValue(pedalValues.brake, 0, JOY_MAXPOINT), HID_USAGE_Y);
	devices->vjoy.setAxisValue(VJOY_AXIS_MAX_VALUE* scaleRangeValue(pedalValues.clutch, 0, JOY_MAXPOINT), HID_USAGE_Z);
}

/// <summary>
/// Used by heavytruck to allow throttle-on shifting in ATS/ETS2
/// </summary>
void PedalsManager::unblipThrottle() {
	if (!enabled)
		return;
	if (!unblipTimer.isActive() && devices->getPedalValues().clutch == 0) {
		devices->vjoy.setAxisValue(0, HID_USAGE_X);
		unblipTimer.start();
	}
}
