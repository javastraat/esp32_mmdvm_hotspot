/*
 *   DMR BPTC(196,96) Error Correction for ESP32 MMDVM Hotspot
 *   Adapted from MMDVMHost by Jonathan Naylor G4KLX
 *
 *   Copyright (C) 2012 by Ian Wraith
 *   Copyright (C) 2015,2025 by Jonathan Naylor G4KLX
 *   ESP32 adaptation for MMDVM Hotspot project
 */

#include "DMRBPTC.h"
#include <string.h>

DMRBPTC19696::DMRBPTC19696() :
m_rawData(nullptr),
m_deInterData(nullptr) {
    m_rawData     = new bool[196];
    m_deInterData = new bool[196];
}

DMRBPTC19696::~DMRBPTC19696() {
    delete[] m_rawData;
    delete[] m_deInterData;
}

// The main decode function
void DMRBPTC19696::decode(const uint8_t* in, uint8_t* out) {
    // Get the raw binary
    decodeExtractBinary(in);

    // Deinterleave
    decodeDeInterleave();

    // Error check
    decodeErrorCheck();

    // Extract Data
    decodeExtractData(out);
}

// The main encode function
void DMRBPTC19696::encode(const uint8_t* in, uint8_t* out) {
    // Extract Data
    encodeExtractData(in);

    // Error check
    encodeErrorCheck();

    // Interleave
    encodeInterleave();

    // Get the raw binary
    encodeExtractBinary(out);
}

void DMRBPTC19696::decodeExtractBinary(const uint8_t* in) {
    // First block
    DMRUtils::byteToBitsBE(in[0],  m_rawData + 0);
    DMRUtils::byteToBitsBE(in[1],  m_rawData + 8);
    DMRUtils::byteToBitsBE(in[2],  m_rawData + 16);
    DMRUtils::byteToBitsBE(in[3],  m_rawData + 24);
    DMRUtils::byteToBitsBE(in[4],  m_rawData + 32);
    DMRUtils::byteToBitsBE(in[5],  m_rawData + 40);
    DMRUtils::byteToBitsBE(in[6],  m_rawData + 48);
    DMRUtils::byteToBitsBE(in[7],  m_rawData + 56);
    DMRUtils::byteToBitsBE(in[8],  m_rawData + 64);
    DMRUtils::byteToBitsBE(in[9],  m_rawData + 72);
    DMRUtils::byteToBitsBE(in[10], m_rawData + 80);
    DMRUtils::byteToBitsBE(in[11], m_rawData + 88);
    DMRUtils::byteToBitsBE(in[12], m_rawData + 96);

    // Handle the two bits
    bool bits[8];
    DMRUtils::byteToBitsBE(in[20], bits);
    m_rawData[98] = bits[6];
    m_rawData[99] = bits[7];

    // Second block
    DMRUtils::byteToBitsBE(in[21], m_rawData + 100);
    DMRUtils::byteToBitsBE(in[22], m_rawData + 108);
    DMRUtils::byteToBitsBE(in[23], m_rawData + 116);
    DMRUtils::byteToBitsBE(in[24], m_rawData + 124);
    DMRUtils::byteToBitsBE(in[25], m_rawData + 132);
    DMRUtils::byteToBitsBE(in[26], m_rawData + 140);
    DMRUtils::byteToBitsBE(in[27], m_rawData + 148);
    DMRUtils::byteToBitsBE(in[28], m_rawData + 156);
    DMRUtils::byteToBitsBE(in[29], m_rawData + 164);
    DMRUtils::byteToBitsBE(in[30], m_rawData + 172);
    DMRUtils::byteToBitsBE(in[31], m_rawData + 180);
    DMRUtils::byteToBitsBE(in[32], m_rawData + 188);
}

// Deinterleave the raw data
void DMRBPTC19696::decodeDeInterleave() {
    for (unsigned int i = 0; i < 196; i++)
        m_deInterData[i] = false;

    // The first bit is R(3) which is not used so can be ignored
    for (unsigned int a = 0; a < 196; a++) {
        // Calculate the interleave sequence
        unsigned int interleaveSequence = (a * 181) % 196;
        // Shuffle the data
        m_deInterData[a] = m_rawData[interleaveSequence];
    }
}

