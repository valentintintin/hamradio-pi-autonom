
#include "Gpio.h"
#include "ArduinoLog.h"
#include "Config.h"
#include "System.h"

Gpio::Gpio(System *system) : system(system) {
    setWifi(wifi);
    setNpr(npr);
}

void Gpio::setState(uint8_t pin, bool enabled, const char* name, bool &status, bool inverted) {
    Log.infoln("[GPIO] %s (%d) change to state %d", name, pin, enabled);
    Log2.infoln("[GPIO] %s:%d", name, enabled);

    pinMode(pin, OUTPUT);

    sprintf_P(buffer, PSTR("Pin %s (%d) changed to %d"), name, pin, enabled);
    system->displayText(PSTR("GPIO"), buffer);

    if (inverted) {
        digitalWrite(pin, !enabled);
    } else {
        digitalWrite(pin, enabled);
    }

    status = enabled;
}

void Gpio::setWifi(bool enabled) {
    setState(PIN_WIFI, enabled, PSTR("WIFI"), wifi, true);
}

void Gpio::setNpr(bool enabled) {
    setState(PIN_NPR, enabled, PSTR("NPR"), npr, true);
}
