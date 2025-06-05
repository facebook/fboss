// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/platform/weutil/FbossEepromInterface.h"

namespace facebook::fboss::platform {

class FbossEepromV5 : public FbossEepromInterface {
 private:
  std::vector<EepromFieldEntry> getFieldDictionary() override;
  constexpr int getVersion() const override;

 public:
  std::string productName;
  std::string productPartNumber;
  std::string systemAssemblyPartNumber;
  std::string metaPCBAPartNumber;
  std::string metaPCBPartNumber;
  std::string odmJdmPCBAPartNumber;
  std::string odmJdmPCBASerialNumber;
  std::string productProductionState;
  std::string productVersion;
  std::string productSubVersion;
  std::string productSerialNumber;
  std::string systemManufacturer;
  std::string systemManufacturingDate;
  std::string pcbManufacturer;
  std::string assembledAt;
  std::string eepromLocationOnFabric;
  std::string x86CpuMac;
  std::string bmcMac;
  std::string switchAsicMac;
  std::string metaReservedMac;
  std::string crc16;

  std::string getProductionState() const override;
  std::string getProductionSubState() const override;
  std::string getVariantVersion() const override;
};

} // namespace facebook::fboss::platform
