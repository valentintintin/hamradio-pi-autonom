#include "SettingsManager.hpp"

#include <ArduinoLog.h>
#include <LittleFS.h>
#include <bits/ios_base.h>

#include "controllers/LedController.hpp"

bool SettingsManager::begin()
{
    if (!LittleFS.begin())
    {
        Log.warningln("Impossible to mount FS, format");

        if (!LittleFS.format())
        {
            Log.errorln("Impossible to format FS");

            LedController::getInstance().blink(Error, Settings);

            return false;
        }

        if (!LittleFS.begin())
        {
            Log.errorln("Impossible to mount FS event after format");

            LedController::getInstance().blink(Error, Settings);

            return false;
        }
    }

    loadSaved();

    printSettings();

    return true;
}

bool SettingsManager::loadSaved()
{
    File file = LittleFS.open(SETTINGS_FILE_PATH, "r");
    if (!file) {
        Log.warningln("Fail to open settings, use default one");

        loadDefaults();

        LedController::getInstance().blink(Error, Settings);

        return false;
    }

    file.read(reinterpret_cast<uint8_t *>(&settings), sizeof(settings));
    file.close();

    Log.infoln("Settings read correctly");

    LedController::getInstance().blink(Success, Settings);

    return true;
}

void SettingsManager::loadDefaults()
{
    uint8_t i = 0;

    settings.gpio[i].enabled = true;
    settings.gpio[i++].pin = 11;

    settings.gpio[i].enabled = true;
    settings.gpio[i++].pin = 12;

    settings.gpio[i].enabled = true;
    settings.gpio[i++].pin = 10;
}

bool SettingsManager::saveSettings() const
{
    File file = LittleFS.open(SETTINGS_FILE_PATH, "w");
    if (!file) {
        Log.errorln("Fail to save settings");

        LedController::getInstance().blink(Error, Settings);

        return false;
    }

    file.write(reinterpret_cast<const uint8_t *>(&settings), sizeof(settings));
    file.close();

    Log.infoln("Saved settings to FS");

    LedController::getInstance().blink(Success, Settings);

    return true;
}

void SettingsManager::printSettings()
{
    for (const auto &config : settingsGetSetFunctions) {
        if (config.pointer == 0)
        {
            continue;
        }

        if (config.maxSize == 1)
        {
            printSettings(config);
        }
        else
        {
            for (uint32_t i = 0; i < config.maxSize; i++)
            {
                printSettings(config, i);
            }
        }
    }

    FSInfo info;
    LittleFS.info(info);

    Log.infoln("FS Used/Total: %u/%u", info.usedBytes, info.totalBytes);
}

bool SettingsManager::getSettingFromString(const char* source, char* valueOut, const size_t lengthValueOut)
{
    for (const auto& settingFn : settingsGetSetFunctions)
    {
        char name[NAME_SETTING_LENGTH + 1]{};

        const auto index = parseNameIndex(source, name);

        if (strcmp(settingFn.name, name) != 0)
        {
            continue;
        }

        const auto pointer = getSettingsPointer(settingFn, index);

        Log.infoln("Read setting %s at address %X", settingFn.name, index, pointer);

        switch (settingFn.type)
        {
        case Boolean:
            snprintf(valueOut, lengthValueOut, "%d", *static_cast<bool*>(pointer));
            return true;
        case Int8:
            snprintf(valueOut, lengthValueOut, "%d", *static_cast<int8_t*>(pointer));
            return true;
        case Int16:
            snprintf(valueOut, lengthValueOut, "%d", *static_cast<int16_t*>(pointer));
            return true;
        case Int32:
            snprintf(valueOut, lengthValueOut, "%ld", *static_cast<int32_t*>(pointer));
            return true;
        case Int64:
            snprintf(valueOut, lengthValueOut, "%lld", *static_cast<int64_t*>(pointer));
            return true;
        case UInt8:
            snprintf(valueOut, lengthValueOut, "%u", *static_cast<uint8_t*>(pointer));
            return true;
        case UInt16:
            snprintf(valueOut, lengthValueOut, "%u", *static_cast<uint16_t*>(pointer));
            return true;
        case UInt32:
            snprintf(valueOut, lengthValueOut, "%lu", *static_cast<uint32_t*>(pointer));
            return true;
        case UInt64:
            snprintf(valueOut, lengthValueOut, "%llu", *static_cast<uint64_t*>(pointer));
            return true;
        case Char:
            snprintf(valueOut, lengthValueOut, "%c", *static_cast<char*>(pointer));
            return true;
        case Float:
            snprintf(valueOut, lengthValueOut, "%f", *static_cast<float*>(pointer));
            return true;
        case Double:
            snprintf(valueOut, lengthValueOut, "%lf", *static_cast<double*>(pointer));
            return true;
        case CharString:
            strncpy(valueOut, static_cast<const char*>(pointer), min(settingFn.maxStringLength, lengthValueOut));
            return true;
        default:
            Log.warningln("Config key found but not readable");
            strncpy(valueOut, "KO readable", lengthValueOut);
            return false;
        }
    }

    return false;
}

