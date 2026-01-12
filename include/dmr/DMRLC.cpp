/*
 *   DMR Link Control (LC) for ESP32 MMDVM Hotspot
 *   Adapted from MMDVMHost by Jonathan Naylor G4KLX
 *
 *   Copyright (C) 2015,2016,2019,2021,2022,2025 by Jonathan Naylor G4KLX
 *   ESP32 adaptation for MMDVM Hotspot project
 */

#include "DMRLC.h"

DMRLC::DMRLC(FLCO flco, uint32_t srcId, uint32_t dstId) :
m_PF(false),
m_R(false),
m_FLCO(flco),
m_FID(0),
m_options(0),
m_srcId(srcId),
m_dstId(dstId) {
}

DMRLC::DMRLC(const uint8_t* bytes) :
m_PF(false),
m_R(false),
m_FLCO(FLCO::GROUP),
m_FID(0),
m_options(0),
m_srcId(0),
m_dstId(0) {
    m_PF = (bytes[0] & 0x80) == 0x80;
    m_R  = (bytes[0] & 0x40) == 0x40;

    m_FLCO = FLCO(bytes[0] & 0x3F);

    m_FID = bytes[1];

    m_options = bytes[2];

    m_dstId = bytes[3] << 16 | bytes[4] << 8 | bytes[5];
    m_srcId = bytes[6] << 16 | bytes[7] << 8 | bytes[8];
}

DMRLC::DMRLC(const bool* bits) :
m_PF(false),
m_R(false),
m_FLCO(FLCO::GROUP),
m_FID(0),
m_options(0),
m_srcId(0),
m_dstId(0) {
    m_PF = bits[0];
    m_R  = bits[1];

    uint8_t temp1, temp2, temp3;
    DMRUtils::bitsToByteBE(bits + 0, temp1);
    m_FLCO = FLCO(temp1 & 0x3F);

    DMRUtils::bitsToByteBE(bits + 8, temp2);
    m_FID = temp2;

    DMRUtils::bitsToByteBE(bits + 16, temp3);
    m_options = temp3;

    uint8_t d1, d2, d3;
    DMRUtils::bitsToByteBE(bits + 24, d1);
    DMRUtils::bitsToByteBE(bits + 32, d2);
    DMRUtils::bitsToByteBE(bits + 40, d3);

    uint8_t s1, s2, s3;
    DMRUtils::bitsToByteBE(bits + 48, s1);
    DMRUtils::bitsToByteBE(bits + 56, s2);
    DMRUtils::bitsToByteBE(bits + 64, s3);

    m_srcId = s1 << 16 | s2 << 8 | s3;
    m_dstId = d1 << 16 | d2 << 8 | d3;
}

DMRLC::DMRLC() :
m_PF(false),
m_R(false),
m_FLCO(FLCO::GROUP),
m_FID(0),
m_options(0),
m_srcId(0),
m_dstId(0) {
}

DMRLC::~DMRLC() {
}

void DMRLC::getData(uint8_t* bytes) const {
    bytes[0] = (uint8_t)m_FLCO;

    if (m_PF)
        bytes[0] |= 0x80;

    if (m_R)
        bytes[0] |= 0x40;

    bytes[1] = m_FID;

    bytes[2] = m_options;

    bytes[3] = m_dstId >> 16;
    bytes[4] = m_dstId >> 8;
    bytes[5] = m_dstId >> 0;

    bytes[6] = m_srcId >> 16;
    bytes[7] = m_srcId >> 8;
    bytes[8] = m_srcId >> 0;
}

void DMRLC::getData(bool* bits) const {
    uint8_t bytes[9];
    getData(bytes);

    DMRUtils::byteToBitsBE(bytes[0], bits + 0);
    DMRUtils::byteToBitsBE(bytes[1], bits + 8);
    DMRUtils::byteToBitsBE(bytes[2], bits + 16);
    DMRUtils::byteToBitsBE(bytes[3], bits + 24);
    DMRUtils::byteToBitsBE(bytes[4], bits + 32);
    DMRUtils::byteToBitsBE(bytes[5], bits + 40);
    DMRUtils::byteToBitsBE(bytes[6], bits + 48);
    DMRUtils::byteToBitsBE(bytes[7], bits + 56);
    DMRUtils::byteToBitsBE(bytes[8], bits + 64);
}

bool DMRLC::getPF() const {
    return m_PF;
}

void DMRLC::setPF(bool pf) {
    m_PF = pf;
}

FLCO DMRLC::getFLCO() const {
    return m_FLCO;
}

void DMRLC::setFLCO(FLCO flco) {
    m_FLCO = flco;
}

uint8_t DMRLC::getFID() const {
    return m_FID;
}

void DMRLC::setFID(uint8_t fid) {
    m_FID = fid;
}

bool DMRLC::getOVCM() const {
    return (m_options & 0x04) == 0x04;
}

void DMRLC::setOVCM(bool ovcm) {
    if (ovcm)
        m_options |= 0x04;
    else
        m_options &= 0xFB;
}

uint32_t DMRLC::getSrcId() const {
    return m_srcId;
}

void DMRLC::setSrcId(uint32_t id) {
    m_srcId = id;
}

uint32_t DMRLC::getDstId() const {
    return m_dstId;
}

void DMRLC::setDstId(uint32_t id) {
    m_dstId = id;
}
