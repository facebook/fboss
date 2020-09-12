/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include "fboss/agent/platforms/sai/SaiBcmPlatform.h"
namespace facebook::fboss {

class TomahawkAsic;

class SaiBcmWedge100Platform : public SaiBcmPlatform {
 public:
  explicit SaiBcmWedge100Platform(
      std::unique_ptr<PlatformProductInfo> productInfo);
  ~SaiBcmWedge100Platform() override;
  std::vector<PortID> masterLogicalPortIds() const override;
  HwAsic* getAsic() const override;
  uint32_t numLanesPerCore() const override {
    return 4;
  }
  std::vector<FlexPortMode> getSupportedFlexPortModes() const override {
    return {FlexPortMode::ONEX100G,
            FlexPortMode::TWOX50G,
            FlexPortMode::ONEX40G,
            FlexPortMode::FOURX25G,
            FlexPortMode::FOURX10G};
  }

  void initLEDs() override;

 private:
  std::unique_ptr<TomahawkAsic> asic_;
};

} // namespace facebook::fboss
