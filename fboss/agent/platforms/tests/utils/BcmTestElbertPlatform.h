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

#include "fboss/agent/platforms/tests/utils/BcmTestTomahawk4Platform.h"

namespace facebook::fboss {

class PlatformProductInfo;

class BcmTestElbertPlatform : public BcmTestTomahawk4Platform {
 public:
  explicit BcmTestElbertPlatform(
      std::unique_ptr<PlatformProductInfo> productInfo);

 private:
  // Forbidden copy constructor and assignment operator
  BcmTestElbertPlatform(BcmTestElbertPlatform const&) = delete;
  BcmTestElbertPlatform& operator=(BcmTestElbertPlatform const&) = delete;

  std::unique_ptr<BcmTestPort> createTestPort(PortID id) override;
};

} // namespace facebook::fboss
