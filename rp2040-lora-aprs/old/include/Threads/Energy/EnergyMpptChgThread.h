#ifndef RP2040_LORA_APRS_ENERGYMPPTCHGTHREAD_H
#define RP2040_LORA_APRS_ENERGYMPPTCHGTHREAD_H

#include "Threads/EnergyThread.h"

class EnergyMpptChgThread : public EnergyThread {
public:
    explicit EnergyMpptChgThread(System *system, uint16_t *ocv = nullptr, size_t numOcvPoints = 0);
    void run() override;

    inline bool isNight() const override {
        return _isNight;
    }

    inline bool isAlert() const {
        return _isAlert;
    }

    inline double getTemperature() const {
        return temperature / 10.0;
    }
protected:
    bool init() override;
    bool fetchVoltageBattery() override;
    bool fetchCurrentBattery() override;
    bool fetchVoltageSolar() override;
    bool fetchCurrentSolar() override;

private:
    bool fetchOthersData();
    bool setPowerOnOff() const;

    bool _isNight = false;
    bool _isAlert = false;
    int16_t temperature = 0;
    bool wasAlert = false;
    mpptChg *charger;
};

#endif //RP2040_LORA_APRS_ENERGYMPPTCHGTHREAD_H
