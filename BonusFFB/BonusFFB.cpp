#include "BonusFFB.h"

#include <QDebug>
#include <QUuid>

BOOL CALLBACK BonusFFB::enumDevicesCallback(const DIDEVICEINSTANCE* pInst, VOID* pContext) noexcept
{
    QList<DeviceInfo>* diDevices = static_cast<QList<DeviceInfo>*>(pContext);

    QString devName = QString::fromWCharArray(pInst->tszInstanceName);
    IDirectInputDevice8* dev;
    g_pDI->CreateDevice(pInst->guidInstance, &dev, NULL);

    DIDEVCAPS capabilities;
    capabilities.dwSize = sizeof(DIDEVCAPS);
    dev->GetCapabilities(&capabilities);

    qDebug() << "Device: " << devName << ", dwFlags: " << (capabilities.dwFlags & DIDC_FORCEFEEDBACK) << ", dwFFSamplePeriod: " << capabilities.dwAxes;

    unsigned long guidManufacturer = pInst->guidProduct.Data1;
    QUuid instanceGuid = pInst->guidInstance;
    QUuid productGuid = pInst->guidProduct;
    bool supportsFfb = (capabilities.dwFlags & DIDC_FORCEFEEDBACK);
    DeviceInfo deviceInfo = { devName, instanceGuid, productGuid, supportsFfb, dev};
    diDevices->append(deviceInfo);

    return DIENUM_CONTINUE;
}

/*
BOOL CALLBACK EnumObjectsCallback(const DIDEVICEOBJECTINSTANCE* pdidoi,
    VOID* pDevice) noexcept

    int nSliderCount = 0;  // Number of returned slider controls
    int nPOVCount = 0;     // Number of returned POV controls

    BFFBDIDevice* diDevice = static_cast<BFFBDIDevice*>(pContext);

    // For axes that are returned, set the DIPROP_RANGE property for the
    // enumerated axis in order to scale min/max values.
    if (pdidoi->dwType & DIDFT_AXIS)
    {
        DIPROPRANGE diprg;
        diprg.diph.dwSize = sizeof(DIPROPRANGE);
        diprg.diph.dwHeaderSize = sizeof(DIPROPHEADER);
        diprg.diph.dwHow = DIPH_BYID;
        diprg.diph.dwObj = pdidoi->dwType; // Specify the enumerated axis
        diprg.lMin = -1000;
        diprg.lMax = +1000;

        // Set the range for the axis
        if (FAILED(g_pJoystick->SetProperty(DIPROP_RANGE, &diprg.diph)))
            return DIENUM_STOP;
    }
    }
}*/

BonusFFB::DeviceInfo * BonusFFB::getDeviceFromGuid(QList<DeviceInfo> * deviceList, QUuid guid) {
    for (QList<DeviceInfo>::iterator it = deviceList->begin(); it != deviceList->end(); it++)
    {
        if (it->instanceGuid == guid) {
            return &*it;
        }
    }
}

// Set up the device for reading state. *Does not* configure the device for writing FFB events.
HRESULT BonusFFB::prepare(DeviceInfo* device, HWND* handle) {
    HRESULT hr;
    if (FAILED(hr = device->diDevice->SetDataFormat(&c_dfDIJoystick2))) {
        qDebug() << "SetDataFormat failed, hr: " << unsigned long(hr);
        return hr;
    }

    // Set the cooperative level to let DInput know how this device should
    // interact with the system and with other DInput applications.
    if (FAILED(hr = device->diDevice->SetCooperativeLevel(*handle,
        DISCL_EXCLUSIVE | DISCL_BACKGROUND))) {

        qDebug() << "SetCooperativeLevel failed, hr: " << unsigned long(hr);
        return hr;
    }

    if (FAILED(hr = device->diDevice->Acquire())) {
        qDebug() << "Acquire failed, hr: " << unsigned long(hr);
    }
    return hr;
}

// Safe teardown after preparing device
HRESULT BonusFFB::release(DeviceInfo* device) {
    HRESULT hr = device->diDevice->Unacquire();
    return hr;
}

HRESULT BonusFFB::updateState(DeviceInfo* device, DIJOYSTATE2* joystickState) {
    HRESULT hr = device->diDevice->GetDeviceState(sizeof(DIJOYSTATE2), joystickState);
    return hr;
}

HRESULT BonusFFB::initDirectInput(QList<DeviceInfo>* diDevices) noexcept {
    qDebug() << "InitDirectInput called";
    // Register with the DirectInput subsystem and get a pointer
    // to a IDirectInput interface we can use.
    HRESULT hr = DirectInput8Create(GetModuleHandle(nullptr), DIRECTINPUT_VERSION,
        IID_IDirectInput8, (VOID**)&g_pDI, nullptr);
    if (FAILED(hr))
    {
        return hr;
    }

    // Look for a force feedback device we can use
    if (FAILED(hr = g_pDI->EnumDevices(DI8DEVCLASS_GAMECTRL,
        enumDevicesCallback, diDevices,
        DIEDFL_ATTACHEDONLY)))
    {
        return hr;
    }
}

BOOL CALLBACK BonusFFB::EnumAxesCallback(const DIDEVICEOBJECTINSTANCE* pdidoi,
    VOID* pContext) noexcept
{
    assert(pContext != nullptr);
    auto axisMap = static_cast<QMap<QUuid, QString>*>(pContext);

    axisMap->insert(pdidoi->guidType, QString::fromStdWString(pdidoi->tszName));

    return DIENUM_CONTINUE;
}

QMap<QUuid, QString> BonusFFB::getDeviceAxes(DeviceInfo* pDevice) {
    QMap<QUuid, QString> axisMap;
    pDevice->diDevice->EnumObjects(EnumAxesCallback,
        (VOID*)&axisMap, DIDFT_AXIS);
    return axisMap;
}

long BonusFFB::getAxisReading(DIJOYSTATE2* joyState, QUuid axisGuid) {
    if (axisGuid == GUID_XAxis)
    {
        return joyState->lX;
    }
    else if (axisGuid == GUID_YAxis)
    {
        return joyState->lY;
    }
    else if (axisGuid == GUID_ZAxis)
    {
        return joyState->lZ;
    }
    else if (axisGuid == GUID_RxAxis) {
        return joyState->lRx;
    }
    else if (axisGuid == GUID_RyAxis)
    {
        return joyState->lRy;
    }
    else if (axisGuid == GUID_RzAxis)
    {
        return joyState->lRz;
    }
    else if (axisGuid == GUID_Slider)
    {
        return joyState->rglSlider[0];
    }
    return 0;
}