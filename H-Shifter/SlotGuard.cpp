/*
This file is part of Bonus FFB.

Bonus FFB is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or any later version.

Bonus FFB is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Bonus FFB. If not, see <https://www.gnu.org/licenses/>.
*/

#include "SlotGuard.h"

#include <QDebug>

HRESULT SlotGuard::start(BonusFFB::DeviceInfo* device) {
    eff.dwSize = sizeof(DIEFFECT);
    eff.dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
    eff.dwDuration = INFINITE;
    eff.dwSamplePeriod = 0;
    eff.dwGain = DI_FFNOMINALMAX;
    eff.dwTriggerButton = DIEB_NOTRIGGER;
    eff.dwTriggerRepeatInterval = 0;
    eff.cAxes = 2;
    eff.rgdwAxes = AXES;
    eff.rglDirection = FORWARDBACK;
    eff.lpEnvelope = 0;
    eff.cbTypeSpecificParams = sizeof(DICONDITION) * 2;
    conditions[0] = keepLeft;
    conditions[1] = keepFBCentered;
    eff.lpvTypeSpecificParams = &conditions;
    eff.dwStartDelay = 0;


    HRESULT hr;
    if (lpdiEff == nullptr) {
        hr = device->diDevice->CreateEffect(GUID_Spring,
            &eff, &lpdiEff, nullptr);
        if (FAILED(hr))
            return hr;
    }
    hr = lpdiEff->Start(INFINITE, 0);
    return hr;
}

void SlotGuard::updateSlotGuardEffects(SlotState state) {
    //HRESULT hr;

    if (state == SlotState::NEUTRAL_UNDER_SLOT) {
        // Disable the effect to prevent thrashing
        // We can scale the condition coefficients near the junctions instead, but good enough for now
        conditions[0] = noSpring;
        conditions[1] = noSpring;
        conditions[1].lDeadBand = 500;
    } else if (state == SlotState::NEUTRAL) {
        conditions[0] = noSpring;
        conditions[1] = keepFBCentered;
        conditions[1].lDeadBand = 500;
    } else if (state == SlotState::SLOT_LEFT_FWD || state == SlotState::SLOT_LEFT_BACK) {
        conditions[0] = keepLeft;
        conditions[1] = noSpring;
    } else if (state == SlotState::SLOT_MIDDLE_FWD || state == SlotState::SLOT_MIDDLE_BACK) {
        conditions[0] = keepLRCentered;
        conditions[1] = noSpring;
    } else if (state == SlotState::SLOT_RIGHT_FWD || state == SlotState::SLOT_RIGHT_BACK) {
        conditions[0] = keepRight;
        conditions[1] = noSpring;
    }
    eff.lpvTypeSpecificParams = conditions;
    lpdiEff->SetParameters(&eff, DIEP_TYPESPECIFICPARAMS);
}
