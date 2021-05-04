// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/lib/fpga/MultiPimPlatformSystemContainer.h"

#include "fboss/lib/fpga/MinipackBasePimContainer.h"

namespace facebook::fboss {
// The base system container for Minipack switch family.
class MinipackBaseSystemContainer : public MultiPimPlatformSystemContainer {
 public:
  explicit MinipackBaseSystemContainer(std::unique_ptr<FpgaDevice> fpgaDevice)
      : MultiPimPlatformSystemContainer(std::move(fpgaDevice)) {}

  virtual ~MinipackBaseSystemContainer() {}
  const static int kPimStartNum = 2;
  const static int kNumberPim = 8;
  /*
   * This function should be called before any access attempt
   * to any HW devices.
   * For now, it just allocate FPGA memory map to get it ready.
   */
  void initHW(bool forceReset = false) override;

  // To avoid ambiguity, we explicitly decided the pim number starts from 2.
  uint32_t getPimOffset(int pim) override;

  MinipackBasePimContainer* getPimContainer(int pim) const override;

  uint8_t getPimStartNum() const override {
    return kPimStartNum;
  }

 protected:
  // FPGA Device Info
  static constexpr uint32_t kFacebookFpgaVendorID = 0x1d9b;
  static constexpr uint32_t kFacebookFpgaSmbSize = 0x40000;
  static constexpr uint32_t kFacebookFpgaPimBase = 0x40000;
  static constexpr uint32_t kFacebookFpgaPimSize = 0x8000;

  bool isHwInitialized_{false};
};

} // namespace facebook::fboss
