#ifndef RP2040_LORA_APRS_SENDTELEMETRIESTHREAD_H
#define RP2040_LORA_APRS_SENDTELEMETRIESTHREAD_H

#include "Threads/SendThread.h"

class System;

class SendTelemetriesThread : public SendThread {
public:
    explicit SendTelemetriesThread(System *system);

    long tillRun(unsigned long time) override;
protected:
    bool runOnce() override;
};


#endif //RP2040_LORA_APRS_SENDTELEMETRIESTHREAD_H
