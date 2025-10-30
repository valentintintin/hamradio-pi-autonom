#include "Communication.h"

#include <Threads/Energy/EnergyMpptChgThread.h>

#include "ArduinoLog.h"
#include "utils.h"
#include "System.h"

Communication::Communication(System *system) : system(system) {
}

bool Communication::sendAprsFrame() {
    const size_t size = Aprs::encode(&aprsPacketTx, bufferText);

    if (!size) {
        Log.errorln(F("[APRS] Error during string encode"));
        return false;
    }

    if (size > TRX_BUFFER - 3) {
        Log.errorln(F("[LORA_TX] Error during raw send. Size of %d is out of %d"), size, TRX_BUFFER);
        return false;
    }

    Log.infoln(F("[LORA_TX] Send %d bytes : %s"), size, bufferText);

    buffer[0] = '<';
    buffer[1]= 0xFF;
    buffer[2] = 0x01;

    memcpy(buffer + 3, bufferText, size);

    return system->radio.send(buffer, size + 3);
}

bool Communication::sendMessage(const char* destination, const char* message, const char* ackToConfirm) {
    Aprs::reset(&aprsPacketTx);

    const SettingsAprs settings = system->settings.aprs;

    strcpy(aprsPacketTx.path, settings.path);
    strcpy(aprsPacketTx.source, settings.callsign);
    strcpy(aprsPacketTx.destination, settings.destination);

    strcpy(aprsPacketTx.message.destination, destination);
    strcpy(aprsPacketTx.message.message, message);

    if (strlen(ackToConfirm) > 0) {
        strcpy(aprsPacketTx.message.ackToConfirm, ackToConfirm);
    }

    aprsPacketTx.type = Message;

    return sendAprsFrame();
}

void Communication::prepareTelemetry() {
    aprsPacketTx.telemetries.telemetrySequenceNumber = system->settings.aprs.telemetrySequenceNumber + 1;
    system->settings.aprs.telemetrySequenceNumber = aprsPacketTx.telemetries.telemetrySequenceNumber;
    system->saveSettings();

    sprintf_P(aprsPacketTx.comment, PSTR("Bat:%d%% Up:%ld"), system->energyThread->getBatteryPercentage(), millis() / 1000);

    uint8_t i = 0;
    aprsPacketTx.telemetries.telemetriesAnalog[i++].value = system->energyThread->getVoltageBattery();
    aprsPacketTx.telemetries.telemetriesAnalog[i++].value = system->energyThread->getCurrentBattery();
    aprsPacketTx.telemetries.telemetriesAnalog[i++].value = system->energyThread->getVoltageSolar();
    aprsPacketTx.telemetries.telemetriesAnalog[i++].value = system->energyThread->getCurrentSolar();
    aprsPacketTx.telemetries.telemetriesAnalog[i++].value = system->getTemperatureBox();

    i = 0;

    aprsPacketTx.telemetries.telemetriesBoolean[i++].value = system->watchdogMeshtastic->enabled ? system->watchdogMeshtastic->isFed() : system->watchdogMeshtastic->isGpioOn();
    aprsPacketTx.telemetries.telemetriesBoolean[i++].value = system->watchdogLinux->enabled ? system->watchdogLinux->isFed() : system->watchdogLinux->isGpioOn();
    aprsPacketTx.telemetries.telemetriesBoolean[i++].value = system->getGpio(PSTR("wifi"))->getState() || system->getGpio("npr")->getState();
    aprsPacketTx.telemetries.telemetriesBoolean[i++].value = system->hasError();

    const EnergyMpptChgThread* energyThreadMppt = system->settings.energy.type == mpptchg && !system->energyThread->hasError() ?
        static_cast<EnergyMpptChgThread*>(system->energyThread) : nullptr;

    if (energyThreadMppt != nullptr) {
        aprsPacketTx.telemetries.telemetriesBoolean[i++].value = energyThreadMppt->isAlert();
    }
}

bool Communication::sendTelemetry() {
    Aprs::reset(&aprsPacketTx);

    bool result = false;

    if (shouldSendTelemetryParams) {
        result = sendTelemetryParams();
        shouldSendTelemetryParams = !result;
    }

    const SettingsAprs settings = system->settings.aprs;

    strcpy(aprsPacketTx.path, settings.pathTelemetry);
    strcpy(aprsPacketTx.source, settings.callsign);
    strcpy(aprsPacketTx.destination, settings.destination);

    prepareTelemetry();

    aprsPacketTx.type = Telemetry;
    result |= sendAprsFrame();

    return result;
}

