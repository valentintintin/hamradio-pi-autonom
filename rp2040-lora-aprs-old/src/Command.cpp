#include <stdlib.h>
#include "ArduinoLog.h"

#include "Command.h"
#include "System.h"
#include "../../include/utils/utils.h"
#include "Threads/Energy/EnergyMpptChgThread.h"
#include "I2CSlave.h"
#include "Threads/Energy/EnergyIna3221Thread.h"

System* Command::system;
SettingsAprsCallsignHeard* Command::sortedAprsHeard[APRS_CALLSIGNS_HEARD_NUMBER];

Command::Command(System *system) {
    Command::system = system;

    parser.registerCommand(PSTR("position"), PSTR(""), doPosition);
    parser.registerCommand(PSTR("telem"), PSTR(""), doTelemetry);
    parser.registerCommand(PSTR("telemParams"), PSTR(""), doTelemetryParams);
    parser.registerCommand(PSTR("status"), PSTR(""), doStatus);
    parser.registerCommand(PSTR("lora"), PSTR("s"), doLora);
    parser.registerCommand(PSTR("reboot"), PSTR(""), doReboot);
    parser.registerCommand(PSTR("dfu"), PSTR(""), doDfu);
    parser.registerCommand(PSTR("json"), PSTR(""), doPrintJson);
    parser.registerCommand(PSTR("ping"), PSTR(""), doPing);
    parser.registerCommand(PSTR("gpio"), PSTR("su"), doGpioOutput);
    parser.registerCommand(PSTR("set"), PSTR("ss"), doSetSetting);
    parser.registerCommand(PSTR("get"), PSTR("s"), doGetSetting);

    if (system->settings.energy.type == mpptchg) {
        parser.registerCommand(PSTR("mpptDog"), PSTR("u"), doMpptWatchdog);
    }

    parser.registerCommand(PSTR("objMsh"), PSTR(""), doMeshtasticAprs);
    parser.registerCommand(PSTR("objLinux"), PSTR(""), doLinuxAprs);
    parser.registerCommand(PSTR("box"), PSTR(""), doGetBoxInfo);
    parser.registerCommand(PSTR("error"), PSTR(""), doGetError);
    parser.registerCommand(PSTR("setLoraMode"), PSTR("duuuu"), doSetLora);
    parser.registerCommand(PSTR("sleepLinux"), PSTR("u"), doSleepLinux);

    if (system->settings.i2c.enabled) {
        parser.registerCommand(PSTR("cmdMsh"), PSTR("s"), doCommandMeshtastic);
        parser.registerCommand(PSTR("cmdRepMsh"), PSTR(""), doGetCommandResponseFromMeshtastic);
    }

    parser.registerCommand(PSTR("?APRS?"), PSTR(""), doAprsQueryHelp);
    parser.registerCommand(PSTR("?APRSP"), PSTR(""), doPosition);
    parser.registerCommand(PSTR("?APRSD"), PSTR(""), doAprsHeardWithoutDigi);
    parser.registerCommand(PSTR("?APRSL"), PSTR(""), doAprsHeard);
    parser.registerCommand(PSTR("?APRSH"), PSTR("s"), doAprsHeardSomeone);
    parser.registerCommand(PSTR("?APRSV"), PSTR(""), doAbout);
    parser.registerCommand(PSTR("?PING"), PSTR(""), doAprsPing);
}

bool Command::processCommand(Stream* stream, const char *command) {
    if (strlen(command) < 3) {
        Log.traceln(F("[COMMAND] Command received length %d : %s"), strlen(command), command);
        return false;
    }

    Log.traceln(F("[COMMAND] Process : %s"), command);

    if (!parser.processCommand(command, response)) {
        if (stream != nullptr) {
            stream->print(F("KO "));
            stream->println(response);
        }

        Log.warningln(F("[COMMAND] %s KO (%s)"), command, response);

        return false;
    }

    if (stream != nullptr && strlen(response)) {
        stream->println(response);
    }

    Log.infoln(F("[COMMAND] %s OK (%s)"), command, response);

    return true;
}

void Command::doTelemetry(MyCommandParser::Argument *args, char *response) {
    system->sendTelemetriesThread->forceRun();

    strncpy_P(response, PSTR("OK"), MyCommandParser::MAX_RESPONSE_SIZE);
}

