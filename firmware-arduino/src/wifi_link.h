#pragma once

void wifiBegin();   // connect, waiting at most 20 s
void wifiKeep();    // call from loop(): retries every 5 s without blocking
