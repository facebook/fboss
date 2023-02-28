// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/hw/test/HwTestFabricUtils.h"

#include "fboss/agent/hw/sai/switch/SaiSwitch.h"

namespace facebook::fboss {

void setForceTrafficOverFabric(const HwSwitch* hw, bool force) {
  auto saiSwitch = static_cast<const SaiSwitch*>(hw);
  auto switchID = saiSwitch->getSaiSwitchId();
  SaiApiTable::getInstance()->switchApi().setAttribute(
      switchID, SaiSwitchTraits::Attributes::ForceTrafficOverFabric{force});
}

} // namespace facebook::fboss
