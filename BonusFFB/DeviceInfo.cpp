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

void DeviceInfo::addEffect(QString effName, FFBEffect eff) {
    effects[effName] = eff;
}

HRESULT DeviceInfo::startEffects() {
    HRESULT hr = DI_OK;
    for (auto i = effects.cbegin(), end = effects.cend(); i != end; ++i) {
        //eff = i.value();
        LPDIRECTINPUTEFFECT* non_const_ldpieff = const_cast<LPDIRECTINPUTEFFECT*>(&i.value().ldpieff);
        if (i.value().ldpieff == nullptr) {
            qDebug() << "Creating effect " << i.key();
            hr = diDevice->CreateEffect(i.value().guid, i.value().eff, non_const_ldpieff, nullptr);
            if (FAILED(hr))
            {
                qDebug() << "Failed to create effect " << i.key();
                return hr;
            }
        }
        hr = i.value().ldpieff->Start(INFINITE, 0);
        if (FAILED(hr))
        {
            qDebug() << "Failed to start effect " << i.key();
            return hr;
        }
    }
    return hr;
}

HRESULT DeviceInfo::updateEffect(QString effName) {
    FFBEffect eff = effects[effName];
    HRESULT hr = DI_OK;
    if(eff.ldpieff != nullptr) 
    {
        hr = eff.ldpieff->SetParameters(eff.eff, eff.flags);
        if (hr != DI_OK) {
            qDebug() << "SetParameters failed, reacquiring joystick";
            // Update failed, reacquire device, reload effects, and try again
            reacquire();
            startEffects();
            hr = eff.ldpieff->SetParameters(eff.eff, eff.flags);
        }
    }
    return hr;
}

void DeviceInfo::clearEffects() {
    qDebug() << "Clearing all effects";
    effects.clear();
}

// Set up the device for reading state.
HRESULT DeviceInfo::acquire(HWND* handle, bool exclusive) {
    HRESULT hr;

    if (FAILED(hr = diDevice->SetDataFormat(&c_dfDIJoystick2))) {
        qDebug() << "SetDataFormat failed, hr: " << Qt::hex << unsigned long(hr);
        return hr;
    }

    // Set the cooperative level to let DInput know how this device should
    // interact with the system and with other DInput applications.
    UINT32 exclusiveFlag = DISCL_NONEXCLUSIVE;
    if (exclusive) {
        exclusiveFlag = DISCL_EXCLUSIVE;
    }
    if (FAILED(hr = diDevice->SetCooperativeLevel(*handle,
        exclusiveFlag | DISCL_BACKGROUND))) {
        qDebug() << "SetCooperativeLevel failed, hr: " << unsigned long(hr);
        return hr;
    }

    if (FAILED(hr = diDevice->Acquire())) {
        qDebug() << "Acquire failed, hr: " << unsigned long(hr);
        return hr;
    }
    isAcquired = true;
    return hr;
}

// Safe teardown after preparing device
HRESULT DeviceInfo::release() {
    clearEffects();
    HRESULT hr = diDevice->Unacquire();
    return hr;
}

HRESULT DeviceInfo::reacquire() {
    qDebug() << "Reacquiring joystick...";
    HRESULT hr;
    if (FAILED(hr = diDevice->Unacquire())) {
        qDebug() << "Unacquire failed, hr: " << unsigned long(hr);
        return hr;
    }
    return diDevice->Acquire();
}

HRESULT DeviceInfo::updateState() {
    HRESULT hr = diDevice->GetDeviceState(sizeof(DIJOYSTATE2), &joyState);
    return hr;
}

QMap<QUuid, QString> DeviceInfo::getDeviceAxes() {
    QMap<QUuid, QString> axisMap;
    diDevice->EnumObjects(enumAxesCallback,
        (VOID*)&axisMap, DIDFT_AXIS);
    return axisMap;
}

long DeviceInfo::getAxisReading(QUuid axisGuid) {
    if (axisGuid == GUID_XAxis)
    {
        return joyState.lX;
    }
    else if (axisGuid == GUID_YAxis)
    {
        return joyState.lY;
    }
    else if (axisGuid == GUID_ZAxis)
    {
        return joyState.lZ;
    }
    else if (axisGuid == GUID_RxAxis) {
        return joyState.lRx;
    }
    else if (axisGuid == GUID_RyAxis)
    {
        return joyState.lRy;
    }
    else if (axisGuid == GUID_RzAxis)
    {
        return joyState.lRz;
    }
    else if (axisGuid == GUID_Slider)
    {
        return joyState.rglSlider[0];
    }
    return 0;
}

bool DeviceInfo::isButtonPressed(int buttonIndex) {
    if (joyState.rgbButtons[buttonIndex] & 0x80) {
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
