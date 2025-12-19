#ifndef RP2040_LORA_APRS_ENERGYADCTHREAD_H
#define RP2040_LORA_APRS_ENERGYADCTHREAD_H

#include "Threads/EnergyThread.h"
#include "GpioPin.h"

class EnergyAdcThread : public EnergyThread {
public:
    explicit EnergyAdcThread(System* system, uint16_t *ocv = nullptr, size_t numOcvPoints = 0, uint8_t numCells = 1);
protected:
    bool fetchVoltageBattery() override;
private:
    GpioPin *adc;
};


#endif //RP2040_LORA_APRS_ENERGYADCTHREAD_H
