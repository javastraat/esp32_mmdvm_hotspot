/*
 * SharkRF IP Connector Protocol Handler Implementation
 *
 * Copyright (C) 2026 ESP32 MMDVM Hotspot Project
 */

#include "SharkRFProtocol.h"
#include "mbedtls/md.h"
#include <string.h>

SharkRFProtocol::SharkRFProtocol() {
}

// Initialize packet header
void SharkRFProtocol::initPacketHeader(SRF_Header* header, uint8_t packet_type) {
    memcpy(header->magic, SRF_MAGIC_STR, SRF_MAGIC_LENGTH);
    header->version = 0x00;
    header->packet_type = packet_type;
}

// Validate packet header
bool SharkRFProtocol::isHeaderValid(const SRF_Header* header) {
    return (memcmp(header->magic, SRF_MAGIC_STR, SRF_MAGIC_LENGTH) == 0) &&
           (header->version == 0x00);
}

// Build login packet
size_t SharkRFProtocol::buildLoginPacket(uint8_t* buffer, uint32_t client_id) {
    SRF_Packet* packet = (SRF_Packet*)buffer;
    initPacketHeader(&packet->header, SRF_PACKET_TYPE_LOGIN);
    packet->payload.login.client_id = toBigEndian32(client_id);
    return sizeof(SRF_Header) + sizeof(SRF_Login);
}

// Generate random data
void SharkRFProtocol::generateRandomData(uint8_t* random_data) {
    for (int i = 0; i < 8; i++) {
        random_data[i] = random(0, 256);
    }
}

// Calculate HMAC-SHA256
void SharkRFProtocol::calculateHMAC(const uint8_t* token, const char* password,
                                    const uint8_t* data, size_t data_length,
                                    uint8_t* hmac_output) {
    // Build input: token + password + data
    size_t pass_len = strlen(password);
    size_t input_len = SRF_TOKEN_LENGTH + pass_len + data_length;
    uint8_t* input = new uint8_t[input_len];

    memcpy(input, token, SRF_TOKEN_LENGTH);
    memcpy(input + SRF_TOKEN_LENGTH, password, pass_len);
    memcpy(input + SRF_TOKEN_LENGTH + pass_len, data, data_length);

    // Calculate SHA256
    mbedtls_md_context_t ctx;
    mbedtls_md_init(&ctx);
    mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 0);
    mbedtls_md_starts(&ctx);
    mbedtls_md_update(&ctx, input, input_len);
    mbedtls_md_finish(&ctx, hmac_output);
    mbedtls_md_free(&ctx);

    delete[] input;
}

// Verify HMAC
bool SharkRFProtocol::verifyHMAC(const uint8_t* token, const char* password,
                                 const uint8_t* packet_data, size_t data_length,
                                 const uint8_t* received_hmac) {
    uint8_t calculated_hmac[32];
    calculateHMAC(token, password, packet_data, data_length, calculated_hmac);
    return (memcmp(calculated_hmac, received_hmac, 32) == 0);
}

// Build auth packet
size_t SharkRFProtocol::buildAuthPacket(uint8_t* buffer, const uint8_t* token,
                                        const char* password) {
    SRF_Packet* packet = (SRF_Packet*)buffer;
    initPacketHeader(&packet->header, SRF_PACKET_TYPE_AUTH);

    // Generate random data
    generateRandomData(packet->payload.auth.random_data);

    // Calculate HMAC
    calculateHMAC(token, password, packet->payload.auth.random_data, 8,
                  packet->payload.auth.hmac);

    return sizeof(SRF_Header) + sizeof(SRF_Auth);
}

// Build config packet
size_t SharkRFProtocol::buildConfigPacket(uint8_t* buffer, const uint8_t* token,
                                          const char* password, const char* callsign,
                                          const char* hw_manufacturer, const char* hw_model,
                                          const char* sw_version, uint32_t rx_freq,
                                          uint32_t tx_freq, uint8_t tx_power,
                                          const char* location, const char* description) {
    SRF_Packet* packet = (SRF_Packet*)buffer;
    initPacketHeader(&packet->header, SRF_PACKET_TYPE_CONFIG);

    // Fill config structure
    memset(&packet->payload.config, 0, sizeof(SRF_Config));
    strncpy(packet->payload.config.operator_callsign, callsign, SRF_MAX_CALLSIGN_LENGTH);
    strncpy(packet->payload.config.hw_manufacturer, hw_manufacturer, 16);
    strncpy(packet->payload.config.hw_model, hw_model, 16);
    strncpy(packet->payload.config.hw_version, "1.0", 8);
    strncpy(packet->payload.config.sw_version, sw_version, 8);
    packet->payload.config.rx_freq = toBigEndian32(rx_freq);
    packet->payload.config.tx_freq = toBigEndian32(tx_freq);
    packet->payload.config.tx_power = tx_power;
    packet->payload.config.latitude = 0.0;
    packet->payload.config.longitude = 0.0;
    packet->payload.config.height_agl = 0;
    strncpy(packet->payload.config.location, location, 32);
    strncpy(packet->payload.config.description, description, 32);

    // Calculate HMAC (exclude HMAC field itself)
    calculateHMAC(token, password, (uint8_t*)&packet->payload.config,
                  sizeof(SRF_Config) - 32, packet->payload.config.hmac);

    return sizeof(SRF_Header) + sizeof(SRF_Config);
}

