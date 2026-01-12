/*
 *   DMR Protocol Defines for ESP32 MMDVM Hotspot
 *   Adapted from MMDVMHost by Jonathan Naylor G4KLX
 *
 *   Copyright (C) 2015,2016,2021,2025 by Jonathan Naylor G4KLX
 *   ESP32 adaptation for MMDVM Hotspot project
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 */

#ifndef DMRDefines_H
#define DMRDefines_H

#include <stdint.h>

// DMR Frame Constants
const unsigned int DMR_FRAME_LENGTH_BYTES = 33U;

// DMR Data Types
const uint8_t DT_VOICE_LC_HEADER    = 0x01U;
const uint8_t DT_TERMINATOR_WITH_LC = 0x02U;

// DMR Sync Patterns (MS sourced - Mobile Station / simplex)
const uint8_t MS_SOURCED_AUDIO_SYNC[] = {0x07U, 0xF7U, 0xD5U, 0xDDU, 0x57U, 0xDFU, 0xD0U};
const uint8_t MS_SOURCED_DATA_SYNC[]  = {0x0DU, 0x5DU, 0x7FU, 0x77U, 0xFDU, 0x75U, 0x70U};
// Alternate data sync pattern (observed from actual radio - with color code variation)
const uint8_t MS_SOURCED_DATA_SYNC_ALT[] = {0x6DU, 0x5DU, 0x7FU, 0x77U, 0xFDU, 0x75U, 0x7EU};

// CRC Masks for LC headers
const uint8_t VOICE_LC_HEADER_CRC_MASK[]    = {0x96U, 0x96U, 0x96U};
const uint8_t TERMINATOR_WITH_LC_CRC_MASK[] = {0x99U, 0x99U, 0x99U};

// FLCO - Full LC Opcode (call type)
enum class FLCO : uint8_t {
    GROUP               = 0,    // Group call (talk group)
    USER_USER           = 3,    // Private call
    TALKER_ALIAS_HEADER = 4,
    TALKER_ALIAS_BLOCK1 = 5,
    TALKER_ALIAS_BLOCK2 = 6,
    TALKER_ALIAS_BLOCK3 = 7,
    GPS_INFO            = 8
};

#endif
