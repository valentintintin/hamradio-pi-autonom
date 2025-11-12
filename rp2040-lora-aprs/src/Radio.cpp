#include "Radio.h"

#include <numeric>

#include <ArduinoLog.h>

#include "Settings.h"

volatile InterruptType Radio::radioStatus = IDLE;

void Radio::onISR() {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    handleIRQ(&xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void Radio::handleIRQ(BaseType_t* taskWoken) {
    if (radioStatus == RX) {

    } else if (radioStatus == TX) {

    }
}

Radio::Radio() {
    rxQueue = xQueueCreate(LORA_QUEUE_RX_SIZE, sizeof(LoRaReceived));
    txQueue = xQueueCreate(LORA_QUEUE_TX_SIZE, sizeof(LoRaTransmit));

    xTaskCreate(heartBeatTask, "HeartBeat", 128, nullptr, 1, nullptr);
}

bool Radio::init() {
    Log.infoln(F("[LORA] Init"));

    radioStatus = IDLE;
    hasInterrupt = IDLE;

    SPI1.setSCK(LORA_SCK);
    SPI1.setTX(LORA_MOSI);
    SPI1.setRX(LORA_MISO);
    pinMode(LORA_CS, OUTPUT);
    digitalWrite(LORA_CS, HIGH);
    SPI1.begin(false);

#ifdef SX126X_ANT_SW
    digitalWrite(LORA_ANT_SW, HIGH);
    pinMode(LORA_ANT_SW, OUTPUT);
#endif

#ifdef SX126X_POWER_EN
    digitalWrite(LORA_POWER_EN, HIGH);
    pinMode(LORA_POWER_EN, OUTPUT);
#endif

    const SettingsLoRa settings = system->settings.lora;

    return changeLoRaSettings(settings.frequency, settings.bandwidth, settings.spreadingFactor, settings.codingRate, settings.outputPower, settings.boostedRxGain);
}

bool Radio::changeLoRaSettings(const float frequency, const uint16_t bandwidth, const uint8_t spreadingFactor, const uint8_t codingRate, const uint8_t outputPower, const uint8_t syncWord, bool boostedRxGain) {
    auto state = lora.begin(frequency, bandwidth, spreadingFactor, codingRate, syncWord, outputPower, LORA_PREAMBLE_LENGTH, 0, false);
    if (state != RADIOLIB_ERR_NONE) {
        Log.errorln(F("[LORA] Init KO: %d"), state);
        _hasError = true;
        return false;
    }

    lora.setRfSwitchPins(LORA_DIO4, RADIOLIB_NC);

#ifdef LORA_DIO2_AS_RF_SWITCH
    state = lora.setDio2AsRfSwitch(true);
    if (state != RADIOLIB_ERR_NONE) {
        Log.errorln(F("[LORA] Init KO DIO2 as RF switch: %d"), state);
        _hasError = true;
        return false;
    }
#endif

    state = lora.setRxBoostedGainMode(boostedRxGain);
    if (state != RADIOLIB_ERR_NONE) {
        Log.errorln(F("[LORA] Init KO setRxBoostedGainMode: %d"), state);
        _hasError = true;
        return false;
    }

    state = lora.setCRC(RADIOLIB_SX126X_LORA_CRC_ON);
    if (state != RADIOLIB_ERR_NONE) {
        Log.errorln(F("[LORA] Init KO setCRC: %d"), state);
        _hasError = true;
        return false;
    }

    // https://github.com/jgromes/RadioLib/discussions/489
    state = lora.setCurrentLimit(140);
    if (state != RADIOLIB_ERR_NONE) {
        Log.errorln(F("[LORA] Init KO setCurrentLimit: %d"), state);
        _hasError = true;
        return false;
    }

    slotTimeMsec = computeSlotTimeMsec(bandwidth, spreadingFactor);
    preambleTimeMsec = getPacketTime(bandwidth, spreadingFactor, codingRate, LORA_PREAMBLE_LENGTH, 0);
    maxPacketTimeMsec = getPacketTime(bandwidth, spreadingFactor, codingRate, LORA_PREAMBLE_LENGTH, TRX_BUFFER + 3);

    Log.traceln(F("[LORA] SlotTime %d ms | PreambleTime %d ms | MaxPacketTime %d ms"), slotTimeMsec, preambleTimeMsec, maxPacketTimeMsec);

    if (!startReceive()) {
        return false;
    }

    Log.infoln(F("[LORA] Init OK to frequency: %f, bandwidth: %d, spreading factor: %d, coding rate: %d, sync word: %d, output power: %d, rx boosted : %d"), frequency, bandwidth, spreadingFactor, codingRate, syncWord, outputPower, boostedRxGain);

    // Maybe clear TX and RX queue ?

    return true;
}

bool Radio::receive() {
    if (hasInterrupt != IDLE) {
        const uint16_t irqFlags = lora.getIrqFlags();

        if (irqFlags == 0) {
            return true;
        }

        Log.traceln(F("[LORA] Interrupt %d"), hasInterrupt);
        Log.traceln(F("[LORA] Interrupt with flags : %d"), irqFlags);

        switch (hasInterrupt) {
            case RX: {
                hasInterrupt = IDLE;

                if (isReceiving()) {
                    handleReceived();
                    startReceive();
                }
                break;
            }
            case TX: {
                hasInterrupt = IDLE;

                if (isSending()) {
                    completeSending();
                }
                startReceive();
                break;
            }
            default:
                break;
        }
    }

    if (!txQueue.isEmpty() && radioStatus != TX) {
        if (wantToSend) {
            if (timerNextTx.hasExpired()) {
                wantToSend = false;

                if (!canSendImmediately() || isChannelActive()) {
                    Log.traceln(F("[LORA_TX] Can't send yet"));
                    startReceive();
                } else {
                    const auto [payload, size] = txQueue.getHead();
                    if (startSend(payload, size)) {
                        txQueue.dequeue();
                    }
                }
            }
        } else {
            wantToSend = true;
            timerNextTx.setInterval(getTxDelayMsec());

            Log.infoln(F("[LORA_TX] Next send in %d ms. %d remaining"), timerNextTx.getTimeLeft(), txQueue.itemCount());
        }
    }

    if (!rxQueue.isEmpty()) {
        const auto [payload, size, rssi, snr] = rxQueue.dequeue();
        system->communication.received(payload, size, rssi, snr);
    }

    return true;
}

bool Radio::isChannelActive() {
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

bool Radio::canSendImmediately() {
    // We wait _if_ we are partially though receiving a packet (rather than just merely waiting for one).
    // To do otherwise would be doubly bad because not only would we drop the packet that was on the way in,
    // we almost certainly guarantee no one outside will like the packet we are sending.
    const bool isActivelyReceiving = receiveDetected(lora.getIrqFlags(), RADIOLIB_SX126X_IRQ_HEADER_VALID, RADIOLIB_SX126X_IRQ_PREAMBLE_DETECTED);
    const bool busyRx = isReceiving() && isActivelyReceiving;

    if (isSending() || busyRx) {
        if (isSending()) {
            Log.warningln("[LORA_TX] Can not send yet, busyTx");
        }
        // If we've been trying to send the same packet more than one minute and we haven't gotten a
        // TX IRQ from the radio, the radio is probably broken.
        if (isSending() && !isWithinTimespanMs(lastTxStart, 60000)) {
            Log.errorln("[LORA_TX] Hardware Failure! busyTx for more than 60s, so reboot");
            system->planReboot();
            return false;
        }
        if (busyRx) {
            Log.warningln("[LORA_TX] Can not send yet, busyRx");
        }
        return false;
    }

    return true;
}

void Radio::completeSending() {
    if (isSending()) {
        Log.infoln(F("[LORA_TX] End. %d remaining"), txQueue.itemCount());
        radioStatus = IDLE;
        nbTx++;

        if (system->watchdogSlaveLoraTxThread->enabled) {
            system->watchdogSlaveLoraTxThread->feed();
        }
    }
}

bool Radio::startSend(const uint8_t *payload, uint16_t size) {
    Log.infoln(F("[LORA_TX] Will TX size %d => %s"), size, payload);

    for (auto i = 0; i < size; i++) {
        Log.verboseln(F("[LORA_TX] Payload[%d]=%X %c"), i, payload[i], buffer[i]);
    }

    if (!system->settings.lora.txEnabled) {
        return true;
    }

    radioStatus = TX;

    const auto state = lora.startTransmit(payload, size);
    if (state != RADIOLIB_ERR_NONE) {
        Log.errorln(F("[LORA_TX] Start sending KO error %d"), state);
        completeSending(); // This send failed, but make sure to 'complete' it properly
        startReceive(); // Restart receive mode (because startTransmit failed to put us in xmit mode)
        _hasError = true;
        return false;
    }

    lora.setDio1Action(onISR);
    lastTxStart = millis();
    Log.infoln(F("[LORA_TX] Start sending"));

    return true;
}

bool Radio::send(const uint8_t *payload, const size_t size) {
    Log.infoln(F("[LORA_TX] Want to send payload size %d => %s"), size, payload);

    for (auto i = 0; i < size; i++) {
        Log.verboseln(F("[LORA_TX] Payload[%d]=%X %c"), i, payload[i], buffer[i]);
    }

#if !USE_TX_QUEUE
    radioStatus = TX;

    const auto state = lora.transmit(payload, size);
    if (state != RADIOLIB_ERR_NONE) {
        Log.errorln(F("[LORA_TX] Start sending KO error %d"), state);
        completeSending(); // This send failed, but make sure to 'complete' it properly
        startReceive(); // Restart receive mode (because startTransmit failed to put us in xmit mode)
        _hasError = true;
        return false;
    }

    completeSending(); // This send failed, but make sure to 'complete' it properly
    startReceive(); // Restart receive mode (because startTransmit failed to put us in xmit mode)

    return true;
#endif

    if (txQueue.isFull()) {
        Log.warningln(F("[LORA_TX] Queue is full"));
        return false;
    }

    LoRaTransmit loraTransmit{};
    memset(loraTransmit.payload, '\0', TRX_BUFFER);
    memcpy(loraTransmit.payload, payload, size);
    loraTransmit.size = size;

    if (!txQueue.enqueue(loraTransmit)) {
        Log.errorln(F("[LORA_TX] Impossible to enqueue"));
        return false;
    }

    Log.infoln(F("[LORA_TX] %d in TX queue"), txQueue.itemCount());

    return true;
}

bool Radio::startReceive() {
    Log.traceln(F("[LORA] Start receive"));

    if (!setStandby()) {
        return false;
    }

    if (lora.startReceiveDutyCycleAuto(LORA_PREAMBLE_LENGTH, 8, RADIOLIB_IRQ_RX_DEFAULT_FLAGS) != RADIOLIB_ERR_NONE) {
        Log.errorln(F("[LORA] Start receive KO"));
        _hasError = true;
        return false;
    }

    radioStatus = RX;
    // Must be done AFTER, starting transmit, because startTransmit clears (possibly stale) interrupt pending register bits
    lora.setDio1Action(setHasRxInterrupt);

    Log.traceln(F("[LORA] Start receive OK"));

    return true;
}

bool Radio::setStandby() {
    Log.traceln(F("[LORA] Set standby"));

    if (lora.standby() != RADIOLIB_ERR_NONE) {
        Log.errorln(F("[LORA] Set standby KO"));
        _hasError = true;
        return false;
    }

    radioStatus = IDLE; // If we were receiving, not any more
    activeReceiveStart = 0;

    lora.clearDio1Action();
    completeSending();

    return true;
}

bool Radio::receiveDetected(const uint16_t irq, const ulong syncWordHeaderValidFlag, const ulong preambleDetectedFlag) {
    const bool detected = irq & (syncWordHeaderValidFlag | preambleDetectedFlag);
    // Handle false detections
    if (detected) {
        if (!activeReceiveStart) {
            activeReceiveStart = millis();
        } else if (!isWithinTimespanMs(activeReceiveStart, 2 * preambleTimeMsec) && !(irq & syncWordHeaderValidFlag)) {
            // The HEADER_VALID flag should be set by now if it was really a packet, so ignore PREAMBLE_DETECTED flag
            activeReceiveStart = 0;
            Log.noticeln("[LORA_RX] Ignore false preamble detection");
            return false;
        } else if (!isWithinTimespanMs(activeReceiveStart, maxPacketTimeMsec)) {
            // We should have gotten an RX_DONE IRQ by now if it was really a packet, so ignore HEADER_VALID flag
            activeReceiveStart = 0;
            Log.noticeln("[LORA_RX] Ignore false header detection");
            return false;
        }
    }
    return detected;
}

/** The delay to use when we want to send something */
uint32_t Radio::getTxDelayMsec() const {
    constexpr uint8_t CWmin = 3; // minimum CWsize
    constexpr uint8_t CWmax = 8; // maximum CWsize

    /** We wait a random multiple of 'slotTimes' (see definition in header file) in order to avoid collisions.
    The pool to take a random multiple from is the contention window (CW), which size depends on the
    current channel utilization. */
    constexpr float channelUtil = 0; // airTime->channelUtilizationPercent();
    const uint8_t CWsize = map(channelUtil, 0, 100, CWmin, CWmax);
    // LOG_DEBUG("Current channel utilization is %f so setting CWsize to %d", channelUtil, CWsize);
    return random(0, pow(2, CWsize)) * slotTimeMsec;
}

bool Radio::handleReceived() {
    radioStatus = IDLE;

    const auto size = lora.getPacketLength();
    const auto state = lora.readData(buffer, size);
    buffer[size + 1] = '\0';

    const float rssi = lround(lora.getRSSI());
    const float snr = lora.getSNR();

    Log.infoln(F("[LORA_RX] Payload size %d, RSSI %F, SNR %F => %s"), size, rssi, snr, buffer);

    for (size_t i = 0; i < size; i++) {
        Log.verboseln(F("[LORA_RX] Payload[%d]=%X %c"), i, buffer[i], buffer[i]);
    }

    if (state != RADIOLIB_ERR_NONE || size < 15) { // APRS Frame are always >= 15
        Log.warningln(F("[LORA_RX] Wrong packet received, error (%d)"), state);
        return false;
    }

    nbRx++;

#if !USE_RX_QUEUE
    system->communication.received(buffer, size, rssi, snr);
    return true;
#endif

    if (rxQueue.isEmpty()) {
        system->communication.received(buffer, size, rssi, snr);
        return true;
    }

    if (rxQueue.isFull()) {
        Log.warningln(F("[LORA_RX] Queue full. Ignore packet"));
        return false;
    }

    LoRaReceived loraReceived = {};
    memset(loraReceived.payload, '\0', TRX_BUFFER);
    memcpy(loraReceived.payload, buffer, size);
    loraReceived.size = size;
    loraReceived.rssi = rssi;
    loraReceived.snr = snr;

    if (!rxQueue.enqueue(loraReceived)) {
        Log.errorln(F("[LORA_RX] Impossible to enqueue"));
        return false;
    }

    Log.infoln(F("[LORA_RX] %d packet in RX queue"), rxQueue.itemCount());

    return true;
}

/** Slottime is the time to detect a transmission has started, consisting of:
  - CAD duration;
  - roundtrip air propagation time (assuming max. 30km between nodes);
  - Tx/Rx turnaround time (maximum of SX126x and SX127x);
  - MAC processing time (measured on T-beam) */
uint32_t Radio::computeSlotTimeMsec(const float bw, const uint8_t sf) {
    constexpr float sumPropagationTurnaroundMACTime = 0.2 + 0.4 + 7; // in milliseconds
    const float symbolTime = pow(2, sf) / bw;                    // in milliseconds

    // CAD duration for SX127x is max. 2.25 symbols, for SX126x it is number of symbols + 0.5 symbol
    // Number of symbols used for CAD, 2 is the default since RadioLib 6.3.0 as per AN1200.48
    constexpr auto NUM_SYM_CAD = 2;
    return max(2.25, NUM_SYM_CAD + 0.5) * symbolTime + sumPropagationTurnaroundMACTime;
}

/**
 * Calculate airtime per
 * https://www.rs-online.com/designspark/rel-assets/ds-assets/uploads/knowledge-items/application-notes-for-the-internet-of-things/LoRa%20Design%20Guide.pdf
 * section 4
 *
 * @return num msecs for the packet
 */
uint32_t Radio::getPacketTime(const float bw, const uint8_t sf, const uint8_t cr, const uint16_t preambleLength, const uint32_t packetSize) {
    const float bandwidthHz = bw * 1000.0f;
    constexpr bool headDisable = false; // we currently always use the header
    const float tSym = (1 << sf) / bandwidthHz;

    const bool lowDataOptEn = tSym > 16e-3; // Needed if symbol time is >16ms

    const float tPreamble = (preambleLength + 4.25f) * tSym;
    const float numPayloadSym =
        8 + max(ceilf(((8.0f * packetSize - 4 * sf + 28 + 16 - 20 * headDisable) / (4 * (sf - 2 * lowDataOptEn))) * cr), 0.0f);
    const float tPayload = numPayloadSym * tSym;
    const float tPacket = tPreamble + tPayload;

    const uint32_t msecs = tPacket * 1000;

    return msecs;
}