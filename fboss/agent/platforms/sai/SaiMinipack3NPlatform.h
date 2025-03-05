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

#include "fboss/agent/platforms/sai/SaiYangraPlatform.h"

namespace facebook::fboss {

class SaiMinipack3NPlatform : public SaiYangraPlatform {
 public:
  SaiMinipack3NPlatform(
      std::unique_ptr<PlatformProductInfo> productInfo,
      folly::MacAddress localMac,
      const std::string& platformMappingStr);
  ~SaiMinipack3NPlatform() override;

  const std::unordered_map<std::string, std::string>
  getSaiProfileVendorExtensionValues() const override;
};

} // namespace facebook::fboss
