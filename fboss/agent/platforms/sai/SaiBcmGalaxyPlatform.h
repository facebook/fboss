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

class SaiBcmGalaxyPlatform : public SaiBcmPlatform {
 public:
  explicit SaiBcmGalaxyPlatform(
      std::unique_ptr<PlatformProductInfo> productInfo,
      std::unique_ptr<PlatformMapping> platformMapping)
      : SaiBcmPlatform(std::move(productInfo), std::move(platformMapping)) {}
  uint32_t numLanesPerCore() const override {
    return 4;
  }
  void initLEDs() override;
};

} // namespace facebook::fboss
