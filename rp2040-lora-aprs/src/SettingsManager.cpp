#include "SettingsManager.hpp"

#include <ArduinoLog.h>
#include <LittleFS.h>
#include <bits/ios_base.h>

bool SettingsManager::begin()
{
    LittleFS.begin();

    loadDefaults();

    return true;
}

void SettingsManager::loadDefaults()
{
    uint8_t i = 0;

    settings.pins[i].pin = 11;
    strcpy(settings.pins[i++].name, "wifi");

    settings.pins[i].pin = 12;
    strcpy(settings.pins[i++].name, "linux");

    settings.pins[i].pin = 10;
    strcpy(settings.pins[i++].name, "msh");
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

        Log.infoln("Read setting %s at address %X", settingFn.name, pointer);

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

        Log.infoln("Set setting %s at address %X with value %s", settingFn.name, pointer, value);

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
