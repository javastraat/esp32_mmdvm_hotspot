/*
 *   DMR Utility Functions for ESP32 MMDVM Hotspot
 *   Adapted from MMDVMHost Utils by Jonathan Naylor G4KLX
 *
 *   Copyright (C) 2009,2014,2015,2021 by Jonathan Naylor, G4KLX
 *   ESP32 adaptation for MMDVM Hotspot project
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; version 2 of the License.
 */

#ifndef DMRUtils_H
#define DMRUtils_H

#include <stdint.h>
#include <Arduino.h>

class DMRUtils {
public:
    // Convert byte to bit array (Big Endian - MSB first)
    static inline void byteToBitsBE(uint8_t byte, bool* bits) {
        bits[0] = (byte & 0x80U) == 0x80U;
        bits[1] = (byte & 0x40U) == 0x40U;
        bits[2] = (byte & 0x20U) == 0x20U;
        bits[3] = (byte & 0x10U) == 0x10U;
        bits[4] = (byte & 0x08U) == 0x08U;
        bits[5] = (byte & 0x04U) == 0x04U;
        bits[6] = (byte & 0x02U) == 0x02U;
        bits[7] = (byte & 0x01U) == 0x01U;
    }

    // Convert bit array to byte (Big Endian - MSB first)
    static inline void bitsToByteBE(const bool* bits, uint8_t& byte) {
        byte  = bits[0] ? 0x80U : 0x00U;
        byte |= bits[1] ? 0x40U : 0x00U;
        byte |= bits[2] ? 0x20U : 0x00U;
        byte |= bits[3] ? 0x10U : 0x00U;
        byte |= bits[4] ? 0x08U : 0x00U;
        byte |= bits[5] ? 0x04U : 0x00U;
        byte |= bits[6] ? 0x02U : 0x00U;
        byte |= bits[7] ? 0x01U : 0x00U;
    }
};

#endif
