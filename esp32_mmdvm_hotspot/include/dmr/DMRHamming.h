/*
 *   DMR Hamming Error Correction for ESP32 MMDVM Hotspot
 *   Adapted from MMDVMHost by Jonathan Naylor G4KLX
 *
 *   Copyright (C) 2015,2016 by Jonathan Naylor G4KLX
 *   ESP32 adaptation for MMDVM Hotspot project
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 */

#ifndef DMRHamming_H
#define DMRHamming_H

#include <stdint.h>

class DMRHamming {
public:
    // Hamming (15,11,3) - used for BPTC row error correction
    static void encode15113_2(bool* d);
    static bool decode15113_2(bool* d);

    // Hamming (13,9,3) - used for BPTC column error correction
    static void encode1393(bool* d);
    static bool decode1393(bool* d);
};

// Implementation inline for performance

inline void DMRHamming::encode15113_2(bool* d) {
    // Calculate the checksum this row should have
    d[11] = d[0] ^ d[1] ^ d[2] ^ d[3] ^ d[5] ^ d[7] ^ d[8];
    d[12] = d[1] ^ d[2] ^ d[3] ^ d[4] ^ d[6] ^ d[8] ^ d[9];
    d[13] = d[2] ^ d[3] ^ d[4] ^ d[5] ^ d[7] ^ d[9] ^ d[10];
    d[14] = d[0] ^ d[1] ^ d[2] ^ d[4] ^ d[6] ^ d[7] ^ d[10];
}

inline bool DMRHamming::decode15113_2(bool* d) {
    // Calculate the checksum this row should have
    bool c0 = d[0] ^ d[1] ^ d[2] ^ d[3] ^ d[5] ^ d[7] ^ d[8];
    bool c1 = d[1] ^ d[2] ^ d[3] ^ d[4] ^ d[6] ^ d[8] ^ d[9];
    bool c2 = d[2] ^ d[3] ^ d[4] ^ d[5] ^ d[7] ^ d[9] ^ d[10];
    bool c3 = d[0] ^ d[1] ^ d[2] ^ d[4] ^ d[6] ^ d[7] ^ d[10];

    uint8_t n = 0x00U;
    n |= (c0 != d[11]) ? 0x01U : 0x00U;
    n |= (c1 != d[12]) ? 0x02U : 0x00U;
    n |= (c2 != d[13]) ? 0x04U : 0x00U;
    n |= (c3 != d[14]) ? 0x08U : 0x00U;

    switch (n) {
        // Parity bit errors
        case 0x01U: d[11] = !d[11]; return true;
        case 0x02U: d[12] = !d[12]; return true;
        case 0x04U: d[13] = !d[13]; return true;
        case 0x08U: d[14] = !d[14]; return true;

        // Data bit errors
        case 0x09U: d[0]  = !d[0];  return true;
        case 0x0BU: d[1]  = !d[1];  return true;
        case 0x0FU: d[2] = !d[2];  return true;
        case 0x07U: d[3]  = !d[3];  return true;
        case 0x0EU: d[4]  = !d[4];  return true;
        case 0x05U: d[5]  = !d[5];  return true;
        case 0x0AU: d[6]  = !d[6];  return true;
        case 0x0DU: d[7]  = !d[7];  return true;
        case 0x03U: d[8]  = !d[8];  return true;
        case 0x0CU: d[9]  = !d[9];  return true;
        case 0x06U: d[10] = !d[10]; return true;

        // No bit errors
        default: return false;
    }
}

inline void DMRHamming::encode1393(bool* d) {
    // Calculate the checksum this column should have
    d[9]  = d[0] ^ d[1] ^ d[3] ^ d[5] ^ d[6];
    d[10] = d[0] ^ d[1] ^ d[2] ^ d[4] ^ d[6] ^ d[7];
    d[11] = d[0] ^ d[1] ^ d[2] ^ d[3] ^ d[5] ^ d[7] ^ d[8];
    d[12] = d[0] ^ d[2] ^ d[4] ^ d[5] ^ d[8];
}

inline bool DMRHamming::decode1393(bool* d) {
    // Calculate the checksum this column should have
    bool c0 = d[0] ^ d[1] ^ d[3] ^ d[5] ^ d[6];
    bool c1 = d[0] ^ d[1] ^ d[2] ^ d[4] ^ d[6] ^ d[7];
    bool c2 = d[0] ^ d[1] ^ d[2] ^ d[3] ^ d[5] ^ d[7] ^ d[8];
    bool c3 = d[0] ^ d[2] ^ d[4] ^ d[5] ^ d[8];

    uint8_t n = 0x00U;
    n |= (c0 != d[9])  ? 0x01U : 0x00U;
    n |= (c1 != d[10]) ? 0x02U : 0x00U;
    n |= (c2 != d[11]) ? 0x04U : 0x00U;
    n |= (c3 != d[12]) ? 0x08U : 0x00U;

    switch (n) {
        // Parity bit errors
        case 0x01U: d[9]  = !d[9];  return true;
        case 0x02U: d[10] = !d[10]; return true;
        case 0x04U: d[11] = !d[11]; return true;
        case 0x08U: d[12] = !d[12]; return true;

        // Data bit errors
        case 0x0FU: d[0] = !d[0]; return true;
        case 0x07U: d[1] = !d[1]; return true;
        case 0x0EU: d[2] = !d[2]; return true;
        case 0x05U: d[3] = !d[3]; return true;
        case 0x0AU: d[4] = !d[4]; return true;
        case 0x0DU: d[5] = !d[5]; return true;
        case 0x03U: d[6] = !d[6]; return true;
        case 0x06U: d[7] = !d[7]; return true;
        case 0x0CU: d[8] = !d[8]; return true;

        // No bit errors
        default: return false;
    }
}

#endif
