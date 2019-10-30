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

#include "fboss/agent/platforms/test_platforms/BcmTestWedgeTomahawk3Platform.h"

namespace facebook {
namespace fboss {
class PlatformProductInfo;

class BcmTestWedge400Platform : public BcmTestWedgeTomahawk3Platform {
 public:
  explicit BcmTestWedge400Platform(
      std::unique_ptr<PlatformProductInfo> productInfo);

 private:
  // Forbidden copy constructor and assignment operator
  BcmTestWedge400Platform(BcmTestWedge400Platform const&) = delete;
  BcmTestWedge400Platform& operator=(BcmTestWedge400Platform const&) = delete;

  std::unique_ptr<BcmTestPort> createTestPort(PortID id) override;
};
} // namespace fboss
} // namespace facebook
