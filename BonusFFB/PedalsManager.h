/*
Copyright (C) 2024-2026
Ken Monteith.

This file is part of Bonus FFB.

Bonus FFB is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or any later version.

Bonus FFB is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Bonus FFB. If not, see <https://www.gnu.org/licenses/>.
*/
#include <QObject>
#include <QTimer>

#include "DeviceConfiguration.h"

#pragma once
class PedalsManager : public QObject
{
	Q_OBJECT;

public slots:
	void toggleVirtualPedals(bool);
	void updateVirtualPedals();
	void unblipThrottle();

private:
	bool enabled = false;

public:
	void start(DeviceConfiguration*);

	DeviceConfiguration* devices;
	QTimer unblipTimer;	// Timer for the actual throttle un-blip
	QTimer unblipIntervalTimer;	// Timer for the interval between un-blips
	QPair<int, int> lastPedalValues;
};