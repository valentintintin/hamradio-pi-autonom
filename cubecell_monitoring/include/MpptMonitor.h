#ifndef CUBECELL_MONITORING_MPPTMONITOR_H
#define CUBECELL_MONITORING_MPPTMONITOR_H

#include "Timer.h"
#include "mpptChg.h"
#include "Config.h"
#include "Communication.h"

class MpptMonitor {
public:
    explicit MpptMonitor(System *system, TwoWire &wire);

    bool begin();
    bool update();
    bool setWatchdog(uint32_t powerOffTime);

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

    inline int getCurrentCharge() const {
        return is - ib;
    }

    inline uint16_t getStatus() const {
        return status;
    }

    inline bool isNight() const {
        return night;
    }

    inline bool isAlert() const {
        return alert;
    }

    inline bool isPowerEnabled() const {
        return powerEnabled;
    }

    inline bool isWatchdogEnabled() const {
        return watchdogEnabled;
    }

    inline uint8_t getWatchdogCounter() const {
        return watchdogCounter;
    }

    inline uint16_t getWatchdogPowerOffTime() const {
        return watchdogPowerOffTime;
    }
private:
    System *system;
    TwoWire &wire;
    mpptChg charger;
    Timer timer = Timer(INTERVAL_MPPT, true);

    char bufferText[256]{};

    bool init = false, night = false, alert = false, watchdogEnabled = false, powerEnabled = false;
    uint8_t watchdogCounter = 0;
    uint16_t status = 0, watchdogPowerOffTime = 0;
    int16_t vs = 0, is = 0, vb = 0, ib = 0;

    void updateWatchdog();
    void checkAnormalCase();
};

#endif //CUBECELL_MONITORING_MPPTMONITOR_H