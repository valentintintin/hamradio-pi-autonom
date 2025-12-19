#include "Threads/Energy/EnergyDummyThread.h"

EnergyDummyThread::EnergyDummyThread(System *system) : EnergyThread(system, PSTR("ENERGY_DUMMY"), new uint16_t[2] { 13000, 11000 }, 2) {}

bool EnergyDummyThread::init() {
    // Init a random ?
    return true;
}

bool EnergyDummyThread::fetchVoltageBattery() {
    vb = 12345;
    return true;
}

bool EnergyDummyThread::fetchCurrentBattery() {
    ib = 123;
    return true;
}

bool EnergyDummyThread::fetchVoltageSolar() {
    vs = 23456;
    return true;
}

bool EnergyDummyThread::fetchCurrentSolar() {
    is = 345;
    return true;
}