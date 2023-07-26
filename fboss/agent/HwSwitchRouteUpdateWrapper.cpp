// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/HwSwitchRouteUpdateWrapper.h"

#include "fboss/agent/AgentConfig.h"
#include "fboss/agent/HwSwitch.h"
#include "fboss/agent/Platform.h"
#include "fboss/agent/Utils.h"

namespace facebook::fboss {

HwSwitchRouteUpdateWrapper::HwSwitchRouteUpdateWrapper(
    HwSwitch* hw,
    RoutingInformationBase* rib,
    FibUpdateFunction fibUpdate)
    : RouteUpdateWrapper(
          hw->getPlatform()->scopeResolver(),
          rib,
          std::move(fibUpdate),
          hw),
      hw_(hw) {}

AdminDistance HwSwitchRouteUpdateWrapper::clientIdToAdminDistance(
    ClientID clientId) const {
  auto config = hw_->getPlatform()->getConfig()->thrift.sw();
  return getAdminDistanceForClientId(
      config.value(), static_cast<int>(clientId));
}
} // namespace facebook::fboss
