/*
 * system_wireguard.h - WireGuard VPN Client Task
 */

#ifndef SYSTEM_WIREGUARD_H
#define SYSTEM_WIREGUARD_H

#include <Arduino.h>
#include "../src/wireguard/WireGuardVPN.h"

// WireGuard status
extern bool wireguardConnected;
extern TaskHandle_t wireguardTaskHandle;

// Initialize WireGuard task
void initWireguardTask();

// WireGuard task function
void wireguardTask(void *parameter);

#endif // SYSTEM_WIREGUARD_H
