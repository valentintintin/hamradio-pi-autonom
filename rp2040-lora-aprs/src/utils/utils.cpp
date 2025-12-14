#include "utils/utils.h"

#include <ctime>

void getDateTimeStringFromEpoch(const uint64_t epoch, char* buffer, const size_t size) {
    const time_t epochTimeT = epoch;
    tm ts{};
    localtime_r(&epochTimeT, &ts);
    strftime(buffer, size, "%Y-%m-%dT%H:%M:%S", &ts);
    // snprintf(buffer, size, "%d-%d-%dT%d:%d:%dZ", ts.tm_year + 1900, ts.tm_mon + 1, ts.tm_mday, ts.tm_hour, ts.tm_min, ts.tm_sec);
}
