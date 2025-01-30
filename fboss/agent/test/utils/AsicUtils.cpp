/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/test/utils/AsicUtils.h"
#include <memory>

#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"

namespace facebook::fboss::utility {

const HwAsic* getAsic(const SwSwitch& sw, PortID port) {
  auto switchId = sw.getScopeResolver()->scope(port).switchId();
  return sw.getHwAsicTable()->getHwAsic(switchId);
}

void checkSameAsicType(const std::vector<const HwAsic*>& asics) {
  std::set<cfg::AsicType> types;
  std::for_each(asics.begin(), asics.end(), [&types](const auto asic) {
    types.insert(asic->getAsicType());
  });
  CHECK_EQ(types.size(), 1) << "Expect 1 asic type, got: " << types.size();
}

const HwAsic* checkSameAndGetAsic(const std::vector<const HwAsic*>& asics) {
  CHECK(!asics.empty()) << " Expect at least one asic to be passed in ";
  checkSameAsicType(asics);
  return *asics.begin();
}

cfg::AsicType checkSameAndGetAsicType(const cfg::SwitchConfig& config) {
  std::set<cfg::AsicType> types;

  for (const auto& entry : *config.switchSettings()->switchIdToSwitchInfo()) {
    types.insert(*entry.second.asicType());
  }
  CHECK_EQ(types.size(), 1) << "Expect 1 asic type, got: " << types.size();
  return *types.begin();
}
} // namespace facebook::fboss::utility
