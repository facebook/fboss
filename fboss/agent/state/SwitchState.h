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

#include <chrono>
#include <memory>

#include <folly/FBString.h>
#include <folly/Memory.h>
#include <folly/dynamic.h>

#include "fboss/agent/gen-cpp2/switch_state_types.h"
#include "fboss/agent/state/AclMap.h"
#include "fboss/agent/state/AclTableGroup.h"
#include "fboss/agent/state/AclTableGroupMap.h"
#include "fboss/agent/state/AggregatePort.h"
#include "fboss/agent/state/AggregatePortMap.h"
#include "fboss/agent/state/BufferPoolConfig.h"
#include "fboss/agent/state/BufferPoolConfigMap.h"
#include "fboss/agent/state/ControlPlane.h"
#include "fboss/agent/state/DsfNodeMap.h"
#include "fboss/agent/state/ForwardingInformationBaseMap.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/InterfaceMap.h"
#include "fboss/agent/state/IpTunnelMap.h"
#include "fboss/agent/state/LabelForwardingInformationBase.h"
#include "fboss/agent/state/LoadBalancerMap.h"
#include "fboss/agent/state/MirrorMap.h"
#include "fboss/agent/state/NodeBase.h"
#include "fboss/agent/state/PortMap.h"
#include "fboss/agent/state/QcmConfig.h"
#include "fboss/agent/state/QosPolicyMap.h"
#include "fboss/agent/state/SflowCollectorMap.h"
#include "fboss/agent/state/SwitchSettings.h"
#include "fboss/agent/state/SystemPortMap.h"
#include "fboss/agent/state/TeFlowTable.h"
#include "fboss/agent/state/TransceiverMap.h"
#include "fboss/agent/state/UdfConfig.h"
#include "fboss/agent/state/Vlan.h"
#include "fboss/agent/state/VlanMap.h"
#include "fboss/agent/types.h"

DECLARE_bool(enable_acl_table_group);

