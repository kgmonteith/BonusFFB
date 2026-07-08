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

	gearLogTimer = new QTimer(this);
	gearLogTimer->setInterval(500);
	gearLogTimer->setSingleShot(true);

	rpmLogTimer = new QTimer(this);
	rpmLogTimer->setInterval(500);
	rpmLogTimer->setSingleShot(true);
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
		return pTelemMap->truck_f.gameThrottle;
	}
	return 0;
}

int Telemetry::getGearForSlot(int slotNumber, RangeSplitterValues* rangeSplitter) {
	if (!slotNumber)
		return 0;
	if (telemetrySource == TelemetrySource::SCS) {
		unsigned int rangeMask = 0;
		if (rangeSplitter->range) {
			rangeMask = 0b01;
		}
		int splitterMask = 0;
		if (pTelemMap->config_ui.selectorCount > 1 && rangeSplitter->splitter) {
			splitterMask = 0b10;
		}
		int gearIndex = 0;
		for (gearIndex = 0; gearIndex < 32; gearIndex++) {
			if (pTelemMap->truck_ui.hshifterPosition[gearIndex] != (slotNumber + 1)) // SCS seems to assume an 8-slot shifter, with the first two slots always unused. Might be wrong about that for custom transmissions.
				continue;
			if (pTelemMap->truck_ui.hshifterBitmask[gearIndex] == (rangeMask | splitterMask))
				break;
		}
		if (gearIndex > 31)
			gearIndex = 0; //
		int gear = pTelemMap->truck_i.hshifterResulting[gearIndex];
		
		/*
		if(!gearLogTimer->isActive()) 
		{
			qDebug() << "\tgetGearForSlot(" << slotNumber << "):";
			qDebug() << "\t\tgears: " << pTelemMap->config_ui.gears;
			qDebug() << "\t\tgears_reverse: " << pTelemMap->config_ui.gears_reverse;
			qDebug() << "\t\tselectorCount: " << pTelemMap->config_ui.selectorCount;
			qDebug() << "\t\ttelemetry range enabled: " << pTelemMap->truck_b.shifterToggle[0];
			qDebug() << "\t\thardware  range enabled: " << rangeSplitter->range;
			qDebug() << "\t\ttelemetry split enabled: " << pTelemMap->truck_b.shifterToggle[1];
			qDebug() << "\t\thardware  split enabled: " << rangeSplitter->splitter;
			qDebug() << "\t\tgearIndex: " << gearIndex;
			qDebug() << "\t\tresulting gear: " << pTelemMap->truck_i.hshifterResulting[gearIndex];
			QString t_str = "";
			QString r_str = "";
			QString s_str = "";
			for (int i = 0; i < std::size(pTelemMap->truck_i.hshifterResulting); i++) {
				t_str += QString::number(pTelemMap->truck_i.hshifterResulting[i]).rightJustified(2, ' ') + " ";
				r_str += QString::number(pTelemMap->truck_ui.hshifterPosition[i]).rightJustified(2, ' ') + " ";
				s_str += QString::number(pTelemMap->truck_ui.hshifterBitmask[i]).rightJustified(2, ' ') + " ";
			}
			qDebug() << "\t\tpTelemMap->truck_i.hshifterResulting[]: " << t_str;
			qDebug() << "\t\tpTelemMap->truck_i.hshifterPosition[]:  " << r_str;
			qDebug() << "\t\tpTelemMap->truck_i.hshifterBitmask[]:   " << s_str;
			gearLogTimer->start();
		}
		*/

		return gear;
	}
	return 0;
}

float Telemetry::getTransmissionRPMForGear(int gear) {
	float rpm = 0.0;
	if (telemetrySource != TelemetrySource::SCS || gear == 0) {
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
		// Drive gears
		gearRatio = pTelemMap->config_f.gearRatiosForward[gear-1];
	}
	else {
		// Reverse gears
		gearRatio = pTelemMap->config_f.gearRatiosReverse[std::abs(gear-1)];
	}

	// Unit definitions: https://kniffen.dev/TruckSim-Telemetry/documents/Units.html
	// Wheel velocity is Hz (rotations per second)
	// Engine RPM = Wheel RPM * transmission ratio (Hz) * 60 * differential ratio
	float transmission_rpm = gearRatio * averageDrivenWheelAngularVelocity * 60 * pTelemMap->config_f.gearDifferential;

	/*
	if (!rpmLogTimer->isActive()) {
		qDebug() << "\tgetTransmissionRPMForGear(" << gear << ")";
		qDebug() << "\t\twheelOmegaSum: " << wheelOmegaSum;
		qDebug() << "\t\twheelRadiusSum: " << wheelRadiusSum;
		qDebug() << "\t\tpoweredWheelCt: " << poweredWheelCt;
		qDebug() << "\t\taverageDrivenWheelAngularVelocity: " << averageDrivenWheelAngularVelocity;
		qDebug() << "\t\twheelRadius: " << wheelRadius;
		qDebug() << "\t\tgearRatio: " << gearRatio;
		QString t_str = "";
		for (int i = 0; i < std::size(pTelemMap->config_f.gearRatiosForward); i++) {
			t_str += QString::number(pTelemMap->config_f.gearRatiosForward[i]) + " ";
		}
		qDebug() << "\t\tpTelemMap->config_f.gearRatiosForward[]: " << t_str;
		qDebug() << "\t\tgearDifferential: " << pTelemMap->config_f.gearDifferential;
		qDebug() << "\t\tTransmission RPM: " << transmission_rpm;
		qDebug() << "\t\tEngine RPM: " << pTelemMap->truck_f.engineRpm;
		rpmLogTimer->start();
	}
	*/

	return transmission_rpm;
}