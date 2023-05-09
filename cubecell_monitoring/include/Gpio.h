#ifndef CUBECELL_MONITORING_GPIO_H
#define CUBECELL_MONITORING_GPIO_H

#include <cstdint>

class System;

class Gpio {
public:
    explicit Gpio(System *system);

    void setWifi(bool enabled);
    void setNpr(bool enabled);

    inline bool isWifiEnabled() const {
        return wifi;
    }

    inline bool isNprEnabled() const {
        return npr;
    }
private:
    System *system;
    char buffer[32]{};

    bool wifi = false;
    bool npr = false;

    void setState(uint8_t pin, bool enabled, const char* name, bool &status, bool inverted = false);
};

#endif //CUBECELL_MONITORING_GPIO_H
