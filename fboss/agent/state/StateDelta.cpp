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
#include "fboss/agent/state/AclTableGroup.h"
#include "fboss/agent/state/AclTableGroupMap.h"
#include "fboss/agent/state/AggregatePort.h"
#include "fboss/agent/state/AggregatePortMap.h"
#include "fboss/agent/state/ArpTable.h"
#include "fboss/agent/state/ControlPlane.h"
#include "fboss/agent/state/DsfNodeMap.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/InterfaceMap.h"
#include "fboss/agent/state/LoadBalancer.h"
#include "fboss/agent/state/NodeMapDelta.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/PortMap.h"
#include "fboss/agent/state/Route.h"

#include "fboss/agent/HwSwitchMatcher.h"
#include "fboss/agent/state/SflowCollector.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/Vlan.h"
#include "fboss/agent/state/VlanMap.h"
#include "fboss/agent/state/VlanMapDelta.h"

#include <folly/dynamic.h>

#include "fboss/agent/FsdbHelper.h"

using std::shared_ptr;

namespace facebook::fboss {

StateDelta::StateDelta(
    std::shared_ptr<SwitchState> oldState,
    std::shared_ptr<SwitchState> newState)
    : old_(oldState), new_(newState) {}

StateDelta::StateDelta(
    std::shared_ptr<SwitchState> oldState,
    fsdb::OperDelta operDelta)
    : old_(oldState), operDelta_(std::move(operDelta)) {
  // compute new state from old state and oper delta
  fsdb::CowStorage<state::SwitchState, SwitchState> cowState{old_->clone()};
  if (auto error = cowState.patch_impl(operDelta_.value())) {
    throw FbossError(
        "Error while applying the patch: ", static_cast<int>(error.value()));
  }
  new_ = cowState.root();
  new_->publish();
}

StateDelta::~StateDelta() {}

thrift_cow::ThriftMapDelta<PortMap> StateDelta::getPortsDelta() const {
  return thrift_cow::ThriftMapDelta<PortMap>(
      old_->getPorts().get(), new_->getPorts().get());
}

VlanMapDelta StateDelta::getVlansDelta() const {
  return VlanMapDelta(old_->getVlans().get(), new_->getVlans().get());
}

InterfaceMapDelta StateDelta::getIntfsDelta() const {
  return InterfaceMapDelta(
      old_->getInterfaces().get(), new_->getInterfaces().get());
}

InterfaceMapDelta StateDelta::getRemoteIntfsDelta() const {
  return InterfaceMapDelta(
      old_->getRemoteInterfaces().get(), new_->getRemoteInterfaces().get());
}

AclMapDelta StateDelta::getAclsDelta(
    cfg::AclStage aclStage,
    std::optional<std::string> tableName) const {
  std::unique_ptr<PrioAclMap> oldAcls, newAcls;

  if (tableName.has_value()) {
    // Multiple ACL Table support
    if (old_->getAclsForTable(aclStage, tableName.value())) {
      oldAcls.reset(new facebook::fboss::PrioAclMap());
      oldAcls->addAcls(old_->getAclsForTable(aclStage, tableName.value()));
    }

    if (new_->getAclsForTable(aclStage, tableName.value())) {
      newAcls.reset(new facebook::fboss::PrioAclMap());
      newAcls->addAcls(new_->getAclsForTable(aclStage, tableName.value()));
    }
  } else {
    // Single ACL Table support
    if (old_->getAcls()) {
      oldAcls.reset(new PrioAclMap());
      oldAcls->addAcls(old_->getAcls());
    }
    if (new_->getAcls()) {
      newAcls.reset(new PrioAclMap());
      newAcls->addAcls(new_->getAcls());
    }
  }

  return AclMapDelta(std::move(oldAcls), std::move(newAcls));
}

thrift_cow::ThriftMapDelta<AclTableMap> StateDelta::getAclTablesDelta(
    cfg::AclStage aclStage) const {
  return thrift_cow::ThriftMapDelta<AclTableMap>(
      old_->getAclTablesForStage(aclStage).get(),
      new_->getAclTablesForStage(aclStage).get());
}

thrift_cow::ThriftMapDelta<AclTableGroupMap>
StateDelta::getAclTableGroupsDelta() const {
  return thrift_cow::ThriftMapDelta<AclTableGroupMap>(
      old_->getAclTableGroups().get(), new_->getAclTableGroups().get());
}

QosPolicyMapDelta StateDelta::getQosPoliciesDelta() const {
  return QosPolicyMapDelta(
      old_->getQosPolicies().get(), new_->getQosPolicies().get());
}

thrift_cow::ThriftMapDelta<AggregatePortMap>
StateDelta::getAggregatePortsDelta() const {
  return thrift_cow::ThriftMapDelta<AggregatePortMap>(
      old_->getAggregatePorts().get(), new_->getAggregatePorts().get());
}

thrift_cow::ThriftMapDelta<SflowCollectorMap>
StateDelta::getSflowCollectorsDelta() const {
  return thrift_cow::ThriftMapDelta<SflowCollectorMap>(
      old_->getSflowCollectors().get(), new_->getSflowCollectors().get());
}

thrift_cow::ThriftMapDelta<LoadBalancerMap> StateDelta::getLoadBalancersDelta()
    const {
  return thrift_cow::ThriftMapDelta<LoadBalancerMap>(
      old_->getLoadBalancers().get(), new_->getLoadBalancers().get());
}

thrift_cow::ThriftMapDelta<UdfGroupMap> StateDelta::getUdfGroupDelta() const {
  std::shared_ptr<UdfGroupMap> oldUdfGroupMap;
  if (old_->getUdfConfig()) {
    oldUdfGroupMap = old_->getUdfConfig()->getUdfGroupMap();
  }
  std::shared_ptr<UdfGroupMap> newUdfGroupMap;
  if (new_->getUdfConfig()) {
    newUdfGroupMap = new_->getUdfConfig()->getUdfGroupMap();
  }
  return thrift_cow::ThriftMapDelta<UdfGroupMap>(
      oldUdfGroupMap.get(), newUdfGroupMap.get());
}

thrift_cow::ThriftMapDelta<UdfPacketMatcherMap>
StateDelta::getUdfPacketMatcherDelta() const {
  std::shared_ptr<UdfPacketMatcherMap> oldUdfPacketMatcherMap;
  if (old_->getUdfConfig()) {
    oldUdfPacketMatcherMap = old_->getUdfConfig()->getUdfPacketMatcherMap();
  }
  std::shared_ptr<UdfPacketMatcherMap> newUdfPacketMatcherMap;
  if (new_->getUdfConfig()) {
    newUdfPacketMatcherMap = new_->getUdfConfig()->getUdfPacketMatcherMap();
  }
  return thrift_cow::ThriftMapDelta<UdfPacketMatcherMap>(
      oldUdfPacketMatcherMap.get(), newUdfPacketMatcherMap.get());
}

thrift_cow::ThriftMapDelta<DsfNodeMap> StateDelta::getDsfNodesDelta() const {
  return thrift_cow::ThriftMapDelta<DsfNodeMap>(
      old_->getDsfNodes().get(), new_->getDsfNodes().get());
}

DeltaValue<ControlPlane> StateDelta::getControlPlaneDelta() const {
  return DeltaValue<ControlPlane>(
      old_->getControlPlane(), new_->getControlPlane());
}

thrift_cow::ThriftMapDelta<MirrorMap> StateDelta::getMirrorsDelta() const {
  auto oldMirrors = old_->cref<switch_state_tags::mirrorMaps>()->getMirrorMapIf(
      HwSwitchMatcher::defaultHwSwitchMatcher());
  auto newMirrors = new_->cref<switch_state_tags::mirrorMaps>()->getMirrorMapIf(
      HwSwitchMatcher::defaultHwSwitchMatcher());

  return thrift_cow::ThriftMapDelta<MirrorMap>(
      oldMirrors.get(), newMirrors.get());
}

thrift_cow::ThriftMapDelta<TransceiverMap> StateDelta::getTransceiversDelta()
    const {
  return thrift_cow::ThriftMapDelta<TransceiverMap>(
      old_->getTransceivers().get(), new_->getTransceivers().get());
}

ForwardingInformationBaseMapDelta StateDelta::getFibsDelta() const {
  return ForwardingInformationBaseMapDelta(
      old_->getFibs().get(), new_->getFibs().get());
}

DeltaValue<SwitchSettings> StateDelta::getSwitchSettingsDelta() const {
  return DeltaValue<SwitchSettings>(
      old_->getSwitchSettings(), new_->getSwitchSettings());
}

DeltaValue<FlowletSwitchingConfig> StateDelta::getFlowletSwitchingConfigDelta()
    const {
  return DeltaValue<FlowletSwitchingConfig>(
      old_->getFlowletSwitchingConfig(), new_->getFlowletSwitchingConfig());
}

thrift_cow::ThriftMapDelta<LabelForwardingInformationBase>
StateDelta::getLabelForwardingInformationBaseDelta() const {
  return thrift_cow::ThriftMapDelta<LabelForwardingInformationBase>(
      old_->getLabelForwardingInformationBase().get(),
      new_->getLabelForwardingInformationBase().get());
}

DeltaValue<QosPolicy> StateDelta::getDefaultDataPlaneQosPolicyDelta() const {
  return DeltaValue<QosPolicy>(
      old_->getDefaultDataPlaneQosPolicy(),
      new_->getDefaultDataPlaneQosPolicy());
}

thrift_cow::ThriftMapDelta<SystemPortMap> StateDelta::getSystemPortsDelta()
    const {
  return thrift_cow::ThriftMapDelta<SystemPortMap>(
      old_->getSystemPorts().get(), new_->getSystemPorts().get());
}

thrift_cow::ThriftMapDelta<SystemPortMap>
StateDelta::getRemoteSystemPortsDelta() const {
  return thrift_cow::ThriftMapDelta<SystemPortMap>(
      old_->getRemoteSystemPorts().get(), new_->getRemoteSystemPorts().get());
}

thrift_cow::ThriftMapDelta<IpTunnelMap> StateDelta::getIpTunnelsDelta() const {
  return thrift_cow::ThriftMapDelta<IpTunnelMap>(
      old_->getTunnels().get(), new_->getTunnels().get());
}

thrift_cow::ThriftMapDelta<TeFlowTable> StateDelta::getTeFlowEntriesDelta()
    const {
  return thrift_cow::ThriftMapDelta<TeFlowTable>(
      old_->getTeFlowTable().get(), new_->getTeFlowTable().get());
}

const fsdb::OperDelta& StateDelta::getOperDelta() {
  if (!operDelta_.has_value()) {
    operDelta_.emplace(computeOperDelta(old_, new_, switchStateRootPath()));
  }
  return operDelta_.value();
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
template class thrift_cow::ThriftMapDelta<InterfaceMap>;
template class thrift_cow::ThriftMapDelta<PortMap>;
template class thrift_cow::ThriftMapDelta<AclMap>;
template class thrift_cow::ThriftMapDelta<AclTableGroupMap>;
template class thrift_cow::ThriftMapDelta<AclTableMap>;
template class thrift_cow::ThriftMapDelta<QosPolicyMap>;
template class thrift_cow::ThriftMapDelta<AggregatePortMap>;
template class thrift_cow::ThriftMapDelta<SflowCollectorMap>;
template class thrift_cow::ThriftMapDelta<LoadBalancerMap>;
template class thrift_cow::ThriftMapDelta<MirrorMap>;
template class thrift_cow::ThriftMapDelta<TransceiverMap>;
template class thrift_cow::ThriftMapDelta<ForwardingInformationBaseV4>;
template class thrift_cow::ThriftMapDelta<ForwardingInformationBaseV6>;
template class thrift_cow::ThriftMapDelta<LabelForwardingInformationBase>;
template class thrift_cow::ThriftMapDelta<SystemPortMap>;
template class thrift_cow::ThriftMapDelta<IpTunnelMap>;
template class thrift_cow::ThriftMapDelta<TeFlowTable>;

} // namespace facebook::fboss
