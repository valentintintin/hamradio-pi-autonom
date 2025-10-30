#ifndef RP2040_LORA_APRS_MYTHREAD_H
#define RP2040_LORA_APRS_MYTHREAD_H

#include "Thread.h"

class System;

class MyThread : public Thread {
public:
    explicit MyThread(System *system, unsigned long interval, const char *name, bool noLog = false, bool enabled = true);

    bool begin();
    void run() override;
    void forceRun();

    long tillRun(unsigned long time) override;

    inline void setRunned() {
        runned();
    }

    inline bool isInitiated() const {
        return _initiated;
    }

    virtual inline bool hasError() const {
        return !_initiated || lastUpdateHasError;
    }

    inline uint64_t timeBeforeRun() const {
        return _cached_next_run - millis();
    }
protected:
    virtual bool init() {
        return true;
    }

    virtual bool runOnce() = 0;

    System *system;
    bool force = false;
    bool _initiated = false;
    bool lastUpdateHasError = false;
    bool noLog = false;
};


#endif //RP2040_LORA_APRS_MYTHREAD_H
