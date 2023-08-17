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

#include "fboss/agent/platforms/tests/utils/BcmTestPlatform.h"

DECLARE_string(volatile_state_dir);
DECLARE_string(persistent_state_dir);

namespace facebook::fboss {

class BcmTestWedgePlatform : public BcmTestPlatform {
 public:
  BcmTestWedgePlatform(
      std::unique_ptr<PlatformProductInfo> productInfo,
      std::unique_ptr<PlatformMapping> platformMapping);

  bool hasLinkScanCapability() const override {
    return true;
  }

 private:
  // Forbidden copy constructor and assignment operator
  BcmTestWedgePlatform(BcmTestWedgePlatform const&) = delete;
  BcmTestWedgePlatform& operator=(BcmTestWedgePlatform const&) = delete;
};

} // namespace facebook::fboss
