#ifndef CUBECELL_MONITORING_MPPTMONITOR_H
#define CUBECELL_MONITORING_MPPTMONITOR_H

#include "Timer.h"
#include "mpptChg.h"
#include "Config.h"
#include "Communication.h"

class MpptMonitor {
public:
    explicit MpptMonitor(Communication *communication);
    bool begin();
    void update();
private:
    Communication *communication;

    uint16_t chgStatus = 0;
    int16_t chgVs = 0, chgIs = 0, chgVb = 0, chgIb = 0;
    mpptChg chg;
    Timer timerChg = Timer(INTERVAL_REFRESH_MPPTCHG);
};

#endif //CUBECELL_MONITORING_MPPTMONITOR_H