// Build ping packet
size_t SharkRFProtocol::buildPingPacket(uint8_t* buffer, const uint8_t* token,
                                        const char* password) {
    SRF_Packet* packet = (SRF_Packet*)buffer;
    initPacketHeader(&packet->header, SRF_PACKET_TYPE_PING);

    // Generate random data
    generateRandomData(packet->payload.ping.random_data);

    // Calculate HMAC
    calculateHMAC(token, password, packet->payload.ping.random_data, 8,
                  packet->payload.ping.hmac);

    return sizeof(SRF_Header) + sizeof(SRF_Ping);
}

// Build DMR data packet
size_t SharkRFProtocol::buildDMRDataPacket(uint8_t* buffer, const uint8_t* token,
                                           const char* password, uint32_t seq_no,
                                           uint32_t call_session_id, uint32_t dst_id,
                                           uint32_t src_id, bool is_group_call,
                                           uint8_t tdma_channel, uint8_t color_code,
                                           uint8_t slot_type, int8_t rssi_dbm,
                                           const uint8_t* dmr_frame) {
    SRF_Packet* packet = (SRF_Packet*)buffer;
    initPacketHeader(&packet->header, SRF_PACKET_TYPE_DATA_DMR);

    // Fill DMR data structure
    packet->payload.data_dmr.seq_no = toBigEndian32(seq_no);
    packet->payload.data_dmr.call_session_id = toBigEndian32(call_session_id);

    // Write 3-byte IDs (big-endian)
    write24bit(packet->payload.data_dmr.dst_id, dst_id);
    write24bit(packet->payload.data_dmr.src_id, src_id);

    // Flags: bit 0 = tdma_channel, bit 1 = call_type, bits 2-5 = color_code
    packet->payload.data_dmr.flags = (tdma_channel & 0x01) |
                                     ((is_group_call ? 1 : 0) << 1) |
                                     ((color_code & 0x0F) << 2);

    packet->payload.data_dmr.slot_type = slot_type;
    packet->payload.data_dmr.rssi_dbm = rssi_dbm;

    // Copy 33-byte DMR frame
    memcpy(packet->payload.data_dmr.data, dmr_frame, 33);

    // Calculate HMAC (exclude HMAC field itself)
    calculateHMAC(token, password, (uint8_t*)&packet->payload.data_dmr,
                  sizeof(SRF_Data_DMR) - 32, packet->payload.data_dmr.hmac);

    return sizeof(SRF_Header) + sizeof(SRF_Data_DMR);
}

// Parse received packet
bool SharkRFProtocol::parsePacket(const uint8_t* buffer, size_t length, SRF_Packet* packet) {
    if (length < sizeof(SRF_Header)) {
        return false;
    }

    memcpy(packet, buffer, min(length, sizeof(SRF_Packet)));
    return isHeaderValid(&packet->header);
}

// Extract DMR frame from SharkRF packet
bool SharkRFProtocol::extractDMRFrame(const SRF_Data_DMR* dmr_packet, uint8_t* dmr_frame,
                                      uint32_t* src_id, uint32_t* dst_id, bool* is_group,
                                      uint8_t* slot) {
    // Copy DMR frame
    memcpy(dmr_frame, dmr_packet->data, 33);

    // Extract IDs
    *src_id = read24bit(dmr_packet->src_id);
    *dst_id = read24bit(dmr_packet->dst_id);

    // Extract flags
    *slot = (dmr_packet->flags & 0x01) + 1;  // Convert 0/1 to 1/2
    *is_group = (dmr_packet->flags & 0x02) >> 1;

    return true;
}

// Byte order conversion functions
uint32_t SharkRFProtocol::toBigEndian32(uint32_t value) {
    return ((value & 0xFF000000) >> 24) |
           ((value & 0x00FF0000) >> 8)  |
           ((value & 0x0000FF00) << 8)  |
           ((value & 0x000000FF) << 24);
}

uint32_t SharkRFProtocol::fromBigEndian32(uint32_t value) {
    return toBigEndian32(value);  // Same operation
}

uint32_t SharkRFProtocol::read24bit(const uint8_t* data) {
    return ((uint32_t)data[0] << 16) | ((uint32_t)data[1] << 8) | data[2];
}

void SharkRFProtocol::write24bit(uint8_t* data, uint32_t value) {
    data[0] = (value >> 16) & 0xFF;
    data[1] = (value >> 8) & 0xFF;
    data[2] = value & 0xFF;
}
