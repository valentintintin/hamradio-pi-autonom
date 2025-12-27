#pragma once

#include <Arduino.h>

struct TelemetryPower
{
    float voltage;
    float current;
};

struct TelemetryBasic
{
    float temperature;
    float humidity;
    float pressure;
};

struct TelemetryWind
{
    uint16_t direction;
    float speedAverage;
    float speedMax;
};

struct TelemetryLight
{
    uint32_t uv;
    uint8_t uvIndex;
    float lux;
};

struct TelemetryOutdoor
{
    TelemetryBasic basic;
    float rain;
    TelemetryWind wind;
    TelemetryLight light;
};

struct Position
{
    double latitude;
    double longitude;
    uint16_t altitude;
};

struct Telemetry
{
    TelemetryPower battery;
    TelemetryPower solar;

    float batteryTemperature;

    TelemetryBasic box;
    TelemetryOutdoor outdoor;

    Position position;

    uint32_t updatedAt;
};