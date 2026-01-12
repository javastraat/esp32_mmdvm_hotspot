/*
 *   DMR BPTC(196,96) Error Correction for ESP32 MMDVM Hotspot
 *   Adapted from MMDVMHost by Jonathan Naylor G4KLX
 *
 *   Copyright (C) 2012 by Ian Wraith
 *   Copyright (C) 2015,2025 by Jonathan Naylor G4KLX
 *   ESP32 adaptation for MMDVM Hotspot project
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 */

#ifndef DMRBPTC_H
#define DMRBPTC_H

#include <stdint.h>
#include "DMRUtils.h"
#include "DMRHamming.h"

class DMRBPTC19696 {
public:
    DMRBPTC19696();
    ~DMRBPTC19696();

    // Decode 196-bit BPTC encoded data to 96-bit (12 byte) LC data
    void decode(const uint8_t* in, uint8_t* out);

    // Encode 96-bit (12 byte) LC data to 196-bit BPTC encoded data
    void encode(const uint8_t* in, uint8_t* out);

private:
    bool* m_rawData;
    bool* m_deInterData;

    void decodeExtractBinary(const uint8_t* in);
    void decodeDeInterleave();
    void decodeErrorCheck();
    void decodeExtractData(uint8_t* data) const;

    void encodeExtractData(const uint8_t* in) const;
    void encodeInterleave();
    void encodeErrorCheck();
    void encodeExtractBinary(uint8_t* data);
};

#endif
