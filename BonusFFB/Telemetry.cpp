/*
This file is part of Bonus FFB.

Bonus FFB is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or any later version.

Bonus FFB is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Bonus FFB. If not, see <https://www.gnu.org/licenses/>.
*/

#include "Telemetry.h"

#include <chrono>
#include <QDebug>

using namespace std::chrono_literals;

Telemetry::Telemetry() {
	timer = new QChronoTimer(1s, this);
	connect(timer, &QChronoTimer::timeout, this, &Telemetry::connectTelemetry);
}

void Telemetry::connectTelemetry()
{
	if (pTelemMap != nullptr) {
		if (pTelemMap->scs_values.game == UnknownGame) {
			// Telemetry has stopped, disconnect the telemetry
			disconnectTelemetry();
			return;
		}
		// Telemetry is already running and good, return
		return;
	}

	pHandle = OpenFileMapping(FILE_MAP_READ, FALSE, SCS_PLUGIN_MMF_NAME);
	if (pHandle == nullptr) {
		return;
	}
	
	pBufferPtr = MapViewOfFile(pHandle, FILE_MAP_READ, 0, 0, SCS_PLUGIN_MMF_SIZE);
	pTelemMap = (scsTelemetryMap_s*)pBufferPtr;

	if (pTelemMap == nullptr) {
		qDebug() << "Failed to map shared memory";
		CloseHandle(pHandle);
		pHandle = nullptr;
	}

	if (pTelemMap->scs_values.game == ATS) {
		emit telemetryConnected("🟢 ATS telemetry connected");
	}
	else if (pTelemMap->scs_values.game == ETS2)
	{
		emit telemetryConnected("🟢 ETS2 telemetry connected");
	}
	else
	{
		// No idea what game this is, disconnect
		disconnectTelemetry();
	}
}

void Telemetry::disconnectTelemetry() {
	qDebug() << "Called disconnectTelemetry";
	UnmapViewOfFile(pBufferPtr);
	CloseHandle(pHandle);
	pHandle = nullptr;
	pBufferPtr = nullptr;
	pTelemMap = nullptr;
	emit telemetryDisconnected("❌ Telemetry disconnected");
}

void Telemetry::startConnectTimer()
{
	timer->start();
}