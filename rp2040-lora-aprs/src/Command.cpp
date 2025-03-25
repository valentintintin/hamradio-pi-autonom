#include <stdlib.h>
#include "ArduinoLog.h"

#include "Command.h"
#include "System.h"
#include "utils.h"
#include "Threads/Energy/EnergyMpptChgThread.h"
#include "I2CSlave.h"
#include "Threads/Energy/EnergyIna3221Thread.h"

System* Command::system;

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
    parser.registerCommand(PSTR("mpptDog"), PSTR("u"), doMpptWatchdog);
    parser.registerCommand(PSTR("objMsh"), PSTR(""), doMeshtasticAprs);
    parser.registerCommand(PSTR("objLinux"), PSTR(""), doLinuxAprs);
    parser.registerCommand(PSTR("box"), PSTR(""), doGetBoxInfo);
    parser.registerCommand(PSTR("error"), PSTR(""), doGetError);
    parser.registerCommand(PSTR("setLoraMode"), PSTR("duuuu"), doSetLora);
    parser.registerCommand(PSTR("sleepLinux"), PSTR("u"), doSleepLinux);

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

    system->gpioLed.setState(true);

    Log.traceln(F("[COMMAND] Process : %s"), command);

    if (!parser.processCommand(command, response)) {
        if (stream != nullptr) {
            stream->print(F("KO "));
            stream->println(response);
        }

        Log.warningln(F("[COMMAND] %s KO (%s)"), command, response);

        ledBlink(2, 500);

        return false;
    }

    if (stream != nullptr && strlen(response)) {
        stream->println(response);
    }

    Log.infoln(F("[COMMAND] %s OK (%s)"), command, response);

    system->gpioLed.setState(false);

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

    const bool ok = system->communication.sendRaw(reinterpret_cast<const uint8_t *>(raw), strlen(raw));

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

    GpioPin *gpio = nullptr;

    if (strcmp_P(what, PSTR("msh")) == 0) {
        gpio = system->getGpio(system->settings.meshtastic.pin);
    } else if (strcmp_P(what, PSTR("linux")) == 0) {
        gpio = system->getGpio(system->settings.linux.pin);
    } else if (strcmp_P(what, PSTR("wifi")) == 0) {
        gpio = system->getGpio(system->settings.linux.wifiPin);
    } else if (strcmp_P(what, PSTR("npr")) == 0) {
        gpio = system->getGpio(system->settings.linux.nprPin);
    } else {
        const int pinNumber = atoi(what);
        if (pinNumber > 0) {
            Log.noticeln(F("[COMMAND_GPIO] Gpio tested as pin number for %d"), pinNumber);
            gpio = system->getGpio(pinNumber);
        }
    }

    if (gpio != nullptr) {
        const auto state = args[1].asUInt64;

        if (state > 1) {
            gpio->setState(false);
            delayWdt(state * 1000);
            gpio->setState(true);
        } else {
            gpio->setState(state == 1);
        }

        strncpy_P(response, PSTR("OK"), MyCommandParser::MAX_RESPONSE_SIZE);
        return;
    }

    Log.warningln(F("[COMMAND_GPIO] Gpio %s not found"), what);
    strncpy_P(response, PSTR("KO"), MyCommandParser::MAX_RESPONSE_SIZE);
}

