#ifndef RP2040_LORA_APRS_MESHTASTICSENDAPRSTHREAD_H
#define RP2040_LORA_APRS_MESHTASTICSENDAPRSTHREAD_H

#include "Threads/SendThread.h"

class System;

class MeshtasticSendAprsThread : public SendThread {
public:
    explicit MeshtasticSendAprsThread(System *system);
    long tillRun(unsigned long time) override;
protected:
    bool runOnce() override;
};


#endif //RP2040_LORA_APRS_MESHTASTICSENDAPRSTHREAD_H
