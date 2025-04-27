#ifndef FINEOFFSETWH65B_H
#define FINEOFFSETWH65B_H

#include <Arduino.h>

typedef struct {
    int id;
    bool battery_ok;
    float temperature_C;
    int humidity;
    int wind_dir_deg;
    float wind_avg_m_s;
    float wind_max_m_s;
    float rainfall_mm;
    int uv;
    int uvi;
    float light_lux;
    bool crc_ok;
} WH65BData;

class FineOffsetWH65B {
public:
  static WH65BData decode(const uint8_t *b);
private:
    static uint8_t crc8(const uint8_t *data, int len, uint8_t poly = 0x31, uint8_t init = 0x00);
};

#endif //FINEOFFSETWH65B_H
