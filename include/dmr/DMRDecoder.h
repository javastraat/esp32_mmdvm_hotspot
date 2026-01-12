/*
 *   DMR Frame Decoder - Simple API for ESP32 MMDVM Hotspot
 *
 *   This is the main interface for decoding DMR frames. The main .ino file
 *   should only need to include this header and call the simple functions below.
 *
 *   Copyright (C) 2026 ESP32 MMDVM Hotspot Project
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 */

#ifndef DMRDecoder_H
#define DMRDecoder_H

#include <stdint.h>

namespace DMRDecoder {

    // Structure to hold decoded LC information
    struct LCInfo {
        uint32_t srcId;     // Source DMR ID
        uint32_t dstId;     // Destination ID (Talk Group or private call ID)
        bool isGroup;       // true = group call (TG), false = private call
        bool valid;         // true if LC was successfully decoded
    };

    /**
     * Extract destination TG/ID from a 33-byte DMR frame
     *
     * @param dmrFrame Pointer to 33-byte DMR frame data
     * @param debug Enable debug logging (default: false)
     * @return LCInfo structure with decoded information
     *
     * Usage example:
     *   DMRDecoder::LCInfo info = DMRDecoder::extractLCInfo(dmrFrame);
     *   if (info.valid) {
     *       Serial.printf("Src: %u, Dst: %u, Group: %s\n",
     *                     info.srcId, info.dstId, info.isGroup ? "YES" : "NO");
     *   }
     */
    LCInfo extractLCInfo(const uint8_t* dmrFrame, bool debug = false);

    /**
     * Check if a DMR frame is a voice header (data sync) frame
     * These frames contain the full Link Control information
     *
     * @param dmrFrame Pointer to 33-byte DMR frame data
     * @return true if frame is a voice LC header
     */
    bool isVoiceHeader(const uint8_t* dmrFrame);

    /**
     * Check if a DMR frame is a terminator with LC frame
     *
     * @param dmrFrame Pointer to 33-byte DMR frame data
     * @return true if frame is a terminator with LC
     */
    bool isTerminator(const uint8_t* dmrFrame);
}

#endif
