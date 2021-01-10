// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/lib/fpga/FbFpgaRegisters.h"

#include <map>

namespace {
using I2CRegisterAddrMap = std::map<int, facebook::fboss::I2CRegisterAddr>;
using facebook::fboss::I2CRegisterType;
const I2CRegisterAddrMap descUpperAddrMap{
    {0, {0x504, 0x20}},
    {1, {0x504, 0x8}}};

const I2CRegisterAddrMap descLowerAddrMap{
    {0, {0x500, 0x20}},
    {1, {0x500, 0x8}}};

const I2CRegisterAddrMap rtcStatusAddrMap{{0, {0x600, 0x4}}, {1, {0x600, 0x4}}};

const std::map<I2CRegisterType, I2CRegisterAddrMap> i2CRegisterAddrMap{
    {I2CRegisterType::DESC_UPPER, descUpperAddrMap},
    {I2CRegisterType::DESC_LOWER, descLowerAddrMap},
    {I2CRegisterType::RTC_STATUS, rtcStatusAddrMap}};
} // namespace

namespace facebook::fboss {

I2CRegisterAddr I2CRegisterAddrConstants::getI2CRegisterAddr(
    int version,
    I2CRegisterType type) {
  auto typeIter = i2CRegisterAddrMap.find(type);
  if (typeIter == i2CRegisterAddrMap.end()) {
    throw std::runtime_error("Unrecognized I2C Register type.");
  }
  auto versionIter = typeIter->second.find(version);
  if (versionIter == typeIter->second.end()) {
    throw std::runtime_error("Unrecognized I2C Register version.");
  }
  return versionIter->second;
}

} // namespace facebook::fboss
