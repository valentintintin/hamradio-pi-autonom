#ifndef RP2040_LORA_APRS_SENDTHREAD_H
#define RP2040_LORA_APRS_SENDTHREAD_H

#include "MyThread.h"

class System;

class SendThread : public MyThread {
public:
    explicit SendThread(System *system, unsigned long interval, const char *name, bool enabled);

    long tillRun(unsigned long time) override;
protected:
    virtual bool runOnce() override = 0;
};

#endif //RP2040_LORA_APRS_SENDTHREAD_H
