#include "Communication.h"

#include <Threads/Energy/EnergyMpptChgThread.h>

#include "ArduinoLog.h"
#include "utils.h"
#include "System.h"

volatile bool Communication::hasInterrupt = false;

Communication::Communication(System *system) : system(system) {
}

bool Communication::begin() {
    Log.infoln(F("[LORA] Init"));

    SPI1.setSCK(LORA_SCK);
    SPI1.setTX(LORA_MOSI);
    SPI1.setRX(LORA_MISO);
    pinMode(LORA_CS, OUTPUT);
    digitalWrite(LORA_CS, HIGH);
    SPI1.begin(false);

    const SettingsLoRa settings = system->settings.lora;

    if (lora.begin(settings.frequency, settings.bandwidth, settings.spreadingFactor, settings.codingRate, RADIOLIB_SX126X_SYNC_WORD_PRIVATE, settings.outputPower, LORA_PREAMBLE_LENGTH, 0, false) != RADIOLIB_ERR_NONE) {
        Log.errorln(F("[LORA] Init error"));
        _hasError = true;
        return false;
    }

    lora.setDio1Action(setHasInterrupt);
    lora.setDio2AsRfSwitch(true);
    lora.setRfSwitchPins(LORA_DIO4, RADIOLIB_NC);

    uint16_t state = lora.setRxBoostedGainMode(true);
    if (state != RADIOLIB_ERR_NONE) {
        Log.errorln(F("[LORA] Init KO setRxBoostedGainMode"));
        _hasError = true;
        return false;
    }

    state = lora.setCRC(RADIOLIB_SX126X_LORA_CRC_ON);
    if (state != RADIOLIB_ERR_NONE) {
        Log.errorln(F("[LORA] Init KO setCRC"));
        _hasError = true;
        return false;
    }

    state = lora.setCurrentLimit(140); // https://github.com/jgromes/RadioLib/discussions/489
    if (state != RADIOLIB_ERR_NONE) {
        Log.errorln(F("[LORA] Init KO setCurrentLimit"));
        _hasError = true;
        return false;
    }

    if (!startReceive()) {
        return false;
    }

    Log.infoln(F("[LORA] Init OK"));

    return true;
}

void Communication::setHasInterrupt() {
    hasInterrupt = true;
}

void Communication::update() {
    if (hasInterrupt) {
        hasInterrupt = false;
        const uint16_t irqFlags = lora.getIrqFlags();

        Log.traceln(F("[LORA] Interrupt with flags : %d"), irqFlags);

        if (irqFlags & RADIOLIB_SX126X_IRQ_RX_DONE) {
            memset(buffer, '\0', TRX_BUFFER);
            const size_t size = lora.getPacketLength();
            const int state = lora.readData(buffer, size);
            if (state == RADIOLIB_ERR_NONE && size >= 15) {
                received(buffer, size, lora.getRSSI(), lora.getSNR());
            }
        } else {
            startReceive();
        }
    }
}

bool Communication::sendAprsFrame() {
    size_t size = Aprs::encode(&aprsPacketTx, bufferText);

    if (!size) {
        Log.errorln(F("[APRS] Error during string encode"));
        return false;
    }

    if (size > TRX_BUFFER - 3) {
        Log.errorln(F("[LORA_TX] Error during raw send. Size of %d is out of %d"), size, TRX_BUFFER);
        return false;
    }

    buffer[0] = '<';
    buffer[1]= 0xFF;
    buffer[2] = 0x01;

    for (uint8_t i = 0; i < size; i++) {
        buffer[i + 3] = bufferText[i];
        Log.verboseln(F("[LORA_TX] Payload[%d]=%X %c"), i + 3, buffer[i + 3], buffer[i + 3]);
    }

    return send(size + 3);
}

bool Communication::sendRaw(const uint8_t* payload, size_t size) {
    if (size > TRX_BUFFER) {
        Log.errorln(F("[LORA_TX] Error during raw send. Size of %d is out of %d"), size, TRX_BUFFER);
        return false;
    }

    memcpy(buffer, payload, size);

    return send(size);
}

bool Communication::changeLoRaSettings(float frequency, uint16_t bandwidth, uint8_t spreadingFactor, uint8_t codingRate,
    uint8_t outputPower) {
    if (!lora.setFrequency(frequency) || !lora.setBandwidth(bandwidth) || !lora.setSpreadingFactor(spreadingFactor) || !lora.setCodingRate(codingRate) || !lora.setOutputPower(outputPower)) {
        Log.errorln(F("[LORA] Error during change changed to frequency: %f, bandwidth: %d, spreading factor: %d, coding rate: %d, output power: %d. Reload default"), frequency, bandwidth, spreadingFactor, codingRate, outputPower);
        begin();
        return false;
    }

    Log.infoln(F("[LORA] Settings changed to frequency: %f, bandwidth: %d, spreading factor: %d, coding rate: %d, output power: %d"), frequency, bandwidth, spreadingFactor, codingRate, outputPower);

    return true;
}

