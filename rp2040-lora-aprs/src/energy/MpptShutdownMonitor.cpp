#include "MpptShutdownMonitor.h"
#include "core/Log.h"
#include <Arduino.h>

#define TAG "ENERGY"

// attachInterrupt exige un pointeur de fonction libre (pas de méthode liée à
// une instance) ; un seul MpptShutdownMonitor existe dans ce firmware (une
// seule carte MPPT), donc un flag global suffit.
static volatile bool g_mppt_isr_flag = false;

void MpptShutdownMonitor::onShutdownAlert() {
  g_mppt_isr_flag = true;
}

void MpptShutdownMonitor::begin() {
  pinMode(MPPT_ALERT_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(MPPT_ALERT_PIN), onShutdownAlert, RISING);
}

void MpptShutdownMonitor::update() {
  bool isr_fired = g_mppt_isr_flag;
  g_mppt_isr_flag = false; // consommé, prêt pour la prochaine interruption

  bool alert = false;
  bool have_status = _mppt->isInitialized() && _mppt->isAlertEnabled(alert);
  bool i2c_says_alert = have_status && alert;

  if ((isr_fired || i2c_says_alert) && !_alert_sent) {
    LOG_E(TAG, "MPPT: extinction imminente détectée (%s)", isr_fired ? "GPIO" : "I2C");
    _aprs->sendStatus("ALERTE MPPT: extinction imminente");
    _alert_sent = true;
  } else if (!isr_fired && !i2c_says_alert) {
    _alert_sent = false; // condition retombée : réarme pour la prochaine fois
  }
}
