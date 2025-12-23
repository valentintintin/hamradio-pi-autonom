#pragma once
#include "settings.h"

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
    void loadDefaults();

    bool getSettingFromString(const char *source, char* valueOut, size_t lengthValueOut);
    bool setSettingFromString(const char *source, const char* value);
private:
    Settings settings{};

    SettingsGetSetFunction settingsGetSetFunctions[NB_SETTINGS] = {
        { "version", UInt16, &settings.version },
        { "slowClock", Boolean, &settings.useSlowClock },
        { "pin.pin", UInt8, &settings.pins[0].pin, MAX_GPIO_USED, sizeof(SettingsPin) },
        { "pin.mode", UInt8, &settings.pins[0].mode, MAX_GPIO_USED, sizeof(SettingsPin) },
        { "pin.i2cAddress", UInt8, &settings.pins[0].i2cAddress, MAX_GPIO_USED, sizeof(SettingsPin) },
        { "pin.inverted", Boolean, &settings.pins[0].inverted, MAX_GPIO_USED, sizeof(SettingsPin) },
        { "pin.name", CharString, &settings.pins[0].name, MAX_GPIO_USED, sizeof(SettingsPin), sizeof(decltype(SettingsPin::name)) },
        { "i2cSlave.enabled", Boolean, &settings.i2c.address },
        { "i2cSlave.address", UInt8, &settings.i2c.address },
    };

    void* getSettingsPointer(const SettingsGetSetFunction &settingFn, uint32_t index) const;
    uint8_t parseNameIndex(const char* source, char* name);
    size_t getElementSize(SettingsType type) const;
};
