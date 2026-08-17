#pragma once

#define PROGMEM
#define PSTR(s) (s)

#define pgm_read_byte(addr)  (*(const unsigned char*)(addr))
#define pgm_read_word(addr)  (*(const unsigned short*)(addr))
#define pgm_read_dword(addr) (*(const unsigned long*)(addr))
#define pgm_read_ptr(addr)   (*(const void* const*)(addr))

#include <cstring>
#define memcpy_P(dest, src, n) memcpy((dest), (src), (n))
#define strcpy_P(dest, src) strcpy((dest), (src))
#define strncpy_P(dest, src, n) strncpy((dest), (src), (n))
#define strncat_P(dest, src, n) strncat((dest), (src), (n))
#define strlen_P(s) strlen(s)
#define strstr_P(haystack, needle) strstr((haystack), (needle))
#define strcmp_P(a, b) strcmp((a), (b))
#define sprintf_P(buf, fmt, ...) sprintf((buf), (fmt), ##__VA_ARGS__)
#define vsnprintf_P(buf, n, fmt, args) vsnprintf((buf), (n), (fmt), (args))
