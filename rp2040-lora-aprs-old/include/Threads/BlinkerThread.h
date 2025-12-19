
#ifndef RP2040_LORA_APRS_BLINKERTHREAD_H
#define RP2040_LORA_APRS_BLINKERTHREAD_H

#include "MyThread.h"
#include "GpioPin.h"

class BlinkerThread : public MyThread {
public:
    explicit BlinkerThread(System *system, GpioPin *gpio);
protected:
    bool runOnce() override;
private:
    GpioPin *gpio;
};


#endif //RP2040_LORA_APRS_BLINKERTHREAD_H
