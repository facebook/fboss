namespace cpp2 facebook.fboss
namespace go neteng.fboss.optic
namespace php fboss
namespace py neteng.fboss.optic
namespace py3 neteng.fboss
namespace py.asyncio neteng.fboss.asyncio.optic

include "fboss/qsfp_service/if/transceiver.thrift"

/*
 * THIS IS DEPRECATED, PLEASE USE TransceiverInfo INSTEAD
 */
struct SfpDomThreshFlags {
  1: bool tempAlarmHigh,
  2: bool tempAlarmLow,
  3: bool tempWarnHigh,
  4: bool tempWarnLow,
  5: bool vccAlarmHigh,
  6: bool vccAlarmLow,
  7: bool vccWarnHigh,
  8: bool vccWarnLow,
  9: bool txPwrAlarmHigh,
  10: bool txPwrAlarmLow,
  11: bool txPwrWarnHigh,
  12: bool txPwrWarnLow,
  13: bool rxPwrAlarmHigh,
  14: bool rxPwrAlarmLow,
  15: bool rxPwrWarnHigh,
  16: bool rxPwrWarnLow,
  17: bool txBiasAlarmHigh,
  18: bool txBiasAlarmLow,
  19: bool txBiasWarnHigh,
  20: bool txBiasWarnLow,
  21: bool deleteMe,
}

struct SfpDomThreshValue {
  1: double tempAlarmHigh,
  2: double tempAlarmLow,
  3: double tempWarnHigh,
  4: double tempWarnLow,
  5: double vccAlarmHigh,
  6: double vccAlarmLow,
  7: double vccWarnHigh,
  8: double vccWarnLow,
  9: double txPwrAlarmHigh,
  10: double txPwrAlarmLow,
  11: double txPwrWarnHigh,
  12: double txPwrWarnLow,
  13: double rxPwrAlarmHigh,
  14: double rxPwrAlarmLow,
  15: double rxPwrWarnHigh,
  16: double rxPwrWarnLow,
  17: double txBiasAlarmHigh,
  18: double txBiasAlarmLow,
  19: double txBiasWarnHigh,
  20: double txBiasWarnLow,
}

struct SfpDomReadValue {
  1: double temp,
  2: double vcc,
  3: double txBias,
  4: double txPwr,
  5: double rxPwr,
}

struct SfpDom {
  1: string name,
  2: bool sfpPresent,
  3: bool domSupported,
  6: optional SfpDomThreshFlags flags,
  7: optional SfpDomThreshValue threshValue,
  8: optional SfpDomReadValue value,
  9: optional transceiver.Vendor vendor,
}
