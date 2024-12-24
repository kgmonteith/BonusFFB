#pragma once
#define SAFE_RELEASE(p) { if(p) { (p)->Release(); (p)=nullptr; } }
#define DIRECTINPUT_VERSION 0x0800

#include "bonusffb_global.h"
#include <dinput.h>
#include <string>
#include <QMap>
#include <QList>

inline bool operator< (const GUID& firstGUID, const GUID& secondGUID) {
    return (memcmp(&firstGUID, &secondGUID, sizeof(GUID)) < 0 ? true : false);
}


struct BFFBDIDevice
{
    QString instanceName;
    QString guidString;
    const DIDEVICEINSTANCE* device;
};

//class BONUSFFB_EXPORT BonusFFB
class BonusFFB
{
public:
    BonusFFB();
    HRESULT initDirectInput() noexcept;

    static QList<BFFBDIDevice> diDevices; // All directInput devices

private:
    LPDIRECTINPUT8 pDI = nullptr;
    LPDIRECTINPUTDEVICE8 pFlightbase = nullptr;
    LPDIRECTINPUTEFFECT pEffect = nullptr;

    //static QMap<std::wstring, DIDEVICEINSTANCE> diDevices; // All directInput devices
    static BOOL CALLBACK enumDevicesCallback(const DIDEVICEINSTANCE* pInst, VOID* pContext) noexcept;
};
