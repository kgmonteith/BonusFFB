/*
This file is part of Bonus FFB.

Bonus FFB is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or any later version.

Bonus FFB is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Bonus FFB. If not, see <https://www.gnu.org/licenses/>.
*/

#include "SynchroGuard.h"
#include "SlotGuard.h"

#include <QDebug>

HRESULT SynchroGuard::start(BonusFFB::DeviceInfo* device) {
    springEff.dwSize = sizeof(DIEFFECT);
    springEff.dwFlags = DIEFF_POLAR | DIEFF_OBJECTOFFSETS;
    springEff.dwDuration = INFINITE;
    springEff.dwSamplePeriod = 0;
    springEff.dwGain = DI_FFNOMINALMAX;
    springEff.dwTriggerButton = DIEB_NOTRIGGER;
    springEff.dwTriggerRepeatInterval = 0;
    springEff.cAxes = 2;
    springEff.rgdwAxes = AXES;
    LONG dir[2] = { DI_DEGREES * 180, 0 };
    springEff.rglDirection = dir;
    springEff.lpEnvelope = 0;
    springEff.cbTypeSpecificParams = sizeof(DICONDITION);
    springEff.lpvTypeSpecificParams = &testCondition;
    springEff.dwStartDelay = 0;


    LONG rglDirection[2] = { 90 * DI_DEGREES, DI_DEGREES };
    rumbleEff.dwSize = sizeof(DIEFFECT);
    rumbleEff.dwFlags = DIEFF_POLAR | DIEFF_OBJECTOFFSETS;
    rumbleEff.dwDuration = INFINITE;
    rumbleEff.dwSamplePeriod = 0;
    rumbleEff.dwGain = DI_FFNOMINALMAX;
    rumbleEff.dwTriggerButton = DIEB_NOTRIGGER;
    rumbleEff.dwTriggerRepeatInterval = 0;
    rumbleEff.cAxes = 2;
    rumbleEff.rgdwAxes = AXES;
    rumbleEff.rglDirection = rglDirection;
    rumbleEff.lpEnvelope = 0;
    rumbleEff.cbTypeSpecificParams = sizeof(DIPERIODIC);
    rumbleEff.lpvTypeSpecificParams = &rumble;
    rumbleEff.dwStartDelay = 0;

    HRESULT hr;
    if (lpdiSpringEff == nullptr) {
        hr = device->diDevice->CreateEffect(GUID_Spring,
            &springEff, &lpdiSpringEff, nullptr);
        if (FAILED(hr)) {
            return hr;
        }
    }
    hr = lpdiSpringEff->Start(INFINITE, 0);

    if (lpdiRumbleEff == nullptr) {
        hr = device->diDevice->CreateEffect(GUID_Sine,
            &rumbleEff, &lpdiRumbleEff, nullptr);
        if (FAILED(hr)) {
            qDebug() << "CreateEffect failed";
            return hr;
        }
    }
    hr = lpdiRumbleEff->Start(INFINITE, 0);
    return hr;
}

void SynchroGuard::update(long lrValue, long fbValue, long clutchValue, long throttleValue, bool gearEngaged) {
    bool under_slot = SlotGuard::is_under_slot(lrValue);
    double clutch_engagement = clutchValue * 0.152587;
    if (under_slot) {
        testCondition.lNegativeCoefficient = -10000 + clutch_engagement;
        springEff.lpvTypeSpecificParams = &testCondition;
        lpdiSpringEff->SetParameters(&springEff, DIEP_TYPESPECIFICPARAMS);
        //qDebug() << "clutch_engagement: " << clutch_engagement << ", fbValue: " << fbValue << ", midpoint: " << MIDPOINT;
        if (clutch_engagement < 3000 && (fbValue > MIDPOINT + 7500 || fbValue < MIDPOINT - 7500)) {
            qDebug() << "Rumbling...";
            rumble.dwMagnitude = 2000;
            rumbleEff.lpvTypeSpecificParams = &rumble;
            HRESULT hr = lpdiRumbleEff->SetParameters(&rumbleEff, DIEP_TYPESPECIFICPARAMS);
            qDebug() << "hr: " << Qt::hex << unsigned long(hr);
            rumbling = true;
        }
        else if (rumbling) {
            rumble.dwMagnitude = 0;
            rumbleEff.lpvTypeSpecificParams = &rumble;
            HRESULT hr = lpdiRumbleEff->SetParameters(&rumbleEff, DIEP_TYPESPECIFICPARAMS);
            rumbling = false;
        }
    }
}