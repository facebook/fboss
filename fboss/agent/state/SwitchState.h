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

#include "fboss/agent/state/AclMap.h"
#include "fboss/agent/state/AggregatePortMap.h"
#include "fboss/agent/state/ControlPlane.h"
#include "fboss/agent/state/ForwardingInformationBaseMap.h"
#include "fboss/agent/state/InterfaceMap.h"
#include "fboss/agent/state/LabelForwardingInformationBase.h"
#include "fboss/agent/state/LoadBalancerMap.h"
#include "fboss/agent/state/MirrorMap.h"
#include "fboss/agent/state/NodeBase.h"
#include "fboss/agent/state/PortMap.h"
#include "fboss/agent/state/QcmConfig.h"
#include "fboss/agent/state/QosPolicyMap.h"
#include "fboss/agent/state/RouteTableMap.h"
#include "fboss/agent/state/SflowCollectorMap.h"
#include "fboss/agent/state/SwitchSettings.h"
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

struct SwitchStateFields {
  SwitchStateFields();

  template <typename Fn>
  void forEachChild(Fn fn) {
    fn(ports.get());
    fn(aggPorts.get());
    fn(vlans.get());
    fn(interfaces.get());
    fn(routeTables.get());
    fn(acls.get());
    fn(sFlowCollectors.get());
    fn(qosPolicies.get());
    fn(controlPlane.get());
    fn(loadBalancers.get());
    fn(mirrors.get());
    fn(fibs.get());
    fn(labelFib.get());
    fn(switchSettings.get());
  }
  /*
   * Serialize to folly::dynamic
   */
  folly::dynamic toFollyDynamic() const;
  /*
   * Reconstruct object from folly::dynamic
   */
  static SwitchStateFields fromFollyDynamic(const folly::dynamic& json);
  // Static state, which can be accessed without locking.
  std::shared_ptr<PortMap> ports;
  std::shared_ptr<AggregatePortMap> aggPorts;
  std::shared_ptr<VlanMap> vlans;
  std::shared_ptr<InterfaceMap> interfaces;
  std::shared_ptr<RouteTableMap> routeTables;
  std::shared_ptr<AclMap> acls;
  std::shared_ptr<SflowCollectorMap> sFlowCollectors;
  std::shared_ptr<QosPolicyMap> qosPolicies;
  std::shared_ptr<ControlPlane> controlPlane;
  std::shared_ptr<LoadBalancerMap> loadBalancers;
  std::shared_ptr<MirrorMap> mirrors;
  std::shared_ptr<ForwardingInformationBaseMap> fibs;
  std::shared_ptr<LabelForwardingInformationBase> labelFib;
  std::shared_ptr<SwitchSettings> switchSettings;
  std::shared_ptr<QcmCfg> qcmCfg;

  VlanID defaultVlan{0};

  std::shared_ptr<QosPolicy> defaultDataPlaneQosPolicy;

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

  static std::shared_ptr<SwitchState> fromFollyDynamic(
      const folly::dynamic& json) {
    const auto& fields = SwitchStateFields::fromFollyDynamic(json);
    return std::make_shared<SwitchState>(fields);
  }

  static std::shared_ptr<SwitchState> fromJson(const folly::fbstring& jsonStr) {
    return fromFollyDynamic(folly::parseJson(jsonStr));
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
  const std::shared_ptr<RouteTableMap>& getRouteTables() const {
    return getFields()->routeTables;
  }

  std::shared_ptr<AclEntry> getAcl(const std::string& name) const;

  const std::shared_ptr<AclMap>& getAcls() const {
    return getFields()->acls;
  }

  std::chrono::seconds getArpTimeout() const {
    return getFields()->arpTimeout;
  }

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

  const std::shared_ptr<LoadBalancerMap>& getLoadBalancers() const;
  const std::shared_ptr<MirrorMap>& getMirrors() const;
  const std::shared_ptr<ForwardingInformationBaseMap>& getFibs() const;
  const std::shared_ptr<LabelForwardingInformationBase>&
  getLabelForwardingInformationBase() const {
    return getFields()->labelFib;
  }

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
  void addRouteTable(const std::shared_ptr<RouteTable>& rt);
  void resetRouteTables(std::shared_ptr<RouteTableMap> rts);
  void addAcl(const std::shared_ptr<AclEntry>& acl);
  void resetAcls(std::shared_ptr<AclMap> acls);
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

  void publish() override {
    using BaseT = NodeBaseT<SwitchState, SwitchStateFields>;
    if (auto defaultDataPlaneQosPolicy = getDefaultDataPlaneQosPolicy()) {
      defaultDataPlaneQosPolicy->publish();
    }
    if (auto qcmCfg = getQcmCfg()) {
      qcmCfg->publish();
    }
    BaseT::publish();
  }

 private:
  // Inherit the constructor required for clone()
  using NodeBaseT::NodeBaseT;
  friend class CloneAllocator;
};

} // namespace facebook::fboss
