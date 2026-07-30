#pragma once

#include "core/RadioActivityNotify.h"
#include <helpers/radiolib/CustomSX1262Wrapper.h>
#include <FreeRTOS.h>
#include <task.h>

#include "core/Log.h"

// ============================================================================
// NotifyingRadioLibWrapper<Handle> — sous-classe project-owned de
// CustomSX1262Wrapper (MeshCore vendoré, INTACT — ce fichier ne le modifie
// pas) qui réveille une tâche FreeRTOS précise depuis l'IRQ DIO1 (paquet reçu
// ou émission terminée), au lieu du polling en vTaskDelay(1) fait ailleurs.
//
// Pourquoi une sous-classe plutôt qu'un patch de RadioLibWrapper (cf.
// lib/MeshCore/src/helpers/radiolib/RadioLibWrappers.cpp) : celui-ci garde
// son état RX/TX ("state") dans une variable fichier-statique PRIVÉE au
// fichier .cpp — invisible depuis l'extérieur, donc impossible de s'y
// "brancher" sans dupliquer les quelques méthodes qui la lisent/l'écrivent.
// Cette classe réimplémente donc ces méthodes (toutes virtuelles côté
// RadioLibWrapper) avec son propre état, en réutilisant _radio/_board/
// n_recv/n_sent/n_recv_errors — protégés dans RadioLibWrapper, donc
// accessibles à une sous-classe — pour ne dupliquer que le strict nécessaire.
// RadioLibWrapper et CustomSX1262Wrapper restent 100% upstream.
//
// `Handle` (référence vers un TaskHandle_t externe, cf.
// core/RadioActivityNotify.h — g_mesh_task_handle ou g_aprs_task_handle) est
// un paramètre de template non-type : chaque instanciation (une par radio
// physique) obtient son propre état statique indépendant ET sait exactement
// quelle tâche réveiller, sans ambiguïté possible — contrairement au `state`
// partagé de RadioLibWrapper (une seule variable pour les deux radios de ce
// projet), qui ne permet pas de savoir laquelle des deux a déclenché l'IRQ.
// ============================================================================

#define STATE_IDLE       0
#define STATE_RX         1
#define STATE_TX_WAIT    3
#define STATE_TX_DONE    4
#define STATE_INT_READY 16

#define NUM_NOISE_FLOOR_SAMPLES  64
#define SAMPLING_THRESHOLD  14

template <TaskHandle_t& Handle>
class NotifyingRadioLibWrapper : public CustomSX1262Wrapper {
public:
  NotifyingRadioLibWrapper(CustomSX1262& hw, mesh::MainBoard& board) : CustomSX1262Wrapper(hw, board) {}

  void begin() override {
    _radio->setPacketReceivedAction(setFlag);  // this is also SentComplete interrupt
    _preamble_sf = getSpreadingFactor();
    _radio->setPreambleLength(preambleLengthForSF(_preamble_sf)); // longer preamble for lower SF improves reliability
    state = STATE_IDLE;

    if (_board->getStartupReason() == BD_STARTUP_RX_PACKET) {  // received a LoRa packet (while in deep sleep)
      setFlag(); // LoRa packet is already received
    }

    _noise_floor = 0;
    _threshold = 0;

    // start average out some samples
    _num_floor_samples = 0;
    _floor_sample_sum = 0;
  }

  int recvRaw(uint8_t* bytes, int sz) override {
    int len = 0;
    if (state & STATE_INT_READY) {
      len = _radio->getPacketLength();
      if (len > 0) {
        if (len > sz) { len = sz; }
        int err = _radio->readData(bytes, len);
        if (err != RADIOLIB_ERR_NONE) {
          LOG_W("RadioLibWrapper", "error: readData(%d)", err);
          len = 0;
          n_recv_errors++;
        } else {
          //  Serial.print("  readData() -> "); Serial.println(len);
          n_recv++;
        }
      }
      state = STATE_IDLE;   // need another startReceive()
    }

    if (state != STATE_RX) {
      int err = _radio->startReceive();
      if (err == RADIOLIB_ERR_NONE) {
        state = STATE_RX;
      } else {
        LOG_W("RadioLibWrapper", "error: startReceive(%d)", err);
      }
    }
    return len;
  }

  bool startSendRaw(const uint8_t* bytes, int len) override {
    _board->onBeforeTransmit();
    int err = _radio->startTransmit((uint8_t *) bytes, len);
    if (err == RADIOLIB_ERR_NONE) {
      state = STATE_TX_WAIT;
      return true;
    }
    LOG_W("RadioLibWrapper", "error: startTransmit(%d)", err);
    idle();   // trigger another startRecv()
    _board->onAfterTransmit();
    return false;
  }

  bool isSendComplete() override {
    if (state & STATE_INT_READY) {
      state = STATE_IDLE;
      n_sent++;
      return true;
    }
    return false;
  }

  void onSendFinished() override {
    _radio->finishTransmit();
    _board->onAfterTransmit();
    state = STATE_IDLE;
  }

  bool isInRecvMode() const override {
    return (state & ~STATE_INT_READY) == STATE_RX;
  }

  void loop() override {
    if (state == STATE_RX && _num_floor_samples < NUM_NOISE_FLOOR_SAMPLES) {
      if (!isReceivingPacket()) {
        int rssi = getCurrentRSSI();
        if (rssi < _noise_floor + SAMPLING_THRESHOLD) {  // only consider samples below current floor + sampling THRESHOLD
          _num_floor_samples++;
          _floor_sample_sum += rssi;
        }
      }
    } else if (_num_floor_samples >= NUM_NOISE_FLOOR_SAMPLES && _floor_sample_sum != 0) {
      _noise_floor = _floor_sample_sum / NUM_NOISE_FLOOR_SAMPLES;
      if (_noise_floor < -120) {
        _noise_floor = -120;    // clamp to lower bound of -120dBi
      }
      _floor_sample_sum = 0;

      LOG_D("RadioLibWrapper", "noise_floor = %d", (int)_noise_floor);
    }
  }

private:
  static volatile uint8_t state;

  static void setFlag() {
    state |= STATE_INT_READY;
    if (Handle) {
      BaseType_t woken = pdFALSE;
      vTaskNotifyGiveFromISR(Handle, &woken);
      portYIELD_FROM_ISR(woken);
    }
  }
};

template <TaskHandle_t& Handle>
volatile uint8_t NotifyingRadioLibWrapper<Handle>::state = 0;

using MeshRadioWrapper = NotifyingRadioLibWrapper<g_mesh_task_handle>;
using AprsRadioWrapper = NotifyingRadioLibWrapper<g_aprs_task_handle>;
