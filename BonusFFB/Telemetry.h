/*
Copyright (C) 2024-2026 Ken Monteith.

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

#include "DeviceConfiguration.h"
#include "ApiPoller.h"

enum class TelemetrySource {
	NONE, SCS, SIMHUB
};

class Telemetry : public QObject
{
	Q_OBJECT;

public:
	Telemetry();
	void connectSCSTelemetry();
	void disconnectSCSTelemetry();
	void startConnectTimer();

	TelemetrySource isConnected();
	QPair<int, int> getGearState();
	float getSpeed();
	float getEngineRPM();
	bool getParkingBrakeState();
	float getTransmissionRPMForGear(int);
	int getActiveGear();
	int getMaxGear();
	int getGearForSlot(int, RangeSplitterValues*);
	float getThrottlePercent();
	//void logTelemetry();
	QString getActiveGame();

	TelemetrySource telemetrySource = TelemetrySource::NONE;

public slots:
	void simhubDataReceived(const QJsonObject&);
	void simhubFailed(const QString&);

signals:
	void telemetryChanged(TelemetrySource);
	void lastUpdate(QString);

private:
	HANDLE pHandle = nullptr;
	void* pBufferPtr = nullptr;

	scsTelemetryMap_s* scsTelem = nullptr;
	ApiPoller shPoller = ApiPoller(QUrl("http://localhost:8888/api/getGameData"), 1000);

	QTimer* checkTelemSourcesTimer;
	QTimer* gearLogTimer;
	QTimer* rpmLogTimer;
};
