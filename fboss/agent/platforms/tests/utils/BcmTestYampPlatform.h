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

class BcmTestYampPlatform : public BcmTestWedgeTomahawk3Platform {
 public:
  explicit BcmTestYampPlatform(
      std::unique_ptr<PlatformProductInfo> productInfo);

 private:
  // Forbidden copy constructor and assignment operator
  BcmTestYampPlatform(BcmTestYampPlatform const&) = delete;
  BcmTestYampPlatform& operator=(BcmTestYampPlatform const&) = delete;

  std::unique_ptr<BcmTestPort> createTestPort(PortID id) override;
};

} // namespace facebook::fboss
