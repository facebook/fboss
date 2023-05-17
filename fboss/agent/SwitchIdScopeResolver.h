// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/HwSwitchMatcher.h"
#include "fboss/agent/types.h"

#include <unordered_set>

namespace facebook::fboss {
namespace cfg {
class Mirror;
class DsfNode;
class IpInIpTunnel;
class AclEntry;
class SystemPort;
class Port;
class BufferPoolConfig;
class QosPolicy;
} // namespace cfg
class Mirror;
class DsfNode;
class IpTunnel;
class AclEntry;
class SystemPort;
class Vlan;
class LoadBalancer;
class TeFlowEntry;
class SwitchState;
class Interface;
class Port;
class AclTableGroup;
class ForwardingInformationBaseContainer;
class BufferPoolCfg;
class QosPolicy;
template <typename T>
class Route;

class SwitchIdScopeResolver {
 public:
  explicit SwitchIdScopeResolver(
      const std::map<int64_t, cfg::SwitchInfo>& switchIdToSwitchInfo);

  const HwSwitchMatcher& scope(const cfg::Mirror& /*m*/) const {
    return l3SwitchMatcher();
  }
  const HwSwitchMatcher& scope(const std::shared_ptr<Mirror>& /*m*/) const {
    return l3SwitchMatcher();
  }
  const HwSwitchMatcher& scope(const cfg::DsfNode& /*m*/) const {
    return allSwitchMatcher();
  }
  const HwSwitchMatcher& scope(const std::shared_ptr<DsfNode>& /*m*/) const {
    return allSwitchMatcher();
  }
  const HwSwitchMatcher& scope(const cfg::LoadBalancer& /*m*/) const {
    return l3SwitchMatcher();
  }
  const HwSwitchMatcher& scope(
      const std::shared_ptr<LoadBalancer>& /*m*/) const {
    return l3SwitchMatcher();
  }
  const HwSwitchMatcher& scope(
      const std::shared_ptr<TeFlowEntry>& /*m*/) const {
    return l3SwitchMatcher();
  }
  HwSwitchMatcher scope(PortID portId) const;
  HwSwitchMatcher scope(const std::shared_ptr<Port>& port) const;
  HwSwitchMatcher scope(const cfg::Port& port) const;
  HwSwitchMatcher scope(const cfg::AggregatePort& aggPort) const;
  const HwSwitchMatcher& scope(const cfg::IpInIpTunnel& /*m*/) const {
    return l3SwitchMatcher();
  }
  const HwSwitchMatcher& scope(const std::shared_ptr<IpTunnel>& /*m*/) const {
    return l3SwitchMatcher();
  }
  const HwSwitchMatcher& scope(const cfg::AclEntry& /*acl*/) const {
    return l3SwitchMatcher();
  }
  const HwSwitchMatcher& scope(const std::shared_ptr<AclEntry>& /*acl*/) const {
    return l3SwitchMatcher();
  }
  const HwSwitchMatcher scope(const std::shared_ptr<Vlan>& vlan) const;

  HwSwitchMatcher scope(SystemPortID sysPortID) const;
  HwSwitchMatcher scope(const std::shared_ptr<SystemPort>& sysPort) const;
  HwSwitchMatcher scope(const cfg::SystemPort& sysPort) const;
  HwSwitchMatcher scope(
      const std::shared_ptr<Interface>& intf,
      const std::shared_ptr<SwitchState>& state) const;
  HwSwitchMatcher scope(
      const std::shared_ptr<Interface>& intf,
      const cfg::SwitchConfig& cfg) const;

  HwSwitchMatcher scope(const cfg::AclTableGroup& aclTableGroup) const;
  HwSwitchMatcher scope(
      const std::shared_ptr<AclTableGroup>& aclTableGroup) const;
  HwSwitchMatcher scope(
      const std::shared_ptr<ForwardingInformationBaseContainer>& fibs) const;
  HwSwitchMatcher scope(const std::shared_ptr<Route<LabelID>>& entry) const;
  const HwSwitchMatcher& scope(const cfg::BufferPoolConfig& /*b*/) const {
    return l3SwitchMatcher();
  }
  const HwSwitchMatcher& scope(
      const std::shared_ptr<BufferPoolCfg>& /*b*/) const {
    return l3SwitchMatcher();
  }
  const HwSwitchMatcher& scope(const cfg::QosPolicy& /*q*/) const {
    return l3SwitchMatcher();
  }
  const HwSwitchMatcher& scope(const std::shared_ptr<QosPolicy>& /*q*/) const {
    return l3SwitchMatcher();
  }

 private:
  void checkL3() const;
  void checkVoq() const;
  const HwSwitchMatcher& l3SwitchMatcher() const;
  const HwSwitchMatcher& allSwitchMatcher() const;
  const HwSwitchMatcher& voqSwitchMatcher() const;
  std::unordered_set<SwitchID> getSwitchIdsOfType(cfg::SwitchType type) const;
  std::map<int64_t, cfg::SwitchInfo> switchIdToSwitchInfo_;
  std::unique_ptr<HwSwitchMatcher> l3SwitchMatcher_;
  std::unique_ptr<HwSwitchMatcher> allSwitchMatcher_;
  std::unique_ptr<HwSwitchMatcher> voqSwitchMatcher_;
};
} // namespace facebook::fboss
