#include "BonusFFB.h"

#include <QDebug>

BonusFFB::BonusFFB()
{
}

BOOL CALLBACK BonusFFB::enumDevicesCallback(const DIDEVICEINSTANCE* pInst, VOID* pContext) noexcept
{   
    //qDebug() << "InstanceName:%s" << QString::fromWCharArray(pInst->tszInstanceName);

    OLECHAR* guidString;
    StringFromCLSID(pInst->guidInstance, &guidString);
    //qDebug() << QString::fromWCharArray(guidString);
    BFFBDIDevice bffd = { QString::fromWCharArray(pInst->tszInstanceName), QString::fromWCharArray(guidString), pInst };
    diDevices.append(bffd);
    //diDevices[std::wstring(guidString)] = *pInst;
    // use guidString...

    // ensure memory is freed
    ::CoTaskMemFree(guidString);

    //diDevices[pInst->guidInstance] = *pInst;
    /*
    auto di = (IDirectInput8*)pContext;
    IDirectInputDevice8* dev;
    di->CreateDevice(pInst->guidInstance, &dev, NULL);

    DIPROPGUIDANDPATH iap = {};
    iap.diph.dwSize = sizeof(DIPROPGUIDANDPATH);
    iap.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    iap.diph.dwHow = DIPH_DEVICE;
    if (SUCCEEDED(dev->GetProperty(DIPROP_GUIDANDPATH, &iap.diph)))
    {
        qDebug(" Path:%s\n", iap.wszPath);
    }

    dev->Release();
    return DIENUM_CONTINUE;
    */

    //
    return DIENUM_CONTINUE;
}

HRESULT BonusFFB::initDirectInput() noexcept {
    qDebug() << "InitDirectInput called";
    // Register with the DirectInput subsystem and get a pointer
    // to a IDirectInput interface we can use.
    HRESULT hr = DirectInput8Create(GetModuleHandle(nullptr), DIRECTINPUT_VERSION,
        IID_IDirectInput8, (VOID**)&pDI, nullptr);
    if (FAILED(hr))
    {
        return hr;
    }

    // Look for a force feedback device we can use
    if (FAILED(hr = pDI->EnumDevices(DI8DEVCLASS_GAMECTRL,
        enumDevicesCallback, nullptr,
        DIEDFL_ATTACHEDONLY)))
    {
        return hr;
    }
}

//QMap<std::wstring, DIDEVICEINSTANCE> BonusFFB::diDevices; 
QList<BFFBDIDevice> BonusFFB::diDevices;