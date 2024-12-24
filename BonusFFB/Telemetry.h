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
