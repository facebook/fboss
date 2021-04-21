// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/lib/fpga/MultiPimPlatformPimContainer.h"

#include "fboss/lib/fpga/FbFpgaI2c.h"
#include "fboss/lib/fpga/FbFpgaPimQsfpController.h"
#include "fboss/lib/fpga/FpgaDevice.h"
#include "fboss/lib/fpga/MinipackLed.h"
#include "fboss/lib/fpga/MinipackPimController.h"

namespace facebook::fboss {

class MinipackBasePimContainer : public MultiPimPlatformPimContainer {
 public:
  // To avoid ambiguity, we explicitly decided the pim number starts from 2.
  MinipackBasePimContainer(
      int pim,
      const std::string& name,
      FpgaDevice* device,
      uint32_t pimStart,
      uint32_t pimSize,
      unsigned int portsPerPim);

  virtual ~MinipackBasePimContainer() {}

  FbFpgaPimQsfpController* getPimQsfpController();

  virtual FbFpgaI2cController* getI2cController(unsigned int port) = 0;

  MinipackLed* getLedController(int qsfp) const;

  MinipackPimController* getController() {
    return pimController_.get();
  }

  bool isPimPresent() const override;

  void initHW(bool /* forceReset */ = false) override {
    // no-op
  }

 protected:
  static constexpr auto kNumLedPerPim = 16;
  std::unique_ptr<FbFpgaPimQsfpController> pimQsfpController_;
  std::unique_ptr<MinipackLed> ledControllers_[kNumLedPerPim];
  std::unique_ptr<MinipackPimController> pimController_;
  int pim_;
};

} // namespace facebook::fboss
