#pragma once

#include "core/RadioActivityNotify.h"
#include <helpers/radiolib/CustomSX1262Wrapper.h>
#include <FreeRTOS.h>
#include <task.h>

#include "core/Log.h"

// Sous-classe de CustomSX1262Wrapper (MeshCore vendoré, non modifié) : réveille
// une tâche FreeRTOS précise depuis l'IRQ DIO1 au lieu de faire du polling.
// RadioLibWrapper garde son état RX/TX dans une statique privée au .cpp,
// inaccessible depuis l'extérieur — d'où la réimplémentation ici avec un état
// propre par instanciation de template (une par radio physique).

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
    _radio->setPacketReceivedAction(setFlag);  // sert aussi d'interruption SentComplete
    _preamble_sf = getSpreadingFactor();
    _radio->setPreambleLength(preambleLengthForSF(_preamble_sf)); // préambule plus long en SF bas = fiabilité accrue
    state = STATE_IDLE;

    if (_board->getStartupReason() == BD_STARTUP_RX_PACKET) {  // réveillé par un paquet LoRa reçu en deep sleep
      setFlag();
    }

    _noise_floor = 0;
    _threshold = 0;

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
          n_recv++;
        }
      }
      state = STATE_IDLE;
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
    idle();
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
        if (rssi < _noise_floor + SAMPLING_THRESHOLD) {
          _num_floor_samples++;
          _floor_sample_sum += rssi;
        }
      }
    } else if (_num_floor_samples >= NUM_NOISE_FLOOR_SAMPLES && _floor_sample_sum != 0) {
      _noise_floor = _floor_sample_sum / NUM_NOISE_FLOOR_SAMPLES;
      if (_noise_floor < -120) {
        _noise_floor = -120;    // plancher du SX1262
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
