/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/delete/protocol/static/route/CmdDeleteProtocolStaticRoute.h"

#include <fmt/format.h>
#include <algorithm>
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/cli/fboss2/CmdHandler.cpp"
#include "fboss/cli/fboss2/session/ConfigSession.h"

namespace facebook::fboss {

template <typename PrefixArgType>
std::string deleteStaticRouteImpl(const PrefixArgType& prefixArg) {
  auto& config = ConfigSession::getInstance().getAgentConfig();
  auto& swConfig = *config.sw();

  const std::string& prefix = prefixArg.getPrefix();
  int32_t vrfId = 0;

  bool found = false;

  auto& nhopRoutes = *swConfig.staticRoutesWithNhops();
  auto nhopIt = std::find_if(
      nhopRoutes.begin(),
      nhopRoutes.end(),
      [vrfId, &prefix](const auto& route) {
        return *route.routerID() == vrfId && *route.prefix() == prefix;
      });
  if (nhopIt != nhopRoutes.end()) {
    nhopRoutes.erase(nhopIt);
    found = true;
  }

  auto& nullRoutes = *swConfig.staticRoutesToNull();
  auto nullIt = std::find_if(
      nullRoutes.begin(),
      nullRoutes.end(),
      [vrfId, &prefix](const auto& route) {
        return *route.routerID() == vrfId && *route.prefix() == prefix;
      });
  if (nullIt != nullRoutes.end()) {
    nullRoutes.erase(nullIt);
    found = true;
  }

  auto& cpuRoutes = *swConfig.staticRoutesToCPU();
  auto cpuIt = std::find_if(
      cpuRoutes.begin(), cpuRoutes.end(), [vrfId, &prefix](const auto& route) {
        return *route.routerID() == vrfId && *route.prefix() == prefix;
      });
  if (cpuIt != cpuRoutes.end()) {
    cpuRoutes.erase(cpuIt);
    found = true;
  }

  if (!found) {
    return fmt::format("Static route {} (VRF {}) not found", prefix, vrfId);
  }

  ConfigSession::getInstance().saveConfig();
  return fmt::format(
      "Successfully deleted static route {} (VRF {})", prefix, vrfId);
}

CmdDeleteProtocolStaticIpRouteTraits::RetType
CmdDeleteProtocolStaticIpRoute::queryClient(
    const HostInfo& /* hostInfo */,
    const ObjectArgType& prefixArg) {
  return deleteStaticRouteImpl(prefixArg);
}

void CmdDeleteProtocolStaticIpRoute::printOutput(const RetType& logMsg) {
  std::cout << logMsg << std::endl;
}

CmdDeleteProtocolStaticIpv6RouteTraits::RetType
CmdDeleteProtocolStaticIpv6Route::queryClient(
    const HostInfo& /* hostInfo */,
    const ObjectArgType& prefixArg) {
  return deleteStaticRouteImpl(prefixArg);
}

void CmdDeleteProtocolStaticIpv6Route::printOutput(const RetType& logMsg) {
  std::cout << logMsg << std::endl;
}

template std::string deleteStaticRouteImpl<StaticRouteDeleteIpPrefixArg>(
    const StaticRouteDeleteIpPrefixArg& prefixArg);

template std::string deleteStaticRouteImpl<StaticRouteDeleteIpv6PrefixArg>(
    const StaticRouteDeleteIpv6PrefixArg& prefixArg);

template void CmdHandler<
    CmdDeleteProtocolStaticIpRoute,
    CmdDeleteProtocolStaticIpRouteTraits>::run();

template void CmdHandler<
    CmdDeleteProtocolStaticIpv6Route,
    CmdDeleteProtocolStaticIpv6RouteTraits>::run();

} // namespace facebook::fboss
