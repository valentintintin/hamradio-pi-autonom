#ifndef RP2040_LORA_APRS_WATCHDOGMASTERPINTHREAD_H
#define RP2040_LORA_APRS_WATCHDOGMASTERPINTHREAD_H

#include "Threads/WatchdogThread.h"
#include "GpioPin.h"
#include "Timer.h"

class WatchdogMasterPinThread : public WatchdogThread {
public:
    explicit WatchdogMasterPinThread(System *system, const char* name, GpioPin *gpio, unsigned long intervalCheck, bool enabled);
    void sleep(uint64_t millis);
    bool feed() override;

    inline bool isGpioOn() const {
        return gpio->getState();
    }
protected:
    bool init() override;
    bool runOnce() override;
private:
    GpioPin *gpio;
    Timer timerSleep;
    bool wantToSleep = false;
};


#endif //RP2040_LORA_APRS_WATCHDOGMASTERPINTHREAD_H
