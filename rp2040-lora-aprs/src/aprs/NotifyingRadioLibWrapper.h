#pragma once

#include "core/RadioActivityNotify.h"
#include <helpers/radiolib/CustomSX1262Wrapper.h>
#include <FreeRTOS.h>
#include <task.h>

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

template <TaskHandle_t& Handle>
class NotifyingRadioLibWrapper : public CustomSX1262Wrapper {
public:
  NotifyingRadioLibWrapper(CustomSX1262& hw, mesh::MainBoard& board) : CustomSX1262Wrapper(hw, board) {}

  void begin() override {
    _preamble_sf = getSpreadingFactor();
    _radio->setPreambleLength(preambleLengthForSF(_preamble_sf));
    _radio->setPacketReceivedAction(&onIrq);  // remplace l'action posée par RadioLibWrapper::begin() (jamais appelée ici)
    s_state = kIdle;

    _noise_floor = 0;
    _threshold = 0;
    _num_floor_samples = 0;
    _floor_sample_sum = 0;
  }

  int recvRaw(uint8_t* bytes, int sz) override {
    int len = 0;
    if (s_state & kIntReady) {
      len = _radio->getPacketLength();
      if (len > 0) {
        if (len > sz) {
          len = sz;
        }
        int err = _radio->readData(bytes, len);
        if (err != RADIOLIB_ERR_NONE) {
          len = 0;
          n_recv_errors++;
        } else {
          n_recv++;
        }
      }
      s_state = kIdle;
    }

    if (s_state != kRx) {
      int err = _radio->startReceive();
      if (err == RADIOLIB_ERR_NONE) {
        s_state = kRx;
      }
    }
    return len;
  }

  bool startSendRaw(const uint8_t* bytes, int len) override {
    _board->onBeforeTransmit();
    int err = _radio->startTransmit((uint8_t*)bytes, len);
    if (err == RADIOLIB_ERR_NONE) {
      s_state = kTxWait;
      return true;
    }
    _radio->standby();
    s_state = kIdle;
    _board->onAfterTransmit();
    return false;
  }

  bool isSendComplete() override {
    if (s_state & kIntReady) {
      s_state = kIdle;
      n_sent++;
      return true;
    }
    return false;
  }

  void onSendFinished() override {
    _radio->finishTransmit();
    _board->onAfterTransmit();
    s_state = kIdle;
  }

  bool isInRecvMode() const override {
    return (s_state & ~kIntReady) == kRx;
  }

  // Calibration de seuil de bruit de RadioLibWrapper::loop() : jamais activée
  // dans ce projet (rien n'appelle triggerNoiseFloorCalibrate()) — no-op
  // plutôt que d'hériter une version qui lirait le `state` de RadioLibWrapper,
  // qu'on ne met plus à jour ici.
  void loop() override {}

private:
  enum : uint8_t { kIdle = 0, kRx = 1, kTxWait = 3, kIntReady = 16 };
  static volatile uint8_t s_state;

  static void onIrq() {
    s_state |= kIntReady;
    if (Handle) {
      BaseType_t woken = pdFALSE;
      vTaskNotifyGiveFromISR(Handle, &woken);
      portYIELD_FROM_ISR(woken);
    }
  }
};

template <TaskHandle_t& Handle>
volatile uint8_t NotifyingRadioLibWrapper<Handle>::s_state = 0;

using MeshRadioWrapper = NotifyingRadioLibWrapper<g_mesh_task_handle>;
using AprsRadioWrapper = NotifyingRadioLibWrapper<g_aprs_task_handle>;
