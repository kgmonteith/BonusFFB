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


void Telemetry::startConnectTimer()
{
	timer->start();
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
	//qDebug() << "Shifter Type Value: " << (QString(pTelemMap->config_s.shifterType)); // values are "automatic", "hshifter", "manual" for sequential, "arcade" for simple automatic
	//qDebug() << "truck_b.shifterToggle[2]: " << pTelemMap->truck_b.shifterToggle[0] << pTelemMap->truck_b.shifterToggle[1];
	return QPair<int, int>(slottedGear, selectedGear);
}

float Telemetry::getSpeed() {
	if (telemetrySource == TelemetrySource::SCS) {
		return pTelemMap->truck_f.speed;
	}
	return 0;
}

float Telemetry::getEngineRPM() {
	if (telemetrySource == TelemetrySource::SCS) {
		return pTelemMap->truck_f.engineRpm;
	}
	return 0;
}

bool Telemetry::getParkingBrakeState() {
	if (telemetrySource == TelemetrySource::SCS) {
		return pTelemMap->truck_b.parkBrake;
	}
	return false;
}

int Telemetry::getActiveGear() {
	if (telemetrySource == TelemetrySource::SCS) {
		return pTelemMap->truck_i.gear;
	}
	return 0;
}

float Telemetry::getThrottlePercent() {
	if (telemetrySource == TelemetrySource::SCS) {
		qDebug() << "truck_f.gameThrottle:" << pTelemMap->truck_f.gameThrottle;
		return pTelemMap->truck_f.gameThrottle;
	}
	return 0;
}

int Telemetry::getGearForSlot(int slotNumber) {
	if (!slotNumber)
		return 0;
	if (telemetrySource == TelemetrySource::SCS) {
		int rangeAdder = 0;
		if (pTelemMap->truck_b.shifterToggle[0]) {
			rangeAdder = 1;
		}
		int splitterAdder = 0;
		if (pTelemMap->truck_b.shifterToggle[1]) {
			splitterAdder = 2;
		}
		int gearIndex = ((slotNumber + 1) * 4) + rangeAdder + splitterAdder;
		int gear = pTelemMap->truck_i.hshifterResulting[gearIndex];
		return gear;
	}
	return 0;
}

float Telemetry::getTransmissionRPMForGear(int gear) {
	float rpm = 0.0;
	if (telemetrySource != TelemetrySource::SCS) {
		return 0.0;
	}

	// Get average powered wheel rotations per second
	double wheelOmegaSum = 0.0;
	double wheelRadiusSum = 0.0;
	int poweredWheelCt = 0;
	for (unsigned int i = 0; i < pTelemMap->config_ui.truckWheelCount; i++) {
		if (!pTelemMap->config_b.truckWheelPowered[i]) {
			continue;
		}
		wheelOmegaSum += pTelemMap->truck_f.truck_wheelVelocity[i];
		wheelRadiusSum += pTelemMap->config_f.truckWheelRadius[i];
		poweredWheelCt++;
	}
	float averageDrivenWheelAngularVelocity = float(wheelOmegaSum / poweredWheelCt);
	float wheelRadius = float(wheelRadiusSum / poweredWheelCt);

	// Get gear ratio for input gear
	float gearRatio = 0.0;
	if (gear > 0) {
		gearRatio = pTelemMap->config_f.gearRatiosForward[gear-1];
	}
	else if (gear < 0) {
		gearRatio = pTelemMap->config_f.gearRatiosReverse[std::abs(gear-1)];
	}
	else
	{
		// In neutral, not relevant
		return 0.0;
	}

	// Unit definitions: https://kniffen.dev/TruckSim-Telemetry/documents/Units.html
	// Wheel velocity is Hz (rotations per second)
	// Engine RPM = Wheel RPM * transmission ratio (Hz) * 60 * differential ratio
	float transmission_rpm = gearRatio * averageDrivenWheelAngularVelocity * 60 * pTelemMap->config_f.gearDifferential;
	return transmission_rpm;
}