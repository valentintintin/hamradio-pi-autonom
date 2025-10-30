#include "Threads/Send/MeshtasticSendAprsThread.h"
#include "System.h"

MeshtasticSendAprsThread::MeshtasticSendAprsThread(System *system) : SendThread(system, system->settings.meshtastic.intervalSendItem, PSTR("SEND_MESHTASTIC_APRS"), system->settings.meshtastic.aprsSendItemEnabled) {}

bool MeshtasticSendAprsThread::runOnce() {
    return system->communication.sendItem(system->settings.meshtastic.itemName,
                                          system->settings.meshtastic.symbol,
                                          system->settings.meshtastic.symbolTable,
                                          system->settings.meshtastic.itemComment,
                                          system->settings.meshtastic.latitude,
                                          system->settings.meshtastic.longitude,
                                          system->settings.meshtastic.altitude,
                                          system->watchdogMeshtastic->isFed() || force);
}

long MeshtasticSendAprsThread::tillRun(const unsigned long time) {
    return system->watchdogMeshtastic->enabled ? SendThread::tillRun(time) : INT_MAX;
}
