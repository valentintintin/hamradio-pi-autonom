#pragma once

#include "BaseController.hpp"
#include "telemetry.h"

#include <timers.h>
#include <JsonWriter.h>

#include "hal/SensorHal.hpp"
#include "hal/Bme280Hal.hpp"
#include "hal/I2CSlaveHal.hpp"
#include "hal/Ina3221Hal.hpp"
#include "hal/MpptChargerHal.hpp"

#define QUERY_DELAY 30000 // TODO settings ?

struct StreamJson
{
    JsonWriter jsonWriter;
    Stream& stream;
};

class SensorController : public BaseController
{
public:
    static SensorController& getInstance()
    {
        static SensorController instance;
        return instance;
    }

    bool begin() override;
    bool queryTelemetries();
    void printJson(StreamJson& streamJson);

    bool isInitialized() const;

    static const Telemetry& getTelemetry()
    {
        return telemetry;
    }

    static void queryTimer(TimerHandle_t timer);

private:
    static Telemetry telemetry;

    TimerHandle_t timer;

    SensorHal* sensors[5]
    {
        &Bme280Hal::getInstance(),
        &Ina3221Hal::getInstance(),
        &MpptChargerHal::getInstance(),
        &I2CSlaveHal::getInstance()
    };

    StreamJson serialJson = {
        .jsonWriter = JsonWriter(&Serial),
        .stream = Serial
    };
    StreamJson serial1Json = {
        .jsonWriter = JsonWriter(&Serial1),
        .stream = Serial1
    };
    StreamJson serial2Json = {
        .jsonWriter = JsonWriter(&Serial2),
        .stream = Serial2
    };

    SensorController();
};
