/*
 * SharkRF IP Connector Protocol Handler for ESP32 MMDVM Hotspot
 *
 * Copyright (C) 2026 ESP32 MMDVM Hotspot Project
 * Based on SharkRF IP Connector Protocol by SharkRF OÜ
 */

#ifndef SHARKRF_PROTOCOL_H
#define SHARKRF_PROTOCOL_H

#include "SharkRFPacket.h"
#include <Arduino.h>

// SharkRF connection states
enum class SRF_State : uint8_t {
    DISCONNECTED,
    WAITING_TOKEN,
    WAITING_ACK,
    CONNECTED,
    ERROR
};

class SharkRFProtocol {
public:
    SharkRFProtocol();

    // Initialize packet header with magic and type
    static void initPacketHeader(SRF_Header* header, uint8_t packet_type);

    // Build login packet
    static size_t buildLoginPacket(uint8_t* buffer, uint32_t client_id);

    // Build auth packet with HMAC
    static size_t buildAuthPacket(uint8_t* buffer, const uint8_t* token,
                                  const char* password);

    // Build config packet
    static size_t buildConfigPacket(uint8_t* buffer, const uint8_t* token,
                                    const char* password, const char* callsign,
                                    const char* hw_manufacturer, const char* hw_model,
                                    const char* sw_version, uint32_t rx_freq,
                                    uint32_t tx_freq, uint8_t tx_power,
                                    const char* location, const char* description);

    // Build ping packet
    static size_t buildPingPacket(uint8_t* buffer, const uint8_t* token,
                                  const char* password);

    // Build DMR data packet
    static size_t buildDMRDataPacket(uint8_t* buffer, const uint8_t* token,
                                     const char* password, uint32_t seq_no,
                                     uint32_t call_session_id, uint32_t dst_id,
                                     uint32_t src_id, bool is_group_call,
                                     uint8_t tdma_channel, uint8_t color_code,
                                     uint8_t slot_type, int8_t rssi_dbm,
                                     const uint8_t* dmr_frame);

    // Parse received packets
    static bool parsePacket(const uint8_t* buffer, size_t length, SRF_Packet* packet);

    // Validate packet header
    static bool isHeaderValid(const SRF_Header* header);

    // Verify HMAC on received packet
    static bool verifyHMAC(const uint8_t* token, const char* password,
                          const uint8_t* packet_data, size_t data_length,
                          const uint8_t* received_hmac);

    // Extract DMR frame from SharkRF DMR packet
    static bool extractDMRFrame(const SRF_Data_DMR* dmr_packet, uint8_t* dmr_frame,
                                uint32_t* src_id, uint32_t* dst_id, bool* is_group,
                                uint8_t* slot);

private:
    // Calculate HMAC-SHA256
    static void calculateHMAC(const uint8_t* token, const char* password,
                             const uint8_t* data, size_t data_length,
                             uint8_t* hmac_output);

    // Generate 8 random bytes
    static void generateRandomData(uint8_t* random_data);

    // Convert 32-bit value to big-endian (renamed to avoid conflict with system macros)
    static uint32_t toBigEndian32(uint32_t value);

    // Convert big-endian to 32-bit value
    static uint32_t fromBigEndian32(uint32_t value);

    // Convert 3-byte big-endian to 32-bit
    static uint32_t read24bit(const uint8_t* data);

    // Convert 32-bit to 3-byte big-endian
    static void write24bit(uint8_t* data, uint32_t value);
};

#endif