namespace facebook::fboss {

class ControlPlane;
class Interface;
template <typename AddressT>
class Route;
class SflowCollector;
class SflowCollectorMap;
class SwitchSettings;
class QcmCfg;
class BufferPoolCfg;
class BufferPoolCfgMap;

struct SwitchStateFields
    : public ThriftyFields<SwitchStateFields, state::SwitchState> {
  SwitchStateFields();

  explicit SwitchStateFields(const state::SwitchState& data) {
    writableData() = data;
  }
  template <typename Fn>
  void forEachChild(Fn fn) {
    fn(ports.get());
    fn(aggPorts.get());
    fn(vlans.get());
    fn(interfaces.get());
    fn(acls.get());
    fn(aclTableGroups.get());
    fn(sFlowCollectors.get());
    fn(qosPolicies.get());
    fn(controlPlane.get());
    fn(loadBalancers.get());
    fn(mirrors.get());
    fn(fibs.get());
    fn(labelFib.get());
    fn(switchSettings.get());
    fn(transceivers.get());
    fn(systemPorts.get());
    fn(ipTunnels.get());
    fn(teFlowTable.get());
    fn(remoteSystemPorts.get());
    fn(remoteInterfaces.get());
    fn(dsfNodes.get());
  }

  state::SwitchState toThrift() const override;
  static SwitchStateFields fromThrift(const state::SwitchState& state);

  // Used for testing thrifty conversion
  bool operator==(const SwitchStateFields& other) const;

  /*
   * Reconstruct object from folly::dynamic
   */
  static SwitchStateFields fromFollyDynamic(const folly::dynamic& json);

  // Static state, which can be accessed without locking.
  std::shared_ptr<PortMap> ports;
  std::shared_ptr<AggregatePortMap> aggPorts;
  std::shared_ptr<VlanMap> vlans;
  std::shared_ptr<InterfaceMap> interfaces;
  std::shared_ptr<AclMap> acls;
  std::shared_ptr<AclTableGroupMap> aclTableGroups;
  std::shared_ptr<SflowCollectorMap> sFlowCollectors;
  std::shared_ptr<QosPolicyMap> qosPolicies;
  std::shared_ptr<ControlPlane> controlPlane;
  std::shared_ptr<LoadBalancerMap> loadBalancers;
  std::shared_ptr<MirrorMap> mirrors;
  std::shared_ptr<ForwardingInformationBaseMap> fibs;
  std::shared_ptr<LabelForwardingInformationBase> labelFib;
  std::shared_ptr<SwitchSettings> switchSettings;
  std::shared_ptr<QcmCfg> qcmCfg;
  std::shared_ptr<BufferPoolCfgMap> bufferPoolCfgs;
  std::shared_ptr<TransceiverMap> transceivers;
  std::shared_ptr<SystemPortMap> systemPorts;
  std::shared_ptr<IpTunnelMap> ipTunnels;
  std::shared_ptr<TeFlowTable> teFlowTable;
  std::shared_ptr<DsfNodeMap> dsfNodes;
  VlanID defaultVlan{0};

  std::shared_ptr<QosPolicy> defaultDataPlaneQosPolicy;
  // Remote objects
  std::shared_ptr<SystemPortMap> remoteSystemPorts;
  std::shared_ptr<InterfaceMap> remoteInterfaces;
  std::shared_ptr<UdfConfig> udfConfig;

  // Timeout settings
  // TODO(aeckert): Figure out a nicer way to store these config fields
  // in an accessible way
  std::chrono::seconds arpTimeout{60};
  std::chrono::seconds ndpTimeout{60};
  std::chrono::seconds arpAgerInterval{5};

  // NeighborCache configuration values
  // Keep maxNeighborProbes sufficiently large
  // so we don't expire arp/ndp entries during
  // agent restart on our neighbors
  uint32_t maxNeighborProbes{300};
  std::chrono::seconds staleEntryInterval{10};
  // source IP of the DHCP relay pkt to the DHCP server
  folly::IPAddressV4 dhcpV4RelaySrc;
  folly::IPAddressV6 dhcpV6RelaySrc;
  // source IP of the DHCP reply pkt to the client host
  folly::IPAddressV4 dhcpV4ReplySrc;
  folly::IPAddressV6 dhcpV6ReplySrc;

  // PFC watchdog recovery action configuration
  std::optional<cfg::PfcWatchdogRecoveryAction> pfcWatchdogRecoveryAction{};
};

USE_THRIFT_COW(SwitchState);
RESOLVE_STRUCT_MEMBER(SwitchState, switch_state_tags::portMap, PortMap);
RESOLVE_STRUCT_MEMBER(SwitchState, switch_state_tags::vlanMap, VlanMap);
RESOLVE_STRUCT_MEMBER(SwitchState, switch_state_tags::aclMap, AclMap);
RESOLVE_STRUCT_MEMBER(
    SwitchState,
    switch_state_tags::transceiverMap,
    TransceiverMap);
RESOLVE_STRUCT_MEMBER(
    SwitchState,
    switch_state_tags::bufferPoolCfgMap,
    BufferPoolCfgMap);
RESOLVE_STRUCT_MEMBER(SwitchState, switch_state_tags::mirrorMap, MirrorMap);
RESOLVE_STRUCT_MEMBER(
    SwitchState,
    switch_state_tags::controlPlane,
    ControlPlane);
RESOLVE_STRUCT_MEMBER(
    SwitchState,
    switch_state_tags::switchSettings,
    SwitchSettings);
RESOLVE_STRUCT_MEMBER(
    SwitchState,
    switch_state_tags::systemPortMap,
    SystemPortMap);
RESOLVE_STRUCT_MEMBER(
    SwitchState,
    switch_state_tags::fibs,
    ForwardingInformationBaseMap);
RESOLVE_STRUCT_MEMBER(
    SwitchState,
    switch_state_tags::labelFib,
    LabelForwardingInformationBase);
RESOLVE_STRUCT_MEMBER(
    SwitchState,
    switch_state_tags::qosPolicyMap,
    QosPolicyMap);
RESOLVE_STRUCT_MEMBER(
    SwitchState,
    switch_state_tags::sflowCollectorMap,
    SflowCollectorMap);
RESOLVE_STRUCT_MEMBER(SwitchState, switch_state_tags::ipTunnelMap, IpTunnelMap);
RESOLVE_STRUCT_MEMBER(SwitchState, switch_state_tags::teFlowTable, TeFlowTable);
RESOLVE_STRUCT_MEMBER(
    SwitchState,
    switch_state_tags::aggregatePortMap,
    AggregatePortMap);
RESOLVE_STRUCT_MEMBER(
    SwitchState,
    switch_state_tags::loadBalancerMap,
    LoadBalancerMap);
RESOLVE_STRUCT_MEMBER(
    SwitchState,
    switch_state_tags::aclTableGroupMap,
    AclTableGroupMap);
RESOLVE_STRUCT_MEMBER(
    SwitchState,
    switch_state_tags::interfaceMap,
    InterfaceMap);
RESOLVE_STRUCT_MEMBER(SwitchState, switch_state_tags::dsfNodes, DsfNodeMap);
RESOLVE_STRUCT_MEMBER(
    SwitchState,
    switch_state_tags::remoteSystemPortMap,
    SystemPortMap);
RESOLVE_STRUCT_MEMBER(
    SwitchState,
    switch_state_tags::remoteInterfaceMap,
    InterfaceMap);
RESOLVE_STRUCT_MEMBER(
    SwitchState,
    switch_state_tags::defaultDataPlaneQosPolicy,
    QosPolicy);
RESOLVE_STRUCT_MEMBER(SwitchState, switch_state_tags::qcmCfg, QcmCfg);
RESOLVE_STRUCT_MEMBER(SwitchState, switch_state_tags::udfConfig, UdfConfig);

/*
 * SwitchState stores the current switch configuration.
 *
 * The SwitchState stores two primary types of data: static data and dynamic
 * data.
 *
 * Static data changes infrequently.  This is generally data that is only
 * changed by human interaction, such as data loaded from the configuration
 * file.  This includes things like port configuration and VLAN configuration.
 *
 * Dynamic data may change more frequently.  This is generally data that is
 * updated automatically, and/or learned from the network.  This includes
 * things like ARP entries.
 *
 *
 * The SwitchState manages static and dynamic data differently.  Static data is
 * read-only within the SwitchState object.  This allows it to be read without
 * locking.  When it does need to be changed, a new copy of the SwitchState is
 * made, and the SwSwitch is updated to point to the new version.  To access
 * static data, callers need to atomically load the current SwitchState from
 * the SwSwitch, but from then on they do not need any further locking or
 * atomic operations.
 *
 * When performing copy-on-write, the SwitchState only copies portions of the
 * state that are actually modified.  Parts of the state that are unchanged
 * will be re-used by the new SwitchState object.
 *
 *
 * Because dynamic data changes more frequently, it is allowed to be modified
 * without making a new copy-on-write version of the SwitchState.  Therefore
 * locks are used to protect access to dynamic data.
 */
class SwitchState : public ThriftStructNode<SwitchState, state::SwitchState> {
 public:
  using BaseT = ThriftStructNode<SwitchState, state::SwitchState>;
  /*
   * Create a new, empty state.
   */
  SwitchState();
  ~SwitchState() override;

