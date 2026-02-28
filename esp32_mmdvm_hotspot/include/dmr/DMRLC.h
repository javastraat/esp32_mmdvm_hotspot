/*
 *   DMR Link Control (LC) for ESP32 MMDVM Hotspot
 *   Adapted from MMDVMHost by Jonathan Naylor G4KLX
 *
 *   Copyright (C) 2015,2016,2019,2021,2022 by Jonathan Naylor G4KLX
 *   ESP32 adaptation for MMDVM Hotspot project
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 */

#ifndef DMRLC_H
#define DMRLC_H

#include "DMRDefines.h"
#include "DMRUtils.h"
#include <stdint.h>

class DMRLC {
public:
    DMRLC(FLCO flco, uint32_t srcId, uint32_t dstId);
    DMRLC(const uint8_t* bytes);
    DMRLC(const bool* bits);
    DMRLC();
    ~DMRLC();

    void getData(uint8_t* bytes) const;
    void getData(bool* bits) const;

    bool getPF() const;
    void setPF(bool pf);

    FLCO getFLCO() const;
    void setFLCO(FLCO flco);

    bool getOVCM() const;
    void setOVCM(bool ovcm);

    uint8_t getFID() const;
    void setFID(uint8_t fid);

    uint32_t getSrcId() const;
    void setSrcId(uint32_t id);

    uint32_t getDstId() const;
    void setDstId(uint32_t id);

private:
    bool      m_PF;
    bool      m_R;
    FLCO      m_FLCO;
    uint8_t   m_FID;
    uint8_t   m_options;
    uint32_t  m_srcId;
    uint32_t  m_dstId;
};

#endif
