#pragma once

#include <Print.h>
#include <stdint.h>
#include <stddef.h>

// ============================================================================
// StringPrint — adapte l'API Print d'Arduino pour écrire dans un buffer fixe
// (utilisé pour renvoyer la réponse d'une commande CLI en message APRS/mesh).
// ============================================================================
class StringPrint : public Print {
public:
  StringPrint(char* buf, size_t len) : _buf(buf), _len(len), _pos(0) {
    if (_len > 0) {
      _buf[0] = '\0';
    }
  }
  size_t write(uint8_t c) override {
    if (_pos < _len - 1) { _buf[_pos++] = c; _buf[_pos] = '\0'; return 1; }
    return 0;
  }
  size_t write(const uint8_t* buf, size_t size) override {
    size_t written = 0;
    for (size_t i = 0; i < size && _pos < _len - 1; i++) {
      _buf[_pos++] = buf[i]; written++;
    }
    _buf[_pos] = '\0';
    return written;
  }
private:
  char* _buf;
  size_t _len;
  size_t _pos;
};
