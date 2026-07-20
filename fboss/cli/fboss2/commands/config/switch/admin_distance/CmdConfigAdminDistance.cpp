/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/config/switch/admin_distance/CmdConfigAdminDistance.h"

#include "fboss/cli/fboss2/CmdHandler.cpp"

#include <fmt/format.h>
#include <folly/Conv.h>
#include <iostream>
#include "fboss/cli/fboss2/session/ConfigSession.h"

namespace facebook::fboss {

namespace {
constexpr int32_t kMaxAdminDistance = 255;

// ClientIDs whose admin distance is hardcoded in the agent and cannot be
// overridden via clientIdToAdminDistance config:
//   STATIC_ROUTE (1)         -> AdminDistance::STATIC_ROUTE
//   INTERFACE_ROUTE (2)      -> AdminDistance::DIRECTLY_CONNECTED
//   LINKLOCAL_ROUTE (3)      -> AdminDistance::DIRECTLY_CONNECTED
//   REMOTE_INTERFACE_ROUTE (4) -> AdminDistance::DIRECTLY_CONNECTED
const std::unordered_map<int32_t, std::string>& forbiddenClients() {
  static const std::unordered_map<int32_t, std::string> kForbidden = {
      {1,
       "STATIC_ROUTE is hardcoded to AdminDistance::STATIC_ROUTE in the agent"},
      {2,
       "INTERFACE_ROUTE is hardcoded to AdminDistance::DIRECTLY_CONNECTED in the agent"},
      {3,
       "LINKLOCAL_ROUTE is hardcoded to AdminDistance::DIRECTLY_CONNECTED in the agent"},
      {4,
       "REMOTE_INTERFACE_ROUTE is hardcoded to AdminDistance::DIRECTLY_CONNECTED in the agent"},
  };
  return kForbidden;
}
} // namespace

AdminDistanceArg::AdminDistanceArg(std::vector<std::string> v) {
  if (v.size() != 2) {
    throw std::invalid_argument(
        fmt::format(
            "Expected exactly two arguments: <client-id> <distance>, got {}",
            v.size()));
  }

  try {
    clientId_ = folly::to<int32_t>(v[0]);
  } catch (const folly::ConversionError&) {
    throw std::invalid_argument(
        fmt::format("Invalid client-id '{}': must be an integer", v[0]));
  }
  if (clientId_ < 0) {
    throw std::invalid_argument(
        fmt::format("client-id must be a non-negative integer, got {}", v[0]));
  }

  const auto& forbidden = forbiddenClients();
  auto it = forbidden.find(clientId_);
  if (it != forbidden.end()) {
    throw std::invalid_argument(
        fmt::format(
            "FBOSS does not allow changing admin distance for client-id {}: {}",
            clientId_,
            it->second));
  }

  try {
    distance_ = folly::to<int32_t>(v[1]);
  } catch (const folly::ConversionError&) {
    throw std::invalid_argument(
        fmt::format("Invalid distance '{}': must be an integer", v[1]));
  }
  if (distance_ < 0 || distance_ > kMaxAdminDistance) {
    throw std::invalid_argument(
        fmt::format(
            "distance must be in the range [0, {}], got {}",
            kMaxAdminDistance,
            v[1]));
  }

  data_ = std::move(v);
}

CmdConfigAdminDistanceTraits::RetType CmdConfigAdminDistance::queryClient(
    const HostInfo& /* hostInfo */,
    const ObjectArgType& arg) {
  auto& session = ConfigSession::getInstance();
  auto& config = session.getAgentConfig();
  auto& swConfig = *config.sw();

  int32_t clientId = arg.getClientId();
  int32_t distance = arg.getDistance();

  // Read-modify-write a single map entry, preserving all other clientId
  // mappings already present in the config.
  auto& adminDistanceMap = *swConfig.clientIdToAdminDistance();
  auto it = adminDistanceMap.find(clientId);
  if (it != adminDistanceMap.end()) {
    if (it->second == distance) {
      return fmt::format(
          "Admin distance for client-id {} is already set to {}",
          clientId,
          distance);
    }
    it->second = distance;
  } else {
    adminDistanceMap.emplace(clientId, distance);
  }

  // clientIdToAdminDistance is only consulted at route-program time; existing
  // routes are not re-stamped when the map changes. A coldboot is required to
  // flush and re-program all routes with the updated admin distances.
  session.saveConfig(
      cli::ServiceType::AGENT, cli::ConfigActionLevel::AGENT_COLDBOOT);

  return fmt::format(
      "Successfully set admin distance for client-id {} to {}. "
      "A coldboot is required to apply the change to existing routes.",
      clientId,
      distance);
}

void CmdConfigAdminDistance::printOutput(const RetType& output) {
  std::cout << output << "\n";
}

// Explicit template instantiation
template void
CmdHandler<CmdConfigAdminDistance, CmdConfigAdminDistanceTraits>::run();

} // namespace facebook::fboss