// Check each row with a Hamming (15,11,3) code and each column with a Hamming (13,9,3) code
void DMRBPTC19696::decodeErrorCheck() {
    bool fixing;
    unsigned int count = 0;
    do {
        fixing = false;

        // Run through each of the 15 columns
        bool col[13];
        for (unsigned int c = 0; c < 15; c++) {
            unsigned int pos = c + 1;
            for (unsigned int a = 0; a < 13; a++) {
                col[a] = m_deInterData[pos];
                pos = pos + 15;
            }

            if (DMRHamming::decode1393(col)) {
                unsigned int pos = c + 1;
                for (unsigned int a = 0; a < 13; a++) {
                    m_deInterData[pos] = col[a];
                    pos = pos + 15;
                }

                fixing = true;
            }
        }

        // Run through each of the 9 rows containing data
        for (unsigned int r = 0; r < 9; r++) {
            unsigned int pos = (r * 15) + 1;
            if (DMRHamming::decode15113_2(m_deInterData + pos))
                fixing = true;
        }

        count++;
    } while (fixing && count < 5);
}

// Extract the 96 bits of payload
void DMRBPTC19696::decodeExtractData(uint8_t* data) const {
    bool bData[96];
    unsigned int pos = 0;
    for (unsigned int a = 4; a <= 11; a++, pos++)
        bData[pos] = m_deInterData[a];

    for (unsigned int a = 16; a <= 26; a++, pos++)
        bData[pos] = m_deInterData[a];

    for (unsigned int a = 31; a <= 41; a++, pos++)
        bData[pos] = m_deInterData[a];

    for (unsigned int a = 46; a <= 56; a++, pos++)
        bData[pos] = m_deInterData[a];

    for (unsigned int a = 61; a <= 71; a++, pos++)
        bData[pos] = m_deInterData[a];

    for (unsigned int a = 76; a <= 86; a++, pos++)
        bData[pos] = m_deInterData[a];

    for (unsigned int a = 91; a <= 101; a++, pos++)
        bData[pos] = m_deInterData[a];

    for (unsigned int a = 106; a <= 116; a++, pos++)
        bData[pos] = m_deInterData[a];

    for (unsigned int a = 121; a <= 131; a++, pos++)
        bData[pos] = m_deInterData[a];

    DMRUtils::bitsToByteBE(bData + 0,  data[0]);
    DMRUtils::bitsToByteBE(bData + 8,  data[1]);
    DMRUtils::bitsToByteBE(bData + 16, data[2]);
    DMRUtils::bitsToByteBE(bData + 24, data[3]);
    DMRUtils::bitsToByteBE(bData + 32, data[4]);
    DMRUtils::bitsToByteBE(bData + 40, data[5]);
    DMRUtils::bitsToByteBE(bData + 48, data[6]);
    DMRUtils::bitsToByteBE(bData + 56, data[7]);
    DMRUtils::bitsToByteBE(bData + 64, data[8]);
    DMRUtils::bitsToByteBE(bData + 72, data[9]);
    DMRUtils::bitsToByteBE(bData + 80, data[10]);
    DMRUtils::bitsToByteBE(bData + 88, data[11]);
}

// Extract the 96 bits of payload
void DMRBPTC19696::encodeExtractData(const uint8_t* in) const {
    bool bData[96];
    DMRUtils::byteToBitsBE(in[0],  bData + 0);
    DMRUtils::byteToBitsBE(in[1],  bData + 8);
    DMRUtils::byteToBitsBE(in[2],  bData + 16);
    DMRUtils::byteToBitsBE(in[3],  bData + 24);
    DMRUtils::byteToBitsBE(in[4],  bData + 32);
    DMRUtils::byteToBitsBE(in[5],  bData + 40);
    DMRUtils::byteToBitsBE(in[6],  bData + 48);
    DMRUtils::byteToBitsBE(in[7],  bData + 56);
    DMRUtils::byteToBitsBE(in[8],  bData + 64);
    DMRUtils::byteToBitsBE(in[9],  bData + 72);
    DMRUtils::byteToBitsBE(in[10], bData + 80);
    DMRUtils::byteToBitsBE(in[11], bData + 88);

    for (unsigned int i = 0; i < 196; i++)
        m_deInterData[i] = false;

    unsigned int pos = 0;
    for (unsigned int a = 4; a <= 11; a++, pos++)
        m_deInterData[a] = bData[pos];

    for (unsigned int a = 16; a <= 26; a++, pos++)
        m_deInterData[a] = bData[pos];

    for (unsigned int a = 31; a <= 41; a++, pos++)
        m_deInterData[a] = bData[pos];

    for (unsigned int a = 46; a <= 56; a++, pos++)
        m_deInterData[a] = bData[pos];

    for (unsigned int a = 61; a <= 71; a++, pos++)
        m_deInterData[a] = bData[pos];

    for (unsigned int a = 76; a <= 86; a++, pos++)
        m_deInterData[a] = bData[pos];

    for (unsigned int a = 91; a <= 101; a++, pos++)
        m_deInterData[a] = bData[pos];

    for (unsigned int a = 106; a <= 116; a++, pos++)
        m_deInterData[a] = bData[pos];

    for (unsigned int a = 121; a <= 131; a++, pos++)
        m_deInterData[a] = bData[pos];
}

