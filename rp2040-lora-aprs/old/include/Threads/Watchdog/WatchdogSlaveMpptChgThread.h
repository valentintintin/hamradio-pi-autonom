#ifndef RP2040_LORA_APRS_WATCHDOGSLAVEMPPTCHGTHREAD_H
#define RP2040_LORA_APRS_WATCHDOGSLAVEMPPTCHGTHREAD_H

#include "Threads/WatchdogThread.h"
#include "mpptChg.h"

class WatchdogSlaveMpptChgThread : public WatchdogThread {
public:
    explicit WatchdogSlaveMpptChgThread(System *system);
    bool feed() override;
    bool setManagedByUser(uint64_t millis);
protected:
    bool init() override;
    bool runOnce() override;
private:
    mpptChg *charger;
    bool managedByUser = false;
};


#endif //RP2040_LORA_APRS_WATCHDOGSLAVEMPPTCHGTHREAD_H
