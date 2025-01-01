/*
This file is part of Bonus FFB.

Bonus FFB is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or any later version.

Bonus FFB is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Bonus FFB. If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include <QObject>
#include <QChronoTimer>

#include <wtypes.h>
#include "scs-telemetry-common.hpp"

enum class TelemetrySource {
	none, ats, ets2
};

class Telemetry : public QObject
{
	Q_OBJECT;

public:
	Telemetry();
	void connectTelemetry();
	void disconnectTelemetry();
	void startConnectTimer();

signals:
	void telemetryConnected(QString);
	void telemetryDisconnected(QString);
	void lastUpdate(QString);

private:
	HANDLE pHandle = nullptr;
	void* pBufferPtr = nullptr;
	scsTelemetryMap_s* pTelemMap = nullptr;

	QChronoTimer* timer;
};
