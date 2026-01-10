#pragma once

#include "settings.h"

#define SETTINGS_FILE_PATH "/config.dat"

class SettingsManager
{
public:
    static SettingsManager& getInstance()
    {
        static SettingsManager instance;
        return instance;
    }

    static const Settings& getSettings()
    {
        return getInstance().settings;
    }

    bool begin();
    bool loadSaved();
    void loadDefaults();
    bool saveSettings() const;

    void printSettings();

    bool getSettingFromString(const char *source, char* valueOut, size_t lengthValueOut);
    bool setSettingFromString(const char *source, const char* value);
private:
    Settings settings{};

    SettingsGetSetFunction settingsGetSetFunctions[29] = {
        { "version", UInt16, &settings.version },
        { "wdt", Boolean, &settings.useWatchdog },
        { "slowClock", Boolean, &settings.useSlowClock },
        { "i2cSlave", Boolean, &settings.i2cSlaveEnabled },
        { "gpio.enabled", Boolean, &settings.gpio[0].enabled, MAX_GPIO_USED, sizeof(SettingsGpio) },
        { "gpio.pin", UInt8, &settings.gpio[0].pin, MAX_GPIO_USED, sizeof(SettingsGpio) },
        { "gpio.mode", UInt8, &settings.gpio[0].mode, MAX_GPIO_USED, sizeof(SettingsGpio) },
        { "gpio.i2cAddress", UInt8, &settings.gpio[0].i2cAddress, MAX_GPIO_USED, sizeof(SettingsGpio) },
        { "gpio.inverted", Boolean, &settings.gpio[0].inverted, MAX_GPIO_USED, sizeof(SettingsGpio) },
        { "mppt.wdt.enabled", Boolean, &settings.mppt.watchdog.enabled },
        { "mppt.wdt.interval", UInt8, &settings.mppt.watchdog.intervalFeed },
        { "mppt.wdt.timeOff", UInt16, &settings.mppt.watchdog.timeOff },
        { "mppt.wdt.timeout", UInt8, &settings.mppt.watchdog.timeout },
        { "mppt.voltLimit.off", UInt16, &settings.mppt.powerOffVoltage },
        { "mppt.voltLimit.on", UInt16, &settings.mppt.powerOnVoltage },
        { "lora.tx", Boolean, &settings.lora.txEnabled },
        { "lora.tx", Boolean, &settings.lora.outputPower },
        { "lora.mode", UInt8, &settings.lora.mode },
        { "lora.modeLoop", UInt32, &settings.lora.intervalLoopMode },
        { "lora.wdt", Boolean, &settings.lora.watchdogRxEnabled },
        { "lora.wdt.timeout", UInt32, &settings.lora.intervalTimeoutWatchdogRx },
        { "lora.modem.enabled", Boolean, &settings.lora.modems[0].enabled, MAX_LORA_MODEM, sizeof(SettingsLoRaModem) },
        { "lora.modem.mode", UInt8, &settings.lora.modems[0].mode, MAX_LORA_MODEM, sizeof(SettingsLoRaModem) },
        { "lora.modem.freq", Float, &settings.lora.modems[0].frequency, MAX_LORA_MODEM, sizeof(SettingsLoRaModem) },
        { "lora.modem.bw", Float, &settings.lora.modems[0].bandwidth, MAX_LORA_MODEM, sizeof(SettingsLoRaModem) },
        { "lora.modem.sf", UInt8, &settings.lora.modems[0].spreadingFactor, MAX_LORA_MODEM, sizeof(SettingsLoRaModem) },
        { "lora.modem.cr", UInt8, &settings.lora.modems[0].codingRate, MAX_LORA_MODEM, sizeof(SettingsLoRaModem) },
        { "lora.modem.syncWord", UInt8, &settings.lora.modems[0].syncWord, MAX_LORA_MODEM, sizeof(SettingsLoRaModem) },
        { "lora.modem.preambleLength", UInt8, &settings.lora.modems[0].preambleLength, MAX_LORA_MODEM, sizeof(SettingsLoRaModem) },
    };

    void* getSettingsPointer(const SettingsGetSetFunction &settingFn, uint32_t index) const;
    uint8_t parseNameIndex(const char* source, char* name);
    size_t getElementSize(SettingsType type) const;
    void printSettings(const SettingsGetSetFunction &settingFn, uint32_t index = 0) const;
};
