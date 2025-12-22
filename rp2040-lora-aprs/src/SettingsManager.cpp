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

bool SettingsManager::getSettingFromString(const char* source, char* value, const size_t lengthValue)
{
    for (const auto& settingFn : settingsGetSetFunctions)
    {
        char name[32 + 1]{};

        const auto index = parseNameIndex(source, name);

        if (strcmp(settingFn.name, name) != 0)
        {
            continue;
        }

        const auto pointer = getSettingsPointer(settingFn, index);

        Log.infoln("Read setting %s at address %X", settingFn.name, pointer);

        switch (settingFn.type) {
            case Boolean:
                snprintf(value, lengthValue, "%d", *static_cast<bool *>(pointer));
                return true;
            case Int8:
                snprintf(value, lengthValue, "%d", *static_cast<int8_t *>(pointer));
                return true;
            case Int16:
                snprintf(value, lengthValue, "%d", *static_cast<int16_t *>(pointer));
                return true;
            case Int32:
                snprintf(value, lengthValue, "%ld", *static_cast<int32_t *>(pointer));
                return true;
            case Int64:
                snprintf(value, lengthValue, "%lld", *static_cast<int64_t *>(pointer));
                return true;
            case UInt8:
                snprintf(value, lengthValue, "%u", *static_cast<uint8_t *>(pointer));
                return true;
            case UInt16:
                snprintf(value, lengthValue, "%u", *static_cast<uint16_t *>(pointer));
                return true;
            case UInt32:
                snprintf(value, lengthValue, "%lu", *static_cast<uint32_t *>(pointer));
                return true;
            case UInt64:
                snprintf(value, lengthValue, "%llu", *static_cast<uint64_t *>(pointer));
                return true;
            case Char:
                snprintf(value, lengthValue, "%c", *static_cast<char *>(pointer));
                return true;
            case Float:
                snprintf(value, lengthValue, "%f", *static_cast<float *>(pointer));
                return true;
            case Double:
                snprintf(value, lengthValue, "%lf", *static_cast<double *>(pointer));
                return true;
            case CharString:
                strncpy(value, static_cast<const char*>(pointer), min(settingFn.maxStringLength, lengthValue));
                return true;
            default:
                Log.warningln("Config key found but not readable");
                strncpy(value, "KO readable", lengthValue);
                return false;
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

// bool SettingsManager::setValue(const SettingsGetSetFunction& settingGetSetFn, uint32_t index, const void* value)
// {
//     switch (settingGetSetFn.type) {
//             case Boolean:
//                 *static_cast<bool *>(settingGetSetFn.pointer) = value['0'] == '1';
//             break;
//             case Int8:
//                 *static_cast<int8_t *>(settingGetSetFn.pointer) = static_cast<int8_t>(strtol(value, nullptr, 0));
//             break;
//             case Int16:
//                 *static_cast<int16_t *>(settingGetSetFn.pointer) = static_cast<int16_t>(strtol(value, nullptr, 0));
//             break;
//             case Int32:
//                 *static_cast<int32_t *>(settingGetSetFn.pointer) = strtol(value, nullptr, 0);
//             break;
//             case Int64:
//                 *static_cast<int64_t *>(settingGetSetFn.pointer) = strtoll(value, nullptr, 0);
//             break;
//             case UInt8:
//                 *static_cast<uint8_t *>(settingGetSetFn.pointer) = static_cast<uint8_t>(strtoul(value, nullptr, 0));
//             break;
//             case UInt16:
//                 *static_cast<uint16_t *>(settingGetSetFn.pointer) = static_cast<uint16_t>(strtoul(value, nullptr, 0));
//             break;
//             case UInt32:
//                 *static_cast<uint32_t *>(settingGetSetFn.pointer) = strtoul(value, nullptr, 0);
//             break;
//             case UInt64:
//                 *static_cast<uint64_t *>(settingGetSetFn.pointer) = strtoull(value, nullptr, 0);
//             break;
//             case Char:
//                 *static_cast<char *>(settingGetSetFn.pointer) = value[0];
//             break;
//             case Float:
//                 *static_cast<float *>(settingGetSetFn.pointer) = strtof(value, nullptr);
//             break;
//             case Double:
//                 *static_cast<double *>(settingGetSetFn.pointer) = strtod(value, nullptr);
//             break;
//             case CharString:
//                 strncpy(*static_cast<char* *>(settingGetSetFn.pointer), value, sizeof(*static_cast<char* *>(settingGetSetFn.pointer)));
//             break;
//             default:
//                 Log.warningln(F("[COMMAND] Config key found but not settable"));
//                 strncpy_P(response, "KO settable"), MyCommandParser::MAX_RESPONSE_SIZE);
//                 break;
//         }
//
//     return false;
// }

uint8_t SettingsManager::parseNameIndex(const char* source, char* name)
{
    const char* bracket = strchr(source, '[');

    // Pas d'index
    if (bracket == nullptr) {
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
