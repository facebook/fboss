/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/config/need_l2_entry_for_neighbor/CmdConfigNeedL2EntryForNeighbor.h"

#include "fboss/cli/fboss2/CmdHandler.cpp"

#include <fmt/format.h>
#include <folly/String.h>
#include <iostream>
#include "fboss/cli/fboss2/session/ConfigSession.h"

namespace facebook::fboss {

namespace {
// NOTE: switch_config.thrift::SwitchSettings::needL2EntryForNeighbor is not
// applied by ApplyThriftConfig or read by HwSwitchHandler implementations —
// both MonolithicHwSwitchHandler and MultiSwitchHwSwitchHandler determine this
// value from ASIC type rather than from the config field. This CLI writes the
// config field for forward-compatibility; it does not affect agent runtime
// behavior until the agent side is updated to honor the field.
constexpr std::string_view kEnabled = "enabled";
constexpr std::string_view kDisabled = "disabled";
} // namespace

NeedL2EntryForNeighborArg::NeedL2EntryForNeighborArg(
    std::vector<std::string> v) {
  if (v.empty()) {
    throw std::invalid_argument("State is required (enabled or disabled)");
  }
  if (v.size() != 1) {
    throw std::invalid_argument(
        "Expected exactly one state (enabled or disabled)");
  }

  std::string state = v[0];
  folly::toLowerAscii(state);
  if (state == kEnabled) {
    enabled_ = true;
  } else if (state == kDisabled) {
    enabled_ = false;
  } else {
    throw std::invalid_argument(
        "Invalid state '" + v[0] + "'. Expected 'enabled' or 'disabled'");
  }
  data_.push_back(v[0]);
}

CmdConfigNeedL2EntryForNeighborTraits::RetType
CmdConfigNeedL2EntryForNeighbor::queryClient(
    const HostInfo& /* hostInfo */,
    const ObjectArgType& stateArg) {
  auto& session = ConfigSession::getInstance();
  auto& config = session.getAgentConfig();
  auto& swConfig = *config.sw();

  bool newValue = stateArg.isEnabled();
  auto currentValue = swConfig.switchSettings()->needL2EntryForNeighbor();

  if (currentValue.has_value() && *currentValue == newValue) {
    return fmt::format(
        "need-l2-entry-for-neighbor is already set to '{}'",
        newValue ? kEnabled : kDisabled);
  }

  swConfig.switchSettings()->needL2EntryForNeighbor() = newValue;

  session.saveConfig(cli::ServiceType::AGENT, cli::ConfigActionLevel::HITLESS);

  return fmt::format(
      "Successfully set need-l2-entry-for-neighbor to '{}'",
      newValue ? kEnabled : kDisabled);
}

void CmdConfigNeedL2EntryForNeighbor::printOutput(const RetType& logMsg) {
  std::cout << logMsg << std::endl;
}

template void CmdHandler<
    CmdConfigNeedL2EntryForNeighbor,
    CmdConfigNeedL2EntryForNeighborTraits>::run();

} // namespace facebook::fboss
