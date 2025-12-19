#include "Threads/SendThread.h"

#include <utils.h>

#include "System.h"
#include "ArduinoLog.h"

SendThread::SendThread(System *system, const unsigned long interval, const char *name, const bool enabled) : MyThread(system, interval, name, false, enabled) {
}

long SendThread::tillRun(const unsigned long time) {
    return isAfterBoot() ? MyThread::tillRun(time) : TIME_AFTER_BOOT - static_cast<long>(millis());
}