  static std::unique_ptr<SwitchState> uniquePtrFromThrift(
      const state::SwitchState& switchState);

  static void modify(std::shared_ptr<SwitchState>* state);

  // Helper function to clone a new SwitchState to modify the original
  // TransceiverMap if there's a change.
  static std::shared_ptr<SwitchState> modifyTransceivers(
      const std::shared_ptr<SwitchState>& state,
      const std::unordered_map<TransceiverID, TransceiverInfo>& currentTcvrs);

  template <typename EntryClassT, typename NTableT>
  static void revertNewNeighborEntry(
      const std::shared_ptr<EntryClassT>& newEntry,
      const std::shared_ptr<EntryClassT>& oldEntry,
      std::shared_ptr<SwitchState>* appliedState);

  template <typename AddressT>
  static void revertNewRouteEntry(
      const RouterID& id,
      const std::shared_ptr<Route<AddressT>>& newRoute,
      const std::shared_ptr<Route<AddressT>>& oldRoute,
      std::shared_ptr<SwitchState>* appliedState);

  static void revertNewTeFlowEntry(
      const std::shared_ptr<TeFlowEntry>& newFlowEntry,
      const std::shared_ptr<TeFlowEntry>& oldFlowEntry,
      std::shared_ptr<SwitchState>* appliedState);

