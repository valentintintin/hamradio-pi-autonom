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

    SettingsGetSetFunction settingsGetSetFunctions[16] = {
        { "version", UInt16, &settings.version },
        { "wdt", Boolean, &settings.useWatchdog },
        { "slowClock", Boolean, &settings.useSlowClock },
        { "gpio.enabled", Boolean, &settings.gpio[0].enabled, MAX_GPIO_USED, sizeof(SettingsGpio) },
        { "gpio.pin", UInt8, &settings.gpio[0].pin, MAX_GPIO_USED, sizeof(SettingsGpio) },
        { "gpio.mode", UInt8, &settings.gpio[0].mode, MAX_GPIO_USED, sizeof(SettingsGpio) },
        { "gpio.i2cAddress", UInt8, &settings.gpio[0].i2cAddress, MAX_GPIO_USED, sizeof(SettingsGpio) },
        { "gpio.inverted", Boolean, &settings.gpio[0].inverted, MAX_GPIO_USED, sizeof(SettingsGpio) },
        { "i2cSlave.enabled", Boolean, &settings.i2c.address },
        { "i2cSlave.address", UInt8, &settings.i2c.address },
        { "mppt.wdt.enabled", Boolean, &settings.mppt.watchdog.enabled },
        { "mppt.wdt.interval", UInt8, &settings.mppt.watchdog.intervalFeed },
        { "mppt.wdt.timeOff", UInt16, &settings.mppt.watchdog.timeOff },
        { "mppt.wdt.timeout", UInt8, &settings.mppt.watchdog.timeout },
        { "mppt.voltLimit.off", UInt16, &settings.mppt.powerOffVoltage },
        { "mppt.voltLimit.on", UInt16, &settings.mppt.powerOnVoltage }
    };

    void* getSettingsPointer(const SettingsGetSetFunction &settingFn, uint32_t index) const;
    uint8_t parseNameIndex(const char* source, char* name);
    size_t getElementSize(SettingsType type) const;
    void printSettings(const SettingsGetSetFunction &settingFn, uint32_t index = 0) const;
};
