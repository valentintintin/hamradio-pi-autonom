#include "Threads/WatchdogThread.h"

#include <config.h>
#include <utils.h>

WatchdogThread::WatchdogThread(System *system, const unsigned long interval, const char *name, const bool enabled) : MyThread(system, interval, name, false, enabled) {
}

long WatchdogThread::tillRun(const unsigned long time) {
    return isAfterBoot() ? MyThread::tillRun(time) : TIME_AFTER_BOOT - static_cast<long>(millis());
}