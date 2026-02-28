/*
 *   DMR Full Link Control Decoder for ESP32 MMDVM Hotspot
 *   Adapted from MMDVMHost by Jonathan Naylor G4KLX
 *
 *   Copyright (C) 2012 by Ian Wraith
 *   Copyright (C) 2015,2016,2025 by Jonathan Naylor G4KLX
 *   ESP32 adaptation for MMDVM Hotspot project
 */

#include "DMRFullLC.h"
#include "DMRDefines.h"
#include "DMRRS.h"

DMRFullLC::DMRFullLC() :
m_bptc() {
}

DMRFullLC::~DMRFullLC() {
}

DMRLC* DMRFullLC::decode(const uint8_t* data, uint8_t type) {
    uint8_t lcData[12];
    m_bptc.decode(data, lcData);

    switch (type) {
        case DT_VOICE_LC_HEADER:
            lcData[9]  ^= VOICE_LC_HEADER_CRC_MASK[0];
            lcData[10] ^= VOICE_LC_HEADER_CRC_MASK[1];
            lcData[11] ^= VOICE_LC_HEADER_CRC_MASK[2];
            break;

        case DT_TERMINATOR_WITH_LC:
            lcData[9]  ^= TERMINATOR_WITH_LC_CRC_MASK[0];
            lcData[10] ^= TERMINATOR_WITH_LC_CRC_MASK[1];
            lcData[11] ^= TERMINATOR_WITH_LC_CRC_MASK[2];
            break;

        default:
            // Unsupported LC type
            return nullptr;
    }

    if (!DMRRS129::check(lcData))
        return nullptr;

    return new DMRLC(lcData);
}

void DMRFullLC::encode(const DMRLC& lc, uint8_t* data, uint8_t type) {
    uint8_t lcData[12];
    lc.getData(lcData);

    uint8_t parity[4];
    DMRRS129::encode(lcData, 9, parity);

    switch (type) {
        case DT_VOICE_LC_HEADER:
            lcData[9]  = parity[2] ^ VOICE_LC_HEADER_CRC_MASK[0];
            lcData[10] = parity[1] ^ VOICE_LC_HEADER_CRC_MASK[1];
            lcData[11] = parity[0] ^ VOICE_LC_HEADER_CRC_MASK[2];
            break;

        case DT_TERMINATOR_WITH_LC:
            lcData[9]  = parity[2] ^ TERMINATOR_WITH_LC_CRC_MASK[0];
            lcData[10] = parity[1] ^ TERMINATOR_WITH_LC_CRC_MASK[1];
            lcData[11] = parity[0] ^ TERMINATOR_WITH_LC_CRC_MASK[2];
            break;

        default:
            // Unsupported LC type
            return;
    }

    m_bptc.encode(lcData, data);
}
