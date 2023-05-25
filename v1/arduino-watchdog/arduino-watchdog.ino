#include <LowPower.h>
#include <mpptChg.h>
#include <Wire.h>
#include <DS3231.h>

#define BLINK_MS 200
#define TRIGGER_WATCHDOG_SECS 80 // 1 minutes 20
#define LOW_BATTERY_VOLTAGE 11400
#define WD_INIT_SECS 1
#define WD_PWROFF_SECS 1
#define I2C_ADDRESS 0x11

byte commandI2C = 0, dataI2C = 0;
uint32_t responseI2C = 0;

byte countSeconds = 0;

bool watchdogEnabled = false, alertAsserted = false, nightDetected = false, powerEnabled = false;
byte watchdogCount = 0;
uint16_t chgStatus = 0, watchdogPowerOff = 0;
uint16_t batteryVoltage = 0;

mpptChg chg;
RTClib RTC;

void receiveData(int byteCount) {
    commandI2C = Wire.read();

    dataI2C = 0;
    if (byteCount > 1) {
        dataI2C = Wire.read();
    }
}

void sendData() {
  if (commandI2C == 1) {
    byte buffer[4];
    buffer[0] = responseI2C & 0xFF;
    buffer[1] = (responseI2C & 0xFF00) >> 8;
    buffer[2] = (responseI2C & 0xFF0000) >> 16;
    buffer[4] = (responseI2C & 0xFF0000) >> 24;
    Wire.write(buffer, 4);
  } else {
    Wire.write(0);
  }
}

void blink(byte nb) {
  for (byte i = 0; i < nb; i++) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(BLINK_MS);
    digitalWrite(LED_BUILTIN, LOW);
    delay(BLINK_MS);
  }
}

void action() {
  Serial.print(F("Watch... "));
  if (
    chg.getStatusValue(SYS_STATUS, &chgStatus) 
    && chg.getWatchdogTimeout(&watchdogCount) 
    && chg.getWatchdogPoweroff(&watchdogPowerOff)
    && chg.getIndexedValue(VAL_VB, &batteryVoltage)
  ) {
    watchdogEnabled = (chgStatus & MPPT_CHG_STATUS_WD_RUN_MASK) != 0;
    powerEnabled = (chgStatus & MPPT_CHG_STATUS_PWR_EN_MASK) != 0;
    alertAsserted = (chgStatus & MPPT_CHG_STATUS_ALERT_MASK) != 0;
    nightDetected = (chgStatus & MPPT_CHG_STATUS_NIGHT_MASK) != 0;
    
    Serial.print(F(" Status : "));
    Serial.print(chgStatus, HEX);
    Serial.print(F(" Battery voltage : "));
    Serial.print(batteryVoltage);
    Serial.print(F(" Watchdog enabled : "));
    Serial.print(watchdogEnabled);
    Serial.print(F(" Watchdog count : "));
    Serial.print(watchdogCount);
    Serial.print(F(" Watchdog poweroff : "));
    Serial.print(watchdogPowerOff);
    Serial.print(F(" Power enabled: "));
    Serial.print(powerEnabled);
    Serial.print(F(" Alert asserted : "));
    Serial.print(alertAsserted);
    Serial.print(F(" Night detected : "));
    Serial.print(nightDetected);
    Serial.print(F(" Count seconds : "));
    Serial.print(countSeconds);

    if (!alertAsserted && !nightDetected && powerEnabled && !watchdogEnabled && batteryVoltage > LOW_BATTERY_VOLTAGE) {
      if (countSeconds >= TRIGGER_WATCHDOG_SECS) {
        watchdogEnabled = true;
        if (chg.setWatchdogPoweroff(WD_PWROFF_SECS) && chg.setWatchdogTimeout(WD_INIT_SECS) && chg.setWatchdogEnable(&watchdogEnabled)) {
          Serial.print(F(" Watchdog forced activated !"));
          blink(3);
          responseI2C = RTC.now().unixtime();
          countSeconds = 0;
        } else {
          Serial.print(F(" Impossible to set wathdog !"));
          blink(4);
          countSeconds += 8;
        }
      } else {
        Serial.print(F(" Watchdog not running !"));
        blink(2);
        countSeconds += 8;
      }
    } else {
      Serial.print(F(" Nothing to do :)"));
      blink(1);
      countSeconds = 0;
    }
  } else {
    countSeconds = 0;
    Serial.print(F(" Communication error with MpptChg charger !"));
    blink(5);
  }
  Serial.println();
  delay(100);
  // LowPower.idle(SLEEP_8S, ADC_OFF, TIMER2_OFF, TIMER1_OFF, TIMER0_OFF, SPI_OFF, USART0_OFF, TWI_OFF); // Normal version
  LowPower.idle(SLEEP_8S, ADC_OFF, TIMER2_OFF, TIMER1_OFF, TIMER0_OFF, SPI_OFF, BOD_OFF, USART0_OFF, TWI_OFF); // Modified @valentin version to gain 2 mA (BOD_OFF)
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);

  Wire.begin(I2C_ADDRESS);

  Wire.onReceive(receiveData);
  Wire.onRequest(sendData);

  Serial.begin(115200);
  Serial.println(F("Started !"));

  chg.begin();
}

void loop() {
  action();
}
