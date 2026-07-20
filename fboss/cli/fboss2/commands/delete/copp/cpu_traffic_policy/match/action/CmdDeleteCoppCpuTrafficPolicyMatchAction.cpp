/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/delete/copp/cpu_traffic_policy/match/action/CmdDeleteCoppCpuTrafficPolicyMatchAction.h"

#include "fboss/cli/fboss2/CmdHandler.cpp"

#include <fmt/format.h>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/cli/fboss2/commands/config/copp/CmdConfigCopp.h"
#include "fboss/cli/fboss2/commands/delete/copp/cpu_traffic_policy/match/CmdDeleteCoppCpuTrafficPolicyMatch.h"
#include "fboss/cli/fboss2/gen-cpp2/cli_metadata_types.h"
#include "fboss/cli/fboss2/session/ConfigSession.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"

namespace facebook::fboss {

using copp_cpu_traffic_policy::kActionCounter;
using copp_cpu_traffic_policy::kActionSendToQueue;
using copp_cpu_traffic_policy::kActionSetTc;

CoppMatchActionArg::CoppMatchActionArg(std::vector<std::string> v) {
  if (v.size() != 1) {
    throw std::invalid_argument(
        fmt::format("Expected exactly one action-type, got {}", v.size()));
  }
  if (!copp_cpu_traffic_policy::isValidActionType(v[0])) {
    throw std::invalid_argument(
        fmt::format(
            "Invalid action-type '{}': must be one of: send-to-queue, counter, "
            "set-tc, user-defined-trap",
            v[0]));
  }
  data_ = std::move(v);
}

CmdDeleteCoppCpuTrafficPolicyMatchActionTraits::RetType
CmdDeleteCoppCpuTrafficPolicyMatchAction::queryClient(
    const HostInfo& /* hostInfo */,
    const CoppMatcherName& matcherName,
    const ObjectArgType& actionType) {
  auto& session = ConfigSession::getInstance();
  auto& agentConfig = session.getAgentConfig();
  auto& swConfig = *agentConfig.sw();

  if (!swConfig.cpuTrafficPolicy().has_value()) {
    throw std::runtime_error(
        "cpuTrafficPolicy is not configured on this device");
  }
  auto& cpuPolicy = *swConfig.cpuTrafficPolicy();

  if (!cpuPolicy.trafficPolicy().has_value()) {
    throw std::runtime_error(
        "cpuTrafficPolicy.trafficPolicy is not configured on this device");
  }
  auto& trafficPolicy = *cpuPolicy.trafficPolicy();

  const auto& name = matcherName.getName();
  auto& matchToActions = *trafficPolicy.matchToAction();

  auto it = copp_cpu_traffic_policy::findMatchToAction(matchToActions, name);

  if (it == matchToActions.end()) {
    throw std::runtime_error(
        fmt::format("No matchToAction entry found for matcher '{}'", name));
  }

  auto& action = *it->action();
  const auto& actionTypeStr = actionType.getActionType();

  auto resetIfPresent = [](auto&& field) {
    bool present = field.has_value();
    field.reset();
    return present;
  };

  bool wasPresent = false;
  if (actionTypeStr == kActionSendToQueue) {
    wasPresent = resetIfPresent(action.sendToQueue());
  } else if (actionTypeStr == kActionCounter) {
    wasPresent = resetIfPresent(action.counter());
  } else if (actionTypeStr == kActionSetTc) {
    wasPresent = resetIfPresent(action.setTc());
  } else {
    // kActionUserDefinedTrap
    wasPresent = resetIfPresent(action.userDefinedTrap());
  }

  // Prune the matcher entry when no action fields remain, even if the
  // requested field was already absent — an entry with an empty action can
  // pre-exist (e.g. from a hand-edited config) and should not be left behind.
  bool erased = false;
  if (action == cfg::MatchAction{}) {
    matchToActions.erase(it);
    erased = true;
  }

  if (!wasPresent && !erased) {
    return fmt::format(
        "Action '{}' is already absent for matcher '{}'", actionTypeStr, name);
  }

  session.saveConfig(cli::ServiceType::AGENT, cli::ConfigActionLevel::HITLESS);

  if (!wasPresent) {
    return fmt::format(
        "Action '{}' was already absent for matcher '{}'; removed the empty "
        "matchToAction entry",
        actionTypeStr,
        name);
  }
  return fmt::format(
      "Successfully deleted action '{}' from matcher '{}'",
      actionTypeStr,
      name);
}

void CmdDeleteCoppCpuTrafficPolicyMatchAction::printOutput(
    const RetType& output) {
  std::cout << output << std::endl;
}

// Explicit template instantiation
template void CmdHandler<
    CmdDeleteCoppCpuTrafficPolicyMatchAction,
    CmdDeleteCoppCpuTrafficPolicyMatchActionTraits>::run();

} // namespace facebook::fboss
