// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/HwSwitchRouteUpdateWrapper.h"
#include "fboss/agent/rib/ForwardingInformationBaseUpdater.h"

#include "fboss/agent/AgentConfig.h"
#include "fboss/agent/HwSwitch.h"
#include "fboss/agent/Platform.h"
#include "fboss/agent/Utils.h"
#include "fboss/agent/state/StateDelta.h"

namespace facebook::fboss {

std::shared_ptr<SwitchState> hwSwitchFibUpdate(
    const SwitchIdScopeResolver* resolver,
    facebook::fboss::RouterID vrf,
    const facebook::fboss::IPv4NetworkToRouteMap& v4NetworkToRoute,
    const facebook::fboss::IPv6NetworkToRouteMap& v6NetworkToRoute,
    const facebook::fboss::LabelToRouteMap& labelToRoute,
    const std::shared_ptr<SwitchState> oldState) {
  facebook::fboss::ForwardingInformationBaseUpdater fibUpdater(
      resolver, vrf, v4NetworkToRoute, v6NetworkToRoute, labelToRoute);
  return fibUpdater(oldState);
}

HwSwitchRouteUpdateWrapper::HwSwitchRouteUpdateWrapper(
    HwSwitch* hw,
    RoutingInformationBase* rib,
    std::function<std::shared_ptr<SwitchState>(const StateDelta&)> apply)
    : RouteUpdateWrapper(
          hw->getPlatform()->scopeResolver(),
          rib,
          [apply = std::move(apply)](
              const SwitchIdScopeResolver* resolver,
              facebook::fboss::RouterID vrf,
              const facebook::fboss::IPv4NetworkToRouteMap& v4NetworkToRoute,
              const facebook::fboss::IPv6NetworkToRouteMap& v6NetworkToRoute,
              const facebook::fboss::LabelToRouteMap& labelToRoute,
              void* cookie) {
            auto hwSwitch = static_cast<HwSwitch*>(cookie);
            auto oldState = hwSwitch->getProgrammedState();
            auto newState = hwSwitchFibUpdate(
                resolver,
                vrf,
                v4NetworkToRoute,
                v6NetworkToRoute,
                labelToRoute,
                oldState);
            if (apply) {
              apply(StateDelta(oldState, newState));
            }
            return newState;
          },
          hw),
      hw_(hw) {}

AdminDistance HwSwitchRouteUpdateWrapper::clientIdToAdminDistance(
    ClientID clientId) const {
  auto config = hw_->getPlatform()->getConfig()->thrift.sw();
  return getAdminDistanceForClientId(
      config.value(), static_cast<int>(clientId));
}
} // namespace facebook::fboss
