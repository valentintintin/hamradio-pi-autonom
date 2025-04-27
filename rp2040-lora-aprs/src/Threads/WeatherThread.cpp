#include "Threads/WeatherThread.h"
#include "../../.pio/libdeps/saint-jean/ArduinoLog/src/ArduinoLog.h"
#include "../../include/System.h"
#include "../../include/utils.h"

volatile bool WeatherThread::cc1101RxInterrupt = false;

void WeatherThread::setHasCC1101RxInterrupt() {
    cc1101RxInterrupt = true;
}

WeatherThread::WeatherThread(System *system) : MyThread(system, system->settings.weather.intervalCheck, PSTR("WEATHER")) {
    enabled = system->settings.weather.enabled;
    force = true;
    setTimerToMidnight();
}

bool WeatherThread::init() {
    if (bmp280.begin()) {
        sensorType = BMP280;

        bmp280.setSampling(Adafruit_BMP280::MODE_FORCED,
                           Adafruit_BMP280::SAMPLING_X1, // Temp. oversampling
                           Adafruit_BMP280::SAMPLING_X1, // Pressure oversampling
                           Adafruit_BMP280::FILTER_OFF, Adafruit_BMP280::STANDBY_MS_1000);

        Log.info("[WEATHER] BMP280 found OK");
    } else if (bme280.begin()) {
        sensorType = BME280;

        bme280.setSampling(Adafruit_BME280::MODE_FORCED,
                           Adafruit_BME280::SAMPLING_X1, // Temp. oversampling
                           Adafruit_BME280::SAMPLING_X1, // Pressure oversampling
                           Adafruit_BME280::SAMPLING_X1, // Humidity oversampling
                           Adafruit_BME280::FILTER_OFF, Adafruit_BME280::STANDBY_MS_1000);

        Log.info("[WEATHER] BME280 found OK");
    } else {
        sensorType = None;
    }

    if (sensorType != None) {
        for (uint8_t i = 0; i < 3; i++) {
            delayWdt(150);
            readTemperature(); // To have correct value for first runOnce
        }

        return true;
    }

    return decodeWH65B ? initCC1101ForWH65BSettings() : false;
}

bool WeatherThread::runOnce() {
    temperature = readTemperature();
    humidity = readHumidity();
    pressure = readPressure();

    bool success = false;

    if (pressure <= 700 || pressure >= 1200 || temperature >= 70 || temperature <= -30) {
        Log.warningln(F("[WEATHER] Error ! Temperature: %FC Humidity: %F%% Pressure: %FhPa"), temperature, humidity, pressure);
        _initiated = false;
    } else {
        pressure *= static_cast<float>(pow((1 - system->settings.aprs.altitude / 44330.0), -5.255));
        Log.infoln(F("[WEATHER] Temperature: %FC Humidity: %F%% Pressure: %FhPa"), temperature, humidity, pressure);
        success = true;
    }

    if (cc1101RxInterrupt) {
        success &= computeCC1101PayloadForWh65B();
    } else if (timerNextWH65B.hasExpired() && !cc1101IsReceiving) {
        Log.traceln(F("[WEATHER_CC1101] Time to wake up"));
        startReceiveCC1101();
    }

    if (timer1h.hasExpired()) {
        rain1hMm = 0;
        timer1h.restart();
    }

    if (timer24h.hasExpired()) {
        rain24hMm = 0;
        timer24h.restart();
    }

    if (system->settings.rtc.enabled && timerToMidnight.hasExpired()) {
        rainSinceMidnightMm = 0;
        setTimerToMidnight();
    }

    return success;
}

float WeatherThread::readTemperature() {
    switch (sensorType) {
        case BME280:
            bme280.takeForcedMeasurement();
            return bme280.readTemperature();
        case BMP280:
            bmp280.takeForcedMeasurement();
            return bmp280.readTemperature();
        default:
            return 0;
    }
}

float WeatherThread::readHumidity() {
    switch (sensorType) {
        case BME280:
            return bme280.readHumidity();
        case BMP280:
        default:
            return 0;
    }
}

float WeatherThread::readPressure() {
    switch (sensorType) {
        case BME280:
            return bme280.readPressure();
        case BMP280:
            return bmp280.readPressure();
        default:
            return 0;
    }
}

