#ifndef RP2040_LORA_APRS_WATCHDOGSLAVELORATXTHREAD_H
#define RP2040_LORA_APRS_WATCHDOGSLAVELORATXTHREAD_H

#include "Threads/WatchdogThread.h"

class WatchdogSlaveLoraTxThread : public WatchdogThread {
public:
    explicit WatchdogSlaveLoraTxThread(System *system);
    bool feed() override;
protected:
    bool runOnce() override;
};


#endif //RP2040_LORA_APRS_WATCHDOGSLAVELORATXTHREAD_H
