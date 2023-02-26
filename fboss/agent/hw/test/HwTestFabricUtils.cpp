// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/hw/test/HwTestFabricUtils.h"
#include "fboss/agent/HwSwitch.h"

#include <gtest/gtest.h>

namespace facebook::fboss {
void checkFabricReachability(const HwSwitch* hw) {
  auto reachability = hw->getFabricReachability();
  EXPECT_GT(reachability.size(), 0);
  for (auto [port, endpoint] : reachability) {
    if (!*endpoint.isAttached()) {
      continue;
    }
    XLOG(DBG2) << " On port: " << port
               << " got switch id: " << *endpoint.switchId();
    EXPECT_EQ(*endpoint.switchId(), *hw->getSwitchId());
    EXPECT_EQ(*endpoint.switchType(), hw->getSwitchType());
  }
}
} // namespace facebook::fboss
