#ifndef CUBECELL_MONITORING_GPIO_H
#define CUBECELL_MONITORING_GPIO_H

#include <cstdint>

class System;

class Gpio {
public:
    explicit Gpio(System *system);

    void printJson() const;

    void setWifi(bool enabled);
    void setNpr(bool enabled);

    uint16_t getLdr() const;

    inline bool isWifiEnabled() const {
        return wifi;
    }

    inline bool isNprEnabled() const {
        return npr;
    }
private:
    System *system;
    bool initialized = false;

    bool wifi = false;
    bool npr = false;

    void setState(uint8_t pin, bool enabled, const char* name, bool &status, bool inverted = false);
    bool getState(uint8_t pin, const char* name);
    uint16_t getAdcState(uint8_t pin, const char* name) const;
};

#endif //CUBECELL_MONITORING_GPIO_H
