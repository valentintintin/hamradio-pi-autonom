#include "Radio.h"

#include <numeric>

#include <ArduinoLog.h>
#include <FreeRTOS.h>
#include <task.h>

#include "Settings.h"

volatile RadioState Radio::radioStatus = IDLE;
QueueHandle_t Radio::irqQueue = xQueueCreate(10, 0);

void Radio::onISR() {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xQueueSendFromISR(irqQueue, nullptr, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

Radio::Radio() {
    rxQueue = xQueueCreate(LORA_QUEUE_RX_SIZE, sizeof(LoRaReceived));
    txQueue = xQueueCreate(LORA_QUEUE_TX_SIZE, sizeof(LoRaTransmit));
    txDone = xSemaphoreCreateBinary();

    xTaskCreate(&Radio::taskProcessInterruptRun, "RadioISRTask", 2048, this, 2, nullptr);
    xTaskCreate(&Radio::taskReceiveRun, "RadioRxTask", 2048, this, 2, nullptr);
    xTaskCreate(&Radio::taskTransmitRun, "RadioTxTask", 2048, this, 2, nullptr);
}

bool Radio::init() {
    Log.infoln(F("[LORA] Init"));

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

    // const SettingsLoRa settings = system->settings.lora;

    return changeLoRaSettings(settings.frequency, settings.bandwidth, settings.spreadingFactor, settings.codingRate,
                              settings.outputPower, settings.boostedRxGain);
}

bool Radio::changeLoRaSettings(const float frequency, const uint16_t bandwidth, const uint8_t spreadingFactor,
                               const uint8_t codingRate, const uint8_t outputPower, const uint8_t syncWord,
                               bool boostedRxGain) {
    radioStatus = IDLE;

    auto state = lora.begin(frequency, bandwidth, spreadingFactor, codingRate, syncWord, outputPower,
                            LORA_PREAMBLE_LENGTH, 0, false);
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

    lora.clearIrqFlags(IRQS);
    lora.setIrqFlags(IRQS);
    lora.setDio1Action(onISR);

    xQueueReset(irqQueue);
    xQueueReset(rxQueue);
    xQueueReset(txQueue);
    xSemaphoreGive(txDone);

    slotTimeMsec = computeSlotTimeMsec(bandwidth, spreadingFactor);
    maxPacketTimeMsec = getPacketTime(bandwidth, spreadingFactor, codingRate, LORA_PREAMBLE_LENGTH, TRX_BUFFER);

    Log.traceln(
        F("[LORA] SlotTime %d ms | PreambleTime %d ms | MaxPacketTime %d ms"), slotTimeMsec, preambleTimeMsec,
        maxPacketTimeMsec);

    if (!startReceive()) {
        return false;
    }

    Log.infoln(
        F(
            "[LORA] Init OK to frequency: %f, bandwidth: %d, spreading factor: %d, coding rate: %d, sync word: %d, output power: %d, rx boosted : %d"),
        frequency, bandwidth, spreadingFactor, codingRate, syncWord, outputPower, boostedRxGain);

    return true;
}

void Radio::processInterruptTask() {
    while (true) {
        switch (getAndClearIrq()) {
            case RxTimeout:
                startReceive();
                break;
            case CadDetected:
                // TODO relancer la detection
                break;
            case CadDone:
                // TODO envoyer
                break;
            case RxDone:
                startReceive();
                break;
            case TxDone:
                startReceive();
                break;
            case CrcError:
                startReceive();
                break;
            case Pending:
            default:
                // TODO error
                break;
        }
    }
}

void Radio::receiveTask() {
    LoRaReceived pkt;

    while (true) {
        if (xQueueReceive(rxQueue, &pkt, portMAX_DELAY) == pdTRUE) {
            // TODO décodage APRS
        }
    }
}

void Radio::transmitTask() {
    LoRaTransmit pkt;

    while (true) {
        if (xQueueReceive(txDone, &pkt, portMAX_DELAY) == pdTRUE) {
        }
    }
}

// void Radio::processTxQueue() {
//     while (true) {
//     if (uxQueueMessagesWaiting(txQueue) > 0 && radioStatus != TX) {
//         if (wantToSend) {
//             if (timerNextTx.hasExpired()) {
//                 wantToSend = false;
//
//                 if (!canSendImmediately() || isChannelActive()) {
//                     Log.traceln(F("[LORA_TX] Can't send yet"));
//                     startReceive();
//                 } else {
//                     LoRaTransmit loraTransmit;
//                     if (xQueuePeek(txQueue, &loraTransmit, 0) == pdTRUE) {
//                         if (startSend(loraTransmit.payload, loraTransmit.size)) {
//                             // Remove from queue after successful start
//                             xQueueReceive(txQueue, &loraTransmit, 0);
//                         }
//                     }
//                 }
//             }
//         } else {
//             wantToSend = true;
//             timerNextTx.setInterval(getTxDelayMsec());
//
//             Log.infoln(F("[LORA_TX] Next send in %d ms. %d remaining"), timerNextTx.getTimeLeft(), uxQueueMessagesWaiting(txQueue));
//         }
//     }
// }

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

    if (uxQueueSpacesAvailable(txQueue) == 0) {
        Log.warningln(F("[LORA_TX] Queue is full"));
        return false;
    }

    LoRaTransmit loraTransmit{};
    memset(loraTransmit.payload, '\0', TRX_BUFFER);
    memcpy(loraTransmit.payload, payload, size);
    loraTransmit.size = size;

    if (xQueueSend(txQueue, &loraTransmit, 0) != pdTRUE) {
        Log.errorln(F("[LORA_TX] Impossible to enqueue"));
        return false;
    }

    Log.infoln(F("[LORA_TX] %d in TX queue"), uxQueueMessagesWaiting(txQueue));

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

    if (xSemaphoreTake(statusMutex, portMAX_DELAY) == pdTRUE) {
        radioStatus = RX;
        xSemaphoreGive(statusMutex);
    }
    // Must be done AFTER, starting transmit, because startTransmit clears (possibly stale) interrupt pending register bits
    // lora.setDio1Action(onISR);

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

    return true;
}

InterruptType Radio::getAndClearIrq() {
    const auto flags = lora.getIrqFlags();

    if (flags & RADIOLIB_IRQ_CAD_DETECTED) {
        lora.clearIrqFlags(RADIOLIB_IRQ_CAD_DETECTED);
        return CadDetected;
    }

    if (flags & RADIOLIB_IRQ_CAD_DONE) {
        lora.clearIrqFlags(RADIOLIB_IRQ_CAD_DONE);
        return CadDone;
    }

    if (flags & RADIOLIB_IRQ_RX_DONE) {
        lora.clearIrqFlags(RADIOLIB_IRQ_RX_DONE);
        return RxDone;
    }

    if (flags & RADIOLIB_IRQ_TX_DONE) {
        lora.clearIrqFlags(RADIOLIB_IRQ_TX_DONE);
        return TxDone;
    }

    if (flags & RADIOLIB_IRQ_TIMEOUT) {
        lora.clearIrqFlags(RADIOLIB_IRQ_TIMEOUT);
        return RxTimeout;
    }

    if (flags & RADIOLIB_IRQ_CRC_ERR) {
        lora.clearIrqFlags(RADIOLIB_IRQ_CRC_ERR);
        return CrcError;
    }

    // Error !
    return Pending;
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

    if (state != RADIOLIB_ERR_NONE || size < 15) {
        // APRS Frame are always >= 15
        Log.warningln(F("[LORA_RX] Wrong packet received, error (%d)"), state);
        return false;
    }

    nbRx++;

#if !USE_RX_QUEUE
    system->communication.received(buffer, size, rssi, snr);
    return true;
#endif

    if (uxQueueMessagesWaiting(rxQueue) == 0) {
        system->communication.received(buffer, size, rssi, snr);
        return true;
    }

    if (uxQueueSpacesAvailable(rxQueue) == 0) {
        Log.warningln(F("[LORA_RX] Queue full. Ignore packet"));
        return false;
    }

    LoRaReceived loraReceived = {};
    memset(loraReceived.payload, '\0', TRX_BUFFER);
    memcpy(loraReceived.payload, buffer, size);
    loraReceived.size = size;
    loraReceived.rssi = rssi;
    loraReceived.snr = snr;

    if (xQueueSend(rxQueue, &loraReceived, 0) != pdTRUE) {
        Log.errorln(F("[LORA_RX] Impossible to enqueue"));
        return false;
    }

    Log.infoln(F("[LORA_RX] %d packet in RX queue"), uxQueueMessagesWaiting(rxQueue));

    return true;
}

/** Slottime is the time to detect a transmission has started, consisting of:
  - CAD duration;
  - roundtrip air propagation time (assuming max. 30km between nodes);
  - Tx/Rx turnaround time (maximum of SX126x and SX127x);
  - MAC processing time (measured on T-beam) */
uint32_t Radio::computeSlotTimeMsec(const float bw, const uint8_t sf) {
    const float symbolTime = pow(2, sf) / bw; // in milliseconds
    // CAD duration for SX127x is max. 2.25 symbols, for SX126x it is number of symbols + 0.5 symbol
    // Number of symbols used for CAD, 2 is the default since RadioLib 6.3.0 as per AN1200.48
    constexpr auto NUM_SYM_CAD = 2;
    return max(2.25, NUM_SYM_CAD + 0.5) * symbolTime;
}

/**
 * Calculate airtime per
 * https://www.rs-online.com/designspark/rel-assets/ds-assets/uploads/knowledge-items/application-notes-for-the-internet-of-things/LoRa%20Design%20Guide.pdf
 * section 4
 *
 * @return num msecs for the packet
 */
uint32_t Radio::getPacketTime(const float bw, const uint8_t sf, const uint8_t cr, const uint16_t preambleLength,
                              const uint32_t packetSize) {
    const float bandwidthHz = bw * 1000.0f;
    constexpr bool headDisable = false; // we currently always use the header
    const float tSym = (1 << sf) / bandwidthHz;

    const bool lowDataOptEn = tSym > 16e-3; // Needed if symbol time is >16ms

    const float tPreamble = (preambleLength + 4.25f) * tSym;
    const float numPayloadSym =
            8 + max(
                ceilf(((8.0f * packetSize - 4 * sf + 28 + 16 - 20 * headDisable) / (4 * (sf - 2 * lowDataOptEn))) * cr),
                0.0f);
    const float tPayload = numPayloadSym * tSym;
    const float tPacket = tPreamble + tPayload;

    const uint32_t msecs = tPacket * 1000;

    return msecs;
}

extern "C" void Radio::taskProcessInterruptRun(void *params) {
    const auto radio = static_cast<Radio *>(params);
    radio->processInterruptTask();
}

extern "C" void Radio::taskReceiveRun(void *params) {
    const auto radio = static_cast<Radio *>(params);
    radio->receiveTask();
}

extern "C" void Radio::taskTransmitRun(void *params) {
    const auto radio = static_cast<Radio *>(params);
    radio->transmitTask();
}
