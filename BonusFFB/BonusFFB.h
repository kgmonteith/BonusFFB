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
