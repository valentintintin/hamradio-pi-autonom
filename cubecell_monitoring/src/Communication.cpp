#include "Communication.h"
#include "ArduinoLog.h"
#include "System.h"

Communication::Communication(System *system) : system(system) {
    system->communication = this;

    strcpy_P(aprsPacketTx.source, PSTR(APRS_CALLSIGN));
    strcpy_P(aprsPacketTx.path, PSTR(APRS_PATH));
    strcpy_P(aprsPacketTx.destination, PSTR(APRS_DESTINATION));

    strcpy_P(aprsPacketTx.telemetries.projectName, PSTR("Data"));

    // Voltage battery between 0 and 15000mV
    strcpy_P(aprsPacketTx.telemetries.telemetriesAnalog[0].name, PSTR("Battery"));
    strcpy_P(aprsPacketTx.telemetries.telemetriesAnalog[0].unit, PSTR("V"));

    // Current charge between -2000mA and 2000mA
    strcpy_P(aprsPacketTx.telemetries.telemetriesAnalog[1].name, PSTR("ICharg"));
    strcpy_P(aprsPacketTx.telemetries.telemetriesAnalog[1].unit, PSTR("mA"));

    // Temperature in degrees
    strcpy_P(aprsPacketTx.telemetries.telemetriesAnalog[2].name, PSTR("Temp"));
    strcpy_P(aprsPacketTx.telemetries.telemetriesAnalog[2].unit, PSTR("°C"));

    // Humidity in percentage
    strcpy_P(aprsPacketTx.telemetries.telemetriesAnalog[3].name, PSTR("Humdt"));
    strcpy_P(aprsPacketTx.telemetries.telemetriesAnalog[3].unit, PSTR("%"));

    // Watchdog poweroff in seconds
    strcpy_P(aprsPacketTx.telemetries.telemetriesAnalog[4].name, PSTR("Slep"));
    strcpy_P(aprsPacketTx.telemetries.telemetriesAnalog[4].unit, PSTR("min"));

    strcpy_P(aprsPacketTx.telemetries.telemetriesBoolean[0].name, PSTR("Night"));
    strcpy_P(aprsPacketTx.telemetries.telemetriesBoolean[1].name, PSTR("Alrt"));
    strcpy_P(aprsPacketTx.telemetries.telemetriesBoolean[2].name, PSTR("WDog"));
    strcpy_P(aprsPacketTx.telemetries.telemetriesBoolean[3].name, PSTR("Wifi"));
    strcpy_P(aprsPacketTx.telemetries.telemetriesBoolean[4].name, PSTR("5V"));
    strcpy_P(aprsPacketTx.telemetries.telemetriesBoolean[5].name, PSTR("Box"));
}

bool Communication::begin(RadioEvents_t *radioEvents) {
    if (Radio.Init(radioEvents)) {
        Log.errorln(F("[RADIO] Init error"));
        system->displayText(PSTR("LoRa error"), PSTR("Init failed"));

        return false;
    }

    Radio.SetChannel(RF_FREQUENCY);

    Radio.SetRxConfig(MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
                      LORA_CODINGRATE, LORA_CODINGRATE, LORA_PREAMBLE_LENGTH,
                      LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
                      0, true, false, 0, LORA_IQ_INVERSION_ON, true);

    Radio.SetTxConfig(MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
                      LORA_SPREADING_FACTOR, LORA_CODINGRATE,
                      LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
                      true, false, 0, LORA_IQ_INVERSION_ON, 3000);

    Radio.Rx(0);

    return true;
}

void Communication::update(bool sendTelemetry, bool sendPosition) {
    Radio.IrqProcess();

    if (sendPosition) {
        this->sendPosition(PSTR(APRS_COMMENT));
    }

    if (sendTelemetry) {
        this->sendTelemetry();
    }
}

