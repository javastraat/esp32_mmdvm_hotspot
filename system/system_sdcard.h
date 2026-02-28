/*
 * SD Card System Module
 * Handles SD card initialization, mounting, and database management
 */

#ifndef SYSTEM_SDCARD_H
#define SYSTEM_SDCARD_H

#include <SD.h>
#include <SPI.h>
#include "../include/config.h"

// Custom SPI instance for SD Card
extern SPIClass sdSPI;

// SD Card mutex for thread-safe access
extern SemaphoreHandle_t sdCardMutex;

// SD Card status
extern bool sdCardMounted;
extern uint64_t sdCardSize;
extern uint64_t sdCardUsed;

// Task handle
extern TaskHandle_t sdCardTaskHandle;

// Function to create and start SD Card task
void initSdCardTask();

// SD Card task function
void sdCardTask(void *parameter);

// Helper functions
bool mountSDCard();
void unmountSDCard();
bool createDatabaseDirectory();
bool checkDatabaseFiles();
void printSDCardInfo();

#endif // SYSTEM_SDCARD_H
