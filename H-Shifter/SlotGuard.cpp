/*
This file is part of Bonus FFB.

Bonus FFB is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or any later version.

Bonus FFB is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Bonus FFB. If not, see <https://www.gnu.org/licenses/>.
*/

#include "SlotGuard.h"

#include <QDebug>

HRESULT SlotGuard::start(BonusFFB::DeviceInfo* device) {
    state = PositionState::NEUTRAL;

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

SlotGuard::PositionState SlotGuard::update(long lrValue, long fbValue) {
    HRESULT hr;
    bool in_neutral = is_in_neutral(fbValue);
    int under_slot = is_under_slot(lrValue);
    
    if (in_neutral && under_slot) {
        if (state != NEUTRAL_UNDER_SLOT) {
           // Disable the effect to prevent thrashing
            // We can scale the condition coefficients near the junctions instead, but good enough for now
            conditions[0] = noSpring;
            conditions[1] = noSpring;
            eff.lpvTypeSpecificParams = conditions;
            hr = lpdiEff->SetParameters(&eff, DIEP_TYPESPECIFICPARAMS);
            state = NEUTRAL_UNDER_SLOT;
        }
    } else if (in_neutral) {
        if (state != NEUTRAL) {
            conditions[0] = noSpring;
            conditions[1] = keepFBCentered;
            eff.lpvTypeSpecificParams = conditions;
            lpdiEff->SetParameters(&eff, DIEP_TYPESPECIFICPARAMS);
            state = NEUTRAL;
        }
    } else if (under_slot == 1) {
        if (state != SLOT_LEFT) {
            conditions[0] = keepLeft;
            conditions[1] = noSpring;
            eff.lpvTypeSpecificParams = conditions;
            lpdiEff->SetParameters(&eff, DIEP_TYPESPECIFICPARAMS);
            state = SLOT_LEFT;
        }
    } else if (under_slot == 2) {
        if (state != SLOT_MIDDLE) {
            conditions[0] = keepLRCentered;
            conditions[1] = noSpring;
            eff.lpvTypeSpecificParams = conditions;
            lpdiEff->SetParameters(&eff, DIEP_TYPESPECIFICPARAMS);
            state = SLOT_MIDDLE;
        }
    } else if (under_slot == 3) {
        if (state != SLOT_RIGHT) {
            conditions[0] = keepRight;
            conditions[1] = noSpring;
            eff.lpvTypeSpecificParams = conditions;
            lpdiEff->SetParameters(&eff, DIEP_TYPESPECIFICPARAMS);
            state = SLOT_RIGHT;
        }
    }
    return state;
}

bool SlotGuard::is_in_neutral(long fbValue) {
    if (fbValue <= MIDPOINT + NEUTRAL_HALF_WIDTH && fbValue >= MIDPOINT - NEUTRAL_HALF_WIDTH) {
        return true;
    }
    return false;
}

// Longwided computation to keep me sane...
int SlotGuard::is_under_slot(long lrValue) {
    if (lrValue <= MINPOINT + SIDE_SLOT_WIDTH) {
        // In neutral under left channel
        return 1;
    }
    else if (lrValue >= MIDPOINT - MIDDLE_SLOT_HALF_WIDTH && lrValue <= MIDPOINT + MIDDLE_SLOT_HALF_WIDTH)
    {
        // In neutral under center channel
        return 2;
    }
    else if (lrValue >= MAXPOINT - SIDE_SLOT_WIDTH) {
        // In neutral under right channel
        return 3;
    }
    return 0;
}