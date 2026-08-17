#include "Arduino.h"

#include <chrono>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <sys/select.h>
#include <unordered_map>

SerialClass Serial;
SerialClass Serial1;
RP2040 rp2040;

static const std::chrono::steady_clock::time_point g_boot_time = std::chrono::steady_clock::now();

unsigned long millis() {
  auto now = std::chrono::steady_clock::now();
  return (unsigned long)std::chrono::duration_cast<std::chrono::milliseconds>(now - g_boot_time).count();
}

unsigned long micros() {
  auto now = std::chrono::steady_clock::now();
  return (unsigned long)std::chrono::duration_cast<std::chrono::microseconds>(now - g_boot_time).count();
}

void delay(unsigned long ms) {
  if (ms == 0) {
    return;
  }
  struct timespec ts;
  ts.tv_sec = ms / 1000;
  ts.tv_nsec = (long)(ms % 1000) * 1000000L;
  nanosleep(&ts, nullptr);
}

void delayMicroseconds(unsigned int us) {
  struct timespec ts;
  ts.tv_sec = us / 1000000;
  ts.tv_nsec = (long)(us % 1000000) * 1000L;
  nanosleep(&ts, nullptr);
}

void randomSeed(unsigned long seed) {
  if (seed != 0) {
    srandom((unsigned int)seed);
  }
}

long random(long max) {
  if (max <= 0) {
    return 0;
  }
  return (long)(::random() % max);
}

long random(long min, long max) {
  if (max <= min) {
    return min;
  }
  return min + (long)(::random() % (max - min));
}

namespace {
constexpr size_t kSerialBufCap = 4096;
uint8_t g_buf[kSerialBufCap];
size_t g_buf_head = 0;
size_t g_buf_tail = 0;

size_t bufAvailable() { return g_buf_tail - g_buf_head; }

void fillFromStdin() {
  fd_set fds;
  FD_ZERO(&fds);
  FD_SET(STDIN_FILENO, &fds);
  struct timeval tv = {0, 0};
  if (select(STDIN_FILENO + 1, &fds, nullptr, nullptr, &tv) <= 0) {
    return;
  }
  if (!FD_ISSET(STDIN_FILENO, &fds)) {
    return;
  }

  if (g_buf_head > 0 && g_buf_tail == kSerialBufCap) {
    size_t remaining = bufAvailable();
    memmove(g_buf, g_buf + g_buf_head, remaining);
    g_buf_head = 0;
    g_buf_tail = remaining;
  }
  if (g_buf_tail >= kSerialBufCap) {
    return;
  }

  ssize_t n = ::read(STDIN_FILENO, g_buf + g_buf_tail, kSerialBufCap - g_buf_tail);
  if (n > 0) {
    g_buf_tail += (size_t)n;
  }
}
}  // namespace

int SerialClass::available() {
  fillFromStdin();
  return (int)bufAvailable();
}

int SerialClass::read() {
  if (bufAvailable() == 0) {
    fillFromStdin();
  }
  if (bufAvailable() == 0) {
    return -1;
  }
  return g_buf[g_buf_head++];
}

int SerialClass::peek() {
  if (bufAvailable() == 0) {
    fillFromStdin();
  }
  return bufAvailable() == 0 ? -1 : g_buf[g_buf_head];
}

size_t SerialClass::readBytes(uint8_t* buf, size_t len) {
  size_t n = 0;
  while (n < len) {
    int c = read();
    if (c < 0) {
      break;
    }
    buf[n++] = (uint8_t)c;
  }
  return n;
}

namespace {
constexpr uint8_t kMaxPins = 64;
uint8_t g_pin_mode[kMaxPins] = {0};
uint8_t g_pin_level[kMaxPins] = {0};
NativeIsr g_pin_isr[kMaxPins] = {nullptr};
}  // namespace

void pinMode(uint8_t pin, uint8_t mode) {
  if (pin < kMaxPins) {
    g_pin_mode[pin] = mode;
  }
}

void digitalWrite(uint8_t pin, uint8_t val) {
  if (pin < kMaxPins) {
    g_pin_level[pin] = val;
  }
}

int digitalRead(uint8_t pin) {
  return (pin < kMaxPins) ? g_pin_level[pin] : LOW;
}

void attachInterrupt(uint8_t pin, NativeIsr isr, int /*mode*/) {
  if (pin < kMaxPins) {
    g_pin_isr[pin] = isr;
  }
}

void detachInterrupt(uint8_t pin) {
  if (pin < kMaxPins) {
    g_pin_isr[pin] = nullptr;
  }
}

uint8_t digitalPinToInterrupt(uint8_t pin) { return pin; }

void RP2040::wdt_begin(uint32_t) {}
void RP2040::wdt_reset() {}

uint32_t RP2040::getFreeHeap() {
  return 128UL * 1024UL * 1024UL;
}

void RP2040::reboot() {
  fprintf(stderr, "[SIM] rp2040.reboot() ignoré en natif\n");
}

void RP2040::rebootToBootloader() {
  fprintf(stderr, "[SIM] rp2040.rebootToBootloader() ignoré en natif\n");
}
