#include "MyThread.h"
#include "ArduinoLog.h"

MyThread::MyThread(System *system, const unsigned long interval, const char *name, const bool noLog, const bool enabled) : system(system), noLog(noLog) {
    this->enabled = enabled;
    Thread::setInterval(interval);
    ThreadName = name;
}

bool MyThread::shouldRun(const unsigned long time) {
    return Thread::shouldRun(time) || force;
}

bool MyThread::begin() {
    if (!enabled && !force) {
        return false;
    }

    Log.infoln(F("[%s] Begin"), ThreadName.c_str());

    _initiated = init();

    if (_initiated) {
        lastUpdateHasError = false;
        Log.infoln(F("[%s] Begin OK"), ThreadName.c_str());
    } else {
        lastUpdateHasError = true;
        Log.errorln(F("[%s] Begin KO"), ThreadName.c_str());
    }

    return _initiated;
}

void MyThread::run() {
    if (!noLog) {
        Log.traceln(F("[%s] Run"), ThreadName.c_str());
    }

    if (!_initiated && !begin()) {
        runned();
        force = false;
        Log.errorln(F("[%s] Run KO"), ThreadName.c_str());
        return;
    }

    if (runOnce()) {
        if (!noLog) {
            Log.infoln(F("[%s] Run OK"), ThreadName.c_str());
        }
        lastUpdateHasError = false;
    } else {
        Log.errorln(F("[%s] Run KO"), ThreadName.c_str());
        lastUpdateHasError = true;
    }

    runned();
    force = false;
}

void MyThread::forceRun() {
    Log.infoln(F("[%s] Set force run"), ThreadName.c_str());
    force = true;
}
