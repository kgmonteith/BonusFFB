/*
Copyright (C) 2024-2025 Ken Monteith.

This file is part of Bonus FFB.

Bonus FFB is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or any later version.

Bonus FFB is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Bonus FFB. If not, see <https://www.gnu.org/licenses/>.
*/

#include "SlotGuard.h"

#include <QDebug>

HRESULT SlotGuard::start(BonusFFB::DeviceInfo* device) {
    slotSpringEff.dwSize = sizeof(DIEFFECT);
    slotSpringEff.dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
    slotSpringEff.dwDuration = INFINITE;
    slotSpringEff.dwSamplePeriod = 0;
    slotSpringEff.dwGain = DI_FFNOMINALMAX;
    slotSpringEff.dwTriggerButton = DIEB_NOTRIGGER;
    slotSpringEff.dwTriggerRepeatInterval = 0;
    slotSpringEff.cAxes = 2;
    slotSpringEff.rgdwAxes = AXES;
    slotSpringEff.rglDirection = FORWARDBACK;
    slotSpringEff.lpEnvelope = 0;
    slotSpringEff.cbTypeSpecificParams = sizeof(DICONDITION) * 2;
    springConditions[0] = keepLeft;
    springConditions[1] = keepFBCentered;
    slotSpringEff.lpvTypeSpecificParams = &springConditions;
    slotSpringEff.dwStartDelay = 0;


    HRESULT hr;
    if (lpdiSlotSpringEff == nullptr) {
        hr = device->diDevice->CreateEffect(GUID_Spring,
            &slotSpringEff, &lpdiSlotSpringEff, nullptr);
        if (FAILED(hr))
            return hr;
    }
    hr = lpdiSlotSpringEff->Start(INFINITE, 0);
    return hr;
}

void SlotGuard::updateSlotGuardEffects(SlotState state) {
    if (state == SlotState::NEUTRAL_UNDER_SLOT) {
        // Disable the effect to prevent thrashing
        // We can scale the condition coefficients near the junctions instead, but good enough for now
        springConditions[0] = noSpring;
        springConditions[1] = noSpring;
        springConditions[1].lDeadBand = 500;
    } else if (state == SlotState::NEUTRAL) {
        springConditions[0] = noSpring;
        springConditions[1] = keepFBCentered;
        springConditions[1].lDeadBand = 500;
    } else if (state == SlotState::SLOT_LEFT_FWD || state == SlotState::SLOT_LEFT_BACK) {
        springConditions[0] = keepLeft;
        springConditions[1] = noSpring;
    } else if (state == SlotState::SLOT_MIDDLE_FWD || state == SlotState::SLOT_MIDDLE_BACK) {
        springConditions[0] = keepLRCentered;
        springConditions[1] = noSpring;
    } else if (state == SlotState::SLOT_RIGHT_FWD || state == SlotState::SLOT_RIGHT_BACK) {
        springConditions[0] = keepRight;
        springConditions[1] = noSpring;
    }
    slotSpringEff.lpvTypeSpecificParams = springConditions;
    lpdiSlotSpringEff->SetParameters(&slotSpringEff, DIEP_TYPESPECIFICPARAMS);
}
