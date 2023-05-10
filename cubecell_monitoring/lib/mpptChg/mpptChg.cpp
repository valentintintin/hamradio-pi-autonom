/*
 * mpptChg.cpp - Code file for danjuliodesigns, LLC MPPT Solar Charger.
 *
 * See mpptChg.h for more information about this software.
 *
 * Compilation of platform dependent components keys on the "ARDUINO" define.
 *
 * Copyright (c) 2018-2022 Dan Julio (dan@danjuliodesigns.com)
 *
 * mpptChg is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * mpptChg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * See <http://www.gnu.org/licenses/>.
 *
 */
#include "mpptChg.h"
#include <stdbool.h>

#ifdef ARDUINO
#include "Arduino.h"
#include "Wire.h"
#else
#include "wiringPi.h"
#include "wiringPiI2C.h"
#endif


// ================================================================================
// Public methods
// ================================================================================
mpptChg::mpptChg()
{
    alertPin = -1;
    nightPin = -1;
}


mpptChg::mpptChg(int aPin)
{
    alertPin = aPin;
    nightPin = -1;
}


mpptChg::mpptChg(int aPin, int nPin)
{
    alertPin = aPin;
    nightPin = nPin;
}


bool mpptChg::begin()
{
#ifdef ARDUINO
    return begin(Wire);
#else
    if (alertPin != -1) {
        pinMode(alertPin, INPUT);
    }
    if (nightPin != -1) {
        pinMode(nightPin, INPUT);
    }

    linuxI2cFd = wiringPiI2CSetup(MPPT_CHG_I2C_ADDR);
	return(linuxI2cFd != -1) && _Read16(0, &val) && val > 0;
#endif
}


bool mpptChg::begin(int sda, int sck)
{
	Wire.begin(sda, sck);
    return begin(Wire);
}


bool mpptChg::begin(TwoWire &wire)
{
	if (alertPin != -1) {
		pinMode(alertPin, INPUT);
	}
	if (nightPin != -1) {
		pinMode(nightPin, INPUT);
	}

    this->wire = wire;

    uint16_t val;
    return _Read16(0, &val) && val > 0;
}


bool mpptChg::getStatusValue(mpptChg_sys_t index, uint16_t* val)
{
    uint8_t reg;

    switch(index) {
        case SYS_ID:
            reg = MPPT_CHG_REG_ID;
            break;
        case SYS_STATUS:
            reg = MPPT_CHG_STATUS;
            break;
        case SYS_BUCK:
            reg = MPPT_CHG_BUCK;
            break;
        default:
            return(false);
    }

    return (_Read16(reg, val));
}


bool mpptChg::getIndexedValue(mpptChg_val_t index, int16_t* val)
{
    bool success;
    uint8_t reg;
    uint16_t t;

    switch(index) {
        case VAL_VS:
            reg = MPPT_CHG_VS;
            break;
        case VAL_IS:
            reg = MPPT_CHG_IS;
            break;
        case VAL_VB:
            reg = MPPT_CHG_VB;
            break;
        case VAL_IB:
            reg = MPPT_CHG_IB;
            break;
        case VAL_IC:
            reg = MPPT_CHG_IC;
            break;
        case VAL_INT_TEMP:
            reg = MPPT_CHG_INT_T;
            break;
        case VAL_EXT_TEMP:
            reg = MPPT_CHG_EXT_T;
            break;
        case VAL_V_MPPT:
            reg = MPPT_CHG_VM;
            break;
        case VAL_V_TH:
            reg = MPPT_CHG_TH;
            break;
        default:
            return(false);
    }

    success = _Read16(reg, &t);
    *val = (int16_t) t;

    return (success);
}


bool mpptChg::getConfigurationValue(mpptChg_cfg_t index, uint16_t* val)
{
    uint8_t reg;

    switch(index) {
        case CFG_BUCK_V_TH:
            reg = MPPT_CHG_BUCK_TH;
            break;
        case CFG_FLOAT_V_TH:
            reg = MPPT_CHG_FLOAT_TH;
            break;
        case CFG_PWR_OFF_TH:
            reg = MPPT_CHG_PWROFF;
            break;
        case CFG_PWR_ON_TH:
            reg = MPPT_CHG_PWRON;
            break;
        default:
            return(false);
    }

    return (_Read16(reg, val));
}


bool mpptChg::setConfigurationValue(mpptChg_cfg_t index, uint16_t val)
{
    uint8_t reg;

    switch(index) {
        case CFG_BUCK_V_TH:
            reg = MPPT_CHG_BUCK_TH;
            break;
        case CFG_FLOAT_V_TH:
            reg = MPPT_CHG_FLOAT_TH;
            break;
        case CFG_PWR_OFF_TH:
            reg = MPPT_CHG_PWROFF;
            break;
        case CFG_PWR_ON_TH:
            reg = MPPT_CHG_PWRON;
            break;
        default:
            return(false);
    }

    return (_Write16(reg, val));
}


bool mpptChg::getWatchdogEnable(bool* val)
{
    bool success;
    uint8_t t;

    success = _Read8(MPPT_WD_EN, &t);
    *val = (t != 0);

    return(success);
}


bool mpptChg::setWatchdogEnable(bool val)
{
    uint16_t t = (val) ? MPPT_CHG_WD_ENABLE : 0;

    return(_Write8(MPPT_WD_EN, t));
}


bool mpptChg::setWatchdogTimeout(uint8_t val)
{
    return(_Write8(MPPT_WD_COUNT, val));
}


bool mpptChg::getWatchdogTimeout(uint8_t* val)
{
    return(_Read8(MPPT_WD_COUNT, val));
}


