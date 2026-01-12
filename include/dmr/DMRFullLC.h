/*
 *   DMR Full Link Control Decoder for ESP32 MMDVM Hotspot
 *   Adapted from MMDVMHost by Jonathan Naylor G4KLX
 *
 *   Copyright (C) 2012 by Ian Wraith
 *   Copyright (C) 2015,2016,2025 by Jonathan Naylor G4KLX
 *   ESP32 adaptation for MMDVM Hotspot project
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 */

#ifndef DMRFullLC_H
#define DMRFullLC_H

#include "DMRLC.h"
#include "DMRBPTC.h"
#include <stdint.h>

class DMRFullLC {
public:
    DMRFullLC();
    ~DMRFullLC();

    // Decode full LC from 33-byte DMR frame
    // Returns pointer to LC object (caller must delete) or nullptr on failure
    DMRLC* decode(const uint8_t* data, uint8_t type);

    // Encode full LC into 33-byte DMR frame
    void encode(const DMRLC& lc, uint8_t* data, uint8_t type);

private:
    DMRBPTC19696 m_bptc;
};

#endif
