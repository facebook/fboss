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

#include "fboss/agent/HwSwitchMatcher.h"
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
#include "fboss/agent/state/FlowletSwitchingConfig.h"
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
class FlowletSwitchingConfig;

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
RESOLVE_STRUCT_MEMBER(
    SwitchState,
    switch_state_tags::flowletSwitchingConfig,
    FlowletSwitchingConfig);

/* multi npu maps */
RESOLVE_STRUCT_MEMBER(
    SwitchState,
    switch_state_tags::fibsMap,
    MultiSwitchForwardingInformationBaseMap);
RESOLVE_STRUCT_MEMBER(
    SwitchState,
    switch_state_tags::mirrorMaps,
    MultiSwitchMirrorMap);
RESOLVE_STRUCT_MEMBER(
    SwitchState,
    switch_state_tags::sflowCollectorMaps,
    MultiSwitchSflowCollectorMap);
RESOLVE_STRUCT_MEMBER(
    SwitchState,
    switch_state_tags::labelFibMap,
    MultiLabelForwardingInformationBase);
RESOLVE_STRUCT_MEMBER(
    SwitchState,
    switch_state_tags::qosPolicyMaps,
    MultiSwitchQosPolicyMap);
RESOLVE_STRUCT_MEMBER(
    SwitchState,
    switch_state_tags::ipTunnelMaps,
    MultiSwitchIpTunnelMap);
RESOLVE_STRUCT_MEMBER(
    SwitchState,
    switch_state_tags::teFlowTables,
    MultiTeFlowTable);
RESOLVE_STRUCT_MEMBER(
    SwitchState,
    switch_state_tags::aggregatePortMaps,
    MultiSwitchAggregatePortMap);
RESOLVE_STRUCT_MEMBER(
    SwitchState,
    switch_state_tags::loadBalancerMaps,
    MultiSwitchLoadBalancerMap);
RESOLVE_STRUCT_MEMBER(
    SwitchState,
    switch_state_tags::transceiverMaps,
    MultiSwitchTransceiverMap);
RESOLVE_STRUCT_MEMBER(
    SwitchState,
    switch_state_tags::bufferPoolCfgMaps,
    MultiSwitchBufferPoolCfgMap);
RESOLVE_STRUCT_MEMBER(
    SwitchState,
    switch_state_tags::vlanMaps,
    MultiSwitchVlanMap);
RESOLVE_STRUCT_MEMBER(
    SwitchState,
    switch_state_tags::portMaps,
    MultiSwitchPortMap);
RESOLVE_STRUCT_MEMBER(
    SwitchState,
    switch_state_tags::interfaceMaps,
    MultiSwitchInterfaceMap);
RESOLVE_STRUCT_MEMBER(
    SwitchState,
    switch_state_tags::aclTableGroupMaps,
    MultiSwitchAclTableGroupMap);
RESOLVE_STRUCT_MEMBER(
    SwitchState,
    switch_state_tags::dsfNodesMap,
    MultiSwitchDsfNodeMap);
RESOLVE_STRUCT_MEMBER(
    SwitchState,
    switch_state_tags::remoteInterfaceMaps,
    MultiSwitchInterfaceMap);
RESOLVE_STRUCT_MEMBER(
    SwitchState,
    switch_state_tags::remoteSystemPortMaps,
    MultiSwitchSystemPortMap);
RESOLVE_STRUCT_MEMBER(
    SwitchState,
    switch_state_tags::systemPortMaps,
    MultiSwitchSystemPortMap);
RESOLVE_STRUCT_MEMBER(
    SwitchState,
    switch_state_tags::controlPlaneMap,
    MultiControlPlane);
RESOLVE_STRUCT_MEMBER(
    SwitchState,
    switch_state_tags::switchSettingsMap,
    MultiSwitchSettings);