bool Communication::send(const size_t size) {
    system->gpioLed.setState(HIGH);

    Log.infoln(F("[LORA_TX] Start send %d bytes : %s"), size, bufferText);

    uint8_t i = 0;
    while (i++ < 3 && isChannelActive()) {
        startReceive();
        delayWdt(TIME_WAIT_CHANNEL_ACTIVE);
    }
    if (i == 3) {
        Log.errorln(F("[LORA_TX] Can't send because too much signal on channel"));
        return false;
    }

    if (system->settings.lora.txEnabled) {
        const int currentState = lora.transmit(buffer, size);

        if (currentState == RADIOLIB_ERR_NONE) {
            sent();
        } else if (currentState == RADIOLIB_ERR_PACKET_TOO_LONG) {
            Log.errorln(F("[LORA] TX Error too long"));
            _hasError = true;
            return false;
        } else if (currentState == RADIOLIB_ERR_TX_TIMEOUT) {
            Log.errorln(F("[LORA] TX Error timeout"));
            _hasError = true;
            return false;
        } else {
            sprintf_P(bufferText, PSTR("[LORA] TX Error : %d"), currentState);
            _hasError = true;
            Log.errorln(bufferText);
            return false;
        }
    }
    else {
        delayWdt(1000);
        sent();
    }

    return true;
}

bool Communication::sendMessage(const char* destination, const char* message, const char* ackToConfirm) {
    Aprs::reset(&aprsPacketTx);

    const SettingsAprs settings = system->settings.aprs;

    strcpy(aprsPacketTx.path, settings.path);
    strcpy(aprsPacketTx.source, settings.call);
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

    double temperatureBox = 0;
    double temperatureBoxNb = 0;

    const EnergyMpptChgThread* energyThreadMppt = system->settings.energy.type == mpptchg && !system->energyThread->hasError() ?
        static_cast<EnergyMpptChgThread*>(system->energyThread) : nullptr;

    if (energyThreadMppt != nullptr) {
        temperatureBox += energyThreadMppt->getTemperature();
        temperatureBoxNb++;
    }

    if (system->settings.rtc.enabled) {
        temperatureBox += system->rtc.getTemperature();
        temperatureBoxNb++;
    }

    temperatureBox /= temperatureBoxNb;

    uint8_t i = 0;
    aprsPacketTx.telemetries.telemetriesAnalog[i++].value = system->energyThread->getVoltageBattery();
    aprsPacketTx.telemetries.telemetriesAnalog[i++].value = system->energyThread->getCurrentBattery();
    aprsPacketTx.telemetries.telemetriesAnalog[i++].value = system->energyThread->getVoltageSolar();
    aprsPacketTx.telemetries.telemetriesAnalog[i++].value = system->energyThread->getCurrentSolar();
    aprsPacketTx.telemetries.telemetriesAnalog[i++].value = temperatureBox;

    i = 0;

    if (system->ldrBoxOpenedThread->enabled) {
        aprsPacketTx.telemetries.telemetriesBoolean[i++].value = system->ldrBoxOpenedThread->isBoxOpened();
    }
    aprsPacketTx.telemetries.telemetriesBoolean[i++].value = system->watchdogMeshtastic->enabled ? system->watchdogMeshtastic->isFed() : system->watchdogMeshtastic->isGpioOn();
    aprsPacketTx.telemetries.telemetriesBoolean[i++].value = system->watchdogLinux->enabled ? system->watchdogLinux->isFed() : system->watchdogLinux->isGpioOn();
    aprsPacketTx.telemetries.telemetriesBoolean[i++].value = system->getGpio(system->settings.linux.wifiPin)->getState() || system->getGpio(system->settings.linux.nprPin)->getState();
    aprsPacketTx.telemetries.telemetriesBoolean[i++].value = system->hasError();

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

    strcpy(aprsPacketTx.path, settings.path);
    strcpy(aprsPacketTx.source, settings.call);
    strcpy(aprsPacketTx.destination, settings.destination);

    prepareTelemetry();

    aprsPacketTx.type = Telemetry;
    result |= sendAprsFrame();

    return result;
}

