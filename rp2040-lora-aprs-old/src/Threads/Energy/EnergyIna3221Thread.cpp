#include "Threads/Energy/EnergyIna3221Thread.h"
#include "System.h"

EnergyIna3221Thread::EnergyIna3221Thread(System *system, uint16_t *ocv, const size_t numOcvPoints, const uint8_t numCells) :
EnergyThread(system, PSTR("ENERGY_INA3221"), ocv, numOcvPoints, numCells),
channelBattery(system->settings.energy.inaChannelBattery), channelSolar(system->settings.energy.inaChannelSolar) {}

bool EnergyIna3221Thread::init() {
    ina3221.begin();
    ina3221.setShuntRes(100, 100, 100); // 0.1 Ohm shunt resistors

    return ina3221.getDieID() == 0x3220;
}

bool EnergyIna3221Thread::fetchVoltageBattery() {
    vb = ina3221.getVoltage(channelBattery) * 1000;
    return true;
}

bool EnergyIna3221Thread::fetchCurrentBattery() {
    vb = ina3221.getCurrent(channelBattery) * 1000;
    return true;
}

bool EnergyIna3221Thread::fetchVoltageSolar() {
    vb = ina3221.getVoltage(channelSolar) * 1000;
    return true;
}

bool EnergyIna3221Thread::fetchCurrentSolar() {
    vb = ina3221.getCurrent(channelSolar) * 1000;
    return true;
}