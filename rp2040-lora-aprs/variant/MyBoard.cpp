#include "MyBoard.h"
#include <Wire.h>

void MyBoard::begin() {
  startup_reason = BD_STARTUP_NORMAL;
  Wire.begin();
  delay(10); // laisser le temps aux SX1262 de démarrer
}
