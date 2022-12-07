/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include <functional>
#include <memory>
#include <ostream>

#include "fboss/agent/state/AclMap.h"
#include "fboss/agent/state/AclTableGroup.h"
#include "fboss/agent/state/AclTableGroupMap.h"
#include "fboss/agent/state/AclTableMap.h"
#include "fboss/agent/state/AggregatePortMap.h"
#include "fboss/agent/state/DeltaFunctions.h"
#include "fboss/agent/state/DsfNodeMap.h"
#include "fboss/agent/state/ForwardingInformationBaseDelta.h"
#include "fboss/agent/state/ForwardingInformationBaseMap.h"
#include "fboss/agent/state/InterfaceMap.h"
#include "fboss/agent/state/InterfaceMapDelta.h"
#include "fboss/agent/state/IpTunnelMap.h"
#include "fboss/agent/state/LabelForwardingInformationBase.h"
#include "fboss/agent/state/LoadBalancerMap.h"
#include "fboss/agent/state/Mirror.h"
#include "fboss/agent/state/MirrorMap.h"
#include "fboss/agent/state/NodeMapDelta.h"
#include "fboss/agent/state/PortMap.h"
#include "fboss/agent/state/QosPolicyMap.h"
#include "fboss/agent/state/UdfGroupMap.h"
#include "fboss/agent/state/UdfPacketMatcherMap.h"

#include "fboss/agent/state/SflowCollectorMap.h"
#include "fboss/agent/state/SwitchSettings.h"
#include "fboss/agent/state/SystemPortMap.h"
#include "fboss/agent/state/TeFlowTable.h"
#include "fboss/agent/state/TransceiverMap.h"
#include "fboss/agent/state/VlanMapDelta.h"

#include "fboss/thrift_cow/nodes/Types.h"

namespace facebook::fboss {

class SwitchState;
class ControlPlane;

/*
 * StateDelta contains code for examining the differences between two
 * SwitchStates.
 */
class StateDelta {
 public:
  StateDelta() {}
  StateDelta(
      std::shared_ptr<SwitchState> oldState,
      std::shared_ptr<SwitchState> newState)
      : old_(oldState), new_(newState) {}
  virtual ~StateDelta();

  const std::shared_ptr<SwitchState>& oldState() const {
    return old_;
  }
  const std::shared_ptr<SwitchState>& newState() const {
    return new_;
  }
  bool operator==(const StateDelta& other) const {
    return other.oldState() == old_ && other.newState() == new_;
  }

  NodeMapDelta<PortMap> getPortsDelta() const;
  VlanMapDelta getVlansDelta() const;
  InterfaceMapDelta getIntfsDelta() const;
  DeltaValue<QosPolicy> getDefaultDataPlaneQosPolicyDelta() const;
  AclMapDelta getAclsDelta(
      cfg::AclStage aclStage = cfg::AclStage::INGRESS,
      std::optional<std::string> tableName = std::nullopt) const;
  thrift_cow::ThriftMapDelta<AclTableMap> getAclTablesDelta(
      cfg::AclStage aclStage) const;
  thrift_cow::ThriftMapDelta<AclTableGroupMap> getAclTableGroupsDelta() const;
  QosPolicyMapDelta getQosPoliciesDelta() const;
  thrift_cow::ThriftMapDelta<AggregatePortMap> getAggregatePortsDelta() const;
  thrift_cow::ThriftMapDelta<SflowCollectorMap> getSflowCollectorsDelta() const;
  thrift_cow::ThriftMapDelta<LoadBalancerMap> getLoadBalancersDelta() const;
  DeltaValue<ControlPlane> getControlPlaneDelta() const;
  thrift_cow::ThriftMapDelta<UdfPacketMatcherMap> getUdfPacketMatcherDelta()
      const;
  thrift_cow::ThriftMapDelta<UdfGroupMap> getUdfGroupDelta() const;

  thrift_cow::ThriftMapDelta<MirrorMap> getMirrorsDelta() const;

  thrift_cow::ThriftMapDelta<TransceiverMap> getTransceiversDelta() const;
  ForwardingInformationBaseMapDelta getFibsDelta() const;
  NodeMapDelta<LabelForwardingInformationBase>
  getLabelForwardingInformationBaseDelta() const;
  DeltaValue<SwitchSettings> getSwitchSettingsDelta() const;
  thrift_cow::ThriftMapDelta<SystemPortMap> getSystemPortsDelta() const;
  thrift_cow::ThriftMapDelta<IpTunnelMap> getIpTunnelsDelta() const;
  thrift_cow::ThriftMapDelta<TeFlowTable> getTeFlowEntriesDelta() const;
  // Remote object deltas
  thrift_cow::ThriftMapDelta<SystemPortMap> getRemoteSystemPortsDelta() const;
  InterfaceMapDelta getRemoteIntfsDelta() const;
  thrift_cow::ThriftMapDelta<DsfNodeMap> getDsfNodesDelta() const;

 private:
  // Forbidden copy constructor and assignment operator
  StateDelta(StateDelta const&) = delete;
  StateDelta& operator=(StateDelta const&) = delete;

  std::shared_ptr<SwitchState> old_;
  std::shared_ptr<SwitchState> new_;
};

std::ostream& operator<<(std::ostream& out, const StateDelta& stateDelta);

} // namespace facebook::fboss
