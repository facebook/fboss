/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/config/routing/CmdConfigRouting.h"

#include "fboss/cli/fboss2/CmdHandler.cpp"

#include <fmt/format.h>
#include <folly/Conv.h>
#include <folly/String.h>
#include <cstdint>
#include <iostream>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include "fboss/cli/fboss2/gen-cpp2/cli_metadata_types.h"
#include "fboss/cli/fboss2/session/ConfigSession.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"

namespace facebook::fboss {

namespace {
constexpr std::string_view kAttrMaxRouteCounterIds = "max-route-counter-ids";

// Valid routing attribute names accepted by `fboss2-dev config routing <attr>`.
const std::set<std::string_view> kRoutingValidAttrs = {
    kAttrMaxRouteCounterIds,
};
} // namespace

RoutingConfigArgs::RoutingConfigArgs(std::vector<std::string> v) {
  if (v.size() != 2) {
    throw std::invalid_argument(
        fmt::format(
            "Expected <attr> <value>, got {} argument(s). Valid attrs: {}",
            v.size(),
            folly::join(", ", kRoutingValidAttrs)));
  }

  if (kRoutingValidAttrs.find(v[0]) == kRoutingValidAttrs.end()) {
    throw std::invalid_argument(
        fmt::format(
            "Unknown routing attribute '{}'. Valid attrs: {}",
            v[0],
            folly::join(", ", kRoutingValidAttrs)));
  }

  int32_t parsed = 0;
  try {
    parsed = folly::to<int32_t>(v[1]);
  } catch (const folly::ConversionError&) {
    throw std::invalid_argument(
        fmt::format("Value for '{}' must be an integer, got '{}'", v[0], v[1]));
  }

  if (parsed < 0) {
    throw std::invalid_argument(
        fmt::format(
            "Value for '{}' must be non-negative, got {}", v[0], parsed));
  }

  attribute_ = v[0];
  value_ = parsed;
  data_ = std::move(v);
}

CmdConfigRoutingTraits::RetType CmdConfigRouting::queryClient(
    const HostInfo& /* hostInfo */,
    const ObjectArgType& args) {
  auto& session = ConfigSession::getInstance();
  auto& config = session.getAgentConfig();
  auto& swConfig = *config.sw();

  const auto& attr = args.getAttribute();
  int32_t value = args.getValue();

  if (attr == kAttrMaxRouteCounterIds) {
    swConfig.switchSettings()->maxRouteCounterIDs() = value;
  } else {
    throw std::runtime_error(
        fmt::format("Unhandled routing attribute '{}'", attr));
  }

  // maxRouteCounterIDs changes require agent warmboot: the agent must
  // re-process all routes to allocate/free hardware counter objects.
  session.saveConfig(
      cli::ServiceType::AGENT, cli::ConfigActionLevel::AGENT_WARMBOOT);

  return fmt::format("Set routing {} to {}", attr, value);
}

void CmdConfigRouting::printOutput(const RetType& logMsg) {
  std::cout << logMsg << std::endl;
}

// Explicit template instantiation
template void CmdHandler<CmdConfigRouting, CmdConfigRoutingTraits>::run();

} // namespace facebook::fboss