bool Communication::sendTelemetryParams() {
    Aprs::reset(&aprsPacketTx);

    const SettingsAprs settings = system->settings.aprs;

    strcpy(aprsPacketTx.path, settings.pathTelemetry);
    strcpy(aprsPacketTx.source, settings.callsign);
    strcpy(aprsPacketTx.destination, settings.destination);

    uint8_t i = 0;

    // Voltage battery between 0 and 15000mV
    strcpy_P(aprsPacketTx.telemetries.telemetriesAnalog[i].name, PSTR("VBat")); // 7
    strcpy_P(aprsPacketTx.telemetries.telemetriesAnalog[i].unit, PSTR("V"));
    aprsPacketTx.telemetries.telemetriesAnalog[i++].equation.b = 0.001;

    // Current charge between 0mA and 2000mA
    strcpy_P(aprsPacketTx.telemetries.telemetriesAnalog[i].name, PSTR("IBat")); // 6
    strcpy_P(aprsPacketTx.telemetries.telemetriesAnalog[i++].unit, PSTR("mA"));

    // Voltage battery between 0 and 30000mV
    strcpy_P(aprsPacketTx.telemetries.telemetriesAnalog[i].name, PSTR("VSol")); // 5
    strcpy_P(aprsPacketTx.telemetries.telemetriesAnalog[i].unit, PSTR("V"));
    aprsPacketTx.telemetries.telemetriesAnalog[i++].equation.b = 0.001;

    // Current charge between 0mA and 2000mA
    strcpy_P(aprsPacketTx.telemetries.telemetriesAnalog[i].name, PSTR("ISol")); // 5
    strcpy_P(aprsPacketTx.telemetries.telemetriesAnalog[i++].unit, PSTR("mA"));

    strcpy_P(aprsPacketTx.telemetries.telemetriesAnalog[i].name, PSTR("TBox")); // 4
    strcpy_P(aprsPacketTx.telemetries.telemetriesAnalog[i++].unit, PSTR("°C"));

    i = 0;

    strcpy_P(aprsPacketTx.telemetries.telemetriesBoolean[i++].name, PSTR("Msh"));
    strcpy_P(aprsPacketTx.telemetries.telemetriesBoolean[i++].name, PSTR("Lnx"));
    strcpy_P(aprsPacketTx.telemetries.telemetriesBoolean[i++].name, PSTR("Lnk"));
    strcpy_P(aprsPacketTx.telemetries.telemetriesBoolean[i++].name, PSTR("Err"));

    if (system->settings.energy.type == mpptchg) {
        strcpy_P(aprsPacketTx.telemetries.telemetriesBoolean[i++].name, PSTR("Alr"));
    }

    aprsPacketTx.type = TelemetryLabel;
    bool result = sendAprsFrame();

    aprsPacketTx.type = TelemetryUnit;
    result |= sendAprsFrame();

    aprsPacketTx.type = TelemetryEquation;
    result |= sendAprsFrame();

    return result;
}

bool Communication::sendPosition(const char* comment) {
    Aprs::reset(&aprsPacketTx);

    const SettingsAprs settings = system->settings.aprs;

    strcpy(aprsPacketTx.path, settings.path);
    strcpy(aprsPacketTx.source, settings.callsign);
    strcpy(aprsPacketTx.destination, settings.destination);

    aprsPacketTx.position.symbol = settings.symbol;
    aprsPacketTx.position.overlay = settings.symbolTable;
    aprsPacketTx.position.latitude = settings.latitude;
    aprsPacketTx.position.longitude = settings.longitude;
    aprsPacketTx.position.altitudeFeet = settings.altitude * 3.28f;
    aprsPacketTx.position.altitudeInComment = false;

    aprsPacketTx.type = Position;

    const auto weatherThread = system->weatherThread;
    if (system->settings.weather.enabled && !weatherThread->hasError()) {
        aprsPacketTx.position.withWeather =
                aprsPacketTx.weather.useHumidity =
                        aprsPacketTx.weather.useTemperature = true;

        aprsPacketTx.weather.temperatureFahrenheit = static_cast<int16_t>(weatherThread->getTemperature() * 9.0 / 5.0 + 32);
        aprsPacketTx.weather.humidity = static_cast<int16_t>(weatherThread->getHumidity());

        if (weatherThread->getPressure() > 0) {
            aprsPacketTx.weather.usePressure = true;
            aprsPacketTx.weather.pressure = static_cast<int16_t>(weatherThread->getPressure());
        }

        if (weatherThread->getWh65BData() != nullptr) {
            aprsPacketTx.weather.useWindDirection = true;
            aprsPacketTx.weather.windDirectionDegrees = weatherThread->getWindDirectionDeg();

            aprsPacketTx.weather.useWindSpeed = true;
            aprsPacketTx.weather.windSpeedMph = static_cast<uint16_t>(weatherThread->getWindAverageMs() * 2.237);

            aprsPacketTx.weather.useGustSpeed = true;
            aprsPacketTx.weather.gustSpeedMph = static_cast<uint16_t>(weatherThread->getWindMaxMs() * 2.237);

            aprsPacketTx.weather.useRain1Hour = true;
            aprsPacketTx.weather.rainSinceMidnightHundredthsOfAnInch = static_cast<uint16_t>(weatherThread->getRain1hMm() / 25.4);

            aprsPacketTx.weather.useRain24Hour = true;
            aprsPacketTx.weather.rainSinceMidnightHundredthsOfAnInch = static_cast<uint16_t>(weatherThread->getRain24hMm() / 25.4);

            aprsPacketTx.weather.useRainSinceMidnight = system->settings.rtc.enabled;
            aprsPacketTx.weather.rainSinceMidnightHundredthsOfAnInch = static_cast<uint16_t>(weatherThread->getRainSinceMidnightMm() / 25.4);
        }
    }

    if (settings.telemetryInPosition) {
        prepareTelemetry();
        strcat(aprsPacketTx.comment, comment);
        aprsPacketTx.position.withTelemetry = true;
        system->sendTelemetriesThread->setRunned();
    } else {
        strcpy(aprsPacketTx.comment, comment);
    }

    return sendAprsFrame();
}

