/*
This file is part of Bonus FFB.

Bonus FFB is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or any later version.

Bonus FFB is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Bonus FFB. If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once
#define SAFE_RELEASE(p) { if(p) { (p)->Release(); (p)=nullptr; } }
#define DIRECTINPUT_VERSION 0x0800

#include "bonusffb_global.h"
#include <dinput.h>
#include <QObject>
#include <QList>
#include <QUuid>

#define VJOY_PRODUCT_GUID 0xBEAD1234

namespace BonusFFB {
    struct DeviceInfo
    {
        QString name;
        QUuid instanceGuid;
        QUuid productGuid;
        bool supportsFfb;
        LPDIRECTINPUTDEVICE8 diDevice;
    };

    static LPDIRECTINPUT8 g_pDI;
    static int vjoy_device_count = 0;

    HRESULT initDirectInput(QList<DeviceInfo>*) noexcept;
    static BOOL CALLBACK enumDevicesCallback(const DIDEVICEINSTANCE*, VOID*) noexcept;
    DeviceInfo * getDeviceFromGuid(QList<DeviceInfo> *, QUuid);
    HRESULT prepare(DeviceInfo*, HWND*);
    HRESULT release(DeviceInfo*);
    HRESULT updateState(DeviceInfo*, DIJOYSTATE2*);

    BOOL CALLBACK EnumAxesCallback(const DIDEVICEOBJECTINSTANCE*, VOID*) noexcept;
    QMap<QUuid, QString> getDeviceAxes(DeviceInfo*);
    long getAxisReading(DIJOYSTATE2*, QUuid);
};
