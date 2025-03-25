#ifndef RP2040_LORA_APRS_MYTHREAD_H
#define RP2040_LORA_APRS_MYTHREAD_H

#include "Thread.h"
#include "config.h"

class System;

class MyThread : public Thread {
public:
    explicit MyThread(System *system, unsigned long interval, const char *name, bool noLog = false, bool enabled = true);

    bool begin();
    void run() override;
    void forceRun();

    virtual bool shouldRun(unsigned long time);

    inline void setRunned() {
        runned();
    }

    inline bool isInitiated() const {
        return _initiated;
    }

    inline bool hasError() const {
        return !_initiated || lastUpdateHasError;
    }

    inline uint64_t timeBeforeRun() const {
        return _cached_next_run - millis();
    }
protected:
    virtual bool init() {
        return true;
    }

    inline bool isAfterBoot() const {
        return millis() > TIME_AFTER_BOOT;
    }

    virtual bool runOnce() = 0;

    System *system;
    bool force = false;
    bool _initiated = false;
    bool lastUpdateHasError = false;
    bool noLog = false;
};


#endif //RP2040_LORA_APRS_MYTHREAD_H
