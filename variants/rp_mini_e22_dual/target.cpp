#include <Arduino.h>
#include "target.h"
#include <helpers/ArduinoHelpers.h>

PicoWBoard board;

RADIO_CLASS radio = new Module(P_LORA_NSS, P_LORA_DIO_1, P_LORA_RESET, P_LORA_BUSY, SPI1);
RADIO_CLASS radio_2 = new Module(P_LORA_2_NSS, P_LORA_2_DIO_1, P_LORA_2_RESET, P_LORA_2_BUSY, SPI);

WRAPPER_CLASS radio_driver(radio, board);
WRAPPER_CLASS radio_2_driver(radio_2, board);

VolatileRTCClock fallback_clock;
AutoDiscoverRTCClock rtc_clock(fallback_clock);
SensorManager sensors;

bool radio_init() {
  rtc_clock.begin(Wire);
  
  return radio.std_init(&SPI1) && radio_2.std_init(&SPI) && radio_2.setFrequency(LORA_2_FREQ);
}

mesh::LocalIdentity radio_new_identity() {
  RadioNoiseListener rng(radio);
  return mesh::LocalIdentity(&rng);  // create new random identity
}

