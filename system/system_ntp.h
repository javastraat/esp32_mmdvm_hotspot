/*
 * system_ntp.h - NTP Time Synchronization
 */

#ifndef SYSTEM_NTP_H
#define SYSTEM_NTP_H

#include <Arduino.h>
#include <time.h>
#include "../include/config.h"

// NTP status
extern volatile bool ntpSynced;
extern time_t lastSyncTime;

// Initialize NTP time synchronization
void initNtpTask();

// Get current time as formatted string
String getCurrentTime();
String getCurrentDate();
String getCurrentDateTime();

// Get Unix timestamp
time_t getUnixTime();

// Check if time is synced
bool isTimeSynced();

#endif
