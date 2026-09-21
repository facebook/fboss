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

#include "fboss/agent/platforms/tests/utils/BcmTestWedgeTomahawk3Platform.h"

namespace facebook::fboss {

class PlatformProductInfo;

class BcmTestMinipackPlatform : public BcmTestWedgeTomahawk3Platform {
 public:
  explicit BcmTestMinipackPlatform(
      std::unique_ptr<PlatformProductInfo> productInfo);

 private:
  // Forbidden copy constructor and assignment operator
  BcmTestMinipackPlatform(BcmTestMinipackPlatform const&) = delete;
  BcmTestMinipackPlatform& operator=(BcmTestMinipackPlatform const&) = delete;

  std::unique_ptr<BcmTestPort> createTestPort(PortID id) override;
};

} // namespace facebook::fboss
