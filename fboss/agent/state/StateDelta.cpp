/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/state/StateDelta.h"

#include "fboss/agent/state/AclEntry.h"
#include "fboss/agent/state/AclMap.h"
#include "fboss/agent/state/AggregatePort.h"
#include "fboss/agent/state/AggregatePortMap.h"
#include "fboss/agent/state/ArpTable.h"
#include "fboss/agent/state/ControlPlane.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/InterfaceMap.h"
#include "fboss/agent/state/LoadBalancer.h"
#include "fboss/agent/state/NodeMapDelta.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/PortMap.h"
#include "fboss/agent/state/Route.h"
#include "fboss/agent/state/RouteTable.h"
#include "fboss/agent/state/RouteTableMap.h"
#include "fboss/agent/state/RouteTableRib.h"
#include "fboss/agent/state/SflowCollector.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/Vlan.h"
#include "fboss/agent/state/VlanMap.h"
#include "fboss/agent/state/VlanMapDelta.h"

#include "fboss/agent/state/NodeMapDelta-defs.h"

#include <folly/dynamic.h>

using std::shared_ptr;

namespace facebook::fboss {

StateDelta::~StateDelta() {}

NodeMapDelta<PortMap> StateDelta::getPortsDelta() const {
  return NodeMapDelta<PortMap>(old_->getPorts().get(), new_->getPorts().get());
}

VlanMapDelta StateDelta::getVlansDelta() const {
  return VlanMapDelta(old_->getVlans().get(), new_->getVlans().get());
}

NodeMapDelta<InterfaceMap> StateDelta::getIntfsDelta() const {
  return NodeMapDelta<InterfaceMap>(
      old_->getInterfaces().get(), new_->getInterfaces().get());
}

RTMapDelta StateDelta::getRouteTablesDelta() const {
  return RTMapDelta(old_->getRouteTables().get(), new_->getRouteTables().get());
}

AclMapDelta StateDelta::getAclsDelta() const {
  std::unique_ptr<PrioAclMap> oldAcls, newAcls;
  if (old_->getAcls()) {
    oldAcls.reset(new PrioAclMap());
    oldAcls->addAcls(old_->getAcls());
  }
  if (new_->getAcls()) {
    newAcls.reset(new PrioAclMap());
    newAcls->addAcls(new_->getAcls());
  }
  return AclMapDelta(std::move(oldAcls), std::move(newAcls));
}

QosPolicyMapDelta StateDelta::getQosPoliciesDelta() const {
  return QosPolicyMapDelta(
      old_->getQosPolicies().get(), new_->getQosPolicies().get());
}

NodeMapDelta<AggregatePortMap> StateDelta::getAggregatePortsDelta() const {
  return NodeMapDelta<AggregatePortMap>(
      old_->getAggregatePorts().get(), new_->getAggregatePorts().get());
}

NodeMapDelta<SflowCollectorMap> StateDelta::getSflowCollectorsDelta() const {
  return NodeMapDelta<SflowCollectorMap>(
      old_->getSflowCollectors().get(), new_->getSflowCollectors().get());
}

NodeMapDelta<LoadBalancerMap> StateDelta::getLoadBalancersDelta() const {
  return NodeMapDelta<LoadBalancerMap>(
      old_->getLoadBalancers().get(), new_->getLoadBalancers().get());
}

DeltaValue<ControlPlane> StateDelta::getControlPlaneDelta() const {
  return DeltaValue<ControlPlane>(
      old_->getControlPlane(), new_->getControlPlane());
}

NodeMapDelta<MirrorMap> StateDelta::getMirrorsDelta() const {
  return NodeMapDelta<MirrorMap>(
      old_->getMirrors().get(), new_->getMirrors().get());
}

ForwardingInformationBaseMapDelta StateDelta::getFibsDelta() const {
  return ForwardingInformationBaseMapDelta(
      old_->getFibs().get(), new_->getFibs().get());
}

DeltaValue<SwitchSettings> StateDelta::getSwitchSettingsDelta() const {
  return DeltaValue<SwitchSettings>(
      old_->getSwitchSettings(), new_->getSwitchSettings());
}

NodeMapDelta<LabelForwardingInformationBase>
StateDelta::getLabelForwardingInformationBaseDelta() const {
  return NodeMapDelta<LabelForwardingInformationBase>(
      old_->getLabelForwardingInformationBase().get(),
      new_->getLabelForwardingInformationBase().get());
}

DeltaValue<QosPolicy> StateDelta::getDefaultDataPlaneQosPolicyDelta() const {
  return DeltaValue<QosPolicy>(
      old_->getDefaultDataPlaneQosPolicy(),
      new_->getDefaultDataPlaneQosPolicy());
}

std::ostream& operator<<(std::ostream& out, const StateDelta& stateDelta) {
  // Leverage the folly::dynamic printing facilities
  folly::dynamic diff = folly::dynamic::object;

  diff["added"] = folly::dynamic::array;
  diff["removed"] = folly::dynamic::array;
  diff["modified"] = folly::dynamic::array;

  for (const auto& vlanDelta : stateDelta.getVlansDelta()) {
    for (const auto& arpDelta : vlanDelta.getArpDelta()) {
      const auto* oldArpEntry = arpDelta.getOld().get();
      const auto* newArpEntry = arpDelta.getNew().get();

      if (!oldArpEntry /* added */) {
        diff["added"].push_back(newArpEntry->toFollyDynamic());
      } else if (!newArpEntry /* deleted */) {
        diff["removed"].push_back(oldArpEntry->toFollyDynamic());
      } else { /* modified */
        CHECK(oldArpEntry);
        CHECK(newArpEntry);
        folly::dynamic modification = folly::dynamic::object;
        modification["old"] = oldArpEntry->toFollyDynamic();
        modification["new"] = newArpEntry->toFollyDynamic();
        diff["removed"].push_back(modification);
      }
    }
  }

  return out << diff;
}

// Explicit instantiations of NodeMapDelta that are used by StateDelta.
// This prevents users of StateDelta from needing to include
// NodeMapDelta-defs.h
template class NodeMapDelta<InterfaceMap>;
template class NodeMapDelta<PortMap>;
template class NodeMapDelta<RouteTableMap>;
template class NodeMapDelta<AclMap>;
template class NodeMapDelta<QosPolicyMap>;
template class NodeMapDelta<AggregatePortMap>;
template class NodeMapDelta<SflowCollectorMap>;
template class NodeMapDelta<RouteTableRibNodeMap<folly::IPAddressV4>>;
template class NodeMapDelta<RouteTableRibNodeMap<folly::IPAddressV6>>;
template class NodeMapDelta<LoadBalancerMap>;
template class NodeMapDelta<MirrorMap>;
template class NodeMapDelta<
    ForwardingInformationBaseMap,
    ForwardingInformationBaseContainerDelta>;
template class NodeMapDelta<ForwardingInformationBaseV4>;
template class NodeMapDelta<ForwardingInformationBaseV6>;
template class NodeMapDelta<LabelForwardingInformationBase>;

} // namespace facebook::fboss
