#include "hal/LoRaHal.hpp"

#include <ArduinoLog.h>
#include <SPI.h>
#include <RadioLib.h>

#include "controllers/LedController.hpp"

LoRaHal::LoRaHal()
{
    txQueue = xQueueCreate(LORA_QUEUE_TX_SIZE, sizeof(LoRaTxMessage));

    if (txQueue == nullptr)
    {
        Log.errorln("LoRa TX Queue creation failed");

        LedController::getInstance().blink(Error, FreeRtos);
    }

    rxQueue = xQueueCreate(LORA_QUEUE_RX_SIZE, sizeof(LoRaRxMessage));

    if (rxQueue == nullptr)
    {
        Log.errorln("LoRa RX Queue creation failed");

        LedController::getInstance().blink(Error, FreeRtos);
    }
}

bool LoRaHal::begin(const bool txEnabled,
                    const float frequency,
                    const uint16_t bandwidth,
                    const uint8_t spreadingFactor,
                    const uint8_t codingRate,
                    const uint8_t syncWord,
                    const uint8_t outputPower,
                    const uint8_t preambleLength)
{
    Log.infoln("SX1262 Init");

    SPI1.begin();

    if (txTaskHandle == nullptr && xTaskCreate(txTask, "LoRaTxTask", configMINIMAL_STACK_SIZE, this, tskIDLE_PRIORITY, &txTaskHandle) != pdPASS)
    {
        Log.errorln("Échec de création de la tâche TX LoRa");

        LedController::getInstance().blink(Error, Radio);

        return false;
    }

    if (rxTaskHandle == nullptr && xTaskCreate(rxTask, "LoRaRxTask", configMINIMAL_STACK_SIZE, this, tskIDLE_PRIORITY, &rxTaskHandle) != pdPASS)
    {
        Log.errorln("Échec de création de la tâche RX LoRa");

        LedController::getInstance().blink(Error, Radio);

        return false;
    }

    initialized = false;

    float tcxo = 1.6;

#ifdef LORA_DIO3_TCXO_VOLTAGE
    tcxo = LORA_DIO3_TCXO_VOLTAGE;
#endif

    int16_t state = radio.begin(frequency, bandwidth, spreadingFactor, codingRate, syncWord, outputPower, preambleLength, tcxo);

    if (state == RADIOLIB_ERR_SPI_CMD_FAILED || state == RADIOLIB_ERR_SPI_CMD_INVALID)
    {
        Log.warningln("SX1262 failed SPI, try with TCXO 0");

        // if radio init fails with -707/-706, try again with tcxo voltage set to 0
        state = radio.begin(frequency, bandwidth, spreadingFactor, codingRate, syncWord, outputPower, preambleLength, 0);
    }

    if (state != RADIOLIB_ERR_NONE)
    {
        Log.errorln("Échec d'initialisation SX1262: %d", state);
        LedController::getInstance().blink(Error, Radio);

        return false;
    }

    state = radio.setCRC(true);

    if (state != RADIOLIB_ERR_NONE)
    {
        Log.errorln("Échec d'initialisation SX1262: %d", state);

        LedController::getInstance().blink(Error, Radio);

        return false;
    }

#ifdef LORA_DIO2_AS_RF_SWITCH
    state = radio.setDio2AsRfSwitch(LORA_DIO2_AS_RF_SWITCH);

    if (state != RADIOLIB_ERR_NONE)
    {
        Log.errorln("Échec set dio2 as RF Switch SX1262: %d", state);

        LedController::getInstance().blink(Error, Radio);

        return false;
    }
#endif

#ifdef LORA_CURRENT_LIMIT
    state = radio.setCurrentLimit(LORA_CURRENT_LIMIT);

    if (state != RADIOLIB_ERR_NONE)
    {
        Log.warningln("Échec set current limit SX1262: %d", state);

        LedController::getInstance().blink(Error, Radio);
    }
#endif

#ifdef LORA_RX_BOOSTED_GAIN
    state = radio.setRxBoostedGainMode(LORA_RX_BOOSTED_GAIN);

    if (state != RADIOLIB_ERR_NONE)
    {
        Log.warningln("Échec set rx boosted SX1262: %d", state);

        LedController::getInstance().blink(Error, Radio);
    }
#endif

#if defined(LORA_RXEN) || defined(LORA_TXEN)
#ifndef LORA_RXEN
#define LORA_RXEN RADIOLIB_NC
#endif
#ifndef LORA_TXEN
#define LORA_TXEN RADIOLIB_NC
#endif
    radio.setRfSwitchPins(LORA_RXEN, LORA_TXEN);
#endif

    if (!startReceive())
    {
        Log.errorln("Impossible de démarrer la réception");

        LedController::getInstance().blink(Error, Radio);

        return false;
    }

    initialized = true;
    this->txEnabled = txEnabled;

    Log.infoln("SX1262 initialisé avec succès");

    LedController::getInstance().blink(Success, Radio);
    
    return true;
}

bool LoRaHal::send(const uint8_t* data, const size_t length)
{
    if (!initialized)
    {
        Log.warningln("LoRaHal non initialisé");
        return false;
    }

    if (!txEnabled)
    {
        Log.warningln("TX désactivé");
        return false;
    }

    if (length > TRX_BUFFER)
    {
        Log.warningln("Message trop long: %d > %d", length, TRX_BUFFER);
        return false;
    }

    LoRaTxMessage msg;
    memcpy(msg.data, data, length);
    msg.length = length;

    if (xQueueSend(txQueue, &msg, portMAX_DELAY) != pdTRUE)
    {
        Log.errorln("Queue TX pleine");
        return false;
    }

    return true;
}