void Command::doSetSetting(MyCommandParser::Argument *args, char *response) {
    char *key = args[0].asString;
    char *value = args[1].asString;

    if (strlen(value) == 0) {
        Log.warningln(F("[COMMAND] Set %s to nothing impossible"), key);
        strncpy_P(response, PSTR("KO"), MyCommandParser::MAX_RESPONSE_SIZE);
        return;
    }

    bool ok = true;
    bool shouldReboot = false;

    Log.infoln(F("[COMMAND] Set %s to %s"), key, value);

    if (strcmp_P(key, PSTR("time")) == 0) {
        const auto epoch = strtoul(value, nullptr, 0) + 15; // Add 15 seconds (command typing time)

        if (system->settings.rtc.enabled) {
            system->rtc.setEpoch(epoch, true);
        }

        system->setTimeToInternalRtc(epoch);
    } else if (strcmp_P(key, PSTR("reset")) == 0) {
        ok = system->resetSettings();
        shouldReboot = ok;
    } else if (strcmp_P(key, PSTR("lora.frequency")) == 0) {
        system->settings.lora.frequency = strtof(value, nullptr);
        ok = system->communication.begin();
    } else if (strcmp_P(key, PSTR("lora.bandwidth")) == 0) {
        system->settings.lora.bandwidth = static_cast<uint16_t>(strtoul(value, nullptr, 0));
        ok = system->communication.begin();
    } else if (strcmp_P(key, PSTR("lora.spreadingFactor")) == 0) {
        system->settings.lora.spreadingFactor = static_cast<uint8_t>(strtoul(value, nullptr, 0));
        ok = system->communication.begin();
    } else if (strcmp_P(key, PSTR("lora.codingRate")) == 0) {
        system->settings.lora.codingRate = static_cast<uint8_t>(strtoul(value, nullptr, 0));
        ok = system->communication.begin();
    } else if (strcmp_P(key, PSTR("lora.outputPower")) == 0) {
        system->settings.lora.outputPower = static_cast<uint8_t>(strtoul(value, nullptr, 0));
        ok = system->communication.begin();
    } else if (strcmp_P(key, PSTR("lora.txEnabled")) == 0) {
        system->settings.lora.txEnabled = value[0] == '1';
        system->watchdogSlaveLoraTxThread->feed();
    } else if (strcmp_P(key, PSTR("lora.watchdogTxEnabled")) == 0) {
        system->settings.lora.watchdogTxEnabled =
                system->watchdogSlaveLoraTxThread->enabled = value[0] == '1';
    } else if (strcmp_P(key, PSTR("lora.intervalTimeoutWatchdogTx")) == 0) {
        system->settings.lora.intervalTimeoutWatchdogTx = strtoull(value, nullptr, 0);
        system->watchdogSlaveLoraTxThread->setInterval(system->settings.lora.intervalTimeoutWatchdogTx);
    } else if (strcmp_P(key, PSTR("aprs.call")) == 0) {
        strcpy(system->settings.aprs.call, value);
    } else if (strcmp_P(key, PSTR("aprs.destination")) == 0) {
        strcpy(system->settings.aprs.destination, value);
    } else if (strcmp_P(key, PSTR("aprs.path")) == 0) {
        strcpy(system->settings.aprs.path, value);
    } else if (strcmp_P(key, PSTR("aprs.comment")) == 0) {
        strcpy(system->settings.aprs.comment, value);
    } else if (strcmp_P(key, PSTR("aprs.status")) == 0) {
        strcpy(system->settings.aprs.status, value);
    } else if (strcmp_P(key, PSTR("aprs.symbol")) == 0) {
        system->settings.aprs.symbol = value[0];
    } else if (strcmp_P(key, PSTR("aprs.symbolTable")) == 0) {
        system->settings.aprs.symbolTable = value[0];
    } else if (strcmp_P(key, PSTR("aprs.latitude")) == 0) {
        system->settings.aprs.latitude = strtod(value, nullptr);
    } else if (strcmp_P(key, PSTR("aprs.longitude")) == 0) {
        system->settings.aprs.longitude = strtod(value, nullptr);
    } else if (strcmp_P(key, PSTR("aprs.altitude")) == 0) {
        system->settings.aprs.altitude = static_cast<uint16_t>(strtoul(value, nullptr, 0));
    } else if (strcmp_P(key, PSTR("aprs.digipeaterEnabled")) == 0) {
        system->settings.aprs.digipeaterEnabled = value[0] == '1';
    } else if (strcmp_P(key, PSTR("aprs.telemetryEnabled")) == 0) {
        system->settings.aprs.telemetryEnabled =
                system->sendTelemetriesThread->enabled = value[0] == '1';
    } else if (strcmp_P(key, PSTR("aprs.intervalTelemetry")) == 0) {
        system->settings.aprs.intervalTelemetry = strtoull(value, nullptr, 0);
        system->sendTelemetriesThread->setInterval(system->settings.aprs.intervalTelemetry);
    } else if (strcmp_P(key, PSTR("aprs.statusEnabled")) == 0) {
        system->settings.aprs.statusEnabled =
                system->sendStatusThread->enabled = value[0] == '1';
    } else if (strcmp_P(key, PSTR("aprs.intervalStatus")) == 0) {
        system->settings.aprs.intervalStatus = strtoull(value, nullptr, 0);
        system->sendStatusThread->setInterval(system->settings.aprs.intervalStatus);
    } else if (strcmp_P(key, PSTR("aprs.positionWeatherEnabled")) == 0) {
        system->settings.aprs.positionWeatherEnabled =
                system->sendPositionThread->enabled = value[0] == '1';
    } else if (strcmp_P(key, PSTR("aprs.intervalPositionWeather")) == 0) {
        system->settings.aprs.intervalPositionWeather = strtoull(value, nullptr, 0);
        system->sendPositionThread->setInterval(system->settings.aprs.intervalPositionWeather);
    } else if (strcmp_P(key, PSTR("aprs.telemetryInPosition")) == 0) {
        system->settings.aprs.telemetryInPosition = value[0] == '1';
    } else if (strcmp_P(key, PSTR("aprs.telemetrySequenceNumber")) == 0) {
        system->settings.aprs.telemetrySequenceNumber = static_cast<uint16_t>(strtoul(value, nullptr, 0));
    } else if (strcmp_P(key, PSTR("meshtastic.watchdogEnabled")) == 0) {
        system->settings.meshtastic.watchdogEnabled =
                system->watchdogMeshtastic->enabled = value[0] == '1';
        if (system->watchdogMeshtastic->enabled) {
            system->watchdogMeshtastic->feed();
        }
    } else if (strcmp_P(key, PSTR("meshtastic.intervalTimeoutWatchdog")) == 0) {
        system->settings.meshtastic.intervalTimeoutWatchdog = strtoull(value, nullptr, 0);
        system->watchdogMeshtastic->setInterval(system->settings.meshtastic.intervalTimeoutWatchdog);
    } else if (strcmp_P(key, PSTR("meshtastic.pin")) == 0) {
        system->settings.meshtastic.pin = static_cast<uint8_t>(strtoul(value, nullptr, 0));
        if (system->watchdogMeshtastic->enabled) {
            system->planReboot();
            shouldReboot = true;
        }
    } else if (strcmp_P(key, PSTR("meshtastic.i2cSlaveEnabled")) == 0) {
        system->settings.meshtastic.i2cSlaveEnabled = value[0] == '1';
        if (system->settings.meshtastic.i2cSlaveEnabled) {
            I2CSlave::begin(system);
        } else {
            I2CSlave::end();
        }
    } else if (strcmp_P(key, PSTR("meshtastic.i2cSlaveAddress")) == 0) {
        system->settings.meshtastic.i2cSlaveAddress = static_cast<uint8_t>(strtoul(value, nullptr, 0));
        if (system->settings.meshtastic.i2cSlaveEnabled) {
            I2CSlave::end();
            I2CSlave::begin(system);
        }
    } else if (strcmp_P(key, PSTR("meshtastic.aprsSendItemEnabled")) == 0) {
        system->settings.meshtastic.aprsSendItemEnabled =
        system->sendMeshtasticAprsThread->enabled = value[0] == '1';
    } else if (strcmp_P(key, PSTR("meshtastic.intervalSendItem")) == 0) {
        system->settings.meshtastic.intervalSendItem = strtoull(value, nullptr, 0);
        system->sendMeshtasticAprsThread->setInterval(system->settings.meshtastic.intervalSendItem);
    } else if (strcmp_P(key, PSTR("meshtastic.itemName")) == 0) {
        strcpy(system->settings.meshtastic.itemName, value);
    } else if (strcmp_P(key, PSTR("meshtastic.itemComment")) == 0) {
        strcpy(system->settings.meshtastic.itemComment, value);
    } else if (strcmp_P(key, PSTR("meshtastic.symbol")) == 0) {
        system->settings.meshtastic.symbol = value[0];
    } else if (strcmp_P(key, PSTR("meshtastic.symbolTable")) == 0) {
        system->settings.meshtastic.symbolTable = value[0];
    } else if (strcmp_P(key, PSTR("meshtastic.latitude")) == 0) {
        system->settings.meshtastic.latitude = strtod(value, nullptr);
    } else if (strcmp_P(key, PSTR("meshtastic.longitude")) == 0) {
        system->settings.meshtastic.longitude = strtod(value, nullptr);
    } else if (strcmp_P(key, PSTR("meshtastic.altitude")) == 0) {
        system->settings.meshtastic.altitude = static_cast<uint16_t>(strtoul(value, nullptr, 0));
    } else if (strcmp_P(key, PSTR("mpptWatchdog.enabled")) == 0) {
        system->settings.mpptWatchdog.enabled = value[0] == '1';
        if (system->watchdogSlaveMpptChgThread->enabled) {
            system->watchdogSlaveMpptChgThread->feed();
        }
    } else if (strcmp_P(key, PSTR("mpptWatchdog.timeout")) == 0) {
        system->settings.mpptWatchdog.timeout = strtoull(value, nullptr, 0);
        if (system->settings.mpptWatchdog.enabled) {
            system->watchdogSlaveMpptChgThread->feed(); // Set timeout with feed
        }
    } else if (strcmp_P(key, PSTR("mpptWatchdog.intervalFeed")) == 0) {
        system->settings.mpptWatchdog.intervalFeed = strtoull(value, nullptr, 0);
        system->watchdogSlaveMpptChgThread->setInterval(system->settings.mpptWatchdog.intervalFeed);
    } else if (strcmp_P(key, PSTR("mpptWatchdog.timeOff")) == 0) {
        system->settings.mpptWatchdog.timeOff = static_cast<uint16_t>(strtoul(value, nullptr, 0));
        if (system->settings.mpptWatchdog.enabled) {
            system->watchdogSlaveMpptChgThread->feed();
        }
    } else if (strcmp_P(key, PSTR("boxOpened.enabled")) == 0) {
        system->settings.boxOpened.enabled = value[0] == '1';
    } else if (strcmp_P(key, PSTR("boxOpened.intervalCheck")) == 0) {
        system->settings.boxOpened.intervalCheck = strtoull(value, nullptr, 0);
        system->ldrBoxOpenedThread->setInterval(system->settings.boxOpened.intervalCheck);
    } else if (strcmp_P(key, PSTR("boxOpened.pin")) == 0) {
        system->settings.boxOpened.pin = static_cast<uint8_t>(strtoul(value, nullptr, 0));
        if (system->settings.boxOpened.enabled) {
            system->planReboot();
            shouldReboot = true;
        }
    } else if (strcmp_P(key, PSTR("weather.enabled")) == 0) {
        system->settings.weather.enabled = value[0] == '1';
    } else if (strcmp_P(key, PSTR("weather.intervalCheck")) == 0) {
        system->settings.weather.intervalCheck = strtoull(value, nullptr, 0);
        system->weatherThread->setInterval(system->settings.weather.intervalCheck);
    } else if (strcmp_P(key, PSTR("energy.intervalCheck")) == 0) {
        system->settings.energy.intervalCheck = strtoull(value, nullptr, 0);
        system->energyThread->setInterval(system->settings.energy.intervalCheck);
    } else if (strcmp_P(key, PSTR("energy.type")) == 0) {
        system->settings.energy.type = static_cast<TypeEnergySensor>(strtoul(value, nullptr, 0));
        system->planReboot();
        shouldReboot = true;
    } else if (strcmp_P(key, PSTR("energy.adcPin")) == 0) {
        system->settings.energy.adcPin = static_cast<uint8_t>(strtoul(value, nullptr, 0));
        system->planReboot();
        shouldReboot = true;
    } else if (strcmp_P(key, PSTR("energy.inaChannelBattery")) == 0) {
        system->settings.energy.inaChannelBattery =
                static_cast<EnergyIna3221Thread *>(system->energyThread)->channelBattery = static_cast<ina3221_ch_t>(strtoul(value, nullptr, 0));
    } else if (strcmp_P(key, PSTR("energy.inaChannelSolar")) == 0) {
        system->settings.energy.inaChannelSolar =
                static_cast<EnergyIna3221Thread *>(system->energyThread)->channelSolar = static_cast<ina3221_ch_t>(strtoul(value, nullptr, 0));
    } else if (strcmp_P(key, PSTR("energy.mpptPowerOnVoltage")) == 0) {
        system->settings.energy.mpptPowerOnVoltage = static_cast<uint16_t>(strtoul(value, nullptr, 0));
        ok = system->energyThread->begin();
    } else if (strcmp_P(key, PSTR("energy.mpptPowerOffVoltage")) == 0) {
        system->settings.energy.mpptPowerOffVoltage = static_cast<uint16_t>(strtoul(value, nullptr, 0));
        ok = system->energyThread->begin();
    } else if (strcmp_P(key, PSTR("energy.sendAprsMessageWhenAlert")) == 0) {
        system->settings.energy.sendAprsMessageWhenAlert = value[0] == '1';
    } else if (strcmp_P(key, PSTR("linux.watchdogEnabled")) == 0) {
        system->settings.linux.watchdogEnabled =
            system->watchdogLinux->enabled = value[0] == '1';
        if (system->watchdogLinux->enabled) {
            system->watchdogMeshtastic->feed();
        }
    } else if (strcmp_P(key, PSTR("linux.intervalTimeoutWatchdog")) == 0) {
        system->settings.linux.intervalTimeoutWatchdog = strtoull(value, nullptr, 0);
        system->watchdogLinux->setInterval(system->settings.linux.intervalTimeoutWatchdog);
    } else if (strcmp_P(key, PSTR("linux.pin")) == 0) {
        system->settings.linux.pin = static_cast<uint8_t>(strtoul(value, nullptr, 0));
        if (system->watchdogLinux->enabled) {
            system->planReboot();
            shouldReboot = true;
        }
    } else if (strcmp_P(key, PSTR("linux.nprPin")) == 0) {
        system->settings.linux.nprPin = static_cast<uint8_t>(strtoul(value, nullptr, 0));
        system->planReboot();
        shouldReboot = true;
    } else if (strcmp_P(key, PSTR("linux.wifiPin")) == 0) {
        system->settings.linux.wifiPin = static_cast<uint8_t>(strtoul(value, nullptr, 0));
        system->planReboot();
        shouldReboot = true;
    } else if (strcmp_P(key, PSTR("linux.aprsSendItemEnabled")) == 0) {
        system->settings.linux.aprsSendItemEnabled =
        system->sendLinuxAprsThread->enabled = value[0] == '1';
    } else if (strcmp_P(key, PSTR("linux.intervalSendItem")) == 0) {
        system->settings.linux.intervalSendItem = strtoull(value, nullptr, 0);
        system->sendLinuxAprsThread->setInterval(system->settings.linux.intervalSendItem);
    } else if (strcmp_P(key, PSTR("linux.itemName")) == 0) {
        strcpy(system->settings.linux.itemName, value);
    } else if (strcmp_P(key, PSTR("linux.itemComment")) == 0) {
        strcpy(system->settings.linux.itemComment, value);
    } else if (strcmp_P(key, PSTR("linux.symbol")) == 0) {
        system->settings.linux.symbol = value[0];
    } else if (strcmp_P(key, PSTR("linux.symbolTable")) == 0) {
        system->settings.linux.symbolTable = value[0];
    } else if (strcmp_P(key, PSTR("linux.latitude")) == 0) {
        system->settings.linux.latitude = strtod(value, nullptr);
    } else if (strcmp_P(key, PSTR("linux.longitude")) == 0) {
        system->settings.linux.longitude = strtod(value, nullptr);
    } else if (strcmp_P(key, PSTR("linux.altitude")) == 0) {
        system->settings.linux.altitude = static_cast<uint16_t>(strtoul(value, nullptr, 0));
    } else if (strcmp_P(key, PSTR("rtc.enabled")) == 0) {
        system->settings.rtc.enabled = value[0] == '1';
    } else if (strcmp_P(key, PSTR("rtc.wakeUpPin")) == 0) {
        system->settings.rtc.wakeUpPin = static_cast<uint8_t>(strtoul(value, nullptr, 0));
        if (system->settings.rtc.enabled) {
            system->planReboot();
            shouldReboot = true;
        }
    } else if (strcmp_P(key, PSTR("useInternalWatchdog")) == 0) {
        system->settings.useInternalWatchdog = value[0] == '1';
        system->planReboot();
        shouldReboot = true;
    } else if (strcmp_P(key, PSTR("useSlowClock")) == 0) {
        system->settings.useSlowClock = value[0] == '1';
        system->planReboot();
        shouldReboot = true;
    } else if (strcmp_P(key, PSTR("aprsReceived")) == 0) {
        for (auto &[callsign, time, rssi, snr, content, count, digipeaterCallsign, digipeaterCount, reserved] : system->settings.aprsCallsignsHeard) {
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
        ok = false;
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

    if (strcmp_P(key, PSTR("lora.frequency")) == 0) {
        snprintf_P(response, MyCommandParser::MAX_RESPONSE_SIZE, PSTR("%f"), system->settings.lora.frequency);
    } else if (strcmp_P(key, PSTR("lora.bandwidth")) == 0) {
        snprintf_P(response, MyCommandParser::MAX_RESPONSE_SIZE, PSTR("%d"), system->settings.lora.bandwidth);
    } else if (strcmp_P(key, PSTR("lora.spreadingFactor")) == 0) {
        snprintf_P(response, MyCommandParser::MAX_RESPONSE_SIZE, PSTR("%d"), system->settings.lora.spreadingFactor);
    } else if (strcmp_P(key, PSTR("lora.codingRate")) == 0) {
        snprintf_P(response, MyCommandParser::MAX_RESPONSE_SIZE, PSTR("%d"), system->settings.lora.codingRate);
    } else if (strcmp_P(key, PSTR("lora.outputPower")) == 0) {
        snprintf_P(response, MyCommandParser::MAX_RESPONSE_SIZE, PSTR("%d"), system->settings.lora.outputPower);
    } else if (strcmp_P(key, PSTR("lora.txEnabled")) == 0) {
        snprintf_P(response, MyCommandParser::MAX_RESPONSE_SIZE, PSTR("%d"), system->settings.lora.txEnabled);
    } else if (strcmp_P(key, PSTR("lora.watchdogTxEnabled")) == 0) {
        snprintf_P(response, MyCommandParser::MAX_RESPONSE_SIZE, PSTR("%d"), system->settings.lora.watchdogTxEnabled);
    } else if (strcmp_P(key, PSTR("lora.intervalTimeoutWatchdogTx")) == 0) {
        snprintf_P(response, MyCommandParser::MAX_RESPONSE_SIZE, PSTR("%llu"), system->settings.lora.intervalTimeoutWatchdogTx);
    } else if (strcmp_P(key, PSTR("aprs.call")) == 0) {
        snprintf_P(response, MyCommandParser::MAX_RESPONSE_SIZE, PSTR("%s"), system->settings.aprs.call);
    } else if (strcmp_P(key, PSTR("aprs.destination")) == 0) {
        snprintf_P(response, MyCommandParser::MAX_RESPONSE_SIZE, PSTR("%s"), system->settings.aprs.destination);
    } else if (strcmp_P(key, PSTR("aprs.path")) == 0) {
        snprintf_P(response, MyCommandParser::MAX_RESPONSE_SIZE, PSTR("%s"), system->settings.aprs.path);
    } else if (strcmp_P(key, PSTR("aprs.comment")) == 0) {
        snprintf_P(response, MyCommandParser::MAX_RESPONSE_SIZE, PSTR("%s"), system->settings.aprs.comment);
    } else if (strcmp_P(key, PSTR("aprs.status")) == 0) {
        snprintf_P(response, MyCommandParser::MAX_RESPONSE_SIZE, PSTR("%s"), system->settings.aprs.status);
    } else if (strcmp_P(key, PSTR("aprs.symbol")) == 0) {
        snprintf_P(response, MyCommandParser::MAX_RESPONSE_SIZE, PSTR("%c"), system->settings.aprs.symbol);
    } else if (strcmp_P(key, PSTR("aprs.symbolTable")) == 0) {
        snprintf_P(response, MyCommandParser::MAX_RESPONSE_SIZE, PSTR("%c"), system->settings.aprs.symbolTable);
    } else if (strcmp_P(key, PSTR("aprs.latitude")) == 0) {
        snprintf_P(response, MyCommandParser::MAX_RESPONSE_SIZE, PSTR("%lf"), system->settings.aprs.latitude);
    } else if (strcmp_P(key, PSTR("aprs.longitude")) == 0) {
        snprintf_P(response, MyCommandParser::MAX_RESPONSE_SIZE, PSTR("%lf"), system->settings.aprs.longitude);
    } else if (strcmp_P(key, PSTR("aprs.altitude")) == 0) {
        snprintf_P(response, MyCommandParser::MAX_RESPONSE_SIZE, PSTR("%d"), system->settings.aprs.altitude);
    } else if (strcmp_P(key, PSTR("aprs.digipeaterEnabled")) == 0) {
        snprintf_P(response, MyCommandParser::MAX_RESPONSE_SIZE, PSTR("%d"), system->settings.aprs.digipeaterEnabled);
    } else if (strcmp_P(key, PSTR("aprs.telemetryEnabled")) == 0) {
        snprintf_P(response, MyCommandParser::MAX_RESPONSE_SIZE, PSTR("%d"), system->settings.aprs.telemetryEnabled);
    } else if (strcmp_P(key, PSTR("aprs.intervalTelemetry")) == 0) {
        snprintf_P(response, MyCommandParser::MAX_RESPONSE_SIZE, PSTR("%llu"), system->settings.aprs.intervalTelemetry);
    } else if (strcmp_P(key, PSTR("aprs.statusEnabled")) == 0) {
        snprintf_P(response, MyCommandParser::MAX_RESPONSE_SIZE, PSTR("%d"), system->settings.aprs.statusEnabled);
    } else if (strcmp_P(key, PSTR("aprs.intervalStatus")) == 0) {
        snprintf_P(response, MyCommandParser::MAX_RESPONSE_SIZE, PSTR("%llu"), system->settings.aprs.intervalStatus);
    } else if (strcmp_P(key, PSTR("aprs.positionWeatherEnabled")) == 0) {
        snprintf_P(response, MyCommandParser::MAX_RESPONSE_SIZE, PSTR("%d"), system->settings.aprs.positionWeatherEnabled);
    } else if (strcmp_P(key, PSTR("aprs.intervalPositionWeather")) == 0) {
        snprintf_P(response, MyCommandParser::MAX_RESPONSE_SIZE, PSTR("%llu"), system->settings.aprs.intervalPositionWeather);
    } else if (strcmp_P(key, PSTR("aprs.telemetryInPosition")) == 0) {
        snprintf_P(response, MyCommandParser::MAX_RESPONSE_SIZE, PSTR("%d"), system->settings.aprs.telemetryInPosition);
    } else if (strcmp_P(key, PSTR("aprs.telemetrySequenceNumber")) == 0) {
        snprintf_P(response, MyCommandParser::MAX_RESPONSE_SIZE, PSTR("%d"), system->settings.aprs.telemetrySequenceNumber);
    } else if (strcmp_P(key, PSTR("meshtastic.watchdogEnabled")) == 0) {
        snprintf_P(response, MyCommandParser::MAX_RESPONSE_SIZE, PSTR("%d"), system->settings.meshtastic.watchdogEnabled);
    } else if (strcmp_P(key, PSTR("meshtastic.intervalTimeoutWatchdog")) == 0) {
        snprintf_P(response, MyCommandParser::MAX_RESPONSE_SIZE, PSTR("%llu"), system->settings.meshtastic.intervalTimeoutWatchdog);
    } else if (strcmp_P(key, PSTR("meshtastic.pin")) == 0) {
        snprintf_P(response, MyCommandParser::MAX_RESPONSE_SIZE, PSTR("%d"), system->settings.meshtastic.pin);
    } else if (strcmp_P(key, PSTR("meshtastic.i2cSlaveEnabled")) == 0) {
        snprintf_P(response, MyCommandParser::MAX_RESPONSE_SIZE, PSTR("%d"), system->settings.meshtastic.i2cSlaveEnabled);
    } else if (strcmp_P(key, PSTR("meshtastic.i2cSlaveAddress")) == 0) {
        snprintf_P(response, MyCommandParser::MAX_RESPONSE_SIZE, PSTR("0x%x"), system->settings.meshtastic.i2cSlaveAddress);
    } else if (strcmp_P(key, PSTR("meshtastic.aprsSendItemEnabled")) == 0) {
        snprintf_P(response, MyCommandParser::MAX_RESPONSE_SIZE, PSTR("%d"), system->settings.meshtastic.aprsSendItemEnabled);
    } else if (strcmp_P(key, PSTR("meshtastic.intervalSendItem")) == 0) {
        snprintf_P(response, MyCommandParser::MAX_RESPONSE_SIZE, PSTR("%llu"), system->settings.meshtastic.intervalSendItem);
    } else if (strcmp_P(key, PSTR("meshtastic.itemName")) == 0) {
        snprintf_P(response, MyCommandParser::MAX_RESPONSE_SIZE, PSTR("%s"), system->settings.meshtastic.itemName);
    } else if (strcmp_P(key, PSTR("meshtastic.itemComment")) == 0) {
        snprintf_P(response, MyCommandParser::MAX_RESPONSE_SIZE, PSTR("%s"), system->settings.meshtastic.itemComment);
    } else if (strcmp_P(key, PSTR("meshtastic.latitude")) == 0) {
        snprintf_P(response, MyCommandParser::MAX_RESPONSE_SIZE, PSTR("%lf"), system->settings.meshtastic.latitude);
    } else if (strcmp_P(key, PSTR("meshtastic.longitude")) == 0) {
        snprintf_P(response, MyCommandParser::MAX_RESPONSE_SIZE, PSTR("%lf"), system->settings.meshtastic.longitude);
    } else if (strcmp_P(key, PSTR("meshtastic.altitude")) == 0) {
        snprintf_P(response, MyCommandParser::MAX_RESPONSE_SIZE, PSTR("%d"), system->settings.meshtastic.altitude);
    } else if (strcmp_P(key, PSTR("mpptWatchdog.enabled")) == 0) {
        snprintf_P(response, MyCommandParser::MAX_RESPONSE_SIZE, PSTR("%d"), system->settings.mpptWatchdog.enabled);
    } else if (strcmp_P(key, PSTR("mpptWatchdog.timeout")) == 0) {
        snprintf_P(response, MyCommandParser::MAX_RESPONSE_SIZE, PSTR("%llu"), system->settings.mpptWatchdog.timeout);
    } else if (strcmp_P(key, PSTR("mpptWatchdog.intervalFeed")) == 0) {
        snprintf_P(response, MyCommandParser::MAX_RESPONSE_SIZE, PSTR("%llu"), system->settings.mpptWatchdog.intervalFeed);
    } else if (strcmp_P(key, PSTR("mpptWatchdog.timeOff")) == 0) {
        snprintf_P(response, MyCommandParser::MAX_RESPONSE_SIZE, PSTR("%d"), system->settings.mpptWatchdog.timeOff);
    } else if (strcmp_P(key, PSTR("boxOpened.enabled")) == 0) {
        snprintf_P(response, MyCommandParser::MAX_RESPONSE_SIZE, PSTR("%d"), system->settings.boxOpened.enabled);
    } else if (strcmp_P(key, PSTR("boxOpened.intervalCheck")) == 0) {
        snprintf_P(response, MyCommandParser::MAX_RESPONSE_SIZE, PSTR("%llu"), system->settings.boxOpened.intervalCheck);
    } else if (strcmp_P(key, PSTR("boxOpened.pin")) == 0) {
        snprintf_P(response, MyCommandParser::MAX_RESPONSE_SIZE, PSTR("%d"), system->settings.boxOpened.pin);
    } else if (strcmp_P(key, PSTR("weather.enabled")) == 0) {
        snprintf_P(response, MyCommandParser::MAX_RESPONSE_SIZE, PSTR("%d"), system->settings.weather.enabled);
    } else if (strcmp_P(key, PSTR("weather.intervalCheck")) == 0) {
        snprintf_P(response, MyCommandParser::MAX_RESPONSE_SIZE, PSTR("%llu"), system->settings.weather.intervalCheck);
    } else if (strcmp_P(key, PSTR("energy.intervalCheck")) == 0) {
        snprintf_P(response, MyCommandParser::MAX_RESPONSE_SIZE, PSTR("%llu"), system->settings.energy.intervalCheck);
    } else if (strcmp_P(key, PSTR("energy.type")) == 0) {
        snprintf_P(response, MyCommandParser::MAX_RESPONSE_SIZE, PSTR("%d"), system->settings.energy.type);
    } else if (strcmp_P(key, PSTR("energy.adcPin")) == 0) {
        snprintf_P(response, MyCommandParser::MAX_RESPONSE_SIZE, PSTR("%d"), system->settings.energy.adcPin);
    } else if (strcmp_P(key, PSTR("energy.inaChannelBattery")) == 0) {
        snprintf_P(response, MyCommandParser::MAX_RESPONSE_SIZE, PSTR("%d"), system->settings.energy.inaChannelBattery);
    } else if (strcmp_P(key, PSTR("energy.inaChannelSolar")) == 0) {
        snprintf_P(response, MyCommandParser::MAX_RESPONSE_SIZE, PSTR("%d"), system->settings.energy.inaChannelSolar);
    } else if (strcmp_P(key, PSTR("energy.mpptPowerOnVoltage")) == 0) {
        snprintf_P(response, MyCommandParser::MAX_RESPONSE_SIZE, PSTR("%d"), system->settings.energy.mpptPowerOnVoltage);
    } else if (strcmp_P(key, PSTR("energy.mpptPowerOffVoltage")) == 0) {
        snprintf_P(response, MyCommandParser::MAX_RESPONSE_SIZE, PSTR("%d"), system->settings.energy.mpptPowerOffVoltage);
    } else if (strcmp_P(key, PSTR("energy.sendAprsMessageWhenAlert")) == 0) {
        snprintf_P(response, MyCommandParser::MAX_RESPONSE_SIZE, PSTR("%d"), system->settings.energy.sendAprsMessageWhenAlert);
    } else if (strcmp_P(key, PSTR("linux.watchdogEnabled")) == 0) {
        snprintf_P(response, MyCommandParser::MAX_RESPONSE_SIZE, PSTR("%d"), system->settings.linux.watchdogEnabled);
    } else if (strcmp_P(key, PSTR("linux.intervalTimeoutWatchdog")) == 0) {
        snprintf_P(response, MyCommandParser::MAX_RESPONSE_SIZE, PSTR("%llu"), system->settings.linux.intervalTimeoutWatchdog);
    } else if (strcmp_P(key, PSTR("linux.pin")) == 0) {
        snprintf_P(response, MyCommandParser::MAX_RESPONSE_SIZE, PSTR("%d"), system->settings.linux.pin);
    } else if (strcmp_P(key, PSTR("linux.nprPin")) == 0) {
        snprintf_P(response, MyCommandParser::MAX_RESPONSE_SIZE, PSTR("%d"), system->settings.linux.nprPin);
    } else if (strcmp_P(key, PSTR("linux.wifiPin")) == 0) {
        snprintf_P(response, MyCommandParser::MAX_RESPONSE_SIZE, PSTR("%d"), system->settings.linux.wifiPin);
    } else if (strcmp_P(key, PSTR("linux.aprsSendItemEnabled")) == 0) {
        snprintf_P(response, MyCommandParser::MAX_RESPONSE_SIZE, PSTR("%d"), system->settings.linux.aprsSendItemEnabled);
    } else if (strcmp_P(key, PSTR("linux.intervalSendItem")) == 0) {
        snprintf_P(response, MyCommandParser::MAX_RESPONSE_SIZE, PSTR("%llu"), system->settings.linux.intervalSendItem);
    } else if (strcmp_P(key, PSTR("linux.itemName")) == 0) {
        snprintf_P(response, MyCommandParser::MAX_RESPONSE_SIZE, PSTR("%s"), system->settings.linux.itemName);
    } else if (strcmp_P(key, PSTR("linux.itemComment")) == 0) {
        snprintf_P(response, MyCommandParser::MAX_RESPONSE_SIZE, PSTR("%s"), system->settings.linux.itemComment);
    } else if (strcmp_P(key, PSTR("linux.latitude")) == 0) {
        snprintf_P(response, MyCommandParser::MAX_RESPONSE_SIZE, PSTR("%lf"), system->settings.linux.latitude);
    } else if (strcmp_P(key, PSTR("linux.longitude")) == 0) {
        snprintf_P(response, MyCommandParser::MAX_RESPONSE_SIZE, PSTR("%lf"), system->settings.linux.longitude);
    } else if (strcmp_P(key, PSTR("linux.altitude")) == 0) {
        snprintf_P(response, MyCommandParser::MAX_RESPONSE_SIZE, PSTR("%d"), system->settings.linux.altitude);
    } else if (strcmp_P(key, PSTR("rtc.enabled")) == 0) {
        snprintf_P(response, MyCommandParser::MAX_RESPONSE_SIZE, PSTR("%d"), system->settings.rtc.enabled);
    } else if (strcmp_P(key, PSTR("rtc.wakeUpPin")) == 0) {
        snprintf_P(response, MyCommandParser::MAX_RESPONSE_SIZE, PSTR("%d"), system->settings.rtc.wakeUpPin);
    } else if (strcmp_P(key, PSTR("useInternalWatchdog")) == 0) {
        snprintf_P(response, MyCommandParser::MAX_RESPONSE_SIZE, PSTR("%d"), system->settings.useInternalWatchdog);
    } else if (strcmp_P(key, PSTR("useSlowClock")) == 0) {
        snprintf_P(response, MyCommandParser::MAX_RESPONSE_SIZE, PSTR("%d"), system->settings.useSlowClock);
    } else if (strcmp_P(key, PSTR("time")) == 0 || strcmp_P(key, PSTR("now")) == 0) {
        getDateTimeStringFromEpoch(system->getDateTime().unixtime(), response, MyCommandParser::MAX_RESPONSE_SIZE);
    } else if (strcmp_P(key, PSTR("wdReboot")) == 0) {
        snprintf_P(response, MyCommandParser::MAX_RESPONSE_SIZE, PSTR("%d"), watchdog_enable_caused_reboot());
    } else if (strcmp_P(key, PSTR("all")) == 0) {
        system->printSettings();
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
    const auto boxOpened = system->ldrBoxOpenedThread->enabled && system->ldrBoxOpenedThread->isBoxOpened();

    snprintf_P(response, MyCommandParser::MAX_RESPONSE_SIZE, PSTR("RTC: %.2f°C | Bat: %.2f°C | Ouverte: %d"), temperatureRtc, temperatureBattery, boxOpened);
}

void Command::doGetError(MyCommandParser::Argument *args, char *response) {
    snprintf_P(response, MyCommandParser::MAX_RESPONSE_SIZE, PSTR("Energy: %d | Weather: %d | LoRa : %d"), system->energyThread->hasError(), system->weatherThread->hasError(), system->communication.hasError());
}

void Command::doSetLora(MyCommandParser::Argument *args, char *response) {
    const auto frequency = args[0].asDouble;
    const auto bandwidth = args[1].asUInt64;
    const auto spreadingFactor = args[2].asUInt64;
    const auto codingRate = args[3].asUInt64;
    const auto outputPower = args[4].asUInt64;

    bool ok = system->communication.changeLoRaSettings(frequency, bandwidth, spreadingFactor, codingRate, outputPower);

    strncpy_P(response, ok ? PSTR("OK") : PSTR("KO"), MyCommandParser::MAX_RESPONSE_SIZE);
}

void Command::doSleepLinux(MyCommandParser::Argument *args, char *response) {
    const auto duration = args[0].asUInt64;

    system->watchdogLinux->sleep(duration * 1000);
    system->watchdogLinux->forceRun();

    strncpy_P(response, PSTR("OK"), MyCommandParser::MAX_RESPONSE_SIZE);
}

void Command::doAprsQueryHelp(MyCommandParser::Argument *args, char *response) {
    strncpy_P(response, PSTR("?APRSP ?APRSD ?APRSL ?APRSH CALL ?APRSV ?PING"), MyCommandParser::MAX_RESPONSE_SIZE);
}

// ?APRSD
void Command::doAprsHeardWithoutDigi(MyCommandParser::Argument *args, char *response) {
    const auto now = system->getDateTime().unixtime();
    constexpr int hours = 2;
    constexpr int maxTime = 3600 * hours;

    for (auto &[callsign, time, rssi, snr, content, count, digipeaterCallsign, digipeaterCount, reserved] : system->settings.aprsCallsignsHeard) {
        if (strlen(response) >= MyCommandParser::MAX_RESPONSE_SIZE - 10) {
            return;
        }

        if (strlen(callsign) > 0 && strlen(content) > 0 && strlen(digipeaterCallsign) == 0 && digipeaterCount == 0) {
            if (strlen(response) > 0) {
                strncat_P(response, PSTR(" "), MyCommandParser::MAX_RESPONSE_SIZE - strlen(response));
            }

            if (now - time <= maxTime) {
                strncat_P(response, callsign, MyCommandParser::MAX_RESPONSE_SIZE - strlen(response));
            }
        }
    }

    if (strlen(response) == 0) {
        snprintf_P(response, MyCommandParser::MAX_RESPONSE_SIZE, PSTR("Personne depuis %d heures"), hours);
    }
}

// ?APRSL
void Command::doAprsHeard(MyCommandParser::Argument *args, char *response) {
    const auto now = system->getDateTime().unixtime();
    constexpr int hours = 2;
    constexpr int maxTime = 3600 * hours;

    for (auto &[callsign, time, rssi, snr, content, count, digipeaterCallsign, digipeaterCount, reserved] : system->settings.aprsCallsignsHeard) {
        if (strlen(response) >= MyCommandParser::MAX_RESPONSE_SIZE - 10) {
            return;
        }

        if (strlen(callsign) > 0 && strlen(content) > 0) {
            if (strlen(response) > 0) {
                strncat_P(response, PSTR(" "), MyCommandParser::MAX_RESPONSE_SIZE - strlen(response));
            }

            if (now - time <= maxTime) {
                snprintf_P(response + strlen(response), MyCommandParser::MAX_RESPONSE_SIZE - strlen(response), PSTR("%s(%d)"), callsign, digipeaterCount);
            }
        }
    }

    if (strlen(response) == 0) {
        snprintf_P(response, MyCommandParser::MAX_RESPONSE_SIZE, PSTR("Personne depuis %d heures"), hours);
    }
}

// ?APRSH CALL
void Command::doAprsHeardSomeone(MyCommandParser::Argument *args, char *response) {
    for (auto &[callsign, time, rssi, snr, content, count, digipeaterCallsign, digipeaterCount, reserved] : system->settings.aprsCallsignsHeard) {
        if (strcasecmp(callsign, args[0].asString) == 0) {
            getDateTimeStringFromEpoch(time, bufferText, BUFFER_LENGTH);
            snprintf_P(response, MyCommandParser::MAX_RESPONSE_SIZE, PSTR("%s SNR:%.2f RSSI:%.2f Digi:%d Last:%s Count:%llu"), bufferText, snr, rssi, digipeaterCount, digipeaterCallsign, count);
            return;
        }
    }

    snprintf_P(response, MyCommandParser::MAX_RESPONSE_SIZE, PSTR("%s pas entendu"), args[0].asString);
}

void Command::doAbout(MyCommandParser::Argument *args, char *response) {
    snprintf_P(response, MyCommandParser::MAX_RESPONSE_SIZE, PSTR("%s %s %s %s"), system->settings.aprs.comment, system->settings.aprs.status, system->settings.meshtastic.itemComment, system->settings.linux.itemComment);
}

void Command::doAprsPing(MyCommandParser::Argument *args, char *response) {
    if (system->lastAprsHeard != nullptr) {
        snprintf_P(response, MyCommandParser::MAX_RESPONSE_SIZE, "Pong %s! SNR: %.2f RSSI: %.2f", system->lastAprsHeard->callsign, system->lastAprsHeard->snr, system->lastAprsHeard->rssi);
    } else {
        doPing(args, response);
    }
}
