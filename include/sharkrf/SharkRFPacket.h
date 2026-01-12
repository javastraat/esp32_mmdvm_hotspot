/*
 * SharkRF IP Connector Protocol - ESP32 Arduino Adaptation
 * Adapted from SharkRF srf-ip-conn protocol library
 *
 * Copyright (c) 2016 SharkRF OÜ. https://www.sharkrf.com/
 * ESP32 adaptation for MMDVM Hotspot project (C) 2026
 *
 * This is a simplified implementation focused on DMR mode only.
 */

#ifndef SHARKRF_PACKET_H
#define SHARKRF_PACKET_H

#include <stdint.h>

// SharkRF Protocol Constants
#define SRF_MAGIC_STR                   "SRFIPC"
#define SRF_MAGIC_LENGTH                6
#define SRF_TOKEN_LENGTH                8
#define SRF_MAX_PASSWORD_LENGTH         32
#define SRF_MAX_CALLSIGN_LENGTH         10

// Packet Types
#define SRF_PACKET_TYPE_LOGIN           0x00
#define SRF_PACKET_TYPE_TOKEN           0x01
#define SRF_PACKET_TYPE_AUTH            0x02
#define SRF_PACKET_TYPE_ACK             0x03
#define SRF_PACKET_TYPE_NAK             0x04
#define SRF_PACKET_TYPE_CONFIG          0x05
#define SRF_PACKET_TYPE_PING            0x06
#define SRF_PACKET_TYPE_PONG            0x07
#define SRF_PACKET_TYPE_CLOSE           0x08
#define SRF_PACKET_TYPE_DATA_DMR        0x0a

// ACK Result Types
#define SRF_ACK_RESULT_AUTH             0
#define SRF_ACK_RESULT_CONFIG           1
#define SRF_ACK_RESULT_CLOSE            2

// NAK Result Types
#define SRF_NAK_INVALID_CLIENT_ID       0
#define SRF_NAK_INVALID_HMAC            1
#define SRF_NAK_SERVER_FULL             2

// DMR Slot Types
#define SRF_DMR_SLOT_TYPE_VOICE_LC_HEADER       0x01
#define SRF_DMR_SLOT_TYPE_TERMINATOR_WITH_LC    0x02
#define SRF_DMR_SLOT_TYPE_VOICE_DATA_A          0x0a
#define SRF_DMR_SLOT_TYPE_VOICE_DATA_B          0x0b
#define SRF_DMR_SLOT_TYPE_VOICE_DATA_C          0x0c
#define SRF_DMR_SLOT_TYPE_VOICE_DATA_D          0x0d
#define SRF_DMR_SLOT_TYPE_VOICE_DATA_E          0x0e
#define SRF_DMR_SLOT_TYPE_VOICE_DATA_F          0x0f

// Packet header (8 bytes)
struct __attribute__((packed)) SRF_Header {
    char magic[SRF_MAGIC_LENGTH];       // "SRFIPC"
    uint8_t version;                     // 0x00
    uint8_t packet_type;
};

// Login payload (4 bytes)
struct __attribute__((packed)) SRF_Login {
    uint32_t client_id;                  // DMR ID (big-endian)
};

// Token payload (8 bytes)
struct __attribute__((packed)) SRF_Token {
    uint8_t token[SRF_TOKEN_LENGTH];
};

// Auth payload (40 bytes)
struct __attribute__((packed)) SRF_Auth {
    uint8_t random_data[8];
    uint8_t hmac[32];                    // SHA256(token + password + random_data)
};

// ACK payload (41 bytes)
struct __attribute__((packed)) SRF_Ack {
    uint8_t result;
    uint8_t random_data[8];
    uint8_t hmac[32];
};

// NAK payload (41 bytes)
struct __attribute__((packed)) SRF_Nak {
    uint8_t result;
    uint8_t random_data[8];
    uint8_t hmac[32];
};

// Config payload (180 bytes)
struct __attribute__((packed)) SRF_Config {
    char operator_callsign[SRF_MAX_CALLSIGN_LENGTH + 1];
    char hw_manufacturer[17];
    char hw_model[17];
    char hw_version[9];
    char sw_version[9];
    uint32_t rx_freq;                    // Hz (big-endian)
    uint32_t tx_freq;                    // Hz (big-endian)
    uint8_t tx_power;                    // dBm
    float latitude;
    float longitude;
    int16_t height_agl;                  // meters
    char location[33];
    char description[33];
    uint8_t hmac[32];
};

// Ping payload (40 bytes)
struct __attribute__((packed)) SRF_Ping {
    uint8_t random_data[8];
    uint8_t hmac[32];
};

// Pong payload (40 bytes)
struct __attribute__((packed)) SRF_Pong {
    uint8_t random_data[8];
    uint8_t hmac[32];
};

// Close payload (40 bytes)
struct __attribute__((packed)) SRF_Close {
    uint8_t random_data[8];
    uint8_t hmac[32];
};

// DMR Data payload (82 bytes)
struct __attribute__((packed)) SRF_Data_DMR {
    uint32_t seq_no;                     // Sequence number (big-endian)
    uint32_t call_session_id;            // Random 32-bit call ID (big-endian)
    uint8_t dst_id[3];                   // Destination DMR ID (3 bytes, big-endian)
    uint8_t src_id[3];                   // Source DMR ID (3 bytes, big-endian)
    uint8_t flags;                       // Bit 0: tdma_channel, Bit 1: call_type (0=private, 1=group)
                                         // Bits 2-5: color_code, Bits 6-7: reserved
    uint8_t slot_type;                   // DMR slot type
    int8_t rssi_dbm;                     // RSSI in dBm
    uint8_t data[33];                    // Raw 33-byte DMR frame
    uint8_t hmac[32];                    // SHA256(token + password + all fields except hmac)
};

// Generic packet structure (max 274 bytes for DMR data)
struct __attribute__((packed)) SRF_Packet {
    SRF_Header header;
    union {
        SRF_Login login;
        SRF_Token token;
        SRF_Auth auth;
        SRF_Ack ack;
        SRF_Nak nak;
        SRF_Config config;
        SRF_Ping ping;
        SRF_Pong pong;
        SRF_Close close;
        SRF_Data_DMR data_dmr;
        uint8_t raw[266];                // Max payload size
    } payload;
};

#endif
