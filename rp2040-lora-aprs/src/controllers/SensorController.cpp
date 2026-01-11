#include "controllers/SensorController.hpp"

#include <ArduinoLog.h>

#include "controllers/LedController.hpp"
#include "hal/I2CMasterHal.hpp"
#include "utils/utils.h"

Telemetry SensorController::telemetry{};

SensorController::SensorController()
{
    timer = xTimerCreate("queryTelemetries", pdMS_TO_TICKS(QUERY_DELAY), pdTRUE, this, queryTimer);

    if (timer == nullptr)
    {
        Log.warningln("Timer creation failed");

        LedController::getInstance().blink(Error, FreeRtos);
    }
}

void SensorController::queryTimer(const TimerHandle_t timer)
{
    const auto ctrl = static_cast<SensorController*>(pvTimerGetTimerID(timer));
    ctrl->queryTelemetries();
}

bool SensorController::begin()
{
    Wire.begin();
    xTimerStart(timer, 0);

    if (!I2CMasterHal::takeSemaphore())
    {
        Log.warningln("Sensor can not begin, can not have semaphore");

        LedController::getInstance().blink(Error, I2C);

        return false;
    }

    bool result = false;

    for (const auto sensor : sensors)
    {
        result |= sensor->begin();
    }

    I2CMasterHal::releaseSemaphore();

    if (result)
    {
        LedController::getInstance().blink(Success, Sensor);
    }

    return result;
}

bool SensorController::queryTelemetries()
{
    if (!isInitialized() && !begin())
    {
        Log.warningln("Sensor can not query, no one found");

        return false;
    }

    if (!I2CMasterHal::takeSemaphore())
    {
        Log.warningln("Sensor can not query, can not have semaphore");

        LedController::getInstance().blink(Error, I2C);

        return false;
    }

    bool result = false;

    for (const auto sensor : sensors)
    {
        result |= sensor->query(telemetry);
    }

    I2CMasterHal::releaseSemaphore();

    if (result)
    {
        telemetry.updatedAt = getDateTime().unixtime();

        LedController::getInstance().blink(Success, Sensor);

        printJson(serialJson);
        printJson(serial1Json);
        printJson(serial2Json);
    }

    return result;
}

void SensorController::printJson(StreamJson& streamJson)
{
    streamJson.jsonWriter.beginObject()
    .property("updatedAt", telemetry.updatedAt)

    .beginObject("battery")
        .property("voltage", telemetry.battery.voltage)
        .property("current", telemetry.battery.current)
    .endObject()

    .beginObject("board")
        .property("voltage", telemetry.board.voltage)
        .property("current", telemetry.board.current)
    .endObject()

    .beginObject("solar")
        .property("voltage", telemetry.solar.voltage)
        .property("current", telemetry.solar.current)
    .endObject()

    .beginObject("mppt")
        .beginObject("battery")
            .property("voltage", telemetry.mppt.battery.voltage)
            .property("current", telemetry.mppt.battery.current)
        .endObject()

        .beginObject("solar")
            .property("voltage", telemetry.mppt.solar.voltage)
            .property("current", telemetry.mppt.solar.current)
        .endObject()

        .property("batteryTemperature", telemetry.mppt.temperature)
    .endObject()

    .beginObject("box")
        .property("temperature", telemetry.box.temperature)
        .property("humidity", telemetry.box.humidity)
        .property("pressure", telemetry.box.pressure)
    .endObject()

    .beginObject("outdoor")
        .beginObject("basic")
            .property("temperature", telemetry.outdoor.basic.temperature)
            .property("humidity", telemetry.outdoor.basic.humidity)
            .property("pressure", telemetry.outdoor.basic.pressure)
        .endObject()

        .property("rain", telemetry.outdoor.rain)

        .beginObject("wind")
            .property("direction", telemetry.outdoor.wind.direction)
            .property("speedAverage", telemetry.outdoor.wind.speedAverage)
            .property("speedMax", telemetry.outdoor.wind.speedMax)
        .endObject()

        .beginObject("light")
            .property("uv", telemetry.outdoor.light.uv)
            .property("uvIndex", telemetry.outdoor.light.uvIndex)
            .property("lux", telemetry.outdoor.light.lux)
        .endObject()
    .endObject()

    .beginObject("position")
        .property("latitude", telemetry.position.latitude)
        .property("longitude", telemetry.position.longitude)
        .property("altitude", telemetry.position.altitude)
    .endObject()

.endObject();

    streamJson.stream.println();
}

bool SensorController::isInitialized() const
{
    bool initialized = false;

    for (const auto sensor : sensors)
    {
        initialized &= sensor->isInitialized();
    }

    return initialized;
}