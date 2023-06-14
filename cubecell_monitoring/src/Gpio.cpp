
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

    if (inverted) {
        digitalWrite(pin, !enabled);
    } else {
        digitalWrite(pin, enabled);
    }

    status = enabled;

    if (initialized) {
        Log.infoln(F("[GPIO] %s (%d) change to state %d"), name, pin, status);

        printJson();

        // fixme bug freeze if display
//        sprintf_P(buffer, PSTR("Pin %s (%d) changed to %d"), name, pin, status);
//        system->displayText(PSTR("GPIO"), buffer);
    }
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
    return digitalRead(pin);
}

uint16_t Gpio::getAdcState(uint8_t pin, const char *name) const {
    pinMode(pin, INPUT);

    return analogRead(pin);
}

void Gpio::printJson() {
    Log.infoln(F("[GPIO] Wifi: %d NPR: %d Box LDR: %d"), isWifiEnabled(), isNprEnabled(), getLdr());

    serialJsonWriter
            .beginObject()
            .property(F("type"), PSTR("gpio"))
            .property(F("wifi"), isWifiEnabled())
            .property(F("npr"), isNprEnabled())
            .property(F("ldr"), getLdr())
            .endObject(); SerialPiUsed.println();
}
