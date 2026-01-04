#include "hal/LoRaHal.hpp"

#include <ArduinoLog.h>
#include <SPI.h>
#include <RadioLib.h>

#include "controllers/LedController.hpp"

// Callbacks d'interruption statiques
void LoRaHal::onRxInterrupt()
{
    LoRaHal& hal = getInstance();
    
    // DIO1 peut signaler RX_DONE ou TX_DONE, on doit vérifier le statut IRQ
    uint16_t irqStatus = hal.radio.getIrqStatus();
    
    // Vérifier RX_DONE
    if (irqStatus & RADIOLIB_SX126X_IRQ_RX_DONE)
    {
        // Réception terminée, notifier la tâche RX
        if (hal.rxTaskHandle != nullptr)
        {
            BaseType_t xHigherPriorityTaskWoken = pdFALSE;
            vTaskNotifyGiveFromISR(hal.rxTaskHandle, &xHigherPriorityTaskWoken);
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }
    }
    
    // Vérifier TX_DONE
    if (irqStatus & RADIOLIB_SX126X_IRQ_TX_DONE)
    {
        // Transmission terminée, notifier la tâche TX
        if (hal.txTaskHandle != nullptr)
        {
            BaseType_t xHigherPriorityTaskWoken = pdFALSE;
            xTaskNotifyFromISR(hal.txTaskHandle, 1UL, eSetBits, &xHigherPriorityTaskWoken);
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }
    }
}

void LoRaHal::onTxInterrupt()
{
    // Cette fonction peut être utilisée si on configure DIO2 pour TX_DONE
    // Pour l'instant, on utilise DIO1 pour les deux
    onRxInterrupt();
}