  const std::shared_ptr<PortMap>& getPorts() const {
    return cref<switch_state_tags::portMap>();
  }
  std::shared_ptr<Port> getPort(PortID id) const;

  const std::shared_ptr<AggregatePortMap> getAggregatePorts() const {
    return cref<switch_state_tags::aggregatePortMap>();
  }

  const std::shared_ptr<VlanMap>& getVlans() const {
    return cref<switch_state_tags::vlanMap>();
  }

  VlanID getDefaultVlan() const;
  void setDefaultVlan(const VlanID& id);

  const std::shared_ptr<QosPolicy> getDefaultDataPlaneQosPolicy() const {
    return cref<switch_state_tags::defaultDataPlaneQosPolicy>();
  }

  void setDefaultDataPlaneQosPolicy(std::shared_ptr<QosPolicy> qosPolicy) {
    ref<switch_state_tags::defaultDataPlaneQosPolicy>() = qosPolicy;
  }

  const std::shared_ptr<InterfaceMap>& getInterfaces() const {
    return cref<switch_state_tags::interfaceMap>();
  }

  std::shared_ptr<AclEntry> getAcl(const std::string& name) const;

  const std::shared_ptr<AclMap>& getAcls() const {
    return cref<switch_state_tags::aclMap>();
  }

  const std::shared_ptr<AclTableGroupMap>& getAclTableGroups() const {
    return cref<switch_state_tags::aclTableGroupMap>();
  }

  std::chrono::seconds getArpTimeout() const {
    auto arpTimeout = cref<switch_state_tags::arpTimeout>()->toThrift();
    return std::chrono::seconds(arpTimeout);
  }

  std::shared_ptr<const AclMap> getAclsForTable(
      cfg::AclStage aclStage,
      const std::string& tableName) const;

  std::shared_ptr<const AclTableMap> getAclTablesForStage(
      cfg::AclStage aclStage) const;

  const std::shared_ptr<SflowCollectorMap>& getSflowCollectors() const {
    return cref<switch_state_tags::sflowCollectorMap>();
  }

  std::shared_ptr<QosPolicy> getQosPolicy(const std::string& name) const {
    return getQosPolicies()->getQosPolicyIf(name);
  }

  const std::shared_ptr<QosPolicyMap>& getQosPolicies() const {
    return cref<switch_state_tags::qosPolicyMap>();
  }

  const std::shared_ptr<ControlPlane>& getControlPlane() const {
    return cref<switch_state_tags::controlPlane>();
  }

  const std::shared_ptr<SwitchSettings>& getSwitchSettings() const {
    return cref<switch_state_tags::switchSettings>();
  }

  const std::shared_ptr<QcmCfg> getQcmCfg() const {
    return cref<switch_state_tags::qcmCfg>();
  }

  const std::shared_ptr<BufferPoolCfgMap> getBufferPoolCfgs() const {
    return cref<switch_state_tags::bufferPoolCfgMap>();
  }

  void setArpTimeout(std::chrono::seconds timeout);

  std::chrono::seconds getNdpTimeout() const {
    return std::chrono::seconds(
        cref<switch_state_tags::ndpTimeout>()->toThrift());
  }

  void setNdpTimeout(std::chrono::seconds timeout);

  std::chrono::seconds getArpAgerInterval() const {
    return std::chrono::seconds(
        cref<switch_state_tags::arpAgerInterval>()->toThrift());
  }

  void setArpAgerInterval(std::chrono::seconds interval);

  uint32_t getMaxNeighborProbes() const {
    return cref<switch_state_tags::maxNeighborProbes>()->toThrift();
  }
  void setMaxNeighborProbes(uint32_t maxNeighborProbes);

