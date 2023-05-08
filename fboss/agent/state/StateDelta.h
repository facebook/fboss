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
#include "fboss/agent/state/FlowletSwitchingConfig.h"
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

DECLARE_bool(enable_state_oper_delta);
DECLARE_bool(verify_apply_oper_delta);

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
      std::shared_ptr<SwitchState> newState);
  StateDelta(std::shared_ptr<SwitchState> oldState, fsdb::OperDelta operDelta);
  virtual ~StateDelta();

  const std::shared_ptr<SwitchState>& oldState() const {
    return old_;
  }
  const std::shared_ptr<SwitchState>& newState() const {
    return new_;
  }

  ThriftMapDelta<PortMap> getPortsDelta() const;
  VlanMapDelta getVlansDelta() const;
  InterfaceMapDelta getIntfsDelta() const;
  DeltaValue<QosPolicy> getDefaultDataPlaneQosPolicyDelta() const;
  AclMapDelta getAclsDelta(
      cfg::AclStage aclStage = cfg::AclStage::INGRESS,
      std::optional<std::string> tableName = std::nullopt) const;
  ThriftMapDelta<AclTableMap> getAclTablesDelta(cfg::AclStage aclStage) const;
  ThriftMapDelta<AclTableGroupMap> getAclTableGroupsDelta() const;
  QosPolicyMapDelta getQosPoliciesDelta() const;
  ThriftMapDelta<AggregatePortMap> getAggregatePortsDelta() const;
  ThriftMapDelta<SflowCollectorMap> getSflowCollectorsDelta() const;
  ThriftMapDelta<LoadBalancerMap> getLoadBalancersDelta() const;
  DeltaValue<ControlPlane> getControlPlaneDelta() const;
  ThriftMapDelta<UdfPacketMatcherMap> getUdfPacketMatcherDelta() const;
  ThriftMapDelta<UdfGroupMap> getUdfGroupDelta() const;

  MultiSwitchMapDelta<MultiSwitchMirrorMap> getMirrorsDelta() const;

  ThriftMapDelta<TransceiverMap> getTransceiversDelta() const;
  ForwardingInformationBaseMapDelta getFibsDelta() const;
  ThriftMapDelta<LabelForwardingInformationBase>
  getLabelForwardingInformationBaseDelta() const;
  DeltaValue<SwitchSettings> getSwitchSettingsDelta() const;
  DeltaValue<FlowletSwitchingConfig> getFlowletSwitchingConfigDelta() const;
  MultiSwitchMapDelta<MultiSwitchSystemPortMap> getSystemPortsDelta() const;
  ThriftMapDelta<IpTunnelMap> getIpTunnelsDelta() const;
  ThriftMapDelta<TeFlowTable> getTeFlowEntriesDelta() const;
  // Remote object deltas
  MultiSwitchMapDelta<MultiSwitchSystemPortMap> getRemoteSystemPortsDelta()
      const;
  InterfaceMapDelta getRemoteIntfsDelta() const;
  MultiSwitchMapDelta<MultiSwitchDsfNodeMap> getDsfNodesDelta() const;

  const fsdb::OperDelta& getOperDelta() const;

 private:
  // Forbidden copy constructor and assignment operator
  StateDelta(StateDelta const&) = delete;
  StateDelta& operator=(StateDelta const&) = delete;

  std::shared_ptr<SwitchState> old_;
  std::shared_ptr<SwitchState> new_;
  // on-demand populate oper delta and keep it cached
  mutable std::optional<fsdb::OperDelta> operDelta_;
};

std::ostream& operator<<(std::ostream& out, const StateDelta& stateDelta);

} // namespace facebook::fboss
