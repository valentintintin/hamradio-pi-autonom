#include <SparkFunTSL2561.h>
#include "BMP085.h"
#include <Wire.h>
#include <SimpleDHT.h>
#include <LowPower.h>

boolean gain;
unsigned int ms;
int err;

float temperature;
float temperaturePressure;
float humidity;
float pressure;
float atm;
float altitude;
double lux;

SFE_TSL2561 light;
BMP085 myBarometer;
SimpleDHT22 dht22(2);

void setup()
{
  Serial.begin(115200);
  Serial.println("Starting");

  myBarometer.init();
  light.begin();

  unsigned char ID;
  gain = 0;
  unsigned char time = 2;
  light.setTiming(gain, time, ms);
  light.setPowerUp();
  
  Serial.println("Started");
}

void loop()
{
  unsigned int data0, data1;

  if (light.getData(data0,data1) && light.getLux(gain, ms, data0, data1, lux)) {
    Serial.print(F("[LIGHT]"));
    Serial.print(lux);
  }

  temperaturePressure = myBarometer.bmp085GetTemperature(myBarometer.bmp085ReadUT());
  pressure = myBarometer.bmp085GetPressure(myBarometer.bmp085ReadUP());

  Serial.print(F("[PRESSURE_TEMP]"));
  Serial.print(temperaturePressure, 2);

  Serial.print(F("[PRESSURE]"));
  Serial.print(pressure, 0); //whole number only.

  err = SimpleDHTErrSuccess;
  if ((err = dht22.read2(&temperature, &humidity, NULL)) == SimpleDHTErrSuccess) {
    Serial.print(F("[TEMP]"));
    Serial.print(temperature);

    Serial.print(F("[HUMIDITY]"));
    Serial.print(humidity);
  }

  Serial.println();

  delay(100);
  // LowPower.idle(SLEEP_8S, ADC_OFF, TIMER2_OFF, TIMER1_OFF, TIMER0_OFF, SPI_OFF, USART0_OFF, TWI_OFF); // Normal version
  //LowPower.idle(SLEEP_8S, ADC_OFF, TIMER2_OFF, TIMER1_OFF, TIMER0_OFF, SPI_OFF, BOD_OFF, USART0_OFF, TWI_OFF); // Modified @valentin version to gain 2 mA (BOD_OFF)
  //delay(8000);
  delay(900);
}