void Communication::send() {
    delay(100);
    Radio.IrqProcess();

    Log.traceln(F("[LORA_TX] Radio status : %d"), Radio.GetStatus());

    system->turnOnRGB(COLOR_SEND);

    while (USE_RF && Radio.GetStatus() != RF_RX_RUNNING) {
        Log.warningln(F("[LORA_TX] Locked. Radio Status : %d"), Radio.GetStatus());
        delay(2500);
        Radio.IrqProcess();
    }

    Log.traceln(F("[LORA_TX] Radio ready, status : %d"), Radio.GetStatus());

    system->turnOnRGB(COLOR_SEND);

    uint8_t size = Aprs::encode(&aprsPacketTx, bufferText);

    if (!size) {
        Log.errorln(F("[APRS] Error during string encode"));
        system->displayText("LoRa send error", "APRS encode error");
    } else {
        buffer[0] = '<';
        buffer[1]= 0xFF;
        buffer[2] = 0x01;

        for (uint8_t i = 0; i < size; i++) {
            buffer[i + 3] = bufferText[i];
            Log.verboseln(F("[LORA_TX] Payload[%d]=%X %c"), i, buffer[i], buffer[i]);
        }

        if (USE_RF) {
            Radio.Send(buffer, size + 3);
            Radio.IrqProcess();
        }

        serialJsonWriter
                .beginObject()
                .property(F("type"), PSTR("lora"))
                .property(F("state"), PSTR("tx"))
                .property(F("payload"), bufferText)
                .endObject(); SerialPiUsed.println();

        Log.infoln(F("[LORA_TX] Start send : %s"), bufferText);
        system->displayText("LoRa send", bufferText);

        if (!USE_RF) {
            sent();
        }
    }
}

void Communication::sendMessage(const char* destination, const char* message, const char* ackToConfirm) {
    strcpy_P(aprsPacketTx.path, PSTR(APRS_PATH_MESSAGE));
    strcpy_P(aprsPacketTx.source, PSTR(APRS_CALLSIGN));
    strcpy(aprsPacketTx.message.destination, destination);
    strcpy(aprsPacketTx.message.message, message);

    aprsPacketTx.message.ackToAsk[0] = '\0';
    aprsPacketTx.message.ackToConfirm[0] = '\0';
    aprsPacketTx.message.ackToReject[0] = '\0';

    if (strlen(ackToConfirm) > 0) {
        strcpy(aprsPacketTx.message.ackToConfirm, ackToConfirm);
    }

    if (strlen(aprsPacketTx.message.ackToConfirm) == 0) {
        sprintf_P(aprsPacketTx.message.ackToAsk, PSTR("%d"), ackToAsk++);
    }

    aprsPacketTx.type = Message;

    send();
}

void Communication::sendTelemetry() {
    strcpy_P(aprsPacketTx.path, PSTR(APRS_PATH));
    strcpy_P(aprsPacketTx.source, PSTR(APRS_CALLSIGN));
    strcpy_P(aprsPacketTx.destination, PSTR(APRS_DESTINATION));

    sprintf_P(aprsPacketTx.comment, PSTR("Chg:%s Up:%lds"), mpptChg::getStatusAsString(system->mpptMonitor.getStatus()), millis() / 1000);
    aprsPacketTx.telemetries.telemetrySequenceNumber = telemetrySequenceNumber++;
    aprsPacketTx.telemetries.telemetriesAnalog[0].value = system->mpptMonitor.getVoltageBattery();
    aprsPacketTx.telemetries.telemetriesAnalog[1].value = system->mpptMonitor.getCurrentCharge();
    aprsPacketTx.telemetries.telemetriesAnalog[2].value = system->weatherSensors.getTemperature();
    aprsPacketTx.telemetries.telemetriesAnalog[3].value = system->weatherSensors.getHumidity();
    aprsPacketTx.telemetries.telemetriesAnalog[4].value = system->mpptMonitor.isWatchdogEnabled() ? system->mpptMonitor.getWatchdogPowerOffTime() : 0;
    aprsPacketTx.telemetries.telemetriesBoolean[0].value = system->mpptMonitor.isNight();
    aprsPacketTx.telemetries.telemetriesBoolean[1].value = system->mpptMonitor.isAlert();
    aprsPacketTx.telemetries.telemetriesBoolean[2].value = system->mpptMonitor.isWatchdogEnabled();
    aprsPacketTx.telemetries.telemetriesBoolean[3].value = system->gpio.isWifiEnabled();
    aprsPacketTx.telemetries.telemetriesBoolean[4].value = system->mpptMonitor.isPowerEnabled();
    aprsPacketTx.telemetries.telemetriesBoolean[5].value = system->isBoxOpened();

    if (aprsPacketTx.telemetries.telemetrySequenceNumber > 0
        && aprsPacketTx.telemetries.telemetrySequenceNumber % APRS_TELEMETRY_PARAMS_SEQUENCE == 0)
    {
        aprsPacketTx.type = TelemetryLabel;
        send();

        aprsPacketTx.type = TelemetryUnit;
        send();

        if (APRS_TELEMETRY_EQUATIONS_ENABLED) {
            aprsPacketTx.type = TelemetryEquation;
            send();

            aprsPacketTx.type = TelemetryBitSense;
            send();
        }
    }

    aprsPacketTx.type = Telemetry;
    send();
}

