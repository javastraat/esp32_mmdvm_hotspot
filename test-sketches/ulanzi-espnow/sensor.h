#pragma once
#include <Arduino.h>
#include <time.h>

void setupRTC();
void setupSHT31();
void loopSHT31();
bool ds1307Read(struct tm& t);
void ds1307Write(const struct tm& t);
void ds1307Stop();
