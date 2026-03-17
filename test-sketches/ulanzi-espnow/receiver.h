#pragma once
#include "globals.h"

void setupReceiver();

#if RECV_POCSAG
void processPocsagPacket(const EspNowPocsagPacket& pkt);
#endif
