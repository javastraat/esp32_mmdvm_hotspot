/*
 *   DMR Frame Decoder - Simple API for ESP32 MMDVM Hotspot
 *
 *   This is the main implementation of the DMR decoder wrapper.
 *
 *   Copyright (C) 2026 ESP32 MMDVM Hotspot Project
 */

#include "DMRDecoder.h"
#include "DMRDefines.h"
#include "DMRFullLC.h"
#include <string.h>
#include <Arduino.h>

namespace DMRDecoder {

    // Check if frame has MS data sync pattern at bytes 13-19
    bool isVoiceHeader(const uint8_t* dmrFrame) {
        return (memcmp(&dmrFrame[13], MS_SOURCED_DATA_SYNC, 7) == 0) ||
               (memcmp(&dmrFrame[13], MS_SOURCED_DATA_SYNC_ALT, 7) == 0);
    }

    // Check if frame has MS audio sync pattern at bytes 13-19
    bool isTerminator(const uint8_t* dmrFrame) {
        // For terminator, we'd need to check embedded signaling
        // For now, just return false - terminators are less critical
        return false;
    }

    // Extract LC information from a DMR frame
    LCInfo extractLCInfo(const uint8_t* dmrFrame, bool debug) {
        LCInfo info;
        info.srcId = 0;
        info.dstId = 0;
        info.isGroup = true;
        info.valid = false;

        if (debug) {
            Serial.println("[DMR LC] extractLCInfo() called");
        }

        // Check if this is a voice header frame (contains full LC)
        if (!isVoiceHeader(dmrFrame)) {
            if (debug) {
                Serial.println("[DMR LC] Not a voice header frame");
            }
            return info;  // Not a header frame, no LC data
        }

        if (debug) {
            Serial.println("[DMR LC] Voice header detected, attempting decode...");
        }

        // Decode the full LC from the frame
        DMRFullLC fullLC;
        DMRLC* lc = fullLC.decode(dmrFrame, DT_VOICE_LC_HEADER);

        if (lc == nullptr) {
            if (debug) {
                Serial.println("[DMR LC] LC decode FAILED (RS check or BPTC error)");
            }
            return info;  // LC decode failed
        }

        // Extract the information
        info.srcId = lc->getSrcId();
        info.dstId = lc->getDstId();
        info.isGroup = (lc->getFLCO() == FLCO::GROUP);
        info.valid = true;

        if (debug) {
            Serial.printf("[DMR LC] SUCCESS! Src=%u Dst=%u Group=%s\n",
                          info.srcId, info.dstId, info.isGroup ? "YES" : "NO");
        }

        // Clean up
        delete lc;

        return info;
    }
}