  std::chrono::seconds getStaleEntryInterval() const {
    return std::chrono::seconds(
        cref<switch_state_tags::staleEntryInterval>()->toThrift());
  }

  void setStaleEntryInterval(std::chrono::seconds interval);

  // dhcp relay packet IP overrides

  folly::IPAddressV4 getDhcpV4RelaySrc() const {
    return network::toIPAddress(
               cref<switch_state_tags::dhcpV4RelaySrc>()->toThrift())
        .asV4();
  }
  void setDhcpV4RelaySrc(folly::IPAddressV4 v4RelaySrc) {
    set<switch_state_tags::dhcpV4RelaySrc>(
        network::toBinaryAddress(v4RelaySrc));
  }

  folly::IPAddressV6 getDhcpV6RelaySrc() const {
    return network::toIPAddress(
               cref<switch_state_tags::dhcpV6RelaySrc>()->toThrift())
        .asV6();
  }
  void setDhcpV6RelaySrc(folly::IPAddressV6 v6RelaySrc) {
    set<switch_state_tags::dhcpV6RelaySrc>(
        network::toBinaryAddress(v6RelaySrc));
  }

  folly::IPAddressV4 getDhcpV4ReplySrc() const {
    return network::toIPAddress(
               cref<switch_state_tags::dhcpV4ReplySrc>()->toThrift())
        .asV4();
  }
  void setDhcpV4ReplySrc(folly::IPAddressV4 v4ReplySrc) {
    set<switch_state_tags::dhcpV4ReplySrc>(
        network::toBinaryAddress(v4ReplySrc));
  }

  folly::IPAddressV6 getDhcpV6ReplySrc() const {
    return network::toIPAddress(
               cref<switch_state_tags::dhcpV6ReplySrc>()->toThrift())
        .asV6();
  }
  void setDhcpV6ReplySrc(folly::IPAddressV6 v6ReplySrc) {
    set<switch_state_tags::dhcpV6ReplySrc>(
        network::toBinaryAddress(v6ReplySrc));
  }

  // THRIFT_COPY
  std::optional<cfg::PfcWatchdogRecoveryAction> getPfcWatchdogRecoveryAction()
      const {
    auto action = safe_cref<switch_state_tags::pfcWatchdogRecoveryAction>();
    if (!action) {
      return std::nullopt;
    }
    return action->toThrift();
  }

  void setPfcWatchdogRecoveryAction(
      std::optional<cfg::PfcWatchdogRecoveryAction> recoveryAction) {
    if (!recoveryAction) {
      ref<switch_state_tags::pfcWatchdogRecoveryAction>().reset();
    } else {
      set<switch_state_tags::pfcWatchdogRecoveryAction>(*recoveryAction);
    }
  }

  const std::shared_ptr<LoadBalancerMap>& getLoadBalancers() const;
  const std::shared_ptr<MirrorMap>& getMirrors() const;
  const std::shared_ptr<ForwardingInformationBaseMap>& getFibs() const;
  const std::shared_ptr<LabelForwardingInformationBase>&
  getLabelForwardingInformationBase() const {
    return cref<switch_state_tags::labelFib>();
  }

  const std::shared_ptr<TransceiverMap>& getTransceivers() const {
    return cref<switch_state_tags::transceiverMap>();
  }
  const std::shared_ptr<SystemPortMap>& getSystemPorts() const {
    return cref<switch_state_tags::systemPortMap>();
  }
  const std::shared_ptr<IpTunnelMap>& getTunnels() const {
    return cref<switch_state_tags::ipTunnelMap>();
  }
  const std::shared_ptr<TeFlowTable>& getTeFlowTable() const {
    return cref<switch_state_tags::teFlowTable>();
  }

  const std::shared_ptr<DsfNodeMap>& getDsfNodes() const {
    return cref<switch_state_tags::dsfNodes>();
  }

  const std::shared_ptr<UdfConfig>& getUdfConfig() const {
    return cref<switch_state_tags::udfConfig>();
  }