RESOLVE_STRUCT_MEMBER(
    SwitchState,
    switch_state_tags::aclMaps,
    MultiSwitchAclMap);
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
  template <typename T>
  using TypeFor = typename BaseT::Fields::TypeFor<T>;
  using BaseT::modify;
  /*
   * Create a new, empty state.
   */
  SwitchState();
  ~SwitchState() override;

  static std::unique_ptr<SwitchState> uniquePtrFromThrift(
      const state::SwitchState& switchState);

  static void modify(std::shared_ptr<SwitchState>* state);

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

  const std::shared_ptr<MultiSwitchPortMap>& getPorts() const;
  std::shared_ptr<Port> getPort(PortID id) const;

  const std::shared_ptr<MultiSwitchAggregatePortMap>& getAggregatePorts() const;

  const std::shared_ptr<MultiSwitchVlanMap>& getVlans() const;

  VlanID getDefaultVlan() const;

  const std::shared_ptr<QosPolicy> getDefaultDataPlaneQosPolicy() const {
    const auto switchSettings =
        util::getFirstNodeIf(getMultiSwitchSwitchSettings())
        ? util::getFirstNodeIf(getMultiSwitchSwitchSettings())
        : std::make_shared<SwitchSettings>();
    if (switchSettings->getDefaultDataPlaneQosPolicy()) {
      return switchSettings->getDefaultDataPlaneQosPolicy();
    }
    return cref<switch_state_tags::defaultDataPlaneQosPolicy>();
  }

  /*
   * Get switch ID associated with port
   */
  SwitchID getAssociatedSwitchID(PortID portID) const;
  /*
   * Get sys port range associated with this interface. Only applicable
   * for interfaces of type system port
   */
  std::optional<cfg::Range64> getAssociatedSystemPortRangeIf(
      InterfaceID intfID) const;
  /*
   * Get sys port range associated with this port. Only applicable
   * for ports that have intf of type SYS_PORT attached.
   */
  std::optional<cfg::Range64> getAssociatedSystemPortRangeIf(PortID port) const;
  const std::shared_ptr<MultiSwitchInterfaceMap>& getInterfaces() const;
  std::shared_ptr<AclEntry> getAcl(const std::string& name) const;

  const std::shared_ptr<MultiSwitchAclMap>& getAcls() const;

  const std::shared_ptr<MultiSwitchAclTableGroupMap>& getAclTableGroups() const;

  std::chrono::seconds getArpTimeout() const {
    const auto switchSettings =
        util::getFirstNodeIf(getMultiSwitchSwitchSettings())
        ? util::getFirstNodeIf(getMultiSwitchSwitchSettings())
        : std::make_shared<SwitchSettings>();
    auto arpTimeoutSwSettings = switchSettings->getArpTimeout();
    if (arpTimeoutSwSettings.has_value()) {
      return arpTimeoutSwSettings.value();
    }
    auto arpTimeout = cref<switch_state_tags::arpTimeout>()->toThrift();
    return std::chrono::seconds(arpTimeout);
  }

  std::shared_ptr<const AclMap> getAclsForTable(
      cfg::AclStage aclStage,
      const std::string& tableName) const;

  std::shared_ptr<const AclTableMap> getAclTablesForStage(
      cfg::AclStage aclStage) const;

  const std::shared_ptr<MultiSwitchSflowCollectorMap>& getSflowCollectors()
      const;

  const std::shared_ptr<MultiSwitchQosPolicyMap>& getQosPolicies() const;

  const std::shared_ptr<MultiControlPlane>& getControlPlane() const;

  const std::shared_ptr<MultiSwitchSettings>& getMultiSwitchSwitchSettings()
      const;

  const std::shared_ptr<QcmCfg> getQcmCfg() const {
    const auto switchSettings =
        util::getFirstNodeIf(getMultiSwitchSwitchSettings())
        ? util::getFirstNodeIf(getMultiSwitchSwitchSettings())
        : std::make_shared<SwitchSettings>();
    if (auto qcmCfg = switchSettings->getQcmCfg()) {
      return qcmCfg;
    }
    return cref<switch_state_tags::qcmCfg>();
  }

  const std::shared_ptr<MultiSwitchBufferPoolCfgMap> getBufferPoolCfgs() const;

  std::chrono::seconds getNdpTimeout() const {
    const auto switchSettings =
        util::getFirstNodeIf(getMultiSwitchSwitchSettings())
        ? util::getFirstNodeIf(getMultiSwitchSwitchSettings())
        : std::make_shared<SwitchSettings>();
    auto ndpTimeoutSwSettings = switchSettings->getNdpTimeout();
    if (ndpTimeoutSwSettings.has_value()) {
      return ndpTimeoutSwSettings.value();
    }
    return std::chrono::seconds(
        cref<switch_state_tags::ndpTimeout>()->toThrift());
  }

  std::chrono::seconds getArpAgerInterval() const {
    const auto switchSettings =
        util::getFirstNodeIf(getMultiSwitchSwitchSettings())
        ? util::getFirstNodeIf(getMultiSwitchSwitchSettings())
        : std::make_shared<SwitchSettings>();
    auto arpAgeSwSettings = switchSettings->getArpAgerInterval();
    if (arpAgeSwSettings.has_value()) {
      return arpAgeSwSettings.value();
    }
    return std::chrono::seconds(
        cref<switch_state_tags::arpAgerInterval>()->toThrift());
  }

  uint32_t getMaxNeighborProbes() const {
    const auto switchSettings =
        util::getFirstNodeIf(getMultiSwitchSwitchSettings())
        ? util::getFirstNodeIf(getMultiSwitchSwitchSettings())
        : std::make_shared<SwitchSettings>();
    auto maxNeighborProbes = switchSettings->getMaxNeighborProbes();
    if (maxNeighborProbes.has_value()) {
      return maxNeighborProbes.value();
    }
    return cref<switch_state_tags::maxNeighborProbes>()->toThrift();
  }

  std::chrono::seconds getStaleEntryInterval() const {
    const auto switchSettings =
        util::getFirstNodeIf(getMultiSwitchSwitchSettings())
        ? util::getFirstNodeIf(getMultiSwitchSwitchSettings())
        : std::make_shared<SwitchSettings>();
    auto staleEntrySwSettings = switchSettings->getStaleEntryInterval();
    if (staleEntrySwSettings.has_value()) {
      return staleEntrySwSettings.value();
    }
    return std::chrono::seconds(
        cref<switch_state_tags::staleEntryInterval>()->toThrift());
  }

  // dhcp relay packet IP overrides

  folly::IPAddressV4 getDhcpV4RelaySrc() const {
    const auto switchSettings =
        util::getFirstNodeIf(getMultiSwitchSwitchSettings())
        ? util::getFirstNodeIf(getMultiSwitchSwitchSettings())
        : std::make_shared<SwitchSettings>();
    auto dhcpV4RelaySrc = switchSettings->getDhcpV4RelaySrc();
    if (dhcpV4RelaySrc.has_value()) {
      return dhcpV4RelaySrc.value();
    }
    return network::toIPAddress(
               cref<switch_state_tags::dhcpV4RelaySrc>()->toThrift())
        .asV4();
  }

  folly::IPAddressV6 getDhcpV6RelaySrc() const {
    const auto switchSettings =
        util::getFirstNodeIf(getMultiSwitchSwitchSettings())
        ? util::getFirstNodeIf(getMultiSwitchSwitchSettings())
        : std::make_shared<SwitchSettings>();
    auto dhcpV6RelaySrc = switchSettings->getDhcpV6RelaySrc();
    if (dhcpV6RelaySrc.has_value()) {
      return dhcpV6RelaySrc.value();
    }
    return network::toIPAddress(
               cref<switch_state_tags::dhcpV6RelaySrc>()->toThrift())
        .asV6();
  }

  folly::IPAddressV4 getDhcpV4ReplySrc() const {
    const auto switchSettings =
        util::getFirstNodeIf(getMultiSwitchSwitchSettings())
        ? util::getFirstNodeIf(getMultiSwitchSwitchSettings())
        : std::make_shared<SwitchSettings>();
    auto dhcpV4ReplySrc = switchSettings->getDhcpV4ReplySrc();
    if (dhcpV4ReplySrc.has_value()) {
      return dhcpV4ReplySrc.value();
    }
    return network::toIPAddress(
               cref<switch_state_tags::dhcpV4ReplySrc>()->toThrift())
        .asV4();
  }

  folly::IPAddressV6 getDhcpV6ReplySrc() const {
    const auto switchSettings =
        util::getFirstNodeIf(getMultiSwitchSwitchSettings())
        ? util::getFirstNodeIf(getMultiSwitchSwitchSettings())
        : std::make_shared<SwitchSettings>();
    auto dhcpV6ReplySrc = switchSettings->getDhcpV6ReplySrc();
    if (dhcpV6ReplySrc.has_value()) {
      return dhcpV6ReplySrc.value();
    }
    return network::toIPAddress(
               cref<switch_state_tags::dhcpV6ReplySrc>()->toThrift())
        .asV6();
  }

  // THRIFT_COPY
  std::optional<cfg::PfcWatchdogRecoveryAction> getPfcWatchdogRecoveryAction()
      const;

  const std::shared_ptr<MultiSwitchLoadBalancerMap>& getLoadBalancers() const;
  const std::shared_ptr<MultiTeFlowTable>& getTeFlowTable() const;
  const std::shared_ptr<MultiSwitchMirrorMap>& getMirrors() const;
  const std::shared_ptr<MultiSwitchForwardingInformationBaseMap>& getFibs()
      const;
  const std::shared_ptr<MultiLabelForwardingInformationBase>&
  getLabelForwardingInformationBase() const;

  const std::shared_ptr<MultiSwitchTransceiverMap>& getTransceivers() const;
  const std::shared_ptr<MultiSwitchSystemPortMap>& getSystemPorts() const;
  const std::shared_ptr<MultiSwitchIpTunnelMap>& getTunnels() const;

  const std::shared_ptr<MultiSwitchDsfNodeMap>& getDsfNodes() const;

  const std::shared_ptr<UdfConfig>& getUdfConfig() const {
    const auto switchSettings =
        util::getFirstNodeIf(getMultiSwitchSwitchSettings())
        ? util::getFirstNodeIf(getMultiSwitchSwitchSettings())
        : std::make_shared<SwitchSettings>();
    if (switchSettings->getUdfConfig()) {
      return switchSettings->getUdfConfig();
    }
    return cref<switch_state_tags::udfConfig>();
  }

  const std::shared_ptr<FlowletSwitchingConfig>& getFlowletSwitchingConfig()
      const {
    const auto switchSettings =
        util::getFirstNodeIf(getMultiSwitchSwitchSettings())
        ? util::getFirstNodeIf(getMultiSwitchSwitchSettings())
        : std::make_shared<SwitchSettings>();
    if (switchSettings->getFlowletSwitchingConfig()) {
      return switchSettings->getFlowletSwitchingConfig();
    }
    return cref<switch_state_tags::flowletSwitchingConfig>();
  }

  /*
   * Remote objects
   */
  const std::shared_ptr<MultiSwitchSystemPortMap>& getRemoteSystemPorts() const;
  const std::shared_ptr<MultiSwitchInterfaceMap>& getRemoteInterfaces() const;

  /*
   * Get system ports for a given switch id
   */
  std::shared_ptr<SystemPortMap> getSystemPorts(SwitchID switchId) const;
  /*
   * Get interfaces for a given switch id
   */
  std::shared_ptr<InterfaceMap> getInterfaces(SwitchID switchId) const;

  InterfaceID getInterfaceIDForPort(PortID portId) const;
  /*
   * The following functions modify the static state.
   * The should only be called on newly created SwitchState objects that are
   * only visible to a single thread, before they are published as the current
   * state.
   */

  void resetMirrors(const std::shared_ptr<MultiSwitchMirrorMap>& mirrors);
  void resetPorts(std::shared_ptr<MultiSwitchPortMap> ports);
  void resetAggregatePorts(
      std::shared_ptr<MultiSwitchAggregatePortMap> aggPorts);
  void resetVlans(std::shared_ptr<MultiSwitchVlanMap> vlans);
  void resetIntfs(const std::shared_ptr<MultiSwitchInterfaceMap>& intfs);
  void addAclTable(const std::shared_ptr<AclTable>& aclTable);
  void resetAcls(const std::shared_ptr<MultiSwitchAclMap>& acls);
  void resetAclTableGroups(
      std::shared_ptr<MultiSwitchAclTableGroupMap> multiAclTableGroups);
  void resetSflowCollectors(
      const std::shared_ptr<MultiSwitchSflowCollectorMap>& collectors);
  void resetQosPolicies(const std::shared_ptr<QosPolicyMap>& qosPolicyMap);
  void resetQosPolicies(
      const std::shared_ptr<MultiSwitchQosPolicyMap>& qosPolicyMap);
  void resetControlPlane(std::shared_ptr<MultiControlPlane> cpu);
  void resetLoadBalancers(
      std::shared_ptr<MultiSwitchLoadBalancerMap> loadBalancers);
  void resetLabelForwardingInformationBase(
      std::shared_ptr<MultiLabelForwardingInformationBase> labelFib);
  void resetForwardingInformationBases(
      std::shared_ptr<MultiSwitchForwardingInformationBaseMap> fibs);
  void resetSwitchSettings(std::shared_ptr<MultiSwitchSettings> switchSettings);
  void resetBufferPoolCfgs(std::shared_ptr<MultiSwitchBufferPoolCfgMap> cfgs);
  void resetTransceivers(
      std::shared_ptr<MultiSwitchTransceiverMap> transceivers);
  void resetSystemPorts(
      const std::shared_ptr<MultiSwitchSystemPortMap>& systemPorts);

  void resetTunnels(std::shared_ptr<MultiSwitchIpTunnelMap> tunnels);

  void resetTeFlowTable(std::shared_ptr<MultiTeFlowTable> teFlowTable);
  void resetDsfNodes(const std::shared_ptr<MultiSwitchDsfNodeMap>& dsfNodes);
  std::shared_ptr<AclTableGroupMap>& getAclTablesForStage(
      const folly::dynamic& swJson);

  static std::shared_ptr<SwitchState> fromThrift(
      const state::SwitchState& data);
  state::SwitchState toThrift() const;

  bool operator==(const SwitchState& other) const;
  bool operator!=(const SwitchState& other) const {
    return !(*this == other);
  }

  template <typename Tag, typename Type = typename TypeFor<Tag>::element_type>
  static Type* modify(std::shared_ptr<SwitchState>* state);

 private:
  void resetRemoteIntfs(const std::shared_ptr<MultiSwitchInterfaceMap>& intfs);

  template <
      typename MultiMapType,
      typename ThriftType = typename MultiMapType::Node::ThriftType>
  std::optional<ThriftType> toThrift(
      const std::shared_ptr<MultiMapType>& multiMap) const;

  template <typename MultiMapName, typename MapName>
  void fromThrift(bool emptyMnpuMapOk = false);

  template <
      typename MultiMapName,
      typename Map = typename InnerMap<MultiMapName, TypeFor>::type>
  void resetDefaultMap(std::shared_ptr<Map> map);
  template <
      typename MultiMapName,
      typename Map = typename InnerMap<MultiMapName, TypeFor>::type>
  void resetMap(
      const std::shared_ptr<Map>& map,
      const HwSwitchMatcher& matcher);

  template <
      typename MultiMapName,
      typename Map = typename InnerMap<MultiMapName, TypeFor>::type>
  const std::shared_ptr<Map>& getDefaultMap() const;
  template <
      typename MultiMapName,
      typename Map = typename InnerMap<MultiMapName, TypeFor>::type>
  std::shared_ptr<Map>& getDefaultMap();

  template <
      typename MultiMapName,
      typename Map = typename InnerMap<MultiMapName, TypeFor>::type>
  const std::shared_ptr<Map>& getMap(const HwSwitchMatcher& matcher) const;

  template <
      typename MultiMapName,
      typename Map = typename InnerMap<MultiMapName, TypeFor>::type>
  std::shared_ptr<Map>& getMap(const HwSwitchMatcher& matcher);
  bool isLocalSwitchId(SwitchID switchId) const;
  // Inherit the constructor required for clone()
  using BaseT::BaseT;
  friend class CloneAllocator;
};

} // namespace facebook::fboss