void Command::doPosition(MyCommandParser::Argument *args, char *response) {
    system->sendPositionThread->forceRun();

    strncpy_P(response, PSTR("OK"), MyCommandParser::MAX_RESPONSE_SIZE);
}

void Command::doTelemetryParams(MyCommandParser::Argument *args, char *response) {
    system->communication.shouldSendTelemetryParams = true;
    system->sendTelemetriesThread->forceRun();

    strncpy_P(response, PSTR("OK"), MyCommandParser::MAX_RESPONSE_SIZE);
}

void Command::doStatus(MyCommandParser::Argument *args, char *response) {
    system->sendStatusThread->forceRun();

    strncpy_P(response, PSTR("OK"), MyCommandParser::MAX_RESPONSE_SIZE);
}

void Command::doLora(MyCommandParser::Argument *args, char *response) {
    const char *raw = args[0].asString;

    const bool ok = system->radio.send(reinterpret_cast<const uint8_t *>(raw), strlen(raw));

    strncpy_P(response, ok ? PSTR("OK") : PSTR("KO"), MyCommandParser::MAX_RESPONSE_SIZE);
}

void Command::doReboot(MyCommandParser::Argument *args, char *response) {
    system->planReboot();

    strncpy_P(response, PSTR("OK"), MyCommandParser::MAX_RESPONSE_SIZE);
}

void Command::doDfu(MyCommandParser::Argument *args, char *response) {
    system->planDfu();

    strncpy_P(response, PSTR("OK"), MyCommandParser::MAX_RESPONSE_SIZE);
}

void Command::doGpioOutput(MyCommandParser::Argument *args, char *response) {
    const auto what = args[0].asString;

    if (!changeGpio(args[0].asString, args[1].asUInt64)) {
        Log.warningln(F("[COMMAND_GPIO] Gpio %s not found"), what);
        strncpy_P(response, PSTR("KO"), MyCommandParser::MAX_RESPONSE_SIZE);
        return;
    }

    strncpy_P(response, PSTR("OK"), MyCommandParser::MAX_RESPONSE_SIZE);
}