void LoRaHal::txTask(void* pvParameters)
{
    const auto hal = static_cast<LoRaHal*>(pvParameters);
    hal->processTxQueue();
}

void LoRaHal::processTxQueue()
{
    LoRaTxMessage msg;

    while (true)
    {
        // Attendre un message dans la queue
        if (xQueueReceive(txQueue, &msg, portMAX_DELAY) == pdTRUE)
        {
            // Attendre que le canal soit libre avec CAD en interruption
            bool channelFree = false;
            int retries = 10; // Maximum 10 tentatives de CAD

            while (!channelFree && retries > 0)
            {
                // Démarrer le CAD en interruption
                if (!startChannelScan())
                {
                    Log.warningln("Échec du démarrage du CAD");
                    break;
                }

                // Attendre la notification de fin de CAD
                if (ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(TIMEOUT_CAD_DETECTION)) == pdTRUE)
                {
                    // CAD terminé, vérifier le résultat via le statut IRQ
                    const auto irqStatus = radio.getIrqFlags();

                    if (irqStatus & RADIOLIB_SX126X_IRQ_CAD_DONE)
                    {
                        channelFree = true;

                        Log.traceln("Canal libre !");
                    }
                    else
                    {
                        channelFree = false;
                        retries--;

                        Log.traceln("Canal occupé (%d), tentatives restantes : %d", irqStatus, retries);
                    }
                }
                else
                {
                    Log.warningln("Timeout CAD");
                    retries--;
                }
            }

            if (!channelFree)
            {
                Log.warningln("Canal toujours occupé après plusieurs tentatives");
            }

            standby();

            Log.infoln("TX !");

            radio.setDio1Action(onTxInterrupt);
            int16_t state = radio.startTransmit(msg.data, msg.length);

            if (state != RADIOLIB_ERR_NONE)
            {
                Log.errorln("Erreur startTransmit: %d", state);

                startReceive();
                continue;
            }

            // Attendre la notification (timeout de 5 secondes pour sécurité)
            if (ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(5000)) == pdTRUE)
            {
                state = radio.finishTransmit();

                if (state == RADIOLIB_ERR_NONE)
                {
                    Log.traceln("Message TX envoyé: %d octets", msg.length);
                }
                else if (state == RADIOLIB_ERR_TX_TIMEOUT)
                {
                    Log.warningln("Timeout de transmission");
                }
                else
                {
                    Log.errorln("Erreur finishTransmit: %d", state);
                }
            }
            else
            {
                Log.errorln("Timeout en attente de notification de fin de transmission");
                standby(); // Forcer l'arrêt
            }

            // Remettre en mode réception
            startReceive();
        }
    }
}

bool LoRaHal::startChannelScan()
{
    if (!initialized)
    {
        return false;
    }

    Log.infoln("SX1262, démarrage de la détection");

    const int16_t state = radio.startChannelScan();

    if (state != RADIOLIB_ERR_NONE)
    {
        Log.warningln("Erreur startChannelScan: %d", state);
        return false;
    }

    return true;
}

void LoRaHal::onCadInterrupt()
{
    const auto& hal = getInstance();

    if (hal.txTaskHandle != nullptr)
    {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        vTaskNotifyGiveFromISR(hal.txTaskHandle, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

void LoRaHal::onTxInterrupt()
{
    const auto& hal = getInstance();

    if (hal.txTaskHandle != nullptr)
    {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        vTaskNotifyGiveFromISR(hal.txTaskHandle, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

void LoRaHal::onRxInterrupt()
{
    const auto& hal = getInstance();

    if (hal.rxTaskHandle != nullptr)
    {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        vTaskNotifyGiveFromISR(hal.rxTaskHandle, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

bool LoRaHal::receive(LoRaRxMessage& message, const TickType_t timeout)
{
    if (!initialized)
    {
        Log.warningln("LoRaHal non initialisé");
        return false;
    }

    if (xQueueReceive(rxQueue, &message, timeout) == pdTRUE)
    {
        // TODO décodage APRS/MT/MC
        return true;
    }

    return false;
}

void LoRaHal::rxTask(void* pvParameters)
{
    const auto hal = static_cast<LoRaHal*>(pvParameters);
    hal->processRxQueue();
}

void LoRaHal::processRxQueue()
{
    LoRaRxMessage msg;

    while (true)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        const size_t length = radio.readData(msg.data, TRX_BUFFER);

        if (length > 0)
        {
            msg.length = length;
            msg.rssi = radio.getRSSI();
            msg.snr = radio.getSNR();

            Log.traceln("Message RX reçu: %d octets, RSSI: %.2f, SNR: %.2f", length, msg.rssi, msg.snr);

            if (xQueueSend(rxQueue, &msg, 0) != pdTRUE)
            {
                Log.warningln("Queue RX pleine, message perdu");
            }
        }

        startReceive();
    }
}

bool LoRaHal::startReceive()
{
    radio.setDio1Action(onRxInterrupt);

    const auto state = radio.startReceive();

    if (state != RADIOLIB_ERR_NONE)
    {
        Log.errorln("Impossible de démarrer la réception: %d", state);

        LedController::getInstance().blink(Error, Radio);

        return false;
    }

    return true;
}

bool LoRaHal::standby()
{
    radio.clearDio1Action();

    const auto state = radio.standby();

    if (state != RADIOLIB_ERR_NONE)
    {
        Log.errorln("Impossible de se mettre en standby: %d", state);

        LedController::getInstance().blink(Error, Radio);

        return false;
    }

    return true;
}
