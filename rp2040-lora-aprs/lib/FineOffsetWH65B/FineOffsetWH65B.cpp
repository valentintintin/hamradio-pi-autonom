#include "FineOffsetWH65B.h"

WH65BData FineOffsetWH65B::decode(const uint8_t *b) {
    WH65BData out;
    out.crc_ok = false;

    if (b[0] != 0x24) return out; // family code

    uint8_t crc = crc8(b, 15);
    uint8_t sum = 0;
    for (int i = 0; i < 16; i++) sum += b[i];

    if (crc != b[15] || sum != b[16]) return out;
    out.crc_ok = true;

    out.id = b[1];
    int wind_dir_raw = b[2] | ((b[3] & 0x80) << 1);
    out.wind_dir_deg = (wind_dir_raw == 0x1FF) ? -1 : wind_dir_raw;

    out.battery_ok = !(b[3] & 0x08);

    int temp_raw = ((b[3] & 0x07) << 8) | b[4];
    out.temperature_C = (temp_raw == 0x7FF) ? NAN : (temp_raw - 400) * 0.1f;

    out.humidity = (b[5] == 0xFF) ? -1 : b[5];

    int wind_speed_raw = b[6] | ((b[3] & 0x10) << 4);
    out.wind_avg_m_s = (wind_speed_raw == 0x1FF) ? NAN : wind_speed_raw * 0.125f * 0.51f;

    int gust_speed_raw = b[7];
    out.wind_max_m_s = (gust_speed_raw == 0xFF) ? NAN : gust_speed_raw * 0.51f;

    int rain_raw = (b[8] << 8) | b[9];
    out.rainfall_mm = rain_raw * 0.254f;

    int uv_raw = (b[10] << 8) | b[11];
    out.uv = (uv_raw == 0xFFFF) ? -1 : uv_raw;

    static const int uvi_upper[] = {432, 851, 1210, 1570, 2017, 2450, 2761, 3100, 3512, 3918, 4277, 4650, 5029};
    out.uvi = 0;
    if (uv_raw != 0xFFFF) {
        while (out.uvi < 13 && uv_raw > uvi_upper[out.uvi]) out.uvi++;
    }

    int light_raw = (b[12] << 16) | (b[13] << 8) | b[14];
    out.light_lux = (light_raw == 0xFFFFFF) ? NAN : light_raw * 0.1f;

    return out;
}

uint8_t FineOffsetWH65B::crc8(const uint8_t *data, int len, uint8_t poly, uint8_t init) {
    uint8_t crc = init;
    for (int i = 0; i < len; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++)
            crc = (crc & 0x80) ? (crc << 1) ^ poly : (crc << 1);
    }
    return crc;
}
