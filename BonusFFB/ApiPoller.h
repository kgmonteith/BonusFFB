/*
Copyright(C) 2024 - 2026 Ken Monteith.

This file is part of Bonus FFB.

Bonus FFB is free software : you can redistribute it and /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or any later version.

Bonus FFB is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Bonus FFB.If not, see < https://www.gnu.org/licenses/>.
*/

// This file is courtesy of Claude
#pragma once

#include <QObject>
#include <QTimer>
#include <QUrl>
#include <QNetworkAccessManager>
#include <QRestAccessManager>
#include <QRestReply>
#include <QNetworkRequest>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDateTime>

class ApiPoller : public QObject
{
    Q_OBJECT

public:
    explicit ApiPoller(QUrl endpoint, int intervalMs = 5000, QObject* parent = nullptr)
        : QObject(parent)
        , m_endpoint(std::move(endpoint))
        , m_nam(new QNetworkAccessManager(this))
        , m_rest(new QRestAccessManager(m_nam, this))
    {
        m_timer.setInterval(intervalMs);
        connect(&m_timer, &QTimer::timeout, this, &ApiPoller::poll);
    }

    void start() { poll(); m_timer.start(); }
    void stop() { m_timer.stop(); }

    void setInterval(int ms) { m_timer.setInterval(ms); }
    void setEndpoint(QUrl url) { m_endpoint = std::move(url); }

    // --- New: access to the last successful payload ---

    // Returns the most recent successfully received JSON object.
    // Empty QJsonObject if nothing has succeeded yet.
    QJsonObject lastData() const { return m_lastData; }

    // True if at least one successful poll has completed.
    bool hasData() const { return m_hasData; }

    // Timestamp of the last successful poll, for staleness checks.
    QDateTime lastUpdated() const { return m_lastUpdated; }

signals:
    void dataReceived(const QJsonObject& data);
    void requestFailed(const QString& errorString);

private:
    void poll()
    {
        if (m_inFlight) return;
        m_inFlight = true;

        QNetworkRequest request(m_endpoint);
        request.setTransferTimeout(3000);
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

        m_rest->get(request, this, [this](QRestReply& reply) {
            m_inFlight = false;

            if (QNetworkReply* raw = reply.networkReply()) {
                raw->deleteLater();
            }

            if (!reply.isSuccess()) {
                emit requestFailed(reply.errorString());
                return;
            }

            const auto jsonDoc = reply.readJson();
            if (!jsonDoc || !jsonDoc->isObject()) {
                emit requestFailed(QStringLiteral("Response was not a JSON object"));
                return;
            }

            // Cache it before emitting, so anyone reacting to the signal
            // by calling lastData() gets the fresh value, not stale data.
            m_lastData = jsonDoc->object();
            m_hasData = true;
            m_lastUpdated = QDateTime::currentDateTime();

            emit dataReceived(m_lastData);
            });
    }

    QUrl m_endpoint;
    QNetworkAccessManager* m_nam;
    QRestAccessManager* m_rest;
    QTimer m_timer;
    bool m_inFlight = false;

    QJsonObject m_lastData;
    bool m_hasData = false;
    QDateTime m_lastUpdated;
};