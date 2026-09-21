// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/ThriftBasedWarmbootUtils.h"
#include "fboss/agent/HwAsicTable.h"
#include "fboss/agent/HwSwitchThriftClientTable.h"

namespace facebook::fboss {

namespace {

state::WarmbootState getWarmBootStateFromHwSwitch(
    HwSwitchThriftClientTable* hwSwitchThriftClientTable,
    SwitchID switchID) {
  CHECK(hwSwitchThriftClientTable);
  auto hwSwitchState = hwSwitchThriftClientTable->getProgrammedState(switchID);

  state::WarmbootState warmBootState;
  warmBootState.swSwitchState() = hwSwitchState;
  return warmBootState;
}

} // namespace

std::optional<state::WarmbootState> checkAndGetWarmbootStateFromHwSwitch(
    bool isRunModeMultiSwitch,
    const HwAsicTable* asicTable,
    HwSwitchThriftClientTable* hwSwitchThriftClientTable) {
  if (!isRunModeMultiSwitch) {
    return std::nullopt;
  }
  CHECK(asicTable);
  CHECK(hwSwitchThriftClientTable);
  // TODO(zecheng): Support for merging switch state for multiple switches
  if (asicTable->getSwitchIDs().size() != 1) {
    XLOG(DBG2)
        << "SW Agent only supports recover state from 1 hw agent. The support of merging switch states for multiple hw agent is pending.";
    return std::nullopt;
  }
  XLOG(DBG2) << "Checking HwAgent run state for warmboot";
  auto switchId = *asicTable->getSwitchIDs().begin();
  try {
    auto runState = hwSwitchThriftClientTable->getHwSwitchRunState(switchId);
    if (runState == SwitchRunState::CONFIGURED) {
      XLOG(DBG2)
          << "HwAgent is CONFIGURED - Getting Warmboot state from HwSwitch";
      auto warmbootState =
          getWarmBootStateFromHwSwitch(hwSwitchThriftClientTable, switchId);
      return warmbootState;
    }
  } catch (const std::exception& ex) {
    XLOG(WARNING) << "Failed to query HwAgent run state or programmed state: "
                  << ex.what();
  }
  return std::nullopt;
}

} // namespace facebook::fboss
