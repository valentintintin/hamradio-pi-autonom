#ifndef RP2040_LORA_APRS_SENDSTATUSTHREAD_H
#define RP2040_LORA_APRS_SENDSTATUSTHREAD_H

#include "Threads/SendThread.h"

class System;

class SendStatusThread : public SendThread {
public:
    explicit SendStatusThread(System *system);
protected:
    bool runOnce() override;
};


#endif //RP2040_LORA_APRS_SENDSTATUSTHREAD_H
