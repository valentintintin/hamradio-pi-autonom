#pragma once

#include <cstdint>
#include <cstddef>
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <cstdlib>
#include <cmath>

#include "pgmspace.h"

typedef uint8_t byte;

#define HIGH 1
#define LOW  0
#define INPUT 0
#define OUTPUT 1
#define INPUT_PULLUP 2
#define RISING 1
#define FALLING 2
#define CHANGE 3

typedef enum { LSBFIRST = 0, MSBFIRST = 1 } BitOrder;

#define constrain(amt, low, high) ((amt) < (low) ? (low) : ((amt) > (high) ? (high) : (amt)))

template <class T, class L>
auto min(const T a, const L b) -> decltype((b < a) ? b : a) {
  return (b < a) ? b : a;
}
template <class T, class L>
auto max(const T a, const L b) -> decltype((b > a) ? b : a) {
  return (b > a) ? b : a;
}

class __FlashStringHelper;
#define F(string_literal) ((const __FlashStringHelper*)(string_literal))

class String {
public:
  String() { _buf[0] = '\0'; }
  String(const char* s) {
    strncpy(_buf, s ? s : "", sizeof(_buf) - 1);
    _buf[sizeof(_buf) - 1] = '\0';
  }
  const char* c_str() const { return _buf; }

private:
  char _buf[48];
};

class Print {
public:
  virtual ~Print() = default;

  virtual size_t write(uint8_t c) = 0;

  virtual size_t write(const uint8_t* buf, size_t len) {
    size_t n = 0;
    for (size_t i = 0; i < len; i++) {
      n += write(buf[i]);
    }
    return n;
  }

  size_t write(const char* s) { return write((const uint8_t*)s, strlen(s)); }

  size_t print(const char* s) { return write(s); }
  size_t print(char c) { return write((uint8_t)c); }

  // print(valeur, base) requis depuis que ConfigSerializer::def() appelle
  // print(v, 10) explicitement — sans surcharge ici, ça résolvait vers
  // print(char) et tronquait silencieusement la valeur.
  size_t print(unsigned long n, int base = 10) {
    char buf[8 * sizeof(unsigned long) + 1];
    char* p = buf + sizeof(buf) - 1;
    *p = '\0';
    if (base < 2) base = 10;
    do {
      int digit = n % base;
      *--p = digit < 10 ? ('0' + digit) : ('a' + digit - 10);
      n /= base;
    } while (n != 0);
    return write(p);
  }
  size_t print(long n, int base = 10) {
    if (n < 0 && base == 10) return write('-') + print((unsigned long)(-n), base);
    return print((unsigned long)n, base);
  }
  size_t print(int n, int base = 10) { return print((long)n, base); }
  size_t print(unsigned int n, int base = 10) { return print((unsigned long)n, base); }
  size_t print(double n, int digits = 2) {
    char buf[64];
    snprintf(buf, sizeof(buf), "%.*f", digits, n);
    return write(buf);
  }

  size_t println(const char* s) {
    size_t n = write(s);
    n += write((uint8_t)'\n');
    return n;
  }
  size_t println() { return write((uint8_t)'\n'); }

  int printf(const char* fmt, ...) {
    char buf[512];
    va_list args;
    va_start(args, fmt);
    int n = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    if (n > 0) {
      size_t len = (size_t)n < sizeof(buf) ? (size_t)n : sizeof(buf) - 1;
      write((const uint8_t*)buf, len);
    }
    return n;
  }

  virtual void flush() {}
};

class Stream : public Print {
public:
  virtual int available() = 0;
  virtual int read() = 0;
  virtual int peek() = 0;
  virtual size_t readBytes(uint8_t* buf, size_t len) {
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
};

class SerialClass : public Stream {
public:
  void begin(unsigned long) {}
  int available() override;
  int read() override;
  int peek() override;
  size_t readBytes(uint8_t* buf, size_t len) override;

  size_t write(uint8_t c) override {
    putchar(c);
    return 1;
  }
  size_t write(const uint8_t* buf, size_t len) override {
    fwrite(buf, 1, len, stdout);
    return len;
  }
  void flush() override { fflush(stdout); }
};

extern SerialClass Serial;
extern SerialClass Serial1;
typedef SerialClass HardwareSerial;

unsigned long millis();
unsigned long micros();
void delay(unsigned long ms);
void delayMicroseconds(unsigned int us);

void randomSeed(unsigned long seed);
long random(long max);
long random(long min, long max);

inline char* ltoa(long value, char* str, int base) {
  if (base == 10) {
    sprintf(str, "%ld", value);
    return str;
  }
  bool neg = value < 0 && base == 10;
  unsigned long uv = neg ? (unsigned long)(-value) : (unsigned long)value;
  char tmp[34];
  int i = 0;
  if (uv == 0) {
    tmp[i++] = '0';
  }
  while (uv > 0) {
    int d = (int)(uv % (unsigned long)base);
    tmp[i++] = (char)(d < 10 ? ('0' + d) : ('a' + d - 10));
    uv /= (unsigned long)base;
  }
  int j = 0;
  if (neg) {
    str[j++] = '-';
  }
  while (i > 0) {
    str[j++] = tmp[--i];
  }
  str[j] = '\0';
  return str;
}
inline char* itoa(int value, char* str, int base) { return ltoa(value, str, base); }

void pinMode(uint8_t pin, uint8_t mode);
void digitalWrite(uint8_t pin, uint8_t val);
int digitalRead(uint8_t pin);

typedef void (*NativeIsr)();
void attachInterrupt(uint8_t pin, NativeIsr isr, int mode);
void detachInterrupt(uint8_t pin);
uint8_t digitalPinToInterrupt(uint8_t pin);

class RP2040 {
public:
  enum ResetReason { PWRON_RESET = 0, RUN_PIN_RESET, SOFT_RESET, WDT_RESET, DEBUG_RESET };

  ResetReason getResetReason() { return _reset_reason; }
  void wdt_begin(uint32_t timeout_ms);
  void wdt_reset();
  uint32_t getFreeHeap();
  void reboot();
  void rebootToBootloader();

private:
  ResetReason _reset_reason = PWRON_RESET;
};

extern RP2040 rp2040;

#include "task.h"
#include "semphr.h"
