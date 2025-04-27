#include "GpioPin.h"
#include "ArduinoLog.h"

GpioPin::GpioPin(const char* name, const pin_size_t pin, const PinMode mode, const bool isAdc, const bool inverted, const bool state) : pin(pin), mode(mode),
    inverted(inverted), isAdc(isAdc) {
    strncpy(this->name, name, PIN_NAME);

    pinMode(pin, mode);

    if (isOutput()) {
        setState(state);
    }
}

void GpioPin::setState(const bool state) {
    assert(isOutput());

    if (pin != LED_BUILTIN) {
        Log.infoln(F("[GPIO_%s_%d] Set %T"), name, pin, state);
    }

    digitalWrite(pin, inverted == !state);

    currentState = state;
}

bool GpioPin::getState() const {
    if (isOutput()) {
        return currentState;
    }

    assert(!isAdc);

    bool result = digitalRead(pin);

    if (inverted) {
        result = !result;
    }

    Log.traceln(F("[GPIO_%s_%d] Get currentState %T"), name, pin, result);

    return result;
}

uint16_t GpioPin::getValue() const {
    assert(!isOutput());
    assert(isAdc);

    const uint16_t value = analogRead(pin);

    Log.traceln(F("[GPIO_%s_%d] Get value %d"), name, pin, value);

    return value;
}
