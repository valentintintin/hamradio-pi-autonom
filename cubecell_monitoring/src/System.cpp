#include "System.h"
#include "ArduinoLog.h"

bool System::begin() {
    pinMode(Vext, OUTPUT);
    digitalWrite(Vext, LOW);
    pixels.begin();
    pixels.clear();
    turnOnRGB(COLOR_JOINED);

    pinMode(USER_BUTTON,INPUT);

    if (!display.init()) {
        Log.warningln(F("Display error"));

        return false;
    }

    display.wakeup();
    display.flipScreenVertically();

    return true;
}

void System::update() {
    if (timerScreen.hasExpired()) {
        Log.noticeln(F("Display timeout so sleep"));

        display.sleep();
    }

    delay(10);
}

void System::turnOnRGB(uint32_t color) {
    Log.noticeln(F("Turn led color : %l"), color);

    uint8_t red, green, blue;
    red = (uint8_t) (color >> 16);
    green = (uint8_t) (color >> 8);
    blue = (uint8_t) color;
    pixels.setPixelColor(0, CubeCell_NeoPixel::Color(red, green, blue));
    pixels.show();   // Send the updated pixel colors to the hardware.
}

void System::turnOffRGB() {
    turnOnRGB(0);
    //Leave overall management of Vext to the user
    //digitalWrite(Vext, HIGH);
}

void System::turnScreenOn() {
    Log.noticeln(F("Turn screen on"));

    display.wakeup();
    timerScreen.restart();
}

void System::displayText(const char *title, const char *content, uint16_t pause) {
    turnScreenOn();
    display.clear();
    display.drawString(0, 0, title);
    display.drawStringMaxWidth(0, 10, 128, content);
    display.display();
    delay(pause);
}
