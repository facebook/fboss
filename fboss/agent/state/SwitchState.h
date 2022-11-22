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
#include "fboss/agent/state/VlanMap.h"
#include "fboss/agent/types.h"

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
class SwitchState : public NodeBaseT<SwitchState, SwitchStateFields> {
 public:
  /*
   * Create a new, empty state.
   */
  SwitchState();
  ~SwitchState() override;

  state::SwitchState toThrift() const {
    return this->getFields()->toThrift();
  }

  static std::shared_ptr<SwitchState> fromThrift(
      const state::SwitchState& obj) {
    auto fields = SwitchStateFields::fromThrift(obj);
    return std::make_shared<SwitchState>(fields);
  }

  static std::shared_ptr<SwitchState> fromFollyDynamic(
      const folly::dynamic& json) {
    const auto& fields = SwitchStateFields::fromFollyDynamic(json);
    return std::make_shared<SwitchState>(fields);
  }

  static std::shared_ptr<SwitchState> fromJson(const folly::fbstring& jsonStr) {
    return fromFollyDynamic(folly::parseJson(jsonStr));
  }

  static std::unique_ptr<SwitchState> uniquePtrFromThrift(
      const state::SwitchState& switchState) {
    const auto& fields = SwitchStateFields::fromThrift(switchState);
    return std::make_unique<SwitchState>(fields);
  }

  static std::unique_ptr<SwitchState> uniquePtrFromFollyDynamic(
      const folly::dynamic& json) {
    const auto& fields = SwitchStateFields::fromFollyDynamic(json);
    return std::make_unique<SwitchState>(fields);
  }

  static std::unique_ptr<SwitchState> uniquePtrFromJson(
      const folly::fbstring& jsonStr) {
    return uniquePtrFromFollyDynamic(folly::parseJson(jsonStr));
  }

  folly::dynamic toFollyDynamic() const override {
    return getFields()->toFollyDynamic();
  }

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

  bool operator==(const SwitchState& other) const {
    return *getFields() == *other.getFields();
  }

  const std::shared_ptr<PortMap>& getPorts() const {
    return getFields()->ports;
  }
  std::shared_ptr<Port> getPort(PortID id) const;

  const std::shared_ptr<AggregatePortMap> getAggregatePorts() const {
    return getFields()->aggPorts;
  }

  const std::shared_ptr<VlanMap>& getVlans() const {
    return getFields()->vlans;
  }

  VlanID getDefaultVlan() const {
    return getFields()->defaultVlan;
  }
  void setDefaultVlan(VlanID id);

  const std::shared_ptr<QosPolicy> getDefaultDataPlaneQosPolicy() const {
    return getFields()->defaultDataPlaneQosPolicy;
  }

  void setDefaultDataPlaneQosPolicy(std::shared_ptr<QosPolicy> qosPolicy) {
    writableFields()->defaultDataPlaneQosPolicy = std::move(qosPolicy);
  }

  const std::shared_ptr<InterfaceMap>& getInterfaces() const {
    return getFields()->interfaces;
  }

  std::shared_ptr<AclEntry> getAcl(const std::string& name) const;

  const std::shared_ptr<AclMap>& getAcls() const {
    return getFields()->acls;
  }

  const std::shared_ptr<AclTableGroupMap>& getAclTableGroups() const {
    return getFields()->aclTableGroups;
  }

  std::chrono::seconds getArpTimeout() const {
    return getFields()->arpTimeout;
  }

  std::shared_ptr<const AclMap> getAclsForTable(
      cfg::AclStage aclStage,
      const std::string& tableName) const;

  std::shared_ptr<const AclTableMap> getAclTablesForStage(
      cfg::AclStage aclStage) const;

  const std::shared_ptr<SflowCollectorMap>& getSflowCollectors() const {
    return getFields()->sFlowCollectors;
  }

  std::shared_ptr<QosPolicy> getQosPolicy(const std::string& name) const {
    return getFields()->qosPolicies->getQosPolicyIf(name);
  }

  const std::shared_ptr<QosPolicyMap>& getQosPolicies() const {
    return getFields()->qosPolicies;
  }

  const std::shared_ptr<ControlPlane>& getControlPlane() const {
    return getFields()->controlPlane;
  }

  const std::shared_ptr<SwitchSettings>& getSwitchSettings() const {
    return getFields()->switchSettings;
  }

  const std::shared_ptr<QcmCfg> getQcmCfg() const {
    return getFields()->qcmCfg;
  }

  const std::shared_ptr<BufferPoolCfgMap> getBufferPoolCfgs() const {
    return getFields()->bufferPoolCfgs;
  }

  void setArpTimeout(std::chrono::seconds timeout);

  std::chrono::seconds getNdpTimeout() const {
    return getFields()->ndpTimeout;
  }