  /*
   * Remote objects
   */
  const std::shared_ptr<SystemPortMap>& getRemoteSystemPorts() const {
    return cref<switch_state_tags::remoteSystemPortMap>();
  }
  const std::shared_ptr<InterfaceMap>& getRemoteInterfaces() const {
    return cref<switch_state_tags::remoteInterfaceMap>();
  }
  /*
   * Get system ports for a given switch id
   */
  std::shared_ptr<SystemPortMap> getSystemPorts(SwitchID switchId) const;
  /*
   * Get interfaces for a given switch id
   */
  std::shared_ptr<InterfaceMap> getInterfaces(SwitchID switchId) const;
  /*
   * The following functions modify the static state.
   * The should only be called on newly created SwitchState objects that are
   * only visible to a single thread, before they are published as the current
   * state.
   */

  void registerPort(PortID id, const std::string& name);
  void addPort(const std::shared_ptr<Port>& port);
  void resetPorts(std::shared_ptr<PortMap> ports);
  void resetAggregatePorts(std::shared_ptr<AggregatePortMap> aggPorts);
  void resetVlans(std::shared_ptr<VlanMap> vlans);
  void addVlan(const std::shared_ptr<Vlan>& vlan);
  void addIntf(const std::shared_ptr<Interface>& intf);
  void resetIntfs(std::shared_ptr<InterfaceMap> intfs);
  void addAcl(const std::shared_ptr<AclEntry>& acl);
  void addAclTable(const std::shared_ptr<AclTable>& aclTable);
  void resetAcls(std::shared_ptr<AclMap> acls);
  void resetAclTableGroups(std::shared_ptr<AclTableGroupMap> aclTableGroups);
  void resetSflowCollectors(
      const std::shared_ptr<SflowCollectorMap>& collectors);
  void resetQosPolicies(std::shared_ptr<QosPolicyMap> qosPolicyMap);
  void resetControlPlane(std::shared_ptr<ControlPlane> cpu);
  void resetLoadBalancers(std::shared_ptr<LoadBalancerMap> loadBalancers);
  void resetMirrors(std::shared_ptr<MirrorMap> mirrors);
  void resetLabelForwardingInformationBase(
      std::shared_ptr<LabelForwardingInformationBase> labelFib);
  void resetForwardingInformationBases(
      std::shared_ptr<ForwardingInformationBaseMap> fibs);
  void resetSwitchSettings(std::shared_ptr<SwitchSettings> switchSettings);
  void resetQcmCfg(std::shared_ptr<QcmCfg> qcmCfg);
  void resetBufferPoolCfgs(std::shared_ptr<BufferPoolCfgMap> cfgs);
  void addTransceiver(const std::shared_ptr<TransceiverSpec>& transceiver);
  void resetTransceivers(std::shared_ptr<TransceiverMap> transceivers);
  void addSystemPort(const std::shared_ptr<SystemPort>& systemPort);
  void resetSystemPorts(std::shared_ptr<SystemPortMap> systemPorts);
  void addTunnel(const std::shared_ptr<IpTunnel>& tunnel);
  void resetTunnels(std::shared_ptr<IpTunnelMap> tunnels);
  void resetTeFlowTable(std::shared_ptr<TeFlowTable> teFlowTable);
  void resetDsfNodes(std::shared_ptr<DsfNodeMap> dsfNodes);
  std::shared_ptr<AclTableGroupMap>& getAclTablesForStage(
      const folly::dynamic& swJson);
  void resetUdfConfig(std::shared_ptr<UdfConfig> udf);

  void resetRemoteSystemPorts(std::shared_ptr<SystemPortMap> systemPorts);
  void resetRemoteIntfs(std::shared_ptr<InterfaceMap> intfs);

  static std::shared_ptr<SwitchState> fromThrift(
      const state::SwitchState& data);
  state::SwitchState toThrift() const;

  bool operator==(const SwitchState& other) const;
  bool operator!=(const SwitchState& other) const {
    return !(*this == other);
  }

 private:
  bool isLocalSwitchId(SwitchID switchId) const;
  // Inherit the constructor required for clone()
  using BaseT::BaseT;
  friend class CloneAllocator;
};

} // namespace facebook::fboss
