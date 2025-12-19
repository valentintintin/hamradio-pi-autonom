#ifndef RP2040_LORA_APRS_SENDPOSITIONTHREAD_H
#define RP2040_LORA_APRS_SENDPOSITIONTHREAD_H

#include "Threads/SendThread.h"

class System;

class SendPositionThread : public SendThread {
public:
    explicit SendPositionThread(System *system);
protected:
    bool runOnce() override;
};


#endif //RP2040_LORA_APRS_SENDPOSITIONTHREAD_H
