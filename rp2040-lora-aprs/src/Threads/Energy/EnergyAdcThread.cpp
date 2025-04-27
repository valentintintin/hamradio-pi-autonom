#include "Threads/Energy/EnergyAdcThread.h"
#include "ArduinoLog.h"
#include "System.h"

EnergyAdcThread::EnergyAdcThread(System *system, uint16_t *ocv, const size_t numOcvPoints, const uint8_t numCells) : EnergyThread(system, PSTR("ENERGY_ADC"), ocv, numOcvPoints, numCells),
adc(new GpioPin(PSTR("EnergyADC"), system->settings.energy.adcPin, INPUT, true)) {
}

bool EnergyAdcThread::fetchVoltageBattery() {
    uint32_t raw = 0;

    for (uint32_t i = 0; i < ENERGY_ADC_BATTERY_SENSE_SAMPLES; i++) {
        const uint16_t value = adc->getValue();
        Log.verboseln(F("[ENERGY_ADC] Sample %d : %d"), i, value);
        raw += value;
    }

    vb = raw / ENERGY_ADC_BATTERY_SENSE_SAMPLES;
//   vb = ENERGY_ADC_MULTIPLIER * ((1000 * AREF_VOLTAGE) / pow(2, ENERGY_ADC_BATTERY_SENSE_RESOLUTION_BITS)) * raw

    return true;
}