void LoRaHal::onCadInterrupt()
{
    LoRaHal& hal = getInstance();
    
    // CAD terminé, vérifier le résultat via le statut IRQ
    uint16_t irqStatus = hal.radio.getIrqStatus();
    
    if (irqStatus & RADIOLIB_SX126X_IRQ_CAD_DETECTED)
    {
        hal.cadResult = false; // Canal occupé (signal LoRa détecté)
    }
    else if (irqStatus & RADIOLIB_SX126X_IRQ_CAD_DONE)
    {
        hal.cadResult = true; // Canal libre (pas de signal détecté)
    }
    else
    {
        hal.cadResult = false; // Erreur ou état inconnu
    }
    
    hal.cadPending = false;
    
    // Notifier la tâche TX
    if (hal.txTaskHandle != nullptr)
    {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xTaskNotifyFromISR(hal.txTaskHandle, 2UL, eSetBits, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

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

bool LoRaHal::begin(const float frequency,
                    const uint16_t bandwidth,
                    const uint8_t spreadingFactor,
                    const uint8_t codingRate,
                    const uint8_t outputPower,
                    const uint8_t syncWord)
{
    if (initialized)
    {
        return true;
    }

    // Initialisation SPI pour LoRa (SPI1)
    SPI1.setRX(LORA_MISO);
    SPI1.setTX(LORA_MOSI);
    SPI1.setSCK(LORA_SCK);
    SPI1.begin();

    // Configuration SPI
    int16_t state = radio.begin(frequency, bandwidth, spreadingFactor, codingRate, syncWord, outputPower, LORA_PREAMBLE_LENGTH);

    if (state != RADIOLIB_ERR_NONE)
    {
        Log.errorln("Échec d'initialisation SX1262: %d", state);
        return false;
    }

    // Configuration des interruptions DIO1 pour RX_DONE et TX_DONE
    // On utilisera getIrqStatus() dans le callback pour différencier
    state = radio.setDio1Action(onRxInterrupt);

    if (state != RADIOLIB_ERR_NONE)
    {
        Log.warningln("Impossible de configurer l'interruption DIO1: %d", state);
    }

    // Configuration des interruptions DIO2 pour CAD_DONE (si disponible)
    // Note: Si DIO2 n'est pas disponible, on peut utiliser DIO3 ou DIO4
    // Pour SX126x, on peut configurer DIO2 pour CAD_DONE
    state = radio.setDio2Action(onCadInterrupt);
    
    if (state != RADIOLIB_ERR_NONE)
    {
        // Essayer avec DIO3 si DIO2 n'est pas disponible
        state = radio.setDio3Action(onCadInterrupt);
        
        if (state != RADIOLIB_ERR_NONE)
        {
            Log.warningln("Impossible de configurer l'interruption CAD (DIO2/DIO3): %d", state);
        }
    }

    // Démarrer en mode réception
    state = radio.startReceive();

    if (state != RADIOLIB_ERR_NONE)
    {
        Log.errorln("Impossible de démarrer la réception: %d", state);
        return false;
    }

    // Création des tâches FreeRTOS
    if (xTaskCreate(txTask, "LoRaTxTask", configMINIMAL_STACK_SIZE * 2, this, tskIDLE_PRIORITY + 1, &txTaskHandle) != pdPASS)
    {
        Log.errorln("Échec de création de la tâche TX LoRa");
        return false;
    }

    if (xTaskCreate(rxTask, "LoRaRxTask", configMINIMAL_STACK_SIZE * 2, this, tskIDLE_PRIORITY + 1, &rxTaskHandle) != pdPASS)
    {
        Log.errorln("Échec de création de la tâche RX LoRa");
        return false;
    }

    initialized = true;
    Log.infoln("LoRaHal initialisé avec succès");
    
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

    // Allouer de la mémoire pour le message
    uint8_t* messageData = new uint8_t[length];
    if (messageData == nullptr)
    {
        Log.errorln("Échec d'allocation mémoire pour message TX");
        return false;
    }

    memcpy(messageData, data, length);

    LoRaTxMessage msg;
    msg.data = messageData;
    msg.length = length;

    if (xQueueSend(txQueue, &msg, portMAX_DELAY) != pdTRUE)
    {
        delete[] messageData;
        Log.errorln("Échec d'envoi dans la queue TX");
        return false;
    }

    return true;
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
        return true;
    }

    return false;
}

bool LoRaHal::startChannelScan()
{
    if (!initialized)
    {
        return false;
    }

    // Démarrer la détection de canal (CAD) de manière non-bloquante
    int16_t state = radio.startChannelScan();

    if (state != RADIOLIB_ERR_NONE)
    {
        Log.warningln("Erreur startChannelScan: %d", state);
        return false;
    }

    cadPending = true;
    return true;
}

void LoRaHal::txTask(void* pvParameters)
{
    LoRaHal* hal = static_cast<LoRaHal*>(pvParameters);
    hal->processTxQueue();
}

void LoRaHal::rxTask(void* pvParameters)
{
    LoRaHal* hal = static_cast<LoRaHal*>(pvParameters);
    hal->processRxQueue();
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
                
                // Réinitialiser les notifications
                uint32_t ulNotificationValue;
                ulTaskNotifyWait(0, ULONG_MAX, &ulNotificationValue, 0);
                
                // Attendre la notification de fin de CAD (timeout de 1 seconde)
                if (ulTaskNotifyWait(0, ULONG_MAX, &ulNotificationValue, pdMS_TO_TICKS(1000)) == pdTRUE)
                {
                    // Vérifier si c'est une notification CAD (bit 1)
                    if (ulNotificationValue & 2UL)
                    {
                        channelFree = cadResult;
                        
                        if (!channelFree)
                        {
                            Log.traceln("Canal occupé, attente...");
                            vTaskDelay(pdMS_TO_TICKS(100)); // Attendre 100ms avant de réessayer
                            retries--;
                        }
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

            // Arrêter la réception temporairement
            radio.standby();

            // Démarrer la transmission de manière non-bloquante
            int16_t state = radio.startTransmit(msg.data, msg.length);

            if (state != RADIOLIB_ERR_NONE)
            {
                Log.errorln("Erreur startTransmit: %d", state);
                
                // Libérer la mémoire en cas d'erreur
                if (msg.data != nullptr)
                {
                    delete[] msg.data;
                }
                
                // Remettre en mode réception
                radio.startReceive();
                continue;
            }

            // Attendre la notification d'interruption de fin de transmission
            // Réinitialiser les notifications en lisant toutes les notifications en attente
            uint32_t ulNotificationValue;
            ulTaskNotifyWait(0, ULONG_MAX, &ulNotificationValue, 0);
            
            // Attendre la notification (timeout de 5 secondes pour sécurité)
            if (ulTaskNotifyWait(0, ULONG_MAX, &ulNotificationValue, pdMS_TO_TICKS(5000)) == pdTRUE)
            {
                // Vérifier si c'est une notification TX (bit 0)
                if (ulNotificationValue & 1UL)
                {
                    // Vérifier le statut de la radio
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
            }
            else
            {
                Log.errorln("Timeout en attente de notification de fin de transmission");
                radio.standby(); // Forcer l'arrêt
            }

            // Libérer la mémoire
            if (msg.data != nullptr)
            {
                delete[] msg.data;
            }

            // Remettre en mode réception
            radio.startReceive();
        }
    }
}

void LoRaHal::processRxQueue()
{
    LoRaRxMessage msg;

    while (true)
    {
        // Attendre la notification d'interruption de réception
        // Réinitialiser les notifications en lisant toutes les notifications en attente
        uint32_t ulNotificationValue;
        ulTaskNotifyWait(0, ULONG_MAX, &ulNotificationValue, 0);
        
        // Attendre la notification (timeout infini)
        if (ulTaskNotifyWait(0, ULONG_MAX, &ulNotificationValue, portMAX_DELAY) == pdTRUE)
        {
            // Lire directement le paquet (pas besoin de vérifier available())
            size_t length = radio.readData(msg.data, TRX_BUFFER);
            
            if (length > 0)
            {
                msg.length = length;
                msg.rssi = radio.getRSSI();
                msg.snr = radio.getSNR();

                Log.traceln("Message RX reçu: %d octets, RSSI: %.2f, SNR: %.2f", 
                           length, msg.rssi, msg.snr);

                // Envoyer dans la queue RX (non-bloquant)
                if (xQueueSend(rxQueue, &msg, 0) != pdTRUE)
                {
                    Log.warningln("Queue RX pleine, message perdu");
                }
            }

            // Remettre en mode réception
            radio.startReceive();
        }
    }
}
