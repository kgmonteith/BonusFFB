/*
Copyright (C) 2024-2025 Ken Monteith.

This file is part of Bonus FFB.

Bonus FFB is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or any later version.

Bonus FFB is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Bonus FFB. If not, see <https://www.gnu.org/licenses/>.
*/


#include "DeviceInfo.h"
#include <QDebug>
#include <QUuid>
#include <QTextStream>
#include <QThread>

// Set up the device for reading state.
HRESULT DeviceInfo::acquire(HWND* handle, bool exclusive) {
    HRESULT hr;

    if (FAILED(hr = this->diDevice->SetDataFormat(&c_dfDIJoystick2))) {
        qDebug() << "SetDataFormat failed, hr: " << Qt::hex << unsigned long(hr);
        return hr;
    }

    // Set the cooperative level to let DInput know how this device should
    // interact with the system and with other DInput applications.
    UINT32 exclusiveFlag = DISCL_NONEXCLUSIVE;
    if (exclusive) {
        exclusiveFlag = DISCL_EXCLUSIVE;
    }
    if (FAILED(hr = this->diDevice->SetCooperativeLevel(*handle,
        exclusiveFlag | DISCL_BACKGROUND))) {
        qDebug() << "SetCooperativeLevel failed, hr: " << unsigned long(hr);
        return hr;
    }

    if (FAILED(hr = this->diDevice->Acquire())) {
        qDebug() << "Acquire failed, hr: " << unsigned long(hr);
        return hr;
    }
    isAcquired = true;
    return hr;
}

// Safe teardown after preparing device
HRESULT DeviceInfo::release() {
    HRESULT hr = this->diDevice->Unacquire();
    return hr;
}

HRESULT DeviceInfo::updateState() {
    HRESULT hr = this->diDevice->GetDeviceState(sizeof(DIJOYSTATE2), &this->joyState);
    return hr;
}

QMap<QUuid, QString> DeviceInfo::getDeviceAxes() {
    QMap<QUuid, QString> axisMap;
    this->diDevice->EnumObjects(enumAxesCallback,
        (VOID*)&axisMap, DIDFT_AXIS);
    return axisMap;
}

long DeviceInfo::getAxisReading(QUuid axisGuid) {
    if (axisGuid == GUID_XAxis)
    {
        return this->joyState.lX;
    }
    else if (axisGuid == GUID_YAxis)
    {
        return this->joyState.lY;
    }
    else if (axisGuid == GUID_ZAxis)
    {
        return this->joyState.lZ;
    }
    else if (axisGuid == GUID_RxAxis) {
        return this->joyState.lRx;
    }
    else if (axisGuid == GUID_RyAxis)
    {
        return this->joyState.lRy;
    }
    else if (axisGuid == GUID_RzAxis)
    {
        return this->joyState.lRz;
    }
    else if (axisGuid == GUID_Slider)
    {
        return this->joyState.rglSlider[0];
    }
    return 0;
}

bool DeviceInfo::isButtonPressed(int buttonIndex) {
    if (this->joyState.rgbButtons[buttonIndex] & 0x80) {
        return true;
    }
    return false;
}


BOOL CALLBACK enumDevicesCallback(const DIDEVICEINSTANCE* pInst, VOID* pContext) noexcept
{
    QList<DeviceInfo>* diDevices = static_cast<QList<DeviceInfo>*>(pContext);

    IDirectInputDevice8* dev;
    g_pDI->CreateDevice(pInst->guidInstance, &dev, NULL);

    DIDEVCAPS capabilities;
    capabilities.dwSize = sizeof(DIDEVCAPS);
    dev->GetCapabilities(&capabilities);

    QUuid instanceGuid = pInst->guidInstance;
    QUuid productGuid = pInst->guidProduct;
    bool supportsFfb = (capabilities.dwFlags & DIDC_FORCEFEEDBACK);
    int buttonCount = capabilities.dwButtons;
    QString devName = QString::fromWCharArray(pInst->tszInstanceName);
    if (productGuid.data1 == VJOY_PRODUCT_GUID) {
        vjoy_device_count += 1;
        devName.append(QString(" %1").arg(vjoy_device_count));
    }
    DeviceInfo deviceInfo = { devName, instanceGuid, productGuid, supportsFfb, buttonCount, dev };
    diDevices->append(deviceInfo);

    return DIENUM_CONTINUE;
}


HRESULT initDirectInput(QList<DeviceInfo>* diDevices) noexcept {
    // Register with the DirectInput subsystem and get a pointer
    // to a IDirectInput interface we can use.
    HRESULT hr = DirectInput8Create(GetModuleHandle(nullptr), DIRECTINPUT_VERSION,
        IID_IDirectInput8, (VOID**)&g_pDI, nullptr);
    if (FAILED(hr))
    {
        return hr;
    }

    // Look for a force feedback device we can use
    hr = g_pDI->EnumDevices(DI8DEVCLASS_GAMECTRL,
        enumDevicesCallback, diDevices,
        DIEDFL_ATTACHEDONLY);
    return hr;
}


BOOL CALLBACK enumAxesCallback(const DIDEVICEOBJECTINSTANCE* pdidoi, VOID* pContext) noexcept
{
    assert(pContext != nullptr);
    auto axisMap = static_cast<QMap<QUuid, QString>*>(pContext);

    axisMap->insert(pdidoi->guidType, QString::fromStdWString(pdidoi->tszName));

    return DIENUM_CONTINUE;
}

DeviceInfo* getDeviceFromGuid(QList<DeviceInfo>* deviceList, QUuid guid) {
    for (QList<DeviceInfo>::iterator it = deviceList->begin(); it != deviceList->end(); it++)
    {
        if (it->instanceGuid == guid) {
            return &*it;
        }
    }
    qDebug() << "Device not found: " << guid;
    return nullptr;
}
