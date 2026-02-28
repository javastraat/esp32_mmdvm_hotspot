/*
 *   DMR Reed-Solomon (12,9) for ESP32 MMDVM Hotspot
 *   Adapted from MMDVMHost by Jonathan Naylor G4KLX
 *
 *   Copyright (C) 2015,2025 by Jonathan Naylor G4KLX
 *   ESP32 adaptation for MMDVM Hotspot project
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 */

#ifndef DMRRS_H
#define DMRRS_H

#include <stdint.h>

class DMRRS129 {
public:
    // Check RS(12,9) parity on 12-byte LC data
    static bool check(const uint8_t* in);

    // Generate RS(12,9) parity for 9-byte message
    static void encode(const uint8_t* msg, unsigned int nbytes, uint8_t* parity);

private:
    static const unsigned int NPAR = 3;
    static const uint8_t POLY[];
    static const uint8_t EXP_TABLE[];
    static const uint8_t LOG_TABLE[];

    // Galois field multiplication
    static inline uint8_t gmult(uint8_t a, uint8_t b);
};

#endif
