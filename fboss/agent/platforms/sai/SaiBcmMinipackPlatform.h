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
#include "fboss/lib/phy/PhyInterfaceHandler.h"

namespace facebook::fboss {

class Tomahawk3Asic;

class SaiBcmMinipackPlatform : public SaiBcmPlatform {
 public:
  explicit SaiBcmMinipackPlatform(
      std::unique_ptr<PlatformProductInfo> productInfo,
      folly::MacAddress localMac);
  ~SaiBcmMinipackPlatform() override;
  HwAsic* getAsic() const override;
  uint32_t numLanesPerCore() const override {
    return 8;
  }
  void initLEDs() override;

 private:
  std::unique_ptr<Tomahawk3Asic> asic_;

  // The platform contains the top level Phy Interface handler object. The
  // Agent can use this to access the Phy functions.
  std::unique_ptr<PhyInterfaceHandler> phyInterfaceHandler_;
};

} // namespace facebook::fboss
