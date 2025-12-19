#ifndef RP2040_LORA_APRS_LINUXSENDAPRSTHREAD_H
#define RP2040_LORA_APRS_LINUXSENDAPRSTHREAD_H

#include "Threads/SendThread.h"

class System;

class LinuxSendAprsThread : public SendThread {
public:
    explicit LinuxSendAprsThread(System *system);
    long tillRun(unsigned long time) override;
protected:
    bool runOnce() override;
};


#endif //RP2040_LORA_APRS_LINUXSENDAPRSTHREAD_H