void Command::doSetSetting(MyCommandParser::Argument *args, char *response) {
    char *key = args[0].asString;
    char *value = args[1].asString;

    if (strlen(value) == 0) {
        Log.warningln(F("[COMMAND] Set %s to nothing impossible"), key);
        strncpy_P(response, PSTR("KO"), MyCommandParser::MAX_RESPONSE_SIZE);
        return;
    }

    bool ok = false;
    bool shouldReboot = false;

    Log.infoln(F("[COMMAND] Set %s to %s"), key, value);


    for (const auto &config : system->settingsGetSetFunctions) {
        if (strcmp(key, config.name) != 0) {
            continue;
        }

        switch (config.type) {
            case Boolean:
                *static_cast<bool *>(config.pointer) = value['0'] == '1';
            break;
            case Int8:
                *static_cast<int8_t *>(config.pointer) = static_cast<int8_t>(strtol(value, nullptr, 0));
            break;
            case Int16:
                *static_cast<int16_t *>(config.pointer) = static_cast<int16_t>(strtol(value, nullptr, 0));
            break;
            case Int32:
                *static_cast<int32_t *>(config.pointer) = strtol(value, nullptr, 0);
            break;
            case Int64:
                *static_cast<int64_t *>(config.pointer) = strtoll(value, nullptr, 0);
            break;
            case UInt8:
                *static_cast<uint8_t *>(config.pointer) = static_cast<uint8_t>(strtoul(value, nullptr, 0));
            break;
            case UInt16:
                *static_cast<uint16_t *>(config.pointer) = static_cast<uint16_t>(strtoul(value, nullptr, 0));
            break;
            case UInt32:
                *static_cast<uint32_t *>(config.pointer) = strtoul(value, nullptr, 0);
            break;
            case UInt64:
                *static_cast<uint64_t *>(config.pointer) = strtoull(value, nullptr, 0);
            break;
            case Char:
                *static_cast<char *>(config.pointer) = value[0];
            break;
            case Float:
                *static_cast<float *>(config.pointer) = strtof(value, nullptr);
            break;
            case Double:
                *static_cast<double *>(config.pointer) = strtod(value, nullptr);
            break;
            case CharString:
                strncpy(*static_cast<char* *>(config.pointer), value, sizeof(*static_cast<char* *>(config.pointer)));
            break;
            default:
                Log.warningln(F("[COMMAND] Config key found but not settable"));
                strncpy_P(response, PSTR("KO settable"), MyCommandParser::MAX_RESPONSE_SIZE);
                break;
        }

        ok = true;
        shouldReboot = true;
    }

    if (!ok) {
        if (strcmp_P(key, PSTR("time")) == 0) {
            ok = true;

            const auto epoch = strtoul(value, nullptr, 0) + 15; // Add 15 seconds (command typing time)

            if (system->settings.rtc.enabled) {
                system->rtc.setEpoch(epoch, true);
            }

            system->setTimeToInternalRtc(epoch);
        } else if (strcmp_P(key, PSTR("reset")) == 0) {
            if (strcmp_P(value, PSTR("settings")) == 0) {
                ok = system->resetSettings();
            } else if (strcmp_P(value, PSTR("aprs")) == 0) {
                ok = system->resetAprsReceived();
            } else if (strcmp_P(value, PSTR("all")) == 0) {
                ok = system->resetEverything();
            } else {
                Log.warningln(F("[COMMAND] Reset value not found"));
            }
            shouldReboot = ok;
        } else if (strcmp_P(key, PSTR("aprsReceived")) == 0) {
            ok = true;
            for (auto &[callsign, time, rssi, snr, content, count, digipeaterCallsign, digipeaterCount] : system->aprsReceived) {
                callsign[0] = '\0';
                content[0] = '\0';
                time = 0;
                rssi = 0;
                snr = 0;
                count = 0;
                digipeaterCount = 0;
                digipeaterCallsign[0] = '\0';
            }
        } else {
            Log.warningln(F("[COMMAND] Config key not found"));
        }
    }

    if (ok) {
        ok = system->saveSettings();

        if (ok) {
            strncpy_P(response, PSTR("Set OK: "), MyCommandParser::MAX_RESPONSE_SIZE);

            doGetSetting(args, response + strlen(response)); // Can have overflow here

            if (shouldReboot) {
                strncat_P(response, PSTR(" Reboot"), MyCommandParser::MAX_RESPONSE_SIZE - strlen(response));
            }

            return;
        }
    }

    strncpy_P(response, PSTR("KO"), MyCommandParser::MAX_RESPONSE_SIZE);
}

