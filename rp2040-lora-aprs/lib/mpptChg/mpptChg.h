/*
 * mpptChg.h - Header file for danjuliodesigns, LLC MPPT Solar Charger.
 *
 * Copyright (c) 2018-2022 Dan Julio (dan@danjuliodesigns.com)
 *
 * mpptChg is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
#ifndef MPPT_CHG_H_
#define MPPT_CHG_H_

#include <inttypes.h>
#include <pgmspace.h>
#include <Wire.h>

#define MPPT_CHG_I2C_ADDR 0x12

// RO values (16-bits)
#define MPPT_CHG_REG_ID   0
#define MPPT_CHG_STATUS   2
#define MPPT_CHG_BUCK     4
#define MPPT_CHG_VS       6
#define MPPT_CHG_IS       8
#define MPPT_CHG_VB       10
#define MPPT_CHG_IB       12
#define MPPT_CHG_IC       14
#define MPPT_CHG_INT_T    16
#define MPPT_CHG_EXT_T    18
#define MPPT_CHG_VM       20
#define MPPT_CHG_TH       22
// RW Parameters (16-bits)
#define MPPT_CHG_BUCK_TH  24
#define MPPT_CHG_FLOAT_TH 26
#define MPPT_CHG_PWROFF   28
#define MPPT_CHG_PWRON    30
// Watchdog registers (8-bits)
#define MPPT_WD_EN        33
#define MPPT_WD_COUNT     35
#define MPPT_WD_PWROFF    36

// Status Register bit masks
#define MPPT_CHG_STATUS_HW_WD_MASK    0x8000
#define MPPT_CHG_STATUS_SW_WD_MASK    0x4000
#define MPPT_CHG_STATUS_BAD_BATT_MASK 0x2000
#define MPPT_CHG_STATUS_EXT_MISS_MASK 0x1000
#define MPPT_CHG_STATUS_WD_RUN_MASK   0x0100
#define MPPT_CHG_STATUS_PWR_EN_MASK   0x0080
#define MPPT_CHG_STATUS_ALERT_MASK    0x0040
#define MPPT_CHG_STATUS_PCTRL_MASK    0x0020
#define MPPT_CHG_STATUS_T_LIM_MASK    0x0010
#define MPPT_CHG_STATUS_NIGHT_MASK    0x0008
#define MPPT_CHG_STATUS_CHG_ST_MASK   0x0007

// Charge States
#define MPPT_CHG_ST_NIGHT  0
#define MPPT_CHG_ST_IDLE   1
#define MPPT_CHG_ST_VSRCV  2
#define MPPT_CHG_ST_SCAN   3
#define MPPT_CHG_ST_BULK   4
#define MPPT_CHG_ST_ABSORB 5
#define MPPT_CHG_ST_FLOAT  6

// Watchdog enable register value
#define MPPT_CHG_WD_ENABLE 0xEA

typedef enum { SYS_ID = 0, SYS_STATUS, SYS_BUCK } mpptChg_sys_t;

typedef enum {
    VAL_VS = 0, VAL_IS, VAL_VB, VAL_IB, VAL_IC,
    VAL_INT_TEMP, VAL_EXT_TEMP, VAL_V_MPPT, VAL_V_TH
} mpptChg_val_t;

typedef enum {
    CFG_BUCK_V_TH = 0, CFG_FLOAT_V_TH, CFG_PWR_OFF_TH, CFG_PWR_ON_TH
} mpptChg_cfg_t;

class mpptChg {
public:
    mpptChg();
    mpptChg(int aPin);
    mpptChg(int aPin, int nPin);
    bool begin();
    bool begin(TwoWire &wire);
    bool getStatusValue(mpptChg_sys_t index, uint16_t* val);
    bool getIndexedValue(mpptChg_val_t index, int16_t* val);
    bool getConfigurationValue(mpptChg_cfg_t index, uint16_t* val);
    bool setConfigurationValue(mpptChg_cfg_t index, uint16_t val);
    bool getWatchdogEnable(bool* val);
    bool setWatchdogEnable(bool val);
    bool setWatchdogTimeout(uint8_t val);
    bool getWatchdogTimeout(uint8_t* val);
    bool setWatchdogPoweroff(uint16_t val);
    bool getWatchdogPoweroff(uint16_t* val);
    bool isAlert(bool* val);
    bool isNight(bool* val);
    bool isPowerEnabled(bool* val);
    static const char* getStatusAsString(uint16_t s);
    static uint16_t computePowerMw(uint16_t mV, uint16_t mA);

private:
    bool _Read8(uint8_t reg, uint8_t* val);
    bool _Read16(uint8_t reg, uint16_t* val);
    bool _Write8(uint8_t reg, uint8_t val);
    bool _Write16(uint8_t reg, uint16_t val);

    TwoWire &wire = Wire;
    int alertPin;
    int nightPin;
#ifndef ARDUINO
    int linuxI2cFd;
#endif
};

#endif // MPPT_CHG_H_