bool WeatherThread::initCC1101ForWH65BSettings() {
    auto state = cc1101.begin(433.92, 8.21, 57.136417, 270, 10, 32);
    if (state != RADIOLIB_ERR_NONE) {
        Log.errorln(F("[WEATHER_CC1101] Error during change to FSK WH65B: %d"), state);
        _hasErrorWH65B = true;
        return false;
    }

    state = cc1101.setEncoding(RADIOLIB_ENCODING_NRZ);
    if (state != RADIOLIB_ERR_NONE) {
        Log.errorln(F("[WEATHER_CC1101] Error during change encoding: %d"), state);
        _hasErrorWH65B = true;
        return false;
    }

    state = cc1101.setCrcFiltering(false);
    if (state != RADIOLIB_ERR_NONE) {
        Log.errorln(F("[WEATHER_CC1101] Error during change CRC: %d"), state);
        _hasErrorWH65B = true;
        return false;
    }

    state = cc1101.fixedPacketLengthMode(WH65B_MAX_PAYLOAD_LENGTH);
    if (state != RADIOLIB_ERR_NONE) {
        Log.errorln(F("[WEATHER_CC1101] Error during change packet length: %d"), state);
        _hasErrorWH65B = true;
        return false;
    }

    state = cc1101.setSyncWord(0xAA, 0x2D, 0, false);
    if (state != RADIOLIB_ERR_NONE) {
        Log.errorln(F("[WEATHER_CC1101] Error during change syncWord: %d"), state);
        _hasErrorWH65B = true;
        return false;
    }

    cc1101.setPacketReceivedAction(setHasCC1101RxInterrupt);

    Log.infoln(F("[WEATHER_CC1101] Init OK"));

    _hasErrorWH65B = false;

    return startReceiveCC1101();
}

bool WeatherThread::startReceiveCC1101() {
    const auto state = cc1101.startReceive();
    if (state != RADIOLIB_ERR_NONE) {
        Log.errorln(F("[WEATHER_CC1101] Error during startReceive: %d"), state);
        _initiated = false;
        _hasErrorWH65B = true;
        return false;
    }

    Log.infoln(F("[WEATHER_CC1101] startReceive OK"));

    cc1101RxInterrupt = false;
    cc1101IsReceiving = true;
    timerNextWH65B.setExpired();

    return true;
}

bool WeatherThread::sleepCC1101() {
    const auto state = cc1101.sleep();
    if (state != RADIOLIB_ERR_NONE) {
        Log.errorln(F("[WEATHER_CC1101] Error during sleep: %d"), state);
        _initiated = false;
        _hasErrorWH65B = true;
        return false;
    }

    Log.infoln(F("[WEATHER_CC1101] Sleep OK"));

    cc1101RxInterrupt = false;
    cc1101IsReceiving = false;
    timerNextWH65B.restart();

    return true;
}

bool WeatherThread::computeCC1101PayloadForWh65B() {
    cc1101RxInterrupt = false;
    cc1101IsReceiving = false;

    const auto size = cc1101.getPacketLength();
    const auto state = cc1101.readData(buffer, size);
    const auto rssi = lround(cc1101.getRSSI());

    Log.infoln(F("[CC1101_RX] Payload size %d, RSSI %F"), size, rssi);

    for (size_t i = 0; i < size; i++) {
        Log.verboseln(F("[CC1101_RX] Payload[%d]=%X %c"), i, buffer[i], buffer[i]);
    }

    if (state != RADIOLIB_ERR_NONE || size < WH65B_MAX_PAYLOAD_LENGTH) {
        Log.warningln(F("[CC1101_RX] Wrong packet received (size %d), error (%d)"), size, state);
        return false;
    }

    const auto lastRain = wh65BData.rainfall_mm;
    wh65BData = FineOffsetWH65B::decode(buffer);

    if (!wh65BData.crc_ok || wh65BData.temperature_C >= 70 || wh65BData.temperature_C <= -30) {
        Log.warningln(F("[WEATHER] WH65B wrong CRC or temperature %F"), wh65BData.temperature_C);
        _hasErrorWH65B = true;
        startReceiveCC1101();
        return false;
    }

    const auto newRain = wh65BData.rainfall_mm - lastRain;
    rain1hMm += newRain;
    rain24hMm += newRain;
    rainSinceMidnightMm += newRain;

    _hasErrorWH65B = false;
    
    sleepCC1101();

    return true;
}

void WeatherThread::setTimerToMidnight() {
    const auto now = system->getDateTime();
    const uint16_t currentTimeSec = now.hour() * 3600 + now.minute() * 60 + now.second();
    const uint16_t toMidnightSec = 86400 - currentTimeSec;
    Log.noticeln("[WEATHER] Time to midnight: %d", toMidnightSec);
    timerToMidnight.setInterval(toMidnightSec * 1000);
}