  void setNdpTimeout(std::chrono::seconds timeout);

  std::chrono::seconds getArpAgerInterval() const {
    return getFields()->arpAgerInterval;
  }

  void setArpAgerInterval(std::chrono::seconds interval);

  uint32_t getMaxNeighborProbes() const {
    return getFields()->maxNeighborProbes;
  }
  void setMaxNeighborProbes(uint32_t maxNeighborProbes);

  std::chrono::seconds getStaleEntryInterval() const {
    return getFields()->staleEntryInterval;
  }

  void setStaleEntryInterval(std::chrono::seconds interval);

  // dhcp relay packet IP overrides

  folly::IPAddressV4 getDhcpV4RelaySrc() const {
    return getFields()->dhcpV4RelaySrc;
  }
  void setDhcpV4RelaySrc(folly::IPAddressV4 v4RelaySrc) {
    writableFields()->dhcpV4RelaySrc = v4RelaySrc;
  }

  folly::IPAddressV6 getDhcpV6RelaySrc() const {
    return getFields()->dhcpV6RelaySrc;
  }
  void setDhcpV6RelaySrc(folly::IPAddressV6 v6RelaySrc) {
    writableFields()->dhcpV6RelaySrc = v6RelaySrc;
  }

  folly::IPAddressV4 getDhcpV4ReplySrc() const {
    return getFields()->dhcpV4ReplySrc;
  }
  void setDhcpV4ReplySrc(folly::IPAddressV4 v4ReplySrc) {
    writableFields()->dhcpV4ReplySrc = v4ReplySrc;
  }

  folly::IPAddressV6 getDhcpV6ReplySrc() const {
    return getFields()->dhcpV6ReplySrc;
  }
  void setDhcpV6ReplySrc(folly::IPAddressV6 v6ReplySrc) {
    writableFields()->dhcpV6ReplySrc = v6ReplySrc;
  }

  std::optional<cfg::PfcWatchdogRecoveryAction> getPfcWatchdogRecoveryAction()
      const {
    return getFields()->pfcWatchdogRecoveryAction;
  }

  void setPfcWatchdogRecoveryAction(
      std::optional<cfg::PfcWatchdogRecoveryAction> recoveryAction) {
    writableFields()->pfcWatchdogRecoveryAction = recoveryAction;
  }

  const std::shared_ptr<LoadBalancerMap>& getLoadBalancers() const;
  const std::shared_ptr<MirrorMap>& getMirrors() const;
  const std::shared_ptr<ForwardingInformationBaseMap>& getFibs() const;
  const std::shared_ptr<LabelForwardingInformationBase>&
  getLabelForwardingInformationBase() const {
    return getFields()->labelFib;
  }

  const std::shared_ptr<TransceiverMap>& getTransceivers() const {
    return getFields()->transceivers;
  }
  const std::shared_ptr<SystemPortMap>& getSystemPorts() const {
    return getFields()->systemPorts;
  }
  const std::shared_ptr<IpTunnelMap>& getTunnels() const {
    return getFields()->ipTunnels;
  }
  const std::shared_ptr<TeFlowTable>& getTeFlowTable() const {
    return getFields()->teFlowTable;
  }

  const std::shared_ptr<DsfNodeMap>& getDsfNodes() const {
    return getFields()->dsfNodes;
  }

  const std::shared_ptr<UdfConfig>& getUdfConfig() const {
    return getFields()->udfConfig;
  }

  /*
   * Remote objects
   */
  const std::shared_ptr<SystemPortMap>& getRemoteSystemPorts() const {
    return getFields()->remoteSystemPorts;
  }
  const std::shared_ptr<InterfaceMap>& getRemoteInterfaces() const {
    return getFields()->remoteInterfaces;
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
  void resetUdfConfig(std::shared_ptr<UdfConfig> udf);

  void resetRemoteSystemPorts(std::shared_ptr<SystemPortMap> systemPorts);
  void resetRemoteIntfs(std::shared_ptr<InterfaceMap> intfs);
  void publish() override {
    using BaseT = NodeBaseT<SwitchState, SwitchStateFields>;
    if (auto defaultDataPlaneQosPolicy = getDefaultDataPlaneQosPolicy()) {
      defaultDataPlaneQosPolicy->publish();
    }
    if (auto qcmCfg = getQcmCfg()) {
      qcmCfg->publish();
    }
    if (auto bufferPoolCfg = getBufferPoolCfgs()) {
      bufferPoolCfg->publish();
    }
    if (auto udfCfg = getUdfConfig()) {
      udfCfg->publish();
    }
    BaseT::publish();
  }

 private:
  bool isLocalSwitchId(SwitchID switchId) const;
  // Inherit the constructor required for clone()
  using NodeBaseT::NodeBaseT;
  friend class CloneAllocator;
};

} // namespace facebook::fboss
