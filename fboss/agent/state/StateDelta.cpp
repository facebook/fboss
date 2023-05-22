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

#include "ForwardingInformationBaseMap.h"
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

DEFINE_bool(
    enable_state_oper_delta,
    false,
    "Generate and process oper delta for state delta processing");

DEFINE_bool(
    verify_apply_oper_delta,
    false,
    "Make sure oper delta apply is correct, this is expensive operation to be used only in tests");

namespace facebook::fboss {

namespace {
template <
    typename Map,
    typename MultiNpuMap,
    typename Delta = ThriftMapDelta<Map>>
Delta getFirstMapDelta(
    const std::shared_ptr<MultiNpuMap>& oldMnpuMap,
    const std::shared_ptr<MultiNpuMap>& newMnpuMap) {
  auto oldMap =
      oldMnpuMap->size() ? oldMnpuMap->cbegin()->second.get() : nullptr;
  auto newMap =
      newMnpuMap->size() ? newMnpuMap->cbegin()->second.get() : nullptr;
  return Delta(oldMap, newMap);
}
} // namespace

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

MultiSwitchMapDelta<MultiSwitchPortMap> StateDelta::getPortsDelta() const {
  return MultiSwitchMapDelta<MultiSwitchPortMap>(
      old_->getPorts().get(), new_->getPorts().get());
}

MultiSwitchMapDelta<MultiSwitchVlanMap> StateDelta::getVlansDelta() const {
  return MultiSwitchMapDelta<MultiSwitchVlanMap>(
      old_->getMultiSwitchVlans().get(), new_->getMultiSwitchVlans().get());
}

MultiSwitchInterfaceMapDelta StateDelta::getIntfsDelta() const {
  return MultiSwitchInterfaceMapDelta(
      old_->getInterfaces().get(), new_->getInterfaces().get());
}

MultiSwitchInterfaceMapDelta StateDelta::getRemoteIntfsDelta() const {
  return MultiSwitchInterfaceMapDelta(
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
    auto oldMultiSwitchAcls = old_->getAcls();
    oldAcls.reset(new PrioAclMap());
    for (const auto& iter : std::as_const(*oldMultiSwitchAcls)) {
      if (iter.second) {
        oldAcls->addAcls(iter.second);
      }
    }
    auto newMultiSwitchAcls = new_->getAcls();
    newAcls.reset(new PrioAclMap());
    for (const auto& iter : std::as_const(*newMultiSwitchAcls)) {
      if (iter.second) {
        newAcls->addAcls(iter.second);
      }
    }
  }

  return AclMapDelta(std::move(oldAcls), std::move(newAcls));
}

ThriftMapDelta<AclTableMap> StateDelta::getAclTablesDelta(
    cfg::AclStage aclStage) const {
  return ThriftMapDelta<AclTableMap>(
      old_->getAclTablesForStage(aclStage).get(),
      new_->getAclTablesForStage(aclStage).get());
}

MultiSwitchMapDelta<MultiSwitchAclTableGroupMap>
StateDelta::getAclTableGroupsDelta() const {
  return MultiSwitchMapDelta<MultiSwitchAclTableGroupMap>(
      old_->getAclTableGroups().get(), new_->getAclTableGroups().get());
}

MultiSwitchMapDelta<MultiSwitchQosPolicyMap> StateDelta::getQosPoliciesDelta()
    const {
  return MultiSwitchMapDelta<MultiSwitchQosPolicyMap>(
      old_->getQosPolicies().get(), new_->getQosPolicies().get());
}

MultiSwitchMapDelta<MultiSwitchAggregatePortMap>
StateDelta::getAggregatePortsDelta() const {
  return MultiSwitchMapDelta<MultiSwitchAggregatePortMap>(
      old_->getAggregatePorts().get(), new_->getAggregatePorts().get());
}

ThriftMapDelta<SflowCollectorMap> StateDelta::getSflowCollectorsDelta() const {
  return ThriftMapDelta<SflowCollectorMap>(
      old_->getSflowCollectors().get(), new_->getSflowCollectors().get());
}

MultiSwitchMapDelta<MultiSwitchLoadBalancerMap>
StateDelta::getLoadBalancersDelta() const {
  return MultiSwitchMapDelta<MultiSwitchLoadBalancerMap>(
      old_->getLoadBalancers().get(), new_->getLoadBalancers().get());
}

ThriftMapDelta<UdfGroupMap> StateDelta::getUdfGroupDelta() const {
  std::shared_ptr<UdfGroupMap> oldUdfGroupMap;
  if (old_->getUdfConfig()) {
    oldUdfGroupMap = old_->getUdfConfig()->getUdfGroupMap();
  }
  std::shared_ptr<UdfGroupMap> newUdfGroupMap;
  if (new_->getUdfConfig()) {
    newUdfGroupMap = new_->getUdfConfig()->getUdfGroupMap();
  }
  return ThriftMapDelta<UdfGroupMap>(
      oldUdfGroupMap.get(), newUdfGroupMap.get());
}

ThriftMapDelta<UdfPacketMatcherMap> StateDelta::getUdfPacketMatcherDelta()
    const {
  std::shared_ptr<UdfPacketMatcherMap> oldUdfPacketMatcherMap;
  if (old_->getUdfConfig()) {
    oldUdfPacketMatcherMap = old_->getUdfConfig()->getUdfPacketMatcherMap();
  }
  std::shared_ptr<UdfPacketMatcherMap> newUdfPacketMatcherMap;
  if (new_->getUdfConfig()) {
    newUdfPacketMatcherMap = new_->getUdfConfig()->getUdfPacketMatcherMap();
  }
  return ThriftMapDelta<UdfPacketMatcherMap>(
      oldUdfPacketMatcherMap.get(), newUdfPacketMatcherMap.get());
}

MultiSwitchMapDelta<MultiSwitchDsfNodeMap> StateDelta::getDsfNodesDelta()
    const {
  return MultiSwitchMapDelta<MultiSwitchDsfNodeMap>(
      old_->getDsfNodes().get(), new_->getDsfNodes().get());
}

DeltaValue<ControlPlane> StateDelta::getControlPlaneDelta() const {
  return DeltaValue<ControlPlane>(
      old_->getControlPlane(), new_->getControlPlane());
}

MultiSwitchMapDelta<MultiSwitchMirrorMap> StateDelta::getMirrorsDelta() const {
  return MultiSwitchMapDelta<MultiSwitchMirrorMap>(
      old_->getMirrors().get(), new_->getMirrors().get());
}

ThriftMapDelta<TransceiverMap> StateDelta::getTransceiversDelta() const {
  return ThriftMapDelta<TransceiverMap>(
      old_->getTransceivers().get(), new_->getTransceivers().get());
}

MultiSwitchForwardingInformationBaseMapDelta StateDelta::getFibsDelta() const {
  return MultiSwitchForwardingInformationBaseMapDelta(
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

MultiSwitchMapDelta<MultiLabelForwardingInformationBase>
StateDelta::getLabelForwardingInformationBaseDelta() const {
  return MultiSwitchMapDelta<MultiLabelForwardingInformationBase>(
      old_->getLabelForwardingInformationBase().get(),
      new_->getLabelForwardingInformationBase().get());
}

DeltaValue<QosPolicy> StateDelta::getDefaultDataPlaneQosPolicyDelta() const {
  return DeltaValue<QosPolicy>(
      old_->getDefaultDataPlaneQosPolicy(),
      new_->getDefaultDataPlaneQosPolicy());
}

MultiSwitchMapDelta<MultiSwitchSystemPortMap> StateDelta::getSystemPortsDelta()
    const {
  return MultiSwitchMapDelta<MultiSwitchSystemPortMap>(
      old_->getSystemPorts().get(), new_->getSystemPorts().get());
}

MultiSwitchMapDelta<MultiSwitchSystemPortMap>
StateDelta::getRemoteSystemPortsDelta() const {
  return MultiSwitchMapDelta<MultiSwitchSystemPortMap>(
      old_->getRemoteSystemPorts().get(), new_->getRemoteSystemPorts().get());
}

ThriftMapDelta<IpTunnelMap> StateDelta::getIpTunnelsDelta() const {
  return getFirstMapDelta<IpTunnelMap>(old_->getTunnels(), new_->getTunnels());
}

MultiSwitchMapDelta<MultiTeFlowTable> StateDelta::getTeFlowEntriesDelta()
    const {
  return MultiSwitchMapDelta<MultiTeFlowTable>(
      old_->getTeFlowTable().get(), new_->getTeFlowTable().get());
}

const fsdb::OperDelta& StateDelta::getOperDelta() const {
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
template struct ThriftMapDelta<InterfaceMap>;
template struct ThriftMapDelta<PortMap>;
template struct ThriftMapDelta<AclMap>;
template struct ThriftMapDelta<AclTableGroupMap>;
template struct ThriftMapDelta<AclTableMap>;
template struct ThriftMapDelta<QosPolicyMap>;
template struct ThriftMapDelta<AggregatePortMap>;
template struct ThriftMapDelta<SflowCollectorMap>;
template struct ThriftMapDelta<LoadBalancerMap>;
template struct ThriftMapDelta<MirrorMap>;
template struct ThriftMapDelta<TransceiverMap>;
template struct ThriftMapDelta<ForwardingInformationBaseV4>;
template struct ThriftMapDelta<ForwardingInformationBaseV6>;
template struct ThriftMapDelta<LabelForwardingInformationBase>;
template struct ThriftMapDelta<SystemPortMap>;
template struct ThriftMapDelta<IpTunnelMap>;
template struct ThriftMapDelta<TeFlowTable>;

template struct MultiSwitchMapDelta<MultiSwitchMirrorMap>;
template struct MultiSwitchMapDelta<MultiSwitchSflowCollectorMap>;
template struct MultiSwitchMapDelta<MultiLabelForwardingInformationBase>;
template struct MultiSwitchMapDelta<MultiSwitchQosPolicyMap>;
template struct MultiSwitchMapDelta<MultiSwitchIpTunnelMap>;
template struct MultiSwitchMapDelta<MultiTeFlowTable>;
template struct MultiSwitchMapDelta<MultiSwitchAggregatePortMap>;
template struct MultiSwitchMapDelta<MultiSwitchLoadBalancerMap>;
template struct MultiSwitchMapDelta<MultiSwitchTransceiverMap>;
template struct MultiSwitchMapDelta<MultiSwitchBufferPoolCfgMap>;
template struct MultiSwitchMapDelta<MultiSwitchPortMap>;
template struct MultiSwitchMapDelta<MultiSwitchAclTableGroupMap>;
template struct MultiSwitchMapDelta<MultiSwitchDsfNodeMap>;
template struct MultiSwitchMapDelta<MultiSwitchSystemPortMap>;
template struct MultiSwitchMapDelta<MultiSwitchAclMap>;

} // namespace facebook::fboss
