/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/AlpmUtils.h"

#include <utility>

#include <folly/IPAddressV4.h>
#include <folly/IPAddressV6.h>
#include <optional>

#include "fboss/agent/SwitchIdScopeResolver.h"
#include "fboss/agent/Utils.h"
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/agent/state/ForwardingInformationBase.h"
#include "fboss/agent/state/ForwardingInformationBaseContainer.h"
#include "fboss/agent/state/ForwardingInformationBaseMap.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/Route.h"
#include "fboss/agent/state/Vlan.h"

#include "fboss/agent/state/SwitchState.h"

#include <folly/logging/xlog.h>
namespace facebook::fboss {

namespace {

std::pair<uint64_t, uint64_t> numMinV4AndV6AlpmRoutes() {
  uint64_t v4Routes{1}, v6Routes{1};
  return std::make_pair(v4Routes, v6Routes);
}
} // namespace

std::shared_ptr<SwitchState> setupMinAlpmRouteState(
    std::shared_ptr<SwitchState> curState) {
  // In ALPM mode we need to make sure that the first route added is
  // the default route and that the route table always contains a default
  // route
  auto newState = curState->clone();
  auto multiSwitchSwitchSettings = newState->getSwitchSettings();
  CHECK(multiSwitchSwitchSettings->size());
  auto resolver = SwitchIdScopeResolver(
      multiSwitchSwitchSettings->cbegin()->second->getSwitchIdToSwitchInfo());
  RouterID rid(0);
  RoutePrefixV4 defaultPrefix4{folly::IPAddressV4("0.0.0.0"), 0};
  RoutePrefixV6 defaultPrefix6{folly::IPAddressV6("::"), 0};

  auto newFibs = newState->getFibs()->modify(&newState);
  auto defaultVrf = std::make_shared<ForwardingInformationBaseContainer>(rid);
  if (newFibs->getNodeIf(rid)) {
    newFibs->updateForwardingInformationBaseContainer(
        defaultVrf, resolver.scope(defaultVrf));
  } else {
    newFibs->addNode(defaultVrf, resolver.scope(defaultVrf));
  }
  auto setupRoute = [](auto& route) {
    RouteNextHopEntry entry(
        RouteForwardAction::DROP, AdminDistance::MAX_ADMIN_DISTANCE);
    route->update(ClientID::STATIC_INTERNAL, entry);
    route->setResolved(std::move(entry));
  };
  auto v4Route = std::make_shared<RouteV4>(RouteV4::makeThrift(defaultPrefix4));
  auto v6Route = std::make_shared<RouteV6>(RouteV6::makeThrift(defaultPrefix6));
  setupRoute(v4Route);
  setupRoute(v6Route);
  defaultVrf->getFibV4()->addNode(v4Route);
  defaultVrf->getFibV6()->addNode(v6Route);
  return newState;
}

std::shared_ptr<SwitchState> getMinAlpmRouteState(
    const std::shared_ptr<SwitchState>& oldState) {
  // ALPM requires that the default routes (always required to be
  // present for ALPM) be deleted last. When we destroy the HwSwitch
  // and the contained routeTable, there is no notion of a *order* of
  // destruction.
  // So blow away all routes except the min required for ALPM
  // We are going to reset HwSwith anyways, so deleting routes does not
  // matter here.
  // Blowing away all routes means, blowing away 2 tables
  // - Route tables
  // - Interface addresses - for platforms where trapping packets to CPU is
  // done via interfaceToMe routes. So blow away routes and interface
  // addresses.
  auto noRoutesState{oldState->clone()};

  for (const auto& vlanTable : std::as_const(*noRoutesState->getVlans())) {
    for (const auto& idAndVlan : std::as_const(*vlanTable.second)) {
      auto vlan = idAndVlan.second->modify(&noRoutesState);
      vlan->setArpTable(std::make_shared<ArpTable>());
      vlan->setNdpTable(std::make_shared<NdpTable>());
    }
  }

  auto allIntfs = noRoutesState->getInterfaces();
  for (const auto& [_, intfMap] : std::as_const(*allIntfs)) {
    for (const auto& [_, interface] : std::as_const(*intfMap)) {
      CHECK(interface->isPublished());
      auto newIntf = interface->modify(&noRoutesState);
      newIntf->setAddresses(Interface::Addresses{});
    }
  }
  return setupMinAlpmRouteState(noRoutesState);
}

uint64_t numMinAlpmRoutes() {
  auto v4AndV6 = numMinV4AndV6AlpmRoutes();
  return v4AndV6.first + v4AndV6.second;
}

uint64_t numMinAlpmV4Routes() {
  return numMinV4AndV6AlpmRoutes().first;
}

uint64_t numMinAlpmV6Routes() {
  return numMinV4AndV6AlpmRoutes().second;
}

} // namespace facebook::fboss
