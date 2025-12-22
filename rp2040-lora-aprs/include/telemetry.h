#pragma once

#include <Arduino.h>

typedef struct
{
    float voltage;
    float current;
} TelemetryPower;

typedef struct
{
    float temperature;
    float humidity;
    float pressure;
} TelemetryBasic;

typedef struct
{
    uint16_t direction;
    float speedAverage;
    float speedMax;
} TelemetryWind;

typedef struct
{
    uint32_t uv;
    uint8_t uvIndex;
    float lux;
} TelemetryLight;

typedef struct
{
    TelemetryBasic basic;
    float outdoorRain;
    TelemetryWind wind;
    TelemetryLight light;
} TelemetryOutdoor;

typedef struct
{
    double latitude;
    double longitude;
    uint16_t altitude;
} Position;

typedef struct
{
    TelemetryPower battery;
    TelemetryPower solar;

    float batteryTemperature;

    TelemetryBasic box;
    TelemetryOutdoor outdoor;

    Position position;

    uint32_t updatedAt;
} Telemetry;