void Command::doGetSetting(MyCommandParser::Argument *args, char *response) {
    const char *key = args[0].asString;

    for (const auto &config : system->settingsGetSetFunctions) {
        if (strcmp(key, config.name) != 0) {
            continue;
        }

        switch (config.type) {
            case Boolean:
                snprintf_P(response, MyCommandParser::MAX_RESPONSE_SIZE, PSTR("%d"), *static_cast<bool *>(config.pointer));
            break;
            case Int8:
                snprintf_P(response, MyCommandParser::MAX_RESPONSE_SIZE, PSTR("%d"), *static_cast<int8_t *>(config.pointer));
            break;
            case Int16:
                snprintf_P(response, MyCommandParser::MAX_RESPONSE_SIZE, PSTR("%d"), *static_cast<int16_t *>(config.pointer));
            break;
            case Int32:
                snprintf_P(response, MyCommandParser::MAX_RESPONSE_SIZE, PSTR("%d"), *static_cast<int32_t *>(config.pointer));
            break;
            case Int64:
                snprintf_P(response, MyCommandParser::MAX_RESPONSE_SIZE, PSTR("%lld"), *static_cast<int64_t *>(config.pointer));
            break;
            case UInt8:
                snprintf_P(response, MyCommandParser::MAX_RESPONSE_SIZE, PSTR("%u"), *static_cast<uint8_t *>(config.pointer));
            break;
            case UInt16:
                snprintf_P(response, MyCommandParser::MAX_RESPONSE_SIZE, PSTR("%u"), *static_cast<uint16_t *>(config.pointer));
            break;
            case UInt32:
                snprintf_P(response, MyCommandParser::MAX_RESPONSE_SIZE, PSTR("%u"), *static_cast<uint32_t *>(config.pointer));
            break;
            case UInt64:
                snprintf_P(response, MyCommandParser::MAX_RESPONSE_SIZE, PSTR("%llu"), *static_cast<uint64_t *>(config.pointer));
            break;
            case Char:
                snprintf_P(response, MyCommandParser::MAX_RESPONSE_SIZE, PSTR("%c"), *static_cast<char *>(config.pointer));
            break;
            case Float:
                snprintf_P(response, MyCommandParser::MAX_RESPONSE_SIZE, PSTR("%f"), *static_cast<float *>(config.pointer));
            break;
            case Double:
                snprintf_P(response, MyCommandParser::MAX_RESPONSE_SIZE, PSTR("%lf"), *static_cast<double *>(config.pointer));
            break;
            case CharString:
                snprintf_P(response, MyCommandParser::MAX_RESPONSE_SIZE, PSTR("%s"), *static_cast<char* *>(config.pointer));
            break;
            default:
                Log.warningln(F("[COMMAND] Config key found but not printable"));
                strncpy_P(response, PSTR("KO printable"), MyCommandParser::MAX_RESPONSE_SIZE);
                break;
        }

        return;
    }

    if (strcmp_P(key, PSTR("time")) == 0 || strcmp_P(key, PSTR("now")) == 0) {
        getDateTimeStringFromEpoch(system->getDateTime().unixtime(), response, MyCommandParser::MAX_RESPONSE_SIZE);
    } else if (strcmp_P(key, PSTR("info")) == 0) {
        snprintf_P(response, MyCommandParser::MAX_RESPONSE_SIZE, PSTR("Wc:%d Uptime:%ld Head:%d Stack:%d"), watchdog_enable_caused_reboot(), millis() / 1000, rp2040.getFreeHeap(), rp2040.getFreeStack());
    } else if (strcmp_P(key, PSTR("all")) == 0) {
        system->printSettingsAndAprsReceived();
        strncpy_P(response, PSTR("OK"), MyCommandParser::MAX_RESPONSE_SIZE);
    } else if (strcmp_P(key, PSTR("reset")) == 0) {
        // Ignored: case when set command with reset
    } else {
        Log.warningln(F("[COMMAND] Config key not found"));
        strncpy_P(response, PSTR("KO"), MyCommandParser::MAX_RESPONSE_SIZE);
    }
}

void Command::doPrintJson(MyCommandParser::Argument *args, char *response) {
    system->energyThread->run();

    if (system->weatherThread->enabled) {
        system->weatherThread->run();
    }

    system->printJson(false);
    system->printJson(true);

    strncpy_P(response, PSTR(""), MyCommandParser::MAX_RESPONSE_SIZE);
}

void Command::doPing(MyCommandParser::Argument *args, char *response) {
    Log.infoln(F("Pong !"));
    strncpy_P(response, PSTR("Pong!"), MyCommandParser::MAX_RESPONSE_SIZE);
}

void Command::doMpptWatchdog(MyCommandParser::Argument *args, char *response) {
    const uint64_t timeOff = args[0].asUInt64;

    const bool ok = system->settings.energy.type == mpptchg && system->watchdogSlaveMpptChgThread->setManagedByUser(timeOff);

    strncpy_P(response, ok ? PSTR("OK") : PSTR("KO"), MyCommandParser::MAX_RESPONSE_SIZE);
}

void Command::doMeshtasticAprs(MyCommandParser::Argument *args, char *response) {
    system->sendMeshtasticAprsThread->forceRun();

    strncpy_P(response, PSTR("OK"), MyCommandParser::MAX_RESPONSE_SIZE);
}

void Command::doLinuxAprs(MyCommandParser::Argument *args, char *response) {
    system->sendLinuxAprsThread->forceRun();

    strncpy_P(response, PSTR("OK"), MyCommandParser::MAX_RESPONSE_SIZE);
}

