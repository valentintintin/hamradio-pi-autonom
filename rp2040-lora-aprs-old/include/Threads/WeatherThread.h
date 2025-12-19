#ifndef RP2040_LORA_APRS_WEATHERTHREAD_H
#define RP2040_LORA_APRS_WEATHERTHREAD_H

#include <Adafruit_BMP280.h>
#include <Adafruit_BME280.h>
#include <config.h>
#include <RadioLib.h>
#include <Timer.h>

#include "MyThread.h"
#include "FineOffsetWH65B.h"

class System;

enum SensorType {
    None,
    BME280,
    BMP280
};

class WeatherThread : public MyThread {
public:
    explicit WeatherThread(System *system);

    Timer timerNextWH65B = Timer();

    inline float getTemperature() const {
        if (decodeWH65B) {
            if (!_hasErrorWH65B) {
                return (temperature + wh65BData.temperature_C) / 2;
            }
        }

        return temperature;
    }

    inline float getHumidity() const {
        if (decodeWH65B) {
            if (!_hasErrorWH65B) {
                if (sensorType == BME280) {
                    return (humidity + wh65BData.temperature_C) / 2;
                }

                return static_cast<float>(wh65BData.humidity);
            }
        }

        return sensorType == BME280 ? humidity : 0;
    }

    inline float getPressure() const {
        return pressure;
    }

    inline int getWindDirectionDeg() const {
        return windDirectionDeg;
    }

    inline float getWindAverageMs() const {
        return windAverageMs;
    }

    inline float getWindMaxMs() const {
        return windMaxMs;
    }

    inline float getRain1hMm() const {
        return rain1hMm;
    }

    inline float getRain24hMm() const {
        return rain24hMm;
    }

    inline float getRainSinceMidnightMm() const {
        return rainSinceMidnightMm;
    }

    inline const WH65BData* getWh65BData() const {
        return decodeWH65B && _hasErrorWH65B ? &wh65BData : nullptr;
    }

    inline bool hasError() const override {
        return _hasErrorWH65B || MyThread::hasError();
    }
protected:
    bool init() override;
    bool runOnce() override;
private:
    static volatile bool cc1101RxInterrupt;

    static void setHasCC1101RxInterrupt();

    Adafruit_BMP280 bmp280 = Adafruit_BMP280();
    Adafruit_BME280 bme280 = Adafruit_BME280();
    SensorType sensorType = None;
    CC1101 cc1101 = new Module(CC1101_RECEIVER_CS, CC1101_RECEIVER_IRQ, RADIOLIB_NC, CC1101_RECEIVER_GPIO);
    bool decodeWH65B = false;
    bool cc1101IsReceiving = false;
    bool _hasErrorWH65B = false;
    Timer timer1h = Timer(3600000);
    Timer timer24h = Timer(86400000);
    Timer timerToMidnight = Timer();

    float temperature = 0;
    float humidity = 0;
    float pressure = 0;
    int windDirectionDeg = 0;
    float windAverageMs = 0;
    float windMaxMs = 0;
    float rain1hMm = 0;
    float rain24hMm = 0;
    float rainSinceMidnightMm = 0;
    WH65BData wh65BData{};

    float readTemperature();
    float readHumidity();
    float readPressure();
    bool initCC1101ForWH65BSettings();
    bool startReceiveCC1101();
    bool sleepCC1101();
    bool computeCC1101PayloadForWh65B();
    void setTimerToMidnight();
};

#endif //RP2040_LORA_APRS_WEATHERTHREAD_H
