/*
Copyright (C) 2024-2025 Ken Monteith.

This file is part of Bonus FFB.

Bonus FFB is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or any later version.

Bonus FFB is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Bonus FFB. If not, see <https://www.gnu.org/licenses/>.
*/

#include "vJoyFeeder.h"
#include <QDebug>
#include <QTimer>

bool vJoyFeeder::isDriverEnabled() {
	return vJoyEnabled();
}

bool vJoyFeeder::checkVersionMatch() {
	unsigned short versionDLL, versionDriver;
	bool ret = DriverMatch(&versionDLL, &versionDriver);
	// Assume 2.1.8 is compatible with subsequent versions
	if (versionDriver >= 536)
		return true;
	return false;
}

int vJoyFeeder::deviceCount() {
	int ct;
	GetNumberExistingVJD(&ct);
	return ct;
}

int vJoyFeeder::getDeviceIndex() {
	return deviceNum - 1;
}

void vJoyFeeder::setDeviceIndex(unsigned int d) {
	bool acquired = is_acquired();
	if (acquired) {
		release();
	}
	deviceNum = d + 1;	// Device number is 1-indexed, sigh
	if (acquired) {
		acquire();
	}
}

bool vJoyFeeder::acquire() {
	VjdStat status = GetVJDStatus(deviceNum);
	if (status != VJD_STAT_FREE) {
		return false;
	}
	if (!AcquireVJD(deviceNum))
		return false;
	acquired = true;
	return true;
}

bool vJoyFeeder::is_acquired() {
	return acquired;
}

void vJoyFeeder::release() {
	if (acquired) {
		ResetButtons(deviceNum);
		RelinquishVJD(deviceNum);
	}
	acquired = false;
}

void vJoyFeeder::pressButton(int button) {
	bool ret = SetBtn(true, deviceNum, unsigned char(button));
}

void vJoyFeeder::releaseButton(int button) {
	SetBtn(false, deviceNum, unsigned char(button));
}

void vJoyFeeder::shortPressButton(int button) {
	pressButton(button);
	QTimer::singleShot(50, this, [this, button]() {
		releaseButton(button);
	});

}

void vJoyFeeder::updateButtons(int newState) {
	// Only called when state changes
	if (newState != 0) {
		pressButton(newState);
	}
	else {
		releaseButton(pressedButton);
	}
	pressedButton = newState;
}