// Check each row with a Hamming (15,11,3) code and each column with a Hamming (13,9,3) code
void DMRBPTC19696::encodeErrorCheck() {
    // Run through each of the 9 rows containing data
    for (unsigned int r = 0; r < 9; r++) {
        unsigned int pos = (r * 15) + 1;
        DMRHamming::encode15113_2(m_deInterData + pos);
    }

    // Run through each of the 15 columns
    bool col[13];
    for (unsigned int c = 0; c < 15; c++) {
        unsigned int pos = c + 1;
        for (unsigned int a = 0; a < 13; a++) {
            col[a] = m_deInterData[pos];
            pos = pos + 15;
        }

        DMRHamming::encode1393(col);

        pos = c + 1;
        for (unsigned int a = 0; a < 13; a++) {
            m_deInterData[pos] = col[a];
            pos = pos + 15;
        }
    }
}

// Interleave the raw data
void DMRBPTC19696::encodeInterleave() {
    for (unsigned int i = 0; i < 196; i++)
        m_rawData[i] = false;

    // The first bit is R(3) which is not used so can be ignored
    for (unsigned int a = 0; a < 196; a++) {
        // Calculate the interleave sequence
        unsigned int interleaveSequence = (a * 181) % 196;
        // Unshuffle the data
        m_rawData[interleaveSequence] = m_deInterData[a];
    }
}

void DMRBPTC19696::encodeExtractBinary(uint8_t* data) {
    // First block
    DMRUtils::bitsToByteBE(m_rawData + 0,  data[0]);
    DMRUtils::bitsToByteBE(m_rawData + 8,  data[1]);
    DMRUtils::bitsToByteBE(m_rawData + 16, data[2]);
    DMRUtils::bitsToByteBE(m_rawData + 24, data[3]);
    DMRUtils::bitsToByteBE(m_rawData + 32, data[4]);
    DMRUtils::bitsToByteBE(m_rawData + 40, data[5]);
    DMRUtils::bitsToByteBE(m_rawData + 48, data[6]);
    DMRUtils::bitsToByteBE(m_rawData + 56, data[7]);
    DMRUtils::bitsToByteBE(m_rawData + 64, data[8]);
    DMRUtils::bitsToByteBE(m_rawData + 72, data[9]);
    DMRUtils::bitsToByteBE(m_rawData + 80, data[10]);
    DMRUtils::bitsToByteBE(m_rawData + 88, data[11]);

    // Handle the two bits
    uint8_t byte;
    DMRUtils::bitsToByteBE(m_rawData + 96, byte);
    data[12] = (data[12] & 0x3F) | ((byte >> 0) & 0xC0);
    data[20] = (data[20] & 0xFC) | ((byte >> 4) & 0x03);

    // Second block
    DMRUtils::bitsToByteBE(m_rawData + 100, data[21]);
    DMRUtils::bitsToByteBE(m_rawData + 108, data[22]);
    DMRUtils::bitsToByteBE(m_rawData + 116, data[23]);
    DMRUtils::bitsToByteBE(m_rawData + 124, data[24]);
    DMRUtils::bitsToByteBE(m_rawData + 132, data[25]);
    DMRUtils::bitsToByteBE(m_rawData + 140, data[26]);
    DMRUtils::bitsToByteBE(m_rawData + 148, data[27]);
    DMRUtils::bitsToByteBE(m_rawData + 156, data[28]);
    DMRUtils::bitsToByteBE(m_rawData + 164, data[29]);
    DMRUtils::bitsToByteBE(m_rawData + 172, data[30]);
    DMRUtils::bitsToByteBE(m_rawData + 180, data[31]);
    DMRUtils::bitsToByteBE(m_rawData + 188, data[32]);
}
