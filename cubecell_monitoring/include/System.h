#ifndef CUBECELL_MONITORING_SYSTEM_H
#define CUBECELL_MONITORING_SYSTEM_H

#include <cstdint>
#include <CubeCell_NeoPixel.h>
#include <HT_SH1107Wire.h>
#include "Timer.h"
#include "Config.h"

class System {
public:
    bool begin();
    void update();

    void turnOnRGB(uint32_t color);
    void turnOffRGB();

    void turnScreenOn();
    void displayText(const char* title, const char* content, uint16_t pause = 2500);

    SH1107Wire display = SH1107Wire(0x3c, 500000, SDA, SCL, GEOMETRY_128_64, GPIO10);
private:
    CubeCell_NeoPixel pixels = CubeCell_NeoPixel(1, RGB, NEO_GRB + NEO_KHZ800);
    Timer timerScreen = Timer(TIME_SCREEN_ON);
};


#endif //CUBECELL_MONITORING_SYSTEM_H
