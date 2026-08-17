#pragma once

#include <cstdint>
#include "SimWorld.h"

#define MPPT_CHG_I2C_ADDR 0x12

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
#define MPPT_CHG_BUCK_TH  24
#define MPPT_CHG_FLOAT_TH 26
#define MPPT_CHG_PWROFF   28
#define MPPT_CHG_PWRON    30
#define MPPT_WD_EN        33
#define MPPT_WD_COUNT     35
#define MPPT_WD_PWROFF    36

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

#define MPPT_CHG_ST_NIGHT  0
#define MPPT_CHG_ST_IDLE   1
#define MPPT_CHG_ST_VSRCV  2
#define MPPT_CHG_ST_SCAN   3
#define MPPT_CHG_ST_BULK   4
#define MPPT_CHG_ST_ABSORB 5
#define MPPT_CHG_ST_FLOAT  6

#define MPPT_CHG_WD_ENABLE 0xEA  // valeur d'activation attendue par le vrai registre

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
  mpptChg() {}
  mpptChg(int) {}
  mpptChg(int, int) {}

  bool begin() { return true; }
  template <typename Wire>
  bool begin(Wire&) { return true; }

  bool getStatusValue(mpptChg_sys_t index, uint16_t* val) {
    switch (index) {
      case SYS_ID:     *val = 0x0042; return true;
      case SYS_STATUS: *val = statusRegister(); return true;
      case SYS_BUCK:   *val = _cfg[reg(MPPT_CHG_BUCK)]; return true;
    }
    return false;
  }

  bool getIndexedValue(mpptChg_val_t index, int16_t* val) {
    auto& w = SimWorld::instance();
    std::lock_guard<std::mutex> lock(w.mutex);
    if (!w.mppt_present) {
      return false;
    }
    switch (index) {
      case VAL_VS: *val = (int16_t)w.mppt_solar_mv;   return true;
      case VAL_IS: *val = (int16_t)w.mppt_solar_ma;   return true;
      case VAL_VB: *val = (int16_t)w.mppt_battery_mv; return true;
      case VAL_IB: *val = (int16_t)w.mppt_battery_ma; return true;
      default:     *val = 0; return true;
    }
  }

  bool getConfigurationValue(mpptChg_cfg_t index, uint16_t* val) {
    *val = _cfg[reg(cfgReg(index))];
    return true;
  }

  bool setConfigurationValue(mpptChg_cfg_t index, uint16_t val) {
    _cfg[reg(cfgReg(index))] = val;
    return true;
  }

  bool getWatchdogEnable(bool* val) { *val = (_cfg[reg(MPPT_WD_EN)] != 0); return true; }
  bool setWatchdogEnable(bool val) { _cfg[reg(MPPT_WD_EN)] = val ? MPPT_CHG_WD_ENABLE : 0; return true; }
  bool setWatchdogTimeout(uint8_t val) { _cfg[reg(MPPT_WD_COUNT)] = val; return true; }
  bool getWatchdogTimeout(uint8_t* val) { *val = (uint8_t)_cfg[reg(MPPT_WD_COUNT)]; return true; }
  bool setWatchdogPoweroff(uint16_t val) { _cfg[reg(MPPT_WD_PWROFF)] = val; return true; }
  bool getWatchdogPoweroff(uint16_t* val) { *val = _cfg[reg(MPPT_WD_PWROFF)]; return true; }

  bool isAlert(bool* val) {
    *val = (statusRegister() & MPPT_CHG_STATUS_ALERT_MASK) != 0;
    return true;
  }
  bool isNight(bool* val) {
    *val = (statusRegister() & MPPT_CHG_STATUS_NIGHT_MASK) != 0;
    return true;
  }
  bool isPowerEnabled(bool* val) {
    *val = (statusRegister() & MPPT_CHG_STATUS_PWR_EN_MASK) != 0;
    return true;
  }

  static const char* getStatusAsString(uint16_t s) {
    switch (s & MPPT_CHG_STATUS_CHG_ST_MASK) {
      case MPPT_CHG_ST_NIGHT:  return "NIGHT";
      case MPPT_CHG_ST_IDLE:   return "IDLE";
      case MPPT_CHG_ST_VSRCV:  return "VSRCV";
      case MPPT_CHG_ST_SCAN:   return "SCAN";
      case MPPT_CHG_ST_BULK:   return "BULK";
      case MPPT_CHG_ST_ABSORB: return "ABSORB";
      case MPPT_CHG_ST_FLOAT:  return "FLOAT";
      default: return "???";
    }
  }

  static uint16_t computePowerMw(uint16_t mV, uint16_t mA) {
    return (uint16_t)((uint32_t)mV * (uint32_t)mA / 1000);
  }

private:
  uint16_t _cfg[40] = {0};

  static int reg(uint8_t r) { return r < 40 ? r : 0; }

  static uint8_t cfgReg(mpptChg_cfg_t index) {
    switch (index) {
      case CFG_BUCK_V_TH:  return MPPT_CHG_BUCK_TH;
      case CFG_FLOAT_V_TH: return MPPT_CHG_FLOAT_TH;
      case CFG_PWR_OFF_TH: return MPPT_CHG_PWROFF;
      case CFG_PWR_ON_TH:  return MPPT_CHG_PWRON;
    }
    return 0;
  }

  // État de charge piloté par la CLI "sim set mppt status <nom>".
  uint16_t statusRegister() {
    auto& w = SimWorld::instance();
    std::lock_guard<std::mutex> lock(w.mutex);
    uint16_t v = w.mppt_charge_state & MPPT_CHG_STATUS_CHG_ST_MASK;
    if (w.mppt_alert) v |= MPPT_CHG_STATUS_ALERT_MASK;
    if (w.mppt_charge_state == MPPT_CHG_ST_NIGHT) v |= MPPT_CHG_STATUS_NIGHT_MASK;
    return v;
  }
};
