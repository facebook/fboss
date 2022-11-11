// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once
#include <folly/logging/xlog.h>
#include <unordered_map>
#include "fboss/lib/bsp/BspPimContainer.h"
#include "fboss/lib/bsp/BspPlatformMapping.h"
#include "fboss/lib/bsp/gen-cpp2/bsp_platform_mapping_types.h"
#include "fboss/lib/fpga/FpgaDevice.h"

namespace facebook {
namespace fboss {

class BspSystemContainer {
 public:
  // Platforms that need to create a FPGA device (for example to read the PIM
  // type) should use the ctor with FpgaDevice arg and then call
  // initializePimContainers() in the derived class ctor. The base class
  // (BspSystemContainer) ctor will mmap the fpga which is required to do
  // read/write.
  explicit BspSystemContainer(std::unique_ptr<FpgaDevice> fpgaDevice);
  explicit BspSystemContainer(BspPlatformMapping* bspMapping);

  void initializePimContainers();

  void setBspPlatformMapping(BspPlatformMapping* mapping) {
    bspMapping_ = mapping;
  }

  BspPlatformMapping* getBspPlatformMapping() const {
    return bspMapping_;
  }

  FpgaDevice* getFpgaDevice() const {
    return fpgaDevice_.get();
  }

  const BspPimContainer* getPimContainerFromPimID(int pimID) const;
  const BspPimContainer* getPimContainerFromTcvrID(int tcvrID) const;
  int getNumTransceivers() const;
  int getPimIDFromTcvrID(int tcvrID) const;
  int getNumPims() const;

  bool isTcvrPresent(int tcvrID) const;
  void initAllTransceivers() const;
  void clearAllTransceiverReset() const;
  void triggerQsfpHardReset(int tcvrID) const;
  const I2cControllerStats getI2cControllerStats(int tcvrID) const;

  void tcvrRead(
      unsigned int tcvrID,
      const TransceiverAccessParameter& param,
      uint8_t* buf) const;
  void tcvrWrite(
      unsigned int tcvrID,
      const TransceiverAccessParameter& param,
      const uint8_t* buf) const;

 private:
  std::unordered_map<int, std::unique_ptr<BspPimContainer>> pimContainers_;
  std::unique_ptr<FpgaDevice> fpgaDevice_;
  BspPlatformMapping* bspMapping_;
};

} // namespace fboss
} // namespace facebook
