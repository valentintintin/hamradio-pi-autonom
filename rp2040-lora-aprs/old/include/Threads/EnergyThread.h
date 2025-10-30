#ifndef MONITORING_ENERGY_H
#define MONITORING_ENERGY_H

#include "MyThread.h"
#include "mpptChg.h"
#include "../Communication.h"
#include "config.h"

class EnergyThread : public MyThread {
public:
    explicit EnergyThread(System *system, const char *name, uint16_t *ocv = nullptr, size_t numOcvPoints = 0, uint8_t numCells = 1);

    uint8_t getBatteryPercentage() const;

    inline int16_t getVoltageBattery() const {
        return vb;
    }

    inline int16_t getVoltageSolar() const {
        return vs;
    }

    inline int16_t getCurrentBattery() const {
        return ib;
    }

    inline int16_t getCurrentSolar() const {
        return is;
    }

    inline int16_t getCurrentCharge() const {
        return is - ib;
    }

    virtual inline bool isNight() const {
        return vs < 2000;
    }
protected:
    bool runOnce() override;
    virtual bool fetchVoltageBattery() = 0;
    virtual bool fetchCurrentBattery() {
        return true;
    }
    virtual bool fetchVoltageSolar() {
        return true;
    }
    virtual bool fetchCurrentSolar() {
        return true;
    }

    int16_t vs = 0, is = 0, vb = 0, ib = 0;
private:
    uint8_t numCells = 1;
    uint16_t *ocv;
    size_t numOcvPoints = 0;
};

#endif //MONITORING_ENERGY_H