bool Communication::sendTelemetryParams() {
    Aprs::reset(&aprsPacketTx);

    const SettingsAprs settings = system->settings.aprs;

    strcpy(aprsPacketTx.path, settings.path);
    strcpy(aprsPacketTx.source, settings.call);
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

    if (system->ldrBoxOpenedThread->enabled) {
        strcpy_P(aprsPacketTx.telemetries.telemetriesBoolean[i++].name, PSTR("Box"));
    }
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
    strcpy(aprsPacketTx.source, settings.call);
    strcpy(aprsPacketTx.destination, settings.destination);

    aprsPacketTx.position.symbol = settings.symbol;
    aprsPacketTx.position.overlay = settings.symbolTable;
    aprsPacketTx.position.latitude = settings.latitude;
    aprsPacketTx.position.longitude = settings.longitude;
    aprsPacketTx.position.altitudeFeet = settings.altitude * 3.28f;
    aprsPacketTx.position.altitudeInComment = false;

    aprsPacketTx.type = Position;

    if (system->settings.weather.enabled && !system->weatherThread->hasError()) {
        aprsPacketTx.position.withWeather =
                aprsPacketTx.weather.useHumidity =
                        aprsPacketTx.weather.useTemperature =
                                aprsPacketTx.weather.usePressure = true;

        aprsPacketTx.weather.temperatureFahrenheit = static_cast<int16_t>(system->weatherThread->getTemperature() * 9.0 / 5.0 + 32);
        aprsPacketTx.weather.humidity = static_cast<int16_t>(system->weatherThread->getHumidity());
        aprsPacketTx.weather.pressure = static_cast<int16_t>(system->weatherThread->getPressure());
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
    strcpy(aprsPacketTx.source, settings.call);
    strcpy(aprsPacketTx.destination, settings.destination);

    strcpy(aprsPacketTx.comment, comment);

    aprsPacketTx.type = Status;

    return sendAprsFrame();
}

bool Communication::sendItem(const char *name, const char symbol, const char symbolTable, const char* comment, const double latitude, const double longitude, const uint16_t altitude, const bool alive) {
    Aprs::reset(&aprsPacketTx);

    const SettingsAprs settings = system->settings.aprs;

    strcpy(aprsPacketTx.path, settings.path);
    strcpy(aprsPacketTx.source, settings.call);
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

void Communication::sent() {
    system->gpioLed.setState(LOW);

    Log.infoln(F("[LORA_TX] End"));

    if (system->watchdogSlaveLoraTxThread->enabled) {
        system->watchdogSlaveLoraTxThread->feed();
    }

    startReceive();

    delayWdt(TIME_AFTER_TX); // Time for others receivers to return to RX mode. It is a test where I missed some frames
}

void Communication::received(uint8_t * payload, const uint16_t size, const float rssi, const float snr) {
    Log.traceln(F("[LORA_RX] Payload of size %d, RSSI : %F and SNR : %F"), size, rssi, snr);
    Log.infoln(F("[LORA_RX] %s"), payload);

    for (uint16_t i = 0; i < size; i++) {
        Log.verboseln(F("[LORA_RX] Payload[%d]=%X %c"), i, payload[i], payload[i]);
    }

    system->gpioLed.setState(HIGH);

    bool shouldTx = false;

    if (!Aprs::decode(reinterpret_cast<const char *>(payload + sizeof(uint8_t) * 3), &aprsPacketRx)) {
        Log.warningln(F("[APRS] Error during decode, KISS ?"));
        system->sendToKissInterface(payload, size);
    } else {
        Log.traceln(F("[APRS] Decoded from %s to %s via %s"), aprsPacketRx.source, aprsPacketRx.destination, aprsPacketRx.path);

        const SettingsAprs settings = system->settings.aprs;

        if (strcasecmp(aprsPacketRx.source, settings.call) == 0) {
            Log.warningln(F("[APRS] It's from us. Bug ? Ignore it"));
            return;
        }

        system->addAprsFrameReceivedToHistory(&aprsPacketRx, snr, rssi);

        if (strstr(aprsPacketRx.message.destination, settings.call) != nullptr) {
            Log.traceln(F("[APRS] Message for me : %s"), aprsPacketRx.message.message);

            if (strlen(aprsPacketRx.message.message) > 0) {
                if (strlen(aprsPacketRx.message.ackToConfirm) > 0) {
                    shouldTx = sendMessage(aprsPacketRx.source, PSTR(""), aprsPacketRx.message.ackToConfirm);
                }

                system->command.processCommand(nullptr, aprsPacketRx.message.message);

                shouldTx |= sendMessage(aprsPacketRx.source, system->command.response);
            }
        } else if (settings.digipeaterEnabled) {
            shouldTx = Aprs::canBeDigipeated(aprsPacketRx.path, settings.call);

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

    if (!shouldTx) {
        system->gpioLed.setState(LOW);
    }
}

bool Communication::isChannelActive() {
    Log.traceln(F("[LORA] Test channel is active"));

    lora.standby();

    const auto result = lora.scanChannel();
    if (result == RADIOLIB_LORA_DETECTED) {
        Log.warningln(F("[LORA] Channel is already active"));
        return true;
    }

    if (result != RADIOLIB_CHANNEL_FREE) {
        Log.errorln(F("[LORA] Error during test channel free: %d"), result);
    } else {
        Log.traceln(F("[LORA] Channel is free"));
    }

    return false;
}

bool Communication::startReceive() {
    Log.traceln(F("[LORA] Start receive"));

    lora.standby();

    if (lora.startReceiveDutyCycleAuto(LORA_PREAMBLE_LENGTH, 8, RADIOLIB_IRQ_RX_DEFAULT_FLAGS | RADIOLIB_IRQ_PREAMBLE_DETECTED) != RADIOLIB_ERR_NONE) {
        Log.errorln(F("[LORA] Start receive KO"));
        _hasError = true;
        return false;
    }

    Log.traceln(F("[LORA] Start receive OK"));

    return true;
}
