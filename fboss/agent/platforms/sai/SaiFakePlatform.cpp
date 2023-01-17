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
#include "fboss/agent/hw/sai/api/SaiApiTable.h"
#include "fboss/agent/hw/switch_asics/FakeAsic.h"
#include "fboss/agent/platforms/common/fake_test/FakeTestPlatformMapping.h"

#include "fboss/agent/hw/test/ConfigFactory.h"

#include <cstdio>
#include <cstring>
namespace {
std::vector<int> getControllingPortIDs() {
  std::vector<int> results;
  for (int i = 0; i < 128; i += 4) {
    results.push_back(i);
  }
  return results;
}

const folly::MacAddress kLocalMac("02:00:00:00:00:01");
} // namespace

namespace facebook::fboss {

SaiFakePlatform::SaiFakePlatform(
    std::unique_ptr<PlatformProductInfo> productInfo)
    : SaiPlatform(
          std::move(productInfo),
          std::make_unique<FakeTestPlatformMapping>(getControllingPortIDs()),
          kLocalMac) {}

void SaiFakePlatform::setupAsic(
    cfg::SwitchType switchType,
    std::optional<int64_t> switchId,
    std::optional<cfg::Range64> systemPortRange) {
  asic_ = std::make_unique<FakeAsic>(switchType, switchId, systemPortRange);
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

const std::set<sai_api_t>& SaiFakePlatform::getSupportedApiList() const {
  return SaiApiTable::getInstance()->getFullApiList();
}

} // namespace facebook::fboss
