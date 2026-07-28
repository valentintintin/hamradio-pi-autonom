#pragma once

#include "hal/rtc/ExternalRtc.h"

// ============================================================================
// ExternalRtcSim — implémentation native d'IExternalRtc (cf.
// hal/rtc/ExternalRtc.h) : pas de puce RX8025T réelle, rien à synchroniser —
// l'horloge hôte fait déjà foi (cf. variant/InternalRp2040RTCClock.h /
// native_sim/compat/rtc_compat.cpp). Contrepartie réelle :
// variant/ExternalRtcReal.h.
// ============================================================================

class ExternalRtcSim : public IExternalRtc {
public:
  bool begin() override { return false; }
  bool isPresent() const override { return false; }
  bool readTime(uint32_t*) override { return false; }
  bool writeTime(uint32_t) override { return false; }
};
