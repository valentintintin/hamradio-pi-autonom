#ifndef RP2040_LORA_APRS_ENERGYINA3221THREAD_H
#define RP2040_LORA_APRS_ENERGYINA3221THREAD_H

#include "Threads/EnergyThread.h"
#include "INA3221.h"

class EnergyIna3221Thread : public EnergyThread {
public:
    explicit EnergyIna3221Thread(System *system, uint16_t *ocv = nullptr, size_t numOcvPoints = 0, uint8_t numCells = 1);

    ina3221_ch_t channelBattery;
    ina3221_ch_t channelSolar;
protected:
    bool init() override;
    bool fetchVoltageBattery() override;
    bool fetchCurrentBattery() override;
    bool fetchVoltageSolar() override;
    bool fetchCurrentSolar() override;
private:
    INA3221 ina3221 = INA3221(INA3221_ADDR42_SDA);
};


#endif //RP2040_LORA_APRS_ENERGYINA3221THREAD_H
