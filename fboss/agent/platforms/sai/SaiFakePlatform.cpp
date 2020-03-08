/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/sai/SaiFakePlatform.h"
#include "fboss/agent/hw/switch_asics/FakeAsic.h"

#include <cstdio>
#include <cstring>
namespace facebook::fboss {

SaiFakePlatform::SaiFakePlatform(
    std::unique_ptr<PlatformProductInfo> productInfo,
    std::unique_ptr<PlatformMapping> platformMapping)
    : SaiPlatform(std::move(productInfo), std::move(platformMapping)) {
  asic_ = std::make_unique<FakeAsic>();
}

std::string SaiFakePlatform::getVolatileStateDir() const {
  return tmpDir_.path().string() + "/volatile";
}

std::string SaiFakePlatform::getPersistentStateDir() const {
  return tmpDir_.path().string() + "/persist";
}

std::string SaiFakePlatform::getHwConfig() {
  return {};
}

HwAsic* SaiFakePlatform::getAsic() const {
  return asic_.get();
}

SaiFakePlatform::~SaiFakePlatform() {}

} // namespace facebook::fboss
