/*
Copyright (C) 2024-2026 Ken Monteith.

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

TelemetrySource Telemetry::isConnected() {
	return telemetrySource;
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
		return;
	}

	if (pTelemMap->scs_values.game == ATS or pTelemMap->scs_values.game == ETS2) {
		telemetrySource = TelemetrySource::SCS;
		emit telemetryChanged(TelemetrySource::SCS);
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
	telemetrySource = TelemetrySource::NONE;
	emit telemetryChanged(telemetrySource);
}

QPair<int, int> Telemetry::getGearState() {
	int slottedGear = 0;	// Slotted gear is set even when the gear is not correctly engaged
	int selectedGear = 0;	// Selected gear is ONLY set when the gear is correctly engaged
	if (telemetrySource == TelemetrySource::SCS) {
		slottedGear = pTelemMap->truck_ui.shifterSlot;
		selectedGear = pTelemMap->truck_i.gear;
	}
	return QPair<int, int>(slottedGear, selectedGear);
}

float Telemetry::getEngineRPM() {
	float rpm = 0;
	if (telemetrySource == TelemetrySource::SCS) {
		rpm = pTelemMap->truck_f.engineRpm;
	}
	return rpm;
}

bool Telemetry::getParkingBrakeState() {
	return pTelemMap->truck_b.parkBrake;
}

void Telemetry::startConnectTimer()
{
	timer->start();
}