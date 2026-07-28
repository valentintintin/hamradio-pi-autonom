#include "SimWorld.h"
#include <Arduino.h>

SimWorld& SimWorld::instance() {
  static SimWorld w;
  return w;
}

void SimWorld::pushLog(std::deque<RadioLogEntry>& log, RadioLogEntry entry) {
  log.push_back(std::move(entry));
  while (log.size() > kRadioLogMax) {
    log.pop_front();
  }
}

void SimWorld::logTx(std::deque<RadioLogEntry>& log, const uint8_t* data, int len) {
  RadioLogEntry e;
  e.tx = true;
  e.millis_ts = millis();
  e.data.assign(data, data + len);
  pushLog(log, std::move(e));
}

void SimWorld::pushLogLine(const std::string& line) {
  log_ring.push_back(line);
  while (log_ring.size() > kLogRingMax) {
    log_ring.pop_front();
  }
}
