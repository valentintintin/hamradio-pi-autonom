#include "utils/rp2040.h"

#include <RP2040Support.h>
#include <hardware/clocks.h>
#include <hardware/pll.h>
#include <hardware/rtc.h>
#include <hardware/vreg.h>

#include <PicoSleep.h>
#include <FreeRTOS.h>
#include <timers.h>

void setSlowClock() {
    /* Set the system frequency to 18 MHz. */
    set_sys_clock_khz(18 * KHZ, false);
    /* The previous line automatically detached clk_peri from clk_sys, and
       attached it to pll_usb. We need to attach clk_peri back to system PLL to keep SPI
       working at this low speed.
       For details see https://github.com/jgromes/RadioLib/discussions/938
    */
    clock_configure(clk_peri,
                    0, // No glitchless mux
                    CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLKSRC_PLL_SYS, // System PLL on AUX mux
                    18 * MHZ, // Input frequency
                    18 * MHZ // Output (must be same as no divider)
    );
    /* Run also ADC on lower clk_sys. */
    clock_configure(clk_adc, 0, CLOCKS_CLK_ADC_CTRL_AUXSRC_VALUE_CLKSRC_PLL_SYS, 18 * MHZ, 18 * MHZ);
    /* Run RTC from XOSC since USB clock is off */
    clock_configure(clk_rtc, 0, CLOCKS_CLK_RTC_CTRL_AUXSRC_VALUE_XOSC_CLKSRC, 12 * MHZ, 47 * KHZ);
    vreg_set_voltage(VREG_VOLTAGE_0_90);
    /* Turn off USB PLL */
    pll_deinit(pll_usb);
}

void setTimeToInternalRtc(const time_t epoch) {
    datetime_t datetime;
    epoch_to_datetime(epoch, &datetime);
    rtc_set_datetime(&datetime);
}

void rebootTask(TimerHandle_t xTimer) {
    rp2040.reboot();
}

void dfuTask(TimerHandle_t xTimer) {
    rp2040.rebootToBootloader();
}