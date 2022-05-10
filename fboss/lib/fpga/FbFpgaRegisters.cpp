// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/lib/fpga/FbFpgaRegisters.h"

#include <map>

namespace {
using I2CRegisterAddrMap = std::map<int, facebook::fboss::I2CRegisterAddr>;
using facebook::fboss::I2CRegisterType;
using facebook::fboss::SpiRegisterType;

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

const facebook::fboss::SpiRegisterAddr spiMasterCsr{0x1400, 0x80};
const facebook::fboss::SpiRegisterAddr spiCtrlReset{0x1404, 0x80};
const facebook::fboss::SpiRegisterAddr spiDescUpperAddr{0x140C, 0x80};
const facebook::fboss::SpiRegisterAddr spiDescLowerAddr{0x1408, 0x80};
const facebook::fboss::SpiRegisterAddr spiWriteDataBlock{0x4000, 0x400};
const facebook::fboss::SpiRegisterAddr spiReadDataBlock{0x4200, 0x400};

const std::map<SpiRegisterType, facebook::fboss::SpiRegisterAddr>
    spiRegisterAddrMap{
        {SpiRegisterType::SPI_MASTER_CSR, spiMasterCsr},
        {SpiRegisterType::SPI_CTRL_RESET, spiCtrlReset},
        {SpiRegisterType::SPI_DESC_UPPER, spiDescUpperAddr},
        {SpiRegisterType::SPI_DESC_LOWER, spiDescLowerAddr},
        {SpiRegisterType::SPI_WRITE_DATA_BLOCK, spiWriteDataBlock},
        {SpiRegisterType::SPI_READ_DATA_BLOCK, spiReadDataBlock}};
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

SpiRegisterAddr SpiRegisterAddrConstants::getSpiRegisterAddr(
    SpiRegisterType type) {
  auto typeIter = spiRegisterAddrMap.find(type);
  if (typeIter == spiRegisterAddrMap.end()) {
    throw std::runtime_error("Unrecognized Spi Register type.");
  }
  return typeIter->second;
}

} // namespace facebook::fboss
