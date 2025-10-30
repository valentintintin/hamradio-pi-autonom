#ifndef RP2040_LORA_APRS_ENERGYDUMMYTHREAD_H
#define RP2040_LORA_APRS_ENERGYDUMMYTHREAD_H

#include "Threads/EnergyThread.h"

class EnergyDummyThread : public EnergyThread {
public:
    explicit EnergyDummyThread(System* system);
protected:
    bool init() override;
    bool fetchVoltageBattery() override;
    bool fetchCurrentBattery() override;
    bool fetchVoltageSolar() override;
    bool fetchCurrentSolar() override;
};


#endif //RP2040_LORA_APRS_ENERGYDUMMYTHREAD_H
