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

    bool getSettingFromString(const char *source, char* value, size_t lengthValue);
private:
    // bool initialized = false;
    Settings settings{};

    SettingsGetSetFunction settingsGetSetFunctions[NB_SETTINGS] = {
        { "version", UInt16, &settings.version },
        { "slowClock", Boolean, &settings.useSlowClock },
        { "pin.pin", UInt8, &settings.pins[0].pin, MAX_GPIO_USED, sizeof(SettingsPin) },
        { "pin.mode", UInt8, &settings.pins[0].mode, MAX_GPIO_USED, sizeof(SettingsPin) },
        { "pin.i2cAddress", UInt8, &settings.pins[0].i2cAddress, MAX_GPIO_USED, sizeof(SettingsPin) },
        { "pin.inverted", Boolean, &settings.pins[0].inverted, MAX_GPIO_USED, sizeof(SettingsPin) },
        { "pin.name", CharString, &settings.pins[0].name, MAX_GPIO_USED, sizeof(SettingsPin), sizeof(settings.pins[0].name) },
    };

    void* getSettingsPointer(const SettingsGetSetFunction &settingFn, uint32_t index) const;
    // bool setValue(const SettingsGetSetFunction &settingGetSetFn, uint32_t index, const void *value);
    uint8_t parseNameIndex(const char* source, char* name);
    size_t getElementSize(SettingsType type) const;
};
