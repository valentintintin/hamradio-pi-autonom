#pragma once

// ============================================================================
// Arduino.h — shim natif (host Linux, env PlatformIO `native`).
//
// Ne couvre QUE la surface Arduino réellement utilisée par src/ (cf. plan
// /home/.../plans/cheeky-floating-hickey.md) : Serial, millis/delay, GPIO
// digitalWrite/Read/pinMode/attachInterrupt, random/randomSeed, la classe
// Print, et le pseudo-SDK RP2040 (objet global `rp2040`, classe `RP2040`)
// utilisés par CommandHandler/MyBoard/task_watchdog. Pas une émulation
// Arduino générale.
// ============================================================================

#include <cstdint>
#include <cstddef>
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <cstdlib>
#include <cmath>

// Sur les vrais cores Arduino, <Arduino.h> tire <avr/pgmspace.h> (ou
// équivalent) transitivement — PSTR/PROGMEM/pgm_read_*/*_P sont donc
// utilisables partout après un simple `#include <Arduino.h>`, sans include
// séparé (RTClib.cpp, lib/Aprs/src/Aprs.cpp comptent dessus). Même confort
// ici.
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

// Utilisé par Adafruit BusIO (dépendance de RTClib) en branche "SW SPI only"
// (cf. platformio.ini: -D SPI_INTERFACES_COUNT=0) — jamais réellement
// utilisé (pas de SPI natif), juste pour compiler.
typedef enum { LSBFIRST = 0, MSBFIRST = 1 } BitOrder;

#define constrain(amt, low, high) ((amt) < (low) ? (low) : ((amt) > (high) ? (high) : (amt)))

// min/max en fonctions template (comme le fait arduino-pico depuis peu),
// pas en macros : évite les collisions historiques avec std::min/max et les
// en-têtes <algorithm>/<limits> déjà utilisés par ce shim.
template <class T, class L>
auto min(const T a, const L b) -> decltype((b < a) ? b : a) {
  return (b < a) ? b : a;
}
template <class T, class L>
auto max(const T a, const L b) -> decltype((b > a) ? b : a) {
  return (b > a) ? b : a;
}

// ---- String / __FlashStringHelper (sous-ensemble minimal) ------------------
// Nécessaire uniquement pour compiler RTClib (DateTime::timestamp() renvoie
// une String ; le constructeur F(__DATE__)/F(__TIME__) prend des
// __FlashStringHelper*) — le projet lui-même n'utilise pas String.
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

// ---- Print (sous-ensemble d'Arduino::Print) --------------------------------
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

  // Sous-ensemble d'Arduino::Print::print(nombre, base) — requis depuis la
  // maj MeshCore qui a ajouté ConfigSerializer::def(int32_t/uint32_t/...)
  // avec un print(valeur, 10) explicite (auparavant un simple print(valeur)
  // suffisait, résolu par conversion implicite vers print(char), tronquant
  // silencieusement — cf. lignes ci-dessus dans ConfigSerializer.cpp).
  size_t print(unsigned long n, int base = 10) {
    // glibc n'a pas ultoa/ltoa (spécifiques avr-libc) : conversion manuelle.
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

// ---- Stream (sous-ensemble d'Arduino::Stream) ------------------------------
// Nécessaire pour mesh::Identity::readFrom/writeTo(Stream&) (lib/MeshCore) —
// File (LittleFS.h) et SerialClass en héritent.
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

// ---- Serial -----------------------------------------------------------------
// Lit stdin en non-bloquant (select()) — une ligne complète tapée au terminal
// (canonique, gérée par le pilote tty : édition/backspace gratuits) arrive
// d'un coup dans le buffer interne, consommée caractère par caractère comme
// task_cli.cpp le fait déjà pour Serial.available()/read().
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
// VE.Direct (Victron) attendu sur un UART dédié sur la cible réelle — non
// câblé en natif (VictronHal simulé n'y touche pas), objet fourni uniquement
// pour que `VictronHal victron(Serial1);` (main.cpp) compile.
extern SerialClass Serial1;
typedef SerialClass HardwareSerial;

// ---- Temps ------------------------------------------------------------------
unsigned long millis();
unsigned long micros();
void delay(unsigned long ms);
void delayMicroseconds(unsigned int us);

// ---- RNG ----------------------------------------------------------------
void randomSeed(unsigned long seed);
long random(long max);
long random(long min, long max);

// ---- itoa/ltoa (AVR libc, absentes de la libc hôte) ------------------------
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

// ---- GPIO (table de pins fake, cf. native/compat/arduino_compat.cpp) -------
void pinMode(uint8_t pin, uint8_t mode);
void digitalWrite(uint8_t pin, uint8_t val);
int digitalRead(uint8_t pin);

typedef void (*NativeIsr)();
void attachInterrupt(uint8_t pin, NativeIsr isr, int mode);
void detachInterrupt(uint8_t pin);
uint8_t digitalPinToInterrupt(uint8_t pin);

// ---- Pseudo-SDK RP2040 (arduino-pico core) ---------------------------------
// Utilisé directement par cli/CommandHandler.cpp, tasks/task_watchdog.cpp,
// variant/MyBoard.h — non inclus explicitement sur cible réelle (fourni par
// le core arduino-pico via <Arduino.h>), donc fourni ici de la même façon.
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

// arduino-pico expose xTaskCreate/vTaskDelay/... directement après
// `#include <Arduino.h>` (FreeRTOS lié au core) sans include séparé — même
// confort ici, plusieurs fichiers du projet (core/Boot.cpp...) comptent
// dessus.
#include "task.h"
#include "semphr.h"
