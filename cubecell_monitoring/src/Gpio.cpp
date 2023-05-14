
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

uint16_t Gpio::getLdr() {
    return getAdcState(PIN_LDR, PSTR("LDR"));
}

bool Gpio::getState(uint8_t pin, const char* name) {
    pinMode(pin, INPUT);

    bool status = digitalRead(pin);

    Log.infoln("[GPIO] %s (%d) is %d", name, pin, status);
    Log2.infoln("[GPIO] %s:%d", name, status);

    return status;
}

uint16_t Gpio::getAdcState(uint8_t pin, const char *name) {
    pinMode(pin, INPUT);

    uint16_t val = analogRead(pin);

    Log.infoln("[GPIO] %s (%d) is %d", name, pin, val);
    Log2.infoln("[GPIO] %s:%d", name, val);

    return val;
}
