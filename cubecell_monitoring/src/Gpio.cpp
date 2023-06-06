
#include "Gpio.h"
#include "ArduinoLog.h"
#include "Config.h"
#include "System.h"

Gpio::Gpio(System *system) : system(system) {
    setWifi(wifi);
    setNpr(npr);
    initialized = true;
}

void Gpio::setState(uint8_t pin, bool enabled, const char* name, bool &status, bool inverted) {
    pinMode(pin, OUTPUT);

    if (initialized) {
        Log.infoln(F("[GPIO] %s (%d) change to state %d"), name, pin, enabled);

        // fixme bug freeze if display
//        sprintf_P(buffer, PSTR("Pin %s (%d) changed to %d"), name, pin, enabled);
//        system->displayText(PSTR("GPIO"), buffer);
    }

    if (inverted) {
        digitalWrite(pin, !enabled);
        status = enabled;
    } else {
        digitalWrite(pin, enabled);
    }
}

void Gpio::setWifi(bool enabled) {
    setState(PIN_WIFI, enabled, PSTR("WIFI"), wifi);
}

void Gpio::setNpr(bool enabled) {
    setState(PIN_NPR, enabled, PSTR("NPR"), npr);
}

uint16_t Gpio::getLdr() {
    return getAdcState(PIN_LDR, PSTR("LDR"));
}

bool Gpio::getState(uint8_t pin, const char* name) {
    pinMode(pin, INPUT);

    bool enabled = digitalRead(pin);

    if (initialized) {
        Log.infoln(F("[GPIO] %s (%d) is %d"), name, pin, enabled);
    }

    return enabled;
}

uint16_t Gpio::getAdcState(uint8_t pin, const char *name) {
    pinMode(pin, INPUT);

    uint16_t val = analogRead(pin);

    if (initialized) {
        Log.infoln(F("[GPIO] %s (%d) is %d"), name, pin, val);
    }

    return val;
}

void Gpio::printJson() {
    serialJsonWriter
            .beginObject()
            .property(F("type"), PSTR("gpio"))
            .property(F("wifi"), isWifiEnabled() ? PSTR("true") : PSTR("false"))
            .property(F("npr"), isNprEnabled() ? PSTR("true") : PSTR("false"))
            .property(F("ldr"), getLdr())
            .endObject(); SerialPiUsed.println();
}
