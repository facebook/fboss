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

class EbroAsic;

class SaiLassenPlatform : public SaiTajoPlatform {
 public:
  explicit SaiLassenPlatform(
      std::unique_ptr<PlatformProductInfo> productInfo,
      folly::MacAddress localMac);
  ~SaiLassenPlatform() override;
  std::string getHwConfig() override;
  HwAsic* getAsic() const override;

 private:
  void setupAsic(
      cfg::SwitchType switchType,
      std::optional<int64_t> switchId,
      std::optional<cfg::Range64> systemPortRange) override;
  std::unique_ptr<EbroAsic> asic_;
};

} // namespace facebook::fboss