bool SettingsManager::setSettingFromString(const char* source, const char* value)
{
    for (const auto& settingFn : settingsGetSetFunctions)
    {
        char name[NAME_SETTING_LENGTH + 1]{};

        const auto index = parseNameIndex(source, name);

        if (strcmp(settingFn.name, name) != 0)
        {
            continue;
        }

        const auto pointer = getSettingsPointer(settingFn, index);

        Log.infoln("Set setting %s at address %X with value %s", settingFn.name, index, pointer, value);

        switch (settingFn.type)
        {
        case Boolean:
            *static_cast<bool*>(pointer) = value['0'] == '1';
            return true;
        case Int8:
            *static_cast<int8_t*>(pointer) = static_cast<int8_t>(strtol(value, nullptr, 0));
            return true;
        case Int16:
            *static_cast<int16_t*>(pointer) = static_cast<int16_t>(strtol(value, nullptr, 0));
            return true;
        case Int32:
            *static_cast<int32_t*>(pointer) = strtol(value, nullptr, 0);
            return true;
        case Int64:
            *static_cast<int64_t*>(pointer) = strtoll(value, nullptr, 0);
            return true;
        case UInt8:
            *static_cast<uint8_t*>(pointer) = static_cast<uint8_t>(strtoul(value, nullptr, 0));
            return true;
        case UInt16:
            *static_cast<uint16_t*>(pointer) = static_cast<uint16_t>(strtoul(value, nullptr, 0));
            return true;
        case UInt32:
            *static_cast<uint32_t*>(pointer) = strtoul(value, nullptr, 0);
            return true;
        case UInt64:
            *static_cast<uint64_t*>(pointer) = strtoull(value, nullptr, 0);
            return true;
        case Char:
            *static_cast<char*>(pointer) = value[0];
            return true;
        case Float:
            *static_cast<float*>(pointer) = strtof(value, nullptr);
            return true;
        case Double:
            *static_cast<double*>(pointer) = strtod(value, nullptr);
            return true;
        case CharString:
            strncpy(static_cast<char*>(pointer), value, settingFn.maxStringLength);
            return true;
        default:
            Log.warningln("Config key found but not writable");
            break;
        }
    }

    return false;
}

void* SettingsManager::getSettingsPointer(const SettingsGetSetFunction& settingFn, const uint32_t index) const
{
    const auto base = static_cast<uint8_t*>(settingFn.pointer);

    if (settingFn.maxSize <= 1)
    {
        return base;
    }

    return base + index * settingFn.parentSize;
}

uint8_t SettingsManager::parseNameIndex(const char* source, char* name)
{
    // On prend la chaîne jusqu'à [
    const char* bracket = strchr(source, '[');

    // Pas de [
    if (bracket == nullptr)
    {
        strcpy(name, source);
        return 0;
    }

    // Nom avant '['
    const size_t nameLen = bracket - source;
    memcpy(name, source, nameLen);
    name[nameLen] = '\0';

    // Index entre '[' et ']'
    return atoi(bracket + 1);
}


size_t SettingsManager::getElementSize(SettingsType type) const
{
    switch (type)
    {
    case Boolean:
    case Int8:
    case UInt8:
    case Char:
    case CharString:
        return 1;
    case Int16:
    case UInt16:
        return 2;
    case Int32:
    case UInt32:
    case Float:
        return 4;
    case Int64:
    case UInt64:
    case Double:
        return 8;
    default:
        Log.errorln("Type not found %d", type);
        return 1;
    }
}

void SettingsManager::printSettings(const SettingsGetSetFunction& settingFn, const uint32_t index) const
{
    const auto pointer = getSettingsPointer(settingFn, index);

    switch (settingFn.type) {
    case Boolean:
        Log.traceln("Settings: %s[%d] = %T", settingFn.name, index, static_cast<bool *>(pointer));
        break;
    case Int8:
        Log.traceln("Settings: %s[%d] = %d", settingFn.name, index, *static_cast<int8_t *>(pointer));
        break;
    case Int16:
        Log.traceln("Settings: %s[%d] = %u", settingFn.name, index, *static_cast<int16_t *>(pointer));
        break;
    case Int32:
        Log.traceln("Settings: %s[%d] = %u", settingFn.name, index, *static_cast<int32_t *>(pointer));
        break;
    case Int64:
        Log.traceln("Settings: %s[%d] = %u", settingFn.name, index, *static_cast<int64_t *>(pointer));
        break;
    case UInt8:
        Log.traceln("Settings: %s[%d] = %d", settingFn.name, index, *static_cast<uint8_t *>(pointer));
        break;
    case UInt16:
        Log.traceln("Settings: %s[%d] = %u", settingFn.name, index, *static_cast<uint16_t *>(pointer));
        break;
    case UInt32:
        Log.traceln("Settings: %s[%d] = %u", settingFn.name, index, *static_cast<uint32_t *>(pointer));
        break;
    case UInt64:
        Log.traceln("Settings: %s[%d] = %u", settingFn.name, index, *static_cast<uint64_t *>(pointer));
        break;
    case Char:
        Log.traceln("Settings: %s[%d] = %c", settingFn.name, index, *static_cast<char *>(pointer));
        break;
    case Float:
        Log.traceln("Settings: %s[%d] = %F", settingFn.name, index, *static_cast<float *>(pointer));
        break;
    case Double:
        Log.traceln("Settings: %s[%d] = %D", settingFn.name, index, *static_cast<double *>(pointer));
        break;
    case CharString:
        Log.traceln("Settings: %s[%d] = %s", settingFn.name, index, static_cast<char*>(pointer));
        break;
    default:
        Log.traceln("Settings: %s[%d] = not implemented", settingFn.name, index);
        break;
    }
}
