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

#include "fboss/agent/platforms/sai/SaiHwPlatform.h"

namespace facebook::fboss {

class TajoAsic;

class SaiWedge400CPlatform : public SaiHwPlatform {
 public:
  explicit SaiWedge400CPlatform(
      std::unique_ptr<PlatformProductInfo> productInfo);
  ~SaiWedge400CPlatform() override;
  std::vector<PortID> masterLogicalPortIds() const override;
  std::string getHwConfig() override;
  HwAsic* getAsic() const override;
  bool getObjectKeysSupported() const override {
    return false;
  }
  uint32_t numLanesPerCore() const override {
    return 4;
  }

 private:
  std::unique_ptr<TajoAsic> asic_;
};

} // namespace facebook::fboss