void Communication::sendPosition(const char* comment) {
    strcpy_P(aprsPacketTx.path, PSTR(APRS_PATH));
    strcpy_P(aprsPacketTx.source, PSTR(APRS_CALLSIGN));
    strcpy_P(aprsPacketTx.destination, PSTR(APRS_DESTINATION));
    strcpy(aprsPacketTx.comment, comment);

    aprsPacketTx.type = Position;
    aprsPacketTx.position.symbol = APRS_SYMBOL;
    aprsPacketTx.position.overlay = APRS_SYMBOL_TABLE;
    aprsPacketTx.position.latitude = APRS_LATITUDE;
    aprsPacketTx.position.longitude = APRS_LONGITUDE;

    send();
}

void Communication::sent() {
    Radio.Rx(0);
    Radio.IrqProcess();

    system->turnOffRGB();

    Log.traceln(F("[LORA_TX] Done"));
}

void Communication::received(uint8_t * payload, uint16_t size, int16_t rssi, int8_t snr) {
    Log.traceln(F("[LORA_RX] Payload of size %d, RSSI : %d and SNR : %d"), size, rssi, snr);
    Log.infoln(F("[LORA_RX] %s"), payload);

    serialJsonWriter
            .beginObject()
            .property(F("type"), PSTR("lora"))
            .property(F("state"), PSTR("rx"))
            .property(F("payload"), payload)
            .endObject(); SerialPiUsed.println();

    for (uint16_t i = 0; i < size; i++) {
        Log.verboseln(F("[LORA_RX] Payload[%d]=%X %c"), i, payload[i], payload[i]);
    }

    system->turnOnRGB(COLOR_RECEIVED);

    system->displayText("LoRa received", reinterpret_cast<const char *>(payload));

    system->turnOffRGB();

    if (!Aprs::decode(reinterpret_cast<const char *>(payload + sizeof(uint8_t) * 3), &aprsPacketRx)) {
        Log.errorln(F("[APRS] Error during decode"));
    } else {
        Log.traceln(F("[APRS] Decoded from %s to %s via %s"), aprsPacketRx.source, aprsPacketRx.destination, aprsPacketRx.path);

        if (strstr_P(aprsPacketRx.destination, PSTR(APRS_CALLSIGN)) != nullptr) {
            Log.traceln(F("[APRS] Message for me : %s"), aprsPacketRx.content);

            system->displayText(PSTR("[APRS] Received for me"), aprsPacketRx.content);

            if (strlen(aprsPacketRx.message.message) > 0) {
                sendMessage(PSTR(APRS_DESTINATION), PSTR(""), aprsPacketRx.message.ackToConfirm);
            }

            bool processCommandResult = system->command.processCommand(aprsPacketRx.message.message);
            sprintf_P(bufferText, PSTR("Command %s"), processCommandResult, system->command.getResponse());
            sendMessage(PSTR(APRS_DESTINATION), bufferText);
        } else {
            // Digi only for WIDE1-1 or VIA Callsign
            const char *hasWide = strstr_P(aprsPacketRx.path, PSTR("WIDE1-1"));
            const char *hasCallsign = strstr_P(aprsPacketRx.path, PSTR(APRS_CALLSIGN));
            bool shouldTx = hasWide != nullptr || hasCallsign != nullptr;

            Log.traceln(F("[APRS] Message should TX : %d. hasWide : %d, hasCallsign : %d"), shouldTx,
                        hasWide != nullptr, hasCallsign != nullptr);
            if (shouldTx) {
                if (hasCallsign != nullptr && *(hasCallsign + 1) != '*') { // Test if VIA callsign not consumed
                    Log.traceln(F("[APRS] Message via callsign not consumed"));
                    sprintf_P(aprsPacketTx.path, PSTR("%s*%s"), PSTR(APRS_CALLSIGN),
                              hasWide != nullptr ? PSTR(",WIDE1-1") : PSTR(""));
                } else if (hasWide != nullptr && *(hasWide + 1) != '*') { // Test if WIDE not consumed
                    Log.traceln(F("[APRS] Message via WIDE not consumed"));
                    sprintf_P(aprsPacketTx.path, PSTR("%s,WIDE1-1*"), PSTR(APRS_CALLSIGN));
                } else { // We do not TX
                    Log.traceln(F("[APRS] Message via not OK"));
                    shouldTx = false;
                }

                if (shouldTx) {
                    Log.infoln(F("[APRS] Message digipeated"));
                    strcpy(aprsPacketTx.source, aprsPacketRx.source);
                    strcpy(aprsPacketTx.destination, aprsPacketRx.destination);
                    strcpy(aprsPacketTx.content, aprsPacketRx.content);
                    aprsPacketTx.type = RawContent;
                    send();
                }
            }
        }
    }
}
