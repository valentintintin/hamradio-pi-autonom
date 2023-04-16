#include "Communication.h"
#include "ArduinoLog.h"
#include "System.h"

Communication::Communication(System *system) : system(system) {
    Radio.SetChannel(RF_FREQUENCY);

    Radio.SetRxConfig(MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
                      LORA_CODINGRATE, LORA_CODINGRATE, LORA_PREAMBLE_LENGTH,
                      LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
                      0, true, false, 0, LORA_IQ_INVERSION_ON, true);

    Radio.SetTxConfig(MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
                      LORA_SPREADING_FACTOR, LORA_CODINGRATE,
                      LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
                      true, false, 0, LORA_IQ_INVERSION_ON, 3000);

    strcpy_P(aprsPacketTx.source, PSTR(APRS_CALLSIGN));
    strcpy_P(aprsPacketTx.destination, PSTR(APRS_DESTINATION));
    strcpy_P(aprsPacketTx.telemetries.telemetriesAnalog[0].name, PSTR("Vb"));
    strcpy_P(aprsPacketTx.telemetries.telemetriesAnalog[1].name, PSTR("Ib"));
    strcpy_P(aprsPacketTx.telemetries.telemetriesAnalog[2].name, PSTR("Vs"));
    strcpy_P(aprsPacketTx.telemetries.telemetriesAnalog[3].name, PSTR("Is"));
    strcpy_P(aprsPacketTx.telemetries.telemetriesAnalog[0].unit, PSTR("V"));
    strcpy_P(aprsPacketTx.telemetries.telemetriesAnalog[1].unit, PSTR("mA"));
    strcpy_P(aprsPacketTx.telemetries.telemetriesAnalog[2].unit, PSTR("V"));
    strcpy_P(aprsPacketTx.telemetries.telemetriesAnalog[3].unit, PSTR("mA"));
    strcpy_P(aprsPacketTx.telemetries.telemetriesAnalog[4].unit, PSTR("h"));
}

bool Communication::begin(RadioEvents_t *radioEvents) {
    if (Radio.Init(radioEvents)) {
        Log.errorln(F("Radio error"));

        return false;
    }

    Radio.Rx(0);

    return true;
}

void Communication::update() {
    Radio.IrqProcess();;
}

void Communication::send() {
    Log.infoln(F("Send Lora"));

    system->turnOnRGB(COLOR_SEND);

    uint8_t size = Aprs::encode(&aprsPacketTx, stringBuffer);

    if (!size) {
        Log.errorln(F("Error during APRS string encode"));
        system->displayText("LoRa send error", "APRS encode error", 5000);
    } else {
        buffer[0] = '<';
        buffer[1]= 0xFF;
        buffer[2] = 0x01;

        for (uint8_t i = 0; i < size; i++) {
            buffer[i + 3] = stringBuffer[i];
            Log.traceln(F("Lora TX payload[%d]=%X %c"), i, buffer[i], buffer[i]);
        }

        Radio.Send(buffer, size + 3);
        Log.infoln(F("Lora TX started : %s"), stringBuffer);

        system->displayText("LoRa send", stringBuffer, 5000);
    }
}

void Communication::sendMessage(const char* message) {
    strcpy(aprsPacketTx.message.destination, APRS_DESTINATION);
    strcpy(aprsPacketTx.message.message, message);

    aprsPacketTx.message.ackToAsk[0] = '\0';
    aprsPacketTx.type = Message;

    send();
}

void Communication::sendTelemetry(const char* comment, uint8_t uptime, uint8_t vs, uint8_t is, uint8_t vb, uint8_t ib, bool isNight) {
    strcpy(aprsPacketTx.comment, comment);

    aprsPacketTx.type = Telemetry;
    aprsPacketTx.telemetries.telemetrySequenceNumber = telemetrySequenceNumber++;
    aprsPacketTx.telemetries.telemetriesAnalog[0].value = vb;
    aprsPacketTx.telemetries.telemetriesAnalog[1].value = ib;
    aprsPacketTx.telemetries.telemetriesAnalog[2].value = vs;
    aprsPacketTx.telemetries.telemetriesAnalog[3].value = is;
    aprsPacketTx.telemetries.telemetriesAnalog[4].value = uptime;

    send();
}

void Communication::sendPosition(const char* comment) {
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

    system->turnOffRGB();

    Log.noticeln(F("Lora TX done"));
}

void Communication::received(uint8_t * payload, uint16_t size, int16_t rssi, int8_t snr) {
    Log.noticeln(F("Lora RX payload of size %d, RSSI : %d and SNR : %d"), size, rssi, snr);
    Log.infoln(F("Lora RX %s"), payload);

    for (uint16_t i = 0; i < size; i++) {
        Log.traceln(F("Lora RX payload[%d]=%X %c"), i, payload[i], payload[i]);
    }

    system->turnOnRGB(COLOR_RECEIVED);

    system->displayText("LoRa received", reinterpret_cast<const char *>(payload), 5000);

    system->turnOffRGB();

    if (!Aprs::decode(reinterpret_cast<const char *>(payload + sizeof(uint8_t) * 3), &aprsPacketRx)) {
        Log.errorln(F("Error during APRS decode"));
    } else {
        Log.infoln(F("APRS decoded from %s to %s"), aprsPacketRx.source, aprsPacketRx.destination);

        if (strstr_P(reinterpret_cast<const char *>(payload), PSTR(APRS_CALLSIGN)) != nullptr) {
            Log.infoln(F("APRS meesage for me : %s"), aprsPacketRx.content);

            system->display.clear();
            system->display.drawString(0, 0, F("APRS received for me"));
            system->display.drawStringMaxWidth(0, 10, 128, aprsPacketRx.content);
            system->display.display();

            if (strlen(aprsPacketRx.message.ackToConfirm)) {
                strcpy(aprsPacketTx.message.ackToConfirm, aprsPacketRx.message.ackToConfirm);
                delay(250);
                sendMessage(PSTR(""));
            }
        }
    }
}