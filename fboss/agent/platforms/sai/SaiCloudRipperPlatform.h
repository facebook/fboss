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

#include "fboss/agent/platforms/sai/SaiTajoPlatform.h"

namespace facebook::fboss {

class SaiCloudRipperPlatform : public SaiTajoPlatform {
 public:
  explicit SaiCloudRipperPlatform(
      std::unique_ptr<PlatformProductInfo> productInfo,
      folly::MacAddress localMac);
  ~SaiCloudRipperPlatform() override;

  std::string getHwConfig() override;
};

} // namespace facebook::fboss
