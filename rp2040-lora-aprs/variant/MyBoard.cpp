#include "MyBoard.h"
#include <Wire.h>

void MyBoard::begin() {
  startup_reason = BD_STARTUP_NORMAL;

  // I2C pour RTC et capteurs (pins par défaut Pico W : SDA=4, SCL=5)
  // TODO: ajuster si tes pins I2C sont différents
  Wire.begin();

  delay(10); // laisser le temps aux SX1262 de démarrer
}