bool Communication::sendStatus(const char* comment) {
    Aprs::reset(&aprsPacketTx);

    const SettingsAprs settings = system->settings.aprs;

    strcpy(aprsPacketTx.path, settings.path);
    strcpy(aprsPacketTx.source, settings.callsign);
    strcpy(aprsPacketTx.destination, settings.destination);

    strcpy(aprsPacketTx.comment, comment);

    aprsPacketTx.type = Status;

    return sendAprsFrame();
}

bool Communication::sendItem(const char *name, const char symbol, const char symbolTable, const char* comment, const double latitude, const double longitude, const uint16_t altitude, const bool alive) {
    Aprs::reset(&aprsPacketTx);

    const SettingsAprs settings = system->settings.aprs;

    strcpy(aprsPacketTx.path, settings.path);
    strcpy(aprsPacketTx.source, settings.callsign);
    strcpy(aprsPacketTx.destination, settings.destination);

    aprsPacketTx.position.latitude = latitude;
    aprsPacketTx.position.longitude = longitude;
    aprsPacketTx.position.altitudeFeet = altitude * 3.28f;
    aprsPacketTx.position.altitudeInComment = false;

    aprsPacketTx.position.symbol = symbol;
    aprsPacketTx.position.overlay = symbolTable;
    aprsPacketTx.item.active = alive;
    strcpy(aprsPacketTx.item.name, name);
    strcpy(aprsPacketTx.comment, comment);

    aprsPacketTx.type = Item;

    return sendAprsFrame();
}

void Communication::received(const uint8_t * payload, const uint16_t size, const float rssi, const float snr) {
    if (!Aprs::decode(reinterpret_cast<const char *>(payload + sizeof(uint8_t) * 3), &aprsPacketRx)) {
        Log.warningln(F("[APRS] Error during decode, KISS ?"));
        system->sendToKissInterface(payload, size);
    } else {
        bool shouldTx = false;
        Log.traceln(F("[APRS] Decoded from %s to %s via %s"), aprsPacketRx.source, aprsPacketRx.destination, aprsPacketRx.path);

        const SettingsAprs settings = system->settings.aprs;

        snprintf_P(bufferText, BUFFER_LENGTH, PSTR("%s*"), settings.callsign);
        if (strcasecmp(aprsPacketRx.source, settings.callsign) == 0 // own frame
            || strcasecmp(aprsPacketRx.path, bufferText) == 0 // we already have digipeated the frame
        ) {
            Log.warningln(F("[APRS] It's from us or we already have digipeated it. Ignored"));
            return;
        }

        system->addAprsFrameReceivedToHistory(&aprsPacketRx, snr, rssi);

        if (strstr(aprsPacketRx.message.destination, settings.callsign) != nullptr) {
            Log.traceln(F("[APRS] Message for me : %s"), aprsPacketRx.message.message);

            if (strlen(aprsPacketRx.message.message) > 0) {
                if (strlen(aprsPacketRx.message.ackToConfirm) > 0) {
                    shouldTx = sendMessage(aprsPacketRx.source, PSTR(""), aprsPacketRx.message.ackToConfirm);
                }

                system->command.processCommand(nullptr, aprsPacketRx.message.message);

                shouldTx |= sendMessage(aprsPacketRx.source, system->command.response);
            }
        } else if (settings.digipeaterEnabled) {
            shouldTx = Aprs::canBeDigipeated(aprsPacketRx.path, settings.callsign);

            Log.traceln(F("[APRS] Message should TX : %T"), shouldTx);

            if (shouldTx) {
                Log.infoln(F("[APRS] Message digipeated via %s"), aprsPacketRx.path);
                strcpy(aprsPacketTx.source, aprsPacketRx.source);
                strcpy(aprsPacketTx.path, aprsPacketRx.path);
                strcpy(aprsPacketTx.destination, aprsPacketRx.destination);
                strcpy(aprsPacketTx.content, aprsPacketRx.content);
                aprsPacketTx.type = RawContent;
                shouldTx = sendAprsFrame();
            }
        }
    }
}