void Command::doGetBoxInfo(MyCommandParser::Argument *args, char *response) {
    int16_t rawTemperatureBattery = 0;

    if (system->settings.energy.type == mpptchg && !system->mpptChgCharger.getIndexedValue(VAL_INT_TEMP, &rawTemperatureBattery)) {
        Log.warningln(F("[COMMAND] Impossible to get MPPT Temperature"));
    }

    const auto temperatureBattery = system->settings.energy.type == mpptchg && !system->energyThread->hasError() ? rawTemperatureBattery / 10.0 : 0;
    const auto temperatureRtc = system->settings.rtc.enabled ? system->rtc.getTemperature() : 0;

    snprintf_P(response, MyCommandParser::MAX_RESPONSE_SIZE, PSTR("RTC: %.2f°C | Bat: %.2f°C"), temperatureRtc, temperatureBattery);
}

void Command::doGetError(MyCommandParser::Argument *args, char *response) {
    snprintf_P(response, MyCommandParser::MAX_RESPONSE_SIZE, PSTR("RTC: %d | Energy: %d | Weather: %d | LoRa : %d"), system->isRtcHasError(), system->energyThread->hasError(), system->weatherThread->hasError(), system->radio.hasError());
}

// TODO reset to normal operation
void Command::doSetLora(MyCommandParser::Argument *args, char *response) {
    const auto frequency = args[0].asDouble;
    const auto bandwidth = args[1].asUInt64;
    const auto spreadingFactor = args[2].asUInt64;
    const auto codingRate = args[3].asUInt64;
    const auto outputPower = args[4].asUInt64;

    bool ok = system->radio.changeLoRaSettings(frequency, bandwidth, spreadingFactor, codingRate, outputPower);

    strncpy_P(response, ok ? PSTR("OK") : PSTR("KO"), MyCommandParser::MAX_RESPONSE_SIZE);
}

void Command::doSleepLinux(MyCommandParser::Argument *args, char *response) {
    const auto duration = args[0].asUInt64;

    system->watchdogLinux->sleep(duration * 1000);
    system->watchdogLinux->forceRun();

    strncpy_P(response, PSTR("OK"), MyCommandParser::MAX_RESPONSE_SIZE);
}

void Command::doCommandMeshtastic(MyCommandParser::Argument *args, char *response) {
    if (!system->settings.i2c.enabled) {
        strncpy_P(response, PSTR("KO"), MyCommandParser::MAX_RESPONSE_SIZE);
        return;
    }

    I2CSlave::sendCommandToMaster(args[0].asString);
    strncpy_P(response, PSTR("OK"), MyCommandParser::MAX_RESPONSE_SIZE);
}

void Command::doGetCommandResponseFromMeshtastic(MyCommandParser::Argument *args, char *response) {
    if (!system->settings.i2c.enabled) {
        strncpy_P(response, PSTR("KO"), MyCommandParser::MAX_RESPONSE_SIZE);
        return;
    }

    snprintf_P(response, MyCommandParser::MAX_RESPONSE_SIZE, PSTR("OK: %s"), I2CSlave::commandResponseFromMaster);
    I2CSlave::commandResponseFromMaster[0] = '\0';
}

void Command::doAprsQueryHelp(MyCommandParser::Argument *args, char *response) {
    strncpy_P(response, PSTR("?APRSP ?APRSD ?APRSL ?APRSH CALL ?APRSV ?PING"), MyCommandParser::MAX_RESPONSE_SIZE);
}

// ?APRSD
void Command::doAprsHeardWithoutDigi(MyCommandParser::Argument *args, char *response) {
    sortAprsHeard();

    for (const auto heard : sortedAprsHeard) {
        if (strlen(response) >= MyCommandParser::MAX_RESPONSE_SIZE - 10) {
            return;
        }

        if (strlen(heard->callsign) > 0 && strlen(heard->content) > 0 && strlen(heard->digipeaterCallsign) == 0 && heard->digipeaterCount == 0) {
            if (strlen(response) > 0) {
                strncat_P(response, PSTR(" "), MyCommandParser::MAX_RESPONSE_SIZE - strlen(response));
            }

            strncat_P(response, heard->callsign, MyCommandParser::MAX_RESPONSE_SIZE - strlen(response));
        }
    }

    if (strlen(response) == 0) {
        snprintf_P(response, MyCommandParser::MAX_RESPONSE_SIZE, PSTR("Personne entendu"));
    }
}

