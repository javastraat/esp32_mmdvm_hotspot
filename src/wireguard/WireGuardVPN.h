/*
 * WireGuard implementation for ESP32 Arduino by Kenta Ida (fuga@fugafuga.org)
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Renamed to WireGuardVPN to avoid collision with the external WireGuard-ESP32 library
 */
#pragma once
#include <IPAddress.h>

class WireGuardVPN
{
private:
    bool _is_initialized = false;
public:
    bool begin(const IPAddress& localIP, const char* privateKey, const char* remotePeerAddress, const char* remotePeerPublicKey, uint16_t remotePeerPort, const char* dns, const char* allowedIps);
    void end();
    bool is_initialized() const { return this->_is_initialized; }
};
