#ifndef RP2040_LORA_APRS_WATCHDOGTHREAD_H
#define RP2040_LORA_APRS_WATCHDOGTHREAD_H

#include <../../../include/utils/utils.h>

#include "MyThread.h"

class System;

class WatchdogThread : public MyThread {
public:
    explicit WatchdogThread(System *system, unsigned long interval, const char *name, bool enabled = true);
    long tillRun(unsigned long time) override;
    virtual bool feed() = 0;

    inline uint64_t timeSinceFed() const {
        return lastFed > 0 ? millis() - lastFed : 0;
    }

    inline bool isFed() const {
        return isAfterBoot() && timeSinceFed() < interval;
    }
protected:
    uint64_t lastFed = 0;
};


#endif //RP2040_LORA_APRS_WATCHDOGTHREAD_H