// ?APRSL
void Command::doAprsHeard(MyCommandParser::Argument *args, char *response) {
    sortAprsHeard();

    for (const auto heard : sortedAprsHeard) {
        if (strlen(response) >= MyCommandParser::MAX_RESPONSE_SIZE - 10) {
            return;
        }

        if (strlen(heard->callsign) > 0 && strlen(heard->content) > 0) {
            if (strlen(response) > 0) {
                strncat_P(response, PSTR(" "), MyCommandParser::MAX_RESPONSE_SIZE - strlen(response));
            }

            snprintf_P(response + strlen(response), MyCommandParser::MAX_RESPONSE_SIZE - strlen(response), PSTR("%s(%d)"), heard->callsign, heard->digipeaterCount);
        }
    }

    if (strlen(response) == 0) {
        snprintf_P(response, MyCommandParser::MAX_RESPONSE_SIZE, PSTR("Personne entendu"));
    }
}

// ?APRSH CALL
void Command::doAprsHeardSomeone(MyCommandParser::Argument *args, char *response) {
    for (auto &[callsign, time, rssi, snr, content, count, digipeaterCallsign, digipeaterCount] : system->aprsReceived) {
        if (strcasecmp(callsign, args[0].asString) == 0) {
            getDateTimeStringFromEpoch(time, bufferText, BUFFER_LENGTH);
            snprintf_P(response, MyCommandParser::MAX_RESPONSE_SIZE, PSTR("%s\n%s\nSNR:%.2f RSSI:%.2f\nDigi:%d Last:%s\nCount:%llu"), callsign, bufferText, snr, rssi, digipeaterCount, digipeaterCallsign, count);
            return;
        }
    }

    snprintf_P(response, MyCommandParser::MAX_RESPONSE_SIZE, PSTR("%s pas entendu"), args[0].asString);
}

void Command::doAbout(MyCommandParser::Argument *args, char *response) {
    snprintf_P(response, MyCommandParser::MAX_RESPONSE_SIZE, PSTR("%s %s %s %s"), system->settings.aprs.positionComment, system->settings.aprs.status, system->settings.meshtastic.itemComment, system->settings.linux.itemComment);
}

void Command::doAprsPing(MyCommandParser::Argument *args, char *response) {
    if (system->lastAprsReceived != nullptr) {
        snprintf_P(response, MyCommandParser::MAX_RESPONSE_SIZE, "Pong %s! SNR: %.2f RSSI: %.2f", system->lastAprsReceived->callsign, system->lastAprsReceived->snr, system->lastAprsReceived->rssi);
    } else {
        doPing(args, response);
    }
}

void Command::sortAprsHeard() {
    for (int i = 0; i < APRS_CALLSIGNS_HEARD_NUMBER; i++) {
        sortedAprsHeard[i] = &system->aprsReceived[i];
    }

    qsort(sortedAprsHeard, APRS_CALLSIGNS_HEARD_NUMBER, sizeof(SettingsAprsCallsignHeard *), compareAprsHeardTimeDescending);
}

int Command::compareAprsHeardTimeDescending(const void *a, const void *b) {
    const auto first = *(const SettingsAprsCallsignHeard **)a;
    const auto second = *(const SettingsAprsCallsignHeard **)b;

    return (second->time > first->time) - (second->time < first->time);
}

bool Command::changeGpio(const char *what, uint16_t state) {
    GpioPin *gpio = nullptr;

    if (strcmp_P(what, PSTR("wifiLinux")) == 0) {
        if (!changeGpio(PSTR("wifi"), state)) {
            return false;
        }

        gpio = system->getGpio(system->settings.linux.pin.pin);
    } else {
        const int pinNumber = atoi(what);
        if (pinNumber > 0) {
            Log.noticeln(F("[COMMAND_GPIO] Gpio tested as pin number for %d"), pinNumber);
            gpio = system->getGpio(pinNumber);
        } else {
            gpio = system->getGpio(what);
        }
    }

    if (gpio != nullptr) {
        if (state > 1) {
            gpio->setState(false);
            delayWdt(state * 1000);
            gpio->setState(true);
        } else {
            gpio->setState(state == 1);
        }

        return true;
    }

    return false;
}