bool mpptChg::setWatchdogPoweroff(uint16_t val)
{
    return(_Write16(MPPT_WD_PWROFF, val));
}


bool mpptChg::getWatchdogPoweroff(uint16_t* val)
{
    return(_Read16(MPPT_WD_PWROFF, val));
}


bool mpptChg::isAlert(bool* val)
{
    bool success;
    uint16_t t;

    if (alertPin != -1) {
        // Directly read the pin
        *val = (digitalRead(alertPin) == LOW);
        success = true;
    } else {
        // Read the STATUS register
        success = _Read16(MPPT_CHG_STATUS, &t);
        *val = ((t & MPPT_CHG_STATUS_ALERT_MASK) != 0);
    }

    return(success);
}


bool mpptChg::isNight(bool* val)
{
    bool success;
    uint16_t t;

    if (nightPin != -1) {
        // Directly read the pin
        *val = (digitalRead(nightPin) == HIGH);
        success = true;
    } else {
        // Read the STATUS register
        success = _Read16(MPPT_CHG_STATUS, &t);
        *val = ((t & MPPT_CHG_STATUS_NIGHT_MASK) != 0);
    }

    return(success);
}


bool mpptChg::isPowerEnabled(bool* val)
{
    bool success;
    uint16_t t;

    // Read the STATUS register
    success = _Read16(MPPT_CHG_STATUS, &t);
    *val = ((t & MPPT_CHG_STATUS_PWR_EN_MASK) != 0);

    return(success);
}




// ================================================================================
// Private methods
// ================================================================================
bool mpptChg::_Read8(uint8_t reg, uint8_t* val)
{
    bool success;
    int retVal;

#ifdef ARDUINO
    // Set register
    wire.beginTransmission(MPPT_CHG_I2C_ADDR);
    (void) wire.write(reg);
    retVal = wire.endTransmission();

    // Read value
    if (retVal == 0) {
        retVal = wire.requestFrom(MPPT_CHG_I2C_ADDR, 1);
        if (retVal == 1) {
            *val = (uint8_t) wire.read();
            success = true;
        } else {
            success = false;
        }
    } else {
        success = false;
    }
#else
    retVal = wiringPiI2CReadReg8(linuxI2cFd, (int) reg);
	if (retVal != -1) {
		*val = (uint8_t) retVal;
		success = true;
	} else {
		success = false;
	}
#endif

    return(success);
}


bool mpptChg::_Read16(uint8_t reg, uint16_t* val)
{
    bool success;
    int retVal;

#ifdef ARDUINO
    // Set register
    wire.beginTransmission(MPPT_CHG_I2C_ADDR);
    (void) wire.write(reg);
    retVal = wire.endTransmission();

    // Read value
    if (retVal == 0) {
        retVal = wire.requestFrom(MPPT_CHG_I2C_ADDR, 2);
        if (retVal == 2) {
            *val = (uint16_t) wire.read() << 8;
            *val |= (uint16_t) wire.read();
            success = true;
        } else {
            success = false;
        }
    } else {
        success = false;
    }
#else
    retVal = wiringPiI2CReadReg16(linuxI2cFd, (int) reg);
	if (retVal != -1) {
		// Swap bytes because endianness is backwards
		*val = (uint16_t) ((retVal & 0xFF) << 8) | ((retVal >> 8) & 0xFF);
		success = true;
	} else {
		success = false;
	}
#endif

    return(success);
}


bool mpptChg::_Write8(uint8_t reg, uint8_t val)
{
    bool success;
    int retVal;

#ifdef ARDUINO
    // Write register
    wire.beginTransmission(MPPT_CHG_I2C_ADDR);
    (void) wire.write(reg);
    (void) wire.write(val);
    retVal = wire.endTransmission();
    success = (retVal == 0);
#else
    retVal = wiringPiI2CWriteReg8(linuxI2cFd, (int) reg, (int) val);
	success = (retVal >= 0);
#endif

    return(success);
}


bool mpptChg::_Write16(uint8_t reg, uint16_t val)
{
    bool success;
    int retVal;

#ifdef ARDUINO
    // Write register
    wire.beginTransmission(MPPT_CHG_I2C_ADDR);
    (void) wire.write(reg);
    (void) wire.write(val >> 8);
    (void) wire.write(val & 0xFF);
    retVal = wire.endTransmission();
    success = (retVal == 0);
#else
    // Swap bytes because endianness is backwards
	retVal = wiringPiI2CWriteReg16(linuxI2cFd, (int) reg, (int) ((val >> 8) | ((val & 0xFF) << 8)));
	success = (retVal >= 0);
#endif

    return(success);
}

const char *mpptChg::getStatusAsString(uint16_t s) {
    switch (s & MPPT_CHG_STATUS_CHG_ST_MASK) {
        case MPPT_CHG_ST_NIGHT:
            return PSTR("NIGHT");
        case MPPT_CHG_ST_IDLE:
            return PSTR("IDLE");
        case MPPT_CHG_ST_VSRCV:
            return PSTR("VSRCV");
        case MPPT_CHG_ST_SCAN:
            return PSTR("SCAN");
        case MPPT_CHG_ST_BULK:
            return PSTR("BULK");
        case MPPT_CHG_ST_ABSORB:
            return PSTR("ABSORB");
        case MPPT_CHG_ST_FLOAT:
            return PSTR("FLOAT");
        default:
            return PSTR("???");
    }
}

uint16_t mpptChg::computePowerMw(uint16_t mV, uint16_t mA) {
    uint32_t p;

    p = (uint32_t) mV * (uint32_t) mA;
    p = p / (uint32_t) 1000;
    return ((uint16_t) p);
}
