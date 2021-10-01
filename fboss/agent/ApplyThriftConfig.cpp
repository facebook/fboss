/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/ApplyThriftConfig.h"

#include <folly/FileUtil.h>
#include <folly/gen/Base.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>
#include <memory>
#include <optional>
#include <string>

#include "fboss/agent/FbossError.h"
#include "fboss/agent/LacpTypes.h"
#include "fboss/agent/LoadBalancerConfigApplier.h"
#include "fboss/agent/Platform.h"
#include "fboss/agent/RouteUpdateWrapper.h"
#include "fboss/agent/if/gen-cpp2/mpls_types.h"
#include "fboss/agent/normalization/Normalizer.h"
#include "fboss/agent/rib/RoutingInformationBase.h"
#include "fboss/agent/state/AclEntry.h"
#include "fboss/agent/state/AclMap.h"
#include "fboss/agent/state/AclTable.h"
#include "fboss/agent/state/AclTableGroup.h"
#include "fboss/agent/state/AclTableMap.h"
#include "fboss/agent/state/AggregatePort.h"
#include "fboss/agent/state/AggregatePortMap.h"
#include "fboss/agent/state/ArpResponseTable.h"
#include "fboss/agent/state/BufferPoolConfig.h"
#include "fboss/agent/state/BufferPoolConfigMap.h"
#include "fboss/agent/state/ControlPlane.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/InterfaceMap.h"
#include "fboss/agent/state/LabelForwardingInformationBase.h"
#include "fboss/agent/state/Mirror.h"
#include "fboss/agent/state/NdpResponseTable.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/PortMap.h"
#include "fboss/agent/state/PortPgConfig.h"
#include "fboss/agent/state/PortQueue.h"
#include "fboss/agent/state/QcmConfig.h"
#include "fboss/agent/state/QosPolicyMap.h"
#include "fboss/agent/state/Route.h"
#include "fboss/agent/state/RouteTypes.h"
#include "fboss/agent/state/SflowCollector.h"
#include "fboss/agent/state/SflowCollectorMap.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/Vlan.h"
#include "fboss/agent/state/VlanMap.h"

#include <boost/container/flat_map.hpp>
#include <boost/container/flat_set.hpp>
#include <folly/Range.h>
#include <algorithm>
#include <cmath>
#include <utility>
#include <vector>

#include "fboss/agent/rib/ForwardingInformationBaseUpdater.h"
#include "fboss/agent/rib/NetworkToRouteMap.h"

#include "fboss/agent/hw/switch_asics/HwAsic.h"

using boost::container::flat_map;
using boost::container::flat_set;
using folly::CIDRNetwork;
using folly::IPAddress;
using folly::IPAddressFormatException;
using folly::IPAddressV4;
using folly::IPAddressV6;
using folly::MacAddress;
using folly::StringPiece;
using std::make_shared;
using std::shared_ptr;

DEFINE_bool(
    enable_acl_table_group,
    false,
    "Allow multiple acl tables (acl table group)");

namespace {

const uint8_t kV6LinkLocalAddrMask{64};
// Needed until CoPP is removed from code and put into config
const int kAclStartPriority = 100000;

// Only one buffer pool is supported systemwide. Variable to track the name
// and validate during a config change.
std::optional<std::string> sharedBufferPoolName;

std::shared_ptr<facebook::fboss::SwitchState> updateFibFromConfig(
    facebook::fboss::RouterID vrf,
    const facebook::fboss::IPv4NetworkToRouteMap& v4NetworkToRoute,
    const facebook::fboss::IPv6NetworkToRouteMap& v6NetworkToRoute,
    void* cookie) {
  facebook::fboss::ForwardingInformationBaseUpdater fibUpdater(
      vrf, v4NetworkToRoute, v6NetworkToRoute);

  auto nextStatePtr =
      static_cast<std::shared_ptr<facebook::fboss::SwitchState>*>(cookie);

  fibUpdater(*nextStatePtr);
  return *nextStatePtr;
}

} // anonymous namespace

namespace facebook::fboss {

/*
 * A class for implementing applyThriftConfig().
 *
 * This implements a procedural function.  It is defined as a class purely as a
 * convenience for the implementation, to allow easily sharing state between
 * internal helper methods.
 */
class ThriftConfigApplier {
 public:
  ThriftConfigApplier(
      const std::shared_ptr<SwitchState>& orig,
      const cfg::SwitchConfig* config,
      const Platform* platform,
      RoutingInformationBase* rib)
      : orig_(orig), cfg_(config), platform_(platform), rib_(rib) {}
  ThriftConfigApplier(
      const std::shared_ptr<SwitchState>& orig,
      const cfg::SwitchConfig* config,
      const Platform* platform,
      RouteUpdateWrapper* routeUpdater)
      : orig_(orig),
        cfg_(config),
        platform_(platform),
        routeUpdater_(routeUpdater) {}

  std::shared_ptr<SwitchState> run();

 private:
  // Forbidden copy constructor and assignment operator
  ThriftConfigApplier(ThriftConfigApplier const&) = delete;
  ThriftConfigApplier& operator=(ThriftConfigApplier const&) = delete;

  template <typename Node, typename NodeMap>
  bool updateMap(
      NodeMap* map,
      std::shared_ptr<Node> origNode,
      std::shared_ptr<Node> newNode) {
    if (newNode) {
      auto ret = map->emplace(std::make_pair(newNode->getID(), newNode));
      if (!ret.second) {
        throw FbossError("duplicate entry ", newNode->getID());
      }
      return true;
    } else {
      auto ret = map->emplace(std::make_pair(origNode->getID(), origNode));
      if (!ret.second) {
        throw FbossError("duplicate entry ", origNode->getID());
      }
      return false;
    }
  }

  // Interface route prefix. IPAddress has mask applied
  typedef std::pair<InterfaceID, folly::IPAddress> IntfAddress;
  typedef boost::container::flat_map<folly::CIDRNetwork, IntfAddress> IntfRoute;
  typedef boost::container::flat_map<RouterID, IntfRoute> IntfRouteTable;
  IntfRouteTable intfRouteTables_;

  /* The ThriftConfigApplier object exposes a single, top-level method "run()".
   * In this method, a previous SwitchState "orig_" is first cloned and the
   * clone modified until it matches the specifications of the SwitchConfig
   * "cfg_". The private methods of ThriftConfigApplier implement the logic
   * necessary to perform these modifications.
   *
   * These methods generally follow a common scheme to do so based on each
   * SwitchState node being uniquely identified by an ID within the set of nodes
   * of the same type. For instance, a VLAN node is uniquely identified by
   * its "const VlanID id" member variable. No other VLAN may have the same
   * ID. But it is entirely possible for there to exist an Interface node with
   * the same numerical ID (ignoring type incompatibility between VlanID and
   * InterfaceID).
   *
   * There are 3 cases to consider:
   *
   * 1) cfg_ and orig_ both have a node with the same ID
   *    If the specifications in cfg_ differ from those of orig_, then the
   *    clone of the node is updated appropriately. This functionality is
   *    provided by methods such as updateAggPort(), updateVlan(), etc.
   * 2) cfg_ has a node with an ID that does not exist in orig_
   *    A node with this ID is added to the cloned SwitchState. This
   *    functionality is provided by methods such as createAggPort(),
   *    createVlan(), etc.
   * 3) orig_ has a node with an ID that does not exist in cfg_
   *    This node is implicity deleted in the clone.
   *
   * Methods such as updateAggregatePorts(), updateVlans(), etc. encapsulate
   * this logic for each type of NodeBase.
   */

  void processVlanPorts();
  void updateVlanInterfaces(const Interface* intf);
  std::shared_ptr<PortMap> updatePorts();
  std::shared_ptr<Port> updatePort(
      const std::shared_ptr<Port>& orig,
      const cfg::Port* cfg);
  QueueConfig updatePortQueues(
      const std::vector<std::shared_ptr<PortQueue>>& origPortQueues,
      const std::vector<cfg::PortQueue>& cfgPortQueues,
      uint16_t maxQueues,
      cfg::StreamType streamType,
      std::optional<cfg::QosMap> qosMap = std::nullopt);
  // update cfg port queue attribute to state port queue object
  void setPortQueue(
      std::shared_ptr<PortQueue> newQueue,
      const cfg::PortQueue* cfg);
  std::optional<std::vector<PfcPriority>> findEnabledPfcPriorities(
      PortPgConfigs& portPgConfigs);
  std::shared_ptr<PortQueue> updatePortQueue(
      const std::shared_ptr<PortQueue>& orig,
      const cfg::PortQueue* cfg,
      std::optional<TrafficClass> trafficClass,
      std::optional<std::set<PfcPriority>> pfcPriority);
  std::shared_ptr<PortQueue> createPortQueue(
      const cfg::PortQueue* cfg,
      std::optional<TrafficClass> trafficClass,
      std::optional<std::set<PfcPriority>> pfcPriority);
  // pg specific routines
  bool isPgConfigUnchanged(
      std::optional<PortPgConfigs> newPortPgCfgs,
      const shared_ptr<Port>& orig);
  void validateUpdatePgBufferPoolName(
      const PortPgConfigs& portPgCfgs,
      const shared_ptr<Port>& port,
      const std::string& portPgName);
  std::shared_ptr<PortPgConfig> createPortPg(const cfg::PortPgConfig* cfg);
  PortPgConfigs updatePortPgConfigs(
      const std::vector<cfg::PortPgConfig>& newPortPgConfig,
      const shared_ptr<Port>& orig);
  void checkPortQueueAQMValid(
      const std::vector<cfg::ActiveQueueManagement>& aqms);
  std::shared_ptr<AggregatePortMap> updateAggregatePorts();
  std::shared_ptr<AggregatePort> updateAggPort(
      const std::shared_ptr<AggregatePort>& orig,
      const cfg::AggregatePort& cfg);
  std::shared_ptr<AggregatePort> createAggPort(const cfg::AggregatePort& cfg);
  std::vector<AggregatePort::Subport> getSubportsSorted(
      const cfg::AggregatePort& cfg);
  std::pair<folly::MacAddress, uint16_t> getSystemLacpConfig();
  uint8_t computeMinimumLinkCount(const cfg::AggregatePort& cfg);
  std::shared_ptr<VlanMap> updateVlans();
  std::shared_ptr<Vlan> createVlan(const cfg::Vlan* config);
  std::shared_ptr<Vlan> updateVlan(
      const std::shared_ptr<Vlan>& orig,
      const cfg::Vlan* config);
  std::shared_ptr<AclTableGroup> updateAclTableGroup(
      cfg::AclStage aclStage,
      const std::shared_ptr<AclTableGroup>& origAclTableGroup);
  std::shared_ptr<AclTableGroupMap> updateAclTableGroups();
  flat_map<std::string, const cfg::AclEntry*> getAllAclsByName();
  void checkTrafficPolicyAclsExistInConfig(
      const cfg::TrafficPolicyConfig& policy,
      flat_map<std::string, const cfg::AclEntry*> aclByName);
  std::shared_ptr<AclTable> updateAclTable(
      cfg::AclStage aclStage,
      const cfg::AclTable& configTable,
      int* numExistingTablesProcessed);
  std::shared_ptr<AclMap> updateAcls(
      cfg::AclStage aclStage,
      std::vector<cfg::AclEntry> configEntries,
      std::optional<std::string> tableName = std::nullopt);
  std::shared_ptr<AclEntry> createAcl(
      const cfg::AclEntry* config,
      int priority,
      const MatchAction* action = nullptr);
  std::shared_ptr<AclEntry> updateAcl(
      cfg::AclStage aclStage,
      const cfg::AclEntry& acl,
      int priority,
      int* numExistingProcessed,
      bool* changed,
      std::optional<std::string> tableName,
      const MatchAction* action = nullptr);
  // check the acl provided by config is valid
  void checkAcl(const cfg::AclEntry* config) const;
  std::shared_ptr<QosPolicyMap> updateQosPolicies();
  std::shared_ptr<QosPolicy> updateQosPolicy(
      cfg::QosPolicy& qosPolicy,
      int* numExistingProcessed,
      bool* changed);
  std::optional<std::string> getDefaultDataPlaneQosPolicyName() const;
  std::shared_ptr<QosPolicy> updateDataplaneDefaultQosPolicy();
  shared_ptr<QosPolicy> createQosPolicy(const cfg::QosPolicy& qosPolicy);
  bool updateNeighborResponseTables(Vlan* vlan, const cfg::Vlan* config);
  bool updateDhcpOverrides(Vlan* vlan, const cfg::Vlan* config);
  std::shared_ptr<InterfaceMap> updateInterfaces();
  shared_ptr<Interface> createInterface(
      const cfg::Interface* config,
      const Interface::Addresses& addrs);
  shared_ptr<Interface> updateInterface(
      const shared_ptr<Interface>& orig,
      const cfg::Interface* config,
      const Interface::Addresses& addrs);
  std::string getInterfaceName(const cfg::Interface* config);
  folly::MacAddress getInterfaceMac(const cfg::Interface* config);
  Interface::Addresses getInterfaceAddresses(const cfg::Interface* config);
  shared_ptr<SflowCollectorMap> updateSflowCollectors();
  shared_ptr<SflowCollector> createSflowCollector(
      const cfg::SflowCollector* config);
  shared_ptr<SflowCollector> updateSflowCollector(
      const shared_ptr<SflowCollector>& orig,
      const cfg::SflowCollector* config);
  shared_ptr<SwitchSettings> updateSwitchSettings();
  // bufferPool specific configs
  shared_ptr<BufferPoolCfgMap> updateBufferPoolConfigs(bool* changed);
  std::shared_ptr<BufferPoolCfg> createBufferPoolConfig(
      const std::string& id,
      const cfg::BufferPoolConfig& config);
  shared_ptr<QcmCfg> updateQcmCfg(bool* changed);
  shared_ptr<QcmCfg> createQcmCfg(const cfg::QcmConfig& config);
  shared_ptr<ControlPlane> updateControlPlane();
  std::shared_ptr<MirrorMap> updateMirrors();
  std::shared_ptr<Mirror> createMirror(const cfg::Mirror* config);
  std::shared_ptr<Mirror> updateMirror(
      const std::shared_ptr<Mirror>& orig,
      const cfg::Mirror* config);
  std::shared_ptr<ForwardingInformationBaseMap>
  updateForwardingInformationBaseContainers();
  std::optional<cfg::PfcWatchdogRecoveryAction> getPfcWatchdogRecoveryAction();

  std::shared_ptr<LabelForwardingEntry> createLabelForwardingEntry(
      MplsLabel label,
      LabelNextHopEntry::Action action,
      LabelNextHopSet nexthops);

  LabelNextHopEntry getStaticLabelNextHopEntry(
      LabelNextHopEntry::Action action,
      LabelNextHopSet nexthops);

  std::shared_ptr<LabelForwardingInformationBase> updateStaticMplsRoutes(
      const std::vector<cfg::StaticMplsRouteWithNextHops>&
          staticMplsRoutesWithNhops,
      const std::vector<cfg::StaticMplsRouteNoNextHops>& staticMplsRoutesToNull,
      const std::vector<cfg::StaticMplsRouteNoNextHops>& staticMplsRoutesToCPU);

  std::shared_ptr<SwitchState> orig_;
  std::shared_ptr<SwitchState> new_;
  const cfg::SwitchConfig* cfg_{nullptr};
  const Platform* platform_{nullptr};
  RoutingInformationBase* rib_{nullptr};
  RouteUpdateWrapper* routeUpdater_{nullptr};

  struct VlanIpInfo {
    VlanIpInfo(uint8_t mask, MacAddress mac, InterfaceID intf)
        : mask(mask), mac(mac), interfaceID(intf) {}

    uint8_t mask;
    MacAddress mac;
    InterfaceID interfaceID;
  };
  struct VlanInterfaceInfo {
    RouterID routerID{0};
    flat_set<InterfaceID> interfaces;
    flat_map<IPAddress, VlanIpInfo> addresses;
  };

  flat_map<PortID, Port::VlanMembership> portVlans_;
  flat_map<VlanID, Vlan::MemberPorts> vlanPorts_;
  flat_map<VlanID, VlanInterfaceInfo> vlanInterfaces_;
};

shared_ptr<SwitchState> ThriftConfigApplier::run() {
  new_ = orig_->clone();
  bool changed = false;

  {
    auto newSwitchSettings = updateSwitchSettings();
    if (newSwitchSettings) {
      new_->resetSwitchSettings(std::move(newSwitchSettings));
      changed = true;
    }
  }

  {
    bool qcmChanged = false;
    auto newQcmConfig = updateQcmCfg(&qcmChanged);
    if (qcmChanged) {
      new_->resetQcmCfg(newQcmConfig);
      changed = true;
    }
  }

  {
    auto newControlPlane = updateControlPlane();
    if (newControlPlane) {
      new_->resetControlPlane(std::move(newControlPlane));
      changed = true;
    }
  }

  processVlanPorts();

  {
    bool bufferPoolConfigChanged = false;
    auto newBufferPoolCfg = updateBufferPoolConfigs(&bufferPoolConfigChanged);
    if (bufferPoolConfigChanged) {
      new_->resetBufferPoolCfgs(newBufferPoolCfg);
      changed = true;
    }
  }

  {
    auto newPorts = updatePorts();
    if (newPorts) {
      new_->resetPorts(std::move(newPorts));
      changed = true;
    }
  }

  {
    auto newAggPorts = updateAggregatePorts();
    if (newAggPorts) {
      new_->resetAggregatePorts(std::move(newAggPorts));
      changed = true;
    }
  }

  // updateMirrors must be called after updatePorts, mirror needs ports!
  {
    auto newMirrors = updateMirrors();
    if (newMirrors) {
      new_->resetMirrors(std::move(newMirrors));
      changed = true;
    }
  }

  // updateAcls must be called after updateMirrors, acls may need mirror!
  {
    if (FLAGS_enable_acl_table_group) {
      auto newAclTableGroups = updateAclTableGroups();
      if (newAclTableGroups) {
        new_->resetAclTableGroups(std::move(newAclTableGroups));
        changed = true;
      }
    } else {
      auto newAcls = updateAcls(cfg::AclStage::INGRESS, *cfg_->acls_ref());
      if (newAcls) {
        new_->resetAcls(std::move(newAcls));
        changed = true;
      }
    }
  }

  {
    auto newQosPolicies = updateQosPolicies();
    if (newQosPolicies) {
      new_->resetQosPolicies(std::move(newQosPolicies));
      changed = true;
    }
  }

  // reset the default qos policy
  {
    auto newDefaultQosPolicy = updateDataplaneDefaultQosPolicy();
    if (new_->getDefaultDataPlaneQosPolicy() != newDefaultQosPolicy) {
      new_->setDefaultDataPlaneQosPolicy(newDefaultQosPolicy);
      changed = true;
    }
  }

  {
    auto newIntfs = updateInterfaces();
    if (newIntfs) {
      new_->resetIntfs(std::move(newIntfs));
      changed = true;
    }
  }

  // Note: updateInterfaces() must be called before updateVlans(),
  // as updateInterfaces() populates the vlanInterfaces_ data structure.
  {
    auto newVlans = updateVlans();
    if (newVlans) {
      new_->resetVlans(std::move(newVlans));
      changed = true;
    }
  }

  if (routeUpdater_) {
    routeUpdater_->setRoutesToConfig(
        intfRouteTables_,
        *cfg_->staticRoutesWithNhops_ref(),
        *cfg_->staticRoutesToNull_ref(),
        *cfg_->staticRoutesToCPU_ref(),
        *cfg_->staticIp2MplsRoutes_ref());
  } else if (rib_) {
    auto newFibs = updateForwardingInformationBaseContainers();
    if (newFibs) {
      new_->resetForwardingInformationBases(newFibs);
      changed = true;
    }

    rib_->reconfigure(
        intfRouteTables_,
        *cfg_->staticRoutesWithNhops_ref(),
        *cfg_->staticRoutesToNull_ref(),
        *cfg_->staticRoutesToCPU_ref(),
        *cfg_->staticIp2MplsRoutes_ref(),
        &updateFibFromConfig,
        static_cast<void*>(&new_));
  } else {
    // switch state UTs don't necessary care about RIB updates
    XLOG(WARNING)
        << " Ignoring config updates to rib, should never happen outside of tests";
  }

  // resolving mpls next hops may need interfaces to be setup
  // process static mpls routes after processing interfaces
  auto labelFib = updateStaticMplsRoutes(
      *cfg_->staticMplsRoutesWithNhops_ref(),
      *cfg_->staticMplsRoutesToNull_ref(),
      *cfg_->staticMplsRoutesToNull_ref());
  if (labelFib) {
    new_->resetLabelForwardingInformationBase(labelFib);
    changed = true;
  }

  auto newVlans = new_->getVlans();
  VlanID dfltVlan(*cfg_->defaultVlan_ref());
  if (orig_->getDefaultVlan() != dfltVlan) {
    if (newVlans->getVlanIf(dfltVlan) == nullptr) {
      throw FbossError("Default VLAN ", dfltVlan, " does not exist");
    }
    new_->setDefaultVlan(dfltVlan);
    changed = true;
  }

  // Make sure all interfaces refer to valid VLANs.
  for (const auto& vlanInfo : vlanInterfaces_) {
    if (newVlans->getVlanIf(vlanInfo.first) == nullptr) {
      throw FbossError(
          "Interface ",
          *(vlanInfo.second.interfaces.begin()),
          " refers to non-existent VLAN ",
          vlanInfo.first);
    }
    // Make sure there is a one-to-one map between vlan and interface
    // Remove this sanity check if multiple interfaces are allowed per vlans
    auto& entry = vlanInterfaces_[vlanInfo.first];
    if (entry.interfaces.size() != 1) {
      auto cpu_vlan = new_->getDefaultVlan();
      if (vlanInfo.first != cpu_vlan) {
        throw FbossError(
            "Vlan ",
            vlanInfo.first,
            " refers to ",
            entry.interfaces.size(),
            " interfaces ");
      }
    }
  }

  {
    auto pfcWatchdogRecoveryAction = getPfcWatchdogRecoveryAction();
    if (pfcWatchdogRecoveryAction != orig_->getPfcWatchdogRecoveryAction()) {
      new_->setPfcWatchdogRecoveryAction(pfcWatchdogRecoveryAction);
      changed = true;
    }
  }

  std::chrono::seconds arpAgerInterval(*cfg_->arpAgerInterval_ref());
  if (orig_->getArpAgerInterval() != arpAgerInterval) {
    new_->setArpAgerInterval(arpAgerInterval);
    changed = true;
  }

  std::chrono::seconds arpTimeout(*cfg_->arpTimeoutSeconds_ref());
  if (orig_->getArpTimeout() != arpTimeout) {
    new_->setArpTimeout(arpTimeout);

    // TODO(aeckert): add ndpTimeout field to SwitchConfig. For now use the same
    // timeout for both ARP and NDP
    new_->setNdpTimeout(arpTimeout);
    changed = true;
  }

  uint32_t maxNeighborProbes(*cfg_->maxNeighborProbes_ref());
  if (orig_->getMaxNeighborProbes() != maxNeighborProbes) {
    new_->setMaxNeighborProbes(maxNeighborProbes);
    changed = true;
  }

  auto oldDhcpV4RelaySrc = orig_->getDhcpV4RelaySrc();
  auto newDhcpV4RelaySrc = cfg_->dhcpRelaySrcOverrideV4_ref()
      ? IPAddressV4(*cfg_->dhcpRelaySrcOverrideV4_ref())
      : IPAddressV4();
  if (oldDhcpV4RelaySrc != newDhcpV4RelaySrc) {
    new_->setDhcpV4RelaySrc(newDhcpV4RelaySrc);
    changed = true;
  }

  auto oldDhcpV6RelaySrc = orig_->getDhcpV6RelaySrc();
  auto newDhcpV6RelaySrc = cfg_->dhcpRelaySrcOverrideV6_ref()
      ? IPAddressV6(*cfg_->dhcpRelaySrcOverrideV6_ref())
      : IPAddressV6("::");
  if (oldDhcpV6RelaySrc != newDhcpV6RelaySrc) {
    new_->setDhcpV6RelaySrc(newDhcpV6RelaySrc);
    changed = true;
  }

  auto oldDhcpV4ReplySrc = orig_->getDhcpV4ReplySrc();
  auto newDhcpV4ReplySrc = cfg_->dhcpReplySrcOverrideV4_ref()
      ? IPAddressV4(*cfg_->dhcpReplySrcOverrideV4_ref())
      : IPAddressV4();
  if (oldDhcpV4ReplySrc != newDhcpV4ReplySrc) {
    new_->setDhcpV4ReplySrc(newDhcpV4ReplySrc);
    changed = true;
  }

  auto oldDhcpV6ReplySrc = orig_->getDhcpV6ReplySrc();
  auto newDhcpV6ReplySrc = cfg_->dhcpReplySrcOverrideV6_ref()
      ? IPAddressV6(*cfg_->dhcpReplySrcOverrideV6_ref())
      : IPAddressV6("::");
  if (oldDhcpV6ReplySrc != newDhcpV6ReplySrc) {
    new_->setDhcpV6ReplySrc(newDhcpV6ReplySrc);
    changed = true;
  }

  std::chrono::seconds staleEntryInterval(*cfg_->staleEntryInterval_ref());
  if (orig_->getStaleEntryInterval() != staleEntryInterval) {
    new_->setStaleEntryInterval(staleEntryInterval);
    changed = true;
  }

  // Add sFlow collectors
  {
    auto newCollectors = updateSflowCollectors();
    if (newCollectors) {
      new_->resetSflowCollectors(std::move(newCollectors));
      changed = true;
    }
  }

  {
    LoadBalancerConfigApplier loadBalancerConfigApplier(
        orig_->getLoadBalancers(), cfg_->get_loadBalancers(), platform_);
    auto newLoadBalancers = loadBalancerConfigApplier.updateLoadBalancers();
    if (newLoadBalancers) {
      new_->resetLoadBalancers(std::move(newLoadBalancers));
      changed = true;
    }
  }

  // normalizer to refresh counter tags
  if (auto normalizer = Normalizer::getInstance()) {
    normalizer->reloadCounterTags(*cfg_);
  } else {
    XLOG(ERR)
        << "Normalizer failed to initialize, skipping loading counter tags";
  }

  if (!changed) {
    return nullptr;
  }
  return new_;
}

void ThriftConfigApplier::processVlanPorts() {
  // Build the Port --> Vlan mappings
  //
  // The config file has a separate list for this data,
  // but it is stored in the state tree as part of both the PortMap and the
  // VlanMap.
  for (const auto& vp : *cfg_->vlanPorts_ref()) {
    PortID portID(*vp.logicalPort_ref());
    VlanID vlanID(*vp.vlanID_ref());
    auto ret1 = portVlans_[portID].insert(
        std::make_pair(vlanID, Port::VlanInfo(*vp.emitTags_ref())));
    if (!ret1.second) {
      throw FbossError(
          "duplicate VlanPort for port ", portID, ", vlan ", vlanID);
    }
    auto ret2 = vlanPorts_[vlanID].insert(
        std::make_pair(portID, Vlan::PortInfo(*vp.emitTags_ref())));
    if (!ret2.second) {
      // This should never fail if the first insert succeeded above.
      throw FbossError(
          "duplicate VlanPort for vlan ", vlanID, ", port ", portID);
    }
  }
}

void ThriftConfigApplier::updateVlanInterfaces(const Interface* intf) {
  auto& entry = vlanInterfaces_[intf->getVlanID()];

  // Each VLAN can only be used with a single virtual router
  if (entry.interfaces.empty()) {
    entry.routerID = intf->getRouterID();
  } else {
    if (intf->getRouterID() != entry.routerID) {
      throw FbossError(
          "VLAN ",
          intf->getVlanID(),
          " configured in multiple "
          "different virtual routers: ",
          entry.routerID,
          " and ",
          intf->getRouterID());
    }
  }

  auto intfRet = entry.interfaces.insert(intf->getID());
  if (!intfRet.second) {
    // This shouldn't happen
    throw FbossError(
        "interface ",
        intf->getID(),
        " processed twice for "
        "VLAN ",
        intf->getVlanID());
  }

  for (const auto& ipMask : intf->getAddresses()) {
    VlanIpInfo info(ipMask.second, intf->getMac(), intf->getID());
    auto ret = entry.addresses.emplace(ipMask.first, info);
    if (ret.second) {
      continue;
    }
    // Allow multiple interfaces on the same VLAN with the same IP,
    // as long as they also share the same mask and MAC address.
    const auto& oldInfo = ret.first->second;
    if (oldInfo.mask != info.mask) {
      throw FbossError(
          "VLAN ",
          intf->getVlanID(),
          " has IP ",
          ipMask.first,
          " configured multiple times with different masks (",
          oldInfo.mask,
          " and ",
          info.mask,
          ")");
    }
    if (oldInfo.mac != info.mac) {
      throw FbossError(
          "VLAN ",
          intf->getVlanID(),
          " has IP ",
          ipMask.first,
          " configured multiple times with different MACs (",
          oldInfo.mac,
          " and ",
          info.mac,
          ")");
    }
  }

  // Also add the link-local IPv6 address
  IPAddressV6 linkLocalAddr(IPAddressV6::LINK_LOCAL, intf->getMac());
  VlanIpInfo linkLocalInfo(64, intf->getMac(), intf->getID());
  entry.addresses.emplace(IPAddress(linkLocalAddr), linkLocalInfo);
}

shared_ptr<PortMap> ThriftConfigApplier::updatePorts() {
  auto origPorts = orig_->getPorts();
  PortMap::NodeContainer newPorts;
  bool changed = false;

  sharedBufferPoolName.reset();

  // Process all supplied port configs
  for (const auto& portCfg : *cfg_->ports_ref()) {
    PortID id(*portCfg.logicalID_ref());
    auto origPort = origPorts->getPortIf(id);
    std::shared_ptr<Port> newPort;
    if (!origPort) {
      auto port = std::make_shared<Port>(
          PortID(*portCfg.logicalID_ref()), portCfg.name_ref().value_or({}));
      newPort = updatePort(port, &portCfg);
    } else {
      newPort = updatePort(origPort, &portCfg);
    }
    changed |= updateMap(&newPorts, origPort, newPort);
  }

  for (const auto& origPort : *origPorts) {
    // This port was listed in the config, and has already been configured
    if (newPorts.find(origPort->getID()) != newPorts.end()) {
      continue;
    }

    // For platforms that support add/removing ports, we should leave the ports
    // without configs out of the switch state. For BCM tests + hardware that
    // doesn't allow add/remove, we need to leave the ports in the switch state
    // with a default (disabled) config.
    if (platform_->supportsAddRemovePort()) {
      changed = true;
    } else {
      throw FbossError(
          "New config is missing configuration for port ", origPort->getID());
    }
  }

  if (!changed) {
    return nullptr;
  }

  return origPorts->clone(newPorts);
}

void ThriftConfigApplier::checkPortQueueAQMValid(
    const std::vector<cfg::ActiveQueueManagement>& aqms) {
  if (aqms.empty()) {
    return;
  }
  std::set<cfg::QueueCongestionBehavior> behaviors;
  for (const auto& aqm : aqms) {
    if (aqm.detection_ref()->getType() ==
        cfg::QueueCongestionDetection::Type::__EMPTY__) {
      throw FbossError(
          "Active Queue Management must specify a congestion detection method");
    }
    if (behaviors.find(*aqm.behavior_ref()) != behaviors.end()) {
      throw FbossError("Same Active Queue Management behavior already exists");
    }
    behaviors.insert(*aqm.behavior_ref());
  }
}

std::shared_ptr<PortQueue> ThriftConfigApplier::updatePortQueue(
    const std::shared_ptr<PortQueue>& orig,
    const cfg::PortQueue* cfg,
    std::optional<TrafficClass> trafficClass,
    std::optional<std::set<PfcPriority>> pfcPriorities) {
  CHECK_EQ(orig->getID(), *cfg->id_ref());

  if (checkSwConfPortQueueMatch(orig, cfg) &&
      trafficClass == orig->getTrafficClass() &&
      pfcPriorities == orig->getPfcPrioritySet()) {
    return orig;
  }

  // We should always use the PortQueue settings from config, so that if some
  // of the attributes is removed from config, we can make sure that attribute
  // can set back to default
  return createPortQueue(cfg, trafficClass, pfcPriorities);
}

std::shared_ptr<PortQueue> ThriftConfigApplier::createPortQueue(
    const cfg::PortQueue* cfg,
    std::optional<TrafficClass> trafficClass,
    std::optional<std::set<PfcPriority>> pfcPriorities) {
  auto queue =
      std::make_shared<PortQueue>(static_cast<uint8_t>(*cfg->id_ref()));
  queue->setStreamType(*cfg->streamType_ref());
  queue->setScheduling(*cfg->scheduling_ref());
  if (auto weight = cfg->weight_ref()) {
    queue->setWeight(*weight);
  }
  if (auto reservedBytes = cfg->reservedBytes_ref()) {
    queue->setReservedBytes(*reservedBytes);
  }
  if (auto scalingFactor = cfg->scalingFactor_ref()) {
    queue->setScalingFactor(*scalingFactor);
  }
  if (auto aqms = cfg->aqms_ref()) {
    checkPortQueueAQMValid(*aqms);
    queue->resetAqms(*aqms);
  }
  if (auto shareBytes = cfg->sharedBytes_ref()) {
    queue->setSharedBytes(*shareBytes);
  }
  if (auto name = cfg->name_ref()) {
    queue->setName(*name);
  }
  if (auto portQueueRate = cfg->portQueueRate_ref()) {
    queue->setPortQueueRate(*portQueueRate);
  }
  if (auto bandwidthBurstMinKbits = cfg->bandwidthBurstMinKbits_ref()) {
    queue->setBandwidthBurstMinKbits(*bandwidthBurstMinKbits);
  }
  if (auto bandwidthBurstMaxKbits = cfg->bandwidthBurstMaxKbits_ref()) {
    queue->setBandwidthBurstMaxKbits(*bandwidthBurstMaxKbits);
  }
  if (trafficClass) {
    queue->setTrafficClasses(trafficClass.value());
  }
  if (pfcPriorities) {
    queue->setPfcPrioritySet(pfcPriorities.value());
  }
  return queue;
}

std::shared_ptr<PortPgConfig> ThriftConfigApplier::createPortPg(
    const cfg::PortPgConfig* cfg) {
  auto pgCfg =
      std::make_shared<PortPgConfig>(static_cast<uint8_t>(*cfg->id_ref()));
  if (const auto scalingFactor = cfg->scalingFactor_ref()) {
    pgCfg->setScalingFactor(*scalingFactor);
  }

  if (const auto name = cfg->name_ref()) {
    pgCfg->setName(*name);
  }

  pgCfg->setMinLimitBytes(*cfg->minLimitBytes_ref());

  if (const auto headroom = cfg->headroomLimitBytes_ref()) {
    pgCfg->setHeadroomLimitBytes(*headroom);
  }
  if (const auto resumeOffsetBytes = cfg->resumeOffsetBytes_ref()) {
    pgCfg->setResumeOffsetBytes(*resumeOffsetBytes);
  }
  pgCfg->setBufferPoolName(*cfg->bufferPoolName_ref());
  return pgCfg;
}

bool ThriftConfigApplier::isPgConfigUnchanged(
    std::optional<PortPgConfigs> newPortPgCfgs,
    const shared_ptr<Port>& orig) {
  flat_map<int, std::shared_ptr<PortPgConfig>> newPortPgConfigMap;
  const auto& origPortPgConfig = orig->getPortPgConfigs();

  if (!origPortPgConfig && !newPortPgCfgs) {
    // no change before or after
    return true;
  }

  // if go from old pgConfig <-> no pg cfg and vice versa, there is a change
  if ((origPortPgConfig && !newPortPgCfgs) ||
      (!origPortPgConfig && newPortPgCfgs)) {
    return false;
  }

  // both oldPgCfg and newPgCfg exists
  if ((*newPortPgCfgs).size() != (*origPortPgConfig).size()) {
    return false;
  }

  // come here only if we have both orig, and new port pg cfg and have same size
  for (const auto& portPg : *newPortPgCfgs) {
    newPortPgConfigMap.emplace(std::make_pair(portPg->getID(), portPg));
  }

  for (const auto& origPg : *origPortPgConfig) {
    auto newPortPgConfigIter = newPortPgConfigMap.find(origPg->getID());
    if ((newPortPgConfigIter == newPortPgConfigMap.end()) ||
        ((*newPortPgConfigIter->second != *origPg))) {
      // pg id in the original cfg, is no longer there is new one
      // or the contents of the PG doesn't match
      return false;
    }
    // since buffer pool is part of port pg as well, compare to
    // see if it changed as well
    const auto& newBufferPoolCfgPtr =
        newPortPgConfigIter->second->getBufferPoolConfig();
    const auto& oldBufferPoolCfgPtr = origPg->getBufferPoolConfig();
    // old buffer cfg exists and new one doesn't or vice versa
    if ((newBufferPoolCfgPtr && !oldBufferPoolCfgPtr) ||
        (!newBufferPoolCfgPtr && oldBufferPoolCfgPtr)) {
      return false;
    }
    // contents changed in the buffer pool
    if (newBufferPoolCfgPtr && oldBufferPoolCfgPtr) {
      if (*oldBufferPoolCfgPtr != *newBufferPoolCfgPtr) {
        return false;
      }
    }
  }
  return true;
}

PortPgConfigs ThriftConfigApplier::updatePortPgConfigs(
    const std::vector<cfg::PortPgConfig>& newPortPgConfig,
    const shared_ptr<Port>& orig) {
  PortPgConfigs newPortPgConfigs = {};
  if (newPortPgConfig.empty()) {
    // nothing to update, just return
    return newPortPgConfigs;
  }

  // pg value can be [0, PORT_PG_VALUE_MAX]
  if (newPortPgConfig.size() >
      cfg::switch_config_constants::PORT_PG_VALUE_MAX() + 1) {
    throw FbossError(
        "Port",
        orig->getID(),
        " pgConfig size ",
        newPortPgConfig.size(),
        " greater than max supported pgs ",
        cfg::switch_config_constants::PORT_PG_VALUE_MAX() + 1);
  }

  for (const auto& portPg : newPortPgConfig) {
    std::shared_ptr<PortPgConfig> tmpPortPgConfig;
    tmpPortPgConfig = createPortPg(&portPg);
    newPortPgConfigs.push_back(tmpPortPgConfig);
  }

  // sort these Pgs in order PG0 -> PG7
  std::sort(
      newPortPgConfigs.begin(),
      newPortPgConfigs.end(),
      [](const std::shared_ptr<PortPgConfig>& pg1,
         const std::shared_ptr<PortPgConfig>& pg2) {
        return pg1->getID() < pg2->getID();
      });

  return newPortPgConfigs;
}

std::optional<std::vector<PfcPriority>>
ThriftConfigApplier::findEnabledPfcPriorities(PortPgConfigs& portPgCfgs) {
  if (portPgCfgs.empty()) {
    return std::nullopt;
  }

  std::vector<PfcPriority> tmpPfcPri;
  for (auto& portPgCfg : portPgCfgs) {
    // We have a 1:1 mapping between PG id and PFC priority
    tmpPfcPri.push_back(static_cast<PfcPriority>(portPgCfg->getID()));
  }
  if (tmpPfcPri.empty()) {
    return std::nullopt;
  }

  return tmpPfcPri;
}

QueueConfig ThriftConfigApplier::updatePortQueues(
    const QueueConfig& origPortQueues,
    const std::vector<cfg::PortQueue>& cfgPortQueues,
    uint16_t maxQueues,
    cfg::StreamType streamType,
    std::optional<cfg::QosMap> qosMap) {
  QueueConfig newPortQueues;

  /*
   * By default, queue config is picked from defaultPortQueues. However, per
   * port queue config, if specified, overrides it.
   */
  flat_map<int, const cfg::PortQueue*> newQueues;
  for (const auto& queue : cfgPortQueues) {
    if (streamType == queue.streamType_ref()) {
      newQueues.emplace(std::make_pair(*queue.id_ref(), &queue));
    }
  }

  if (newQueues.empty()) {
    for (const auto& queue : *cfg_->defaultPortQueues_ref()) {
      if (streamType == queue.streamType_ref()) {
        newQueues.emplace(std::make_pair(*queue.id_ref(), &queue));
      }
    }
  }

  // Process all supplied queues
  // We retrieve the current port queue values from hardware
  // if there is a config present for any of these queues, we update the
  // PortQueue according to this
  // Otherwise we reset it to the default values for this queue type
  for (auto queueId = 0; queueId < maxQueues; queueId++) {
    auto newQueueIter = newQueues.find(queueId);
    std::shared_ptr<PortQueue> newPortQueue;
    if (newQueueIter != newQueues.end()) {
      std::optional<TrafficClass> trafficClass;
      std::optional<std::set<PfcPriority>> pfcPriorities;
      if (qosMap) {
        // Get traffic class for such queue if exists
        for (auto entry : *qosMap->trafficClassToQueueId_ref()) {
          if (entry.second == queueId) {
            trafficClass = static_cast<TrafficClass>(entry.first);
            break;
          }
        }

        if (const auto& pfcPri2QueueIdMap =
                qosMap->pfcPriorityToQueueId_ref()) {
          std::set<PfcPriority> tmpPfcPriSet;
          for (const auto& pfcPri2QueueId : *pfcPri2QueueIdMap) {
            if (pfcPri2QueueId.second == queueId) {
              auto pfcPriority = static_cast<PfcPriority>(pfcPri2QueueId.first);
              tmpPfcPriSet.insert(pfcPriority);
            }
          }
          if (tmpPfcPriSet.size()) {
            // dont populate port queue priority if empty
            pfcPriorities = tmpPfcPriSet;
          }
        }
      }
      auto origQueueIter = std::find_if(
          origPortQueues.begin(),
          origPortQueues.end(),
          [&](const auto& origQueue) { return queueId == origQueue->getID(); });
      newPortQueue = (origQueueIter != origPortQueues.end())
          ? updatePortQueue(
                *origQueueIter,
                newQueueIter->second,
                trafficClass,
                pfcPriorities)
          : createPortQueue(newQueueIter->second, trafficClass, pfcPriorities);
      newQueues.erase(newQueueIter);
    } else {
      newPortQueue = std::make_shared<PortQueue>(static_cast<uint8_t>(queueId));
      newPortQueue->setStreamType(streamType);
    }
    newPortQueues.push_back(newPortQueue);
  }

  std::sort(
      newPortQueues.begin(),
      newPortQueues.end(),
      [](const std::shared_ptr<PortQueue>& q1,
         const std::shared_ptr<PortQueue>& q2) {
        return q1->getID() < q2->getID();
      });

  if (newQueues.size() > 0) {
    throw FbossError(
        "Port queue config listed for invalid queues. Maximum",
        " number of queues on this platform is ",
        origPortQueues.size());
  }
  return newPortQueues;
}

void ThriftConfigApplier::validateUpdatePgBufferPoolName(
    const PortPgConfigs& portPgCfgs,
    const shared_ptr<Port>& port,
    const std::string& portPgName) {
  // this is processed after bufferPoolConfig changes
  // validate that bufferPool name exists in the cfg
  for (const auto& portPg : portPgCfgs) {
    auto bufferPoolName = portPg->getBufferPoolName();

    // only one buffer pool is supported
    // check buffer pool name is common across pgs and across ports
    if (!sharedBufferPoolName.has_value()) {
      sharedBufferPoolName = bufferPoolName;
      XLOG(DBG2) << "sharedBufferPoolName: " << sharedBufferPoolName.value();
    } else if (sharedBufferPoolName != bufferPoolName) {
      throw FbossError(
          "Port:",
          port->getID(),
          " with pg name: ",
          portPgName,
          " buffer pool name: ",
          bufferPoolName,
          " is different from shared buffer pool name: ",
          sharedBufferPoolName.value(),
          " Only one bufferPool supported! ");
    }

    if (!portPg->getBufferPoolName().empty()) {
      auto bufferPoolCfgMap = new_->getBufferPoolCfgs();
      if (!bufferPoolCfgMap) {
        throw FbossError(
            "Port:",
            port->getID(),
            " with pg name: ",
            portPgName,
            " and buffer pool name: ",
            bufferPoolName,
            " exists but buffer pool map doesn't exist!");
      }
      // bufferPool cfg is keyed on the buffer pool name
      auto bufferPoolCfg = bufferPoolCfgMap->getNodeIf(bufferPoolName);
      if (!bufferPoolCfg) {
        throw FbossError(
            "Port:",
            port->getID(),
            " with pg name: ",
            portPgName,
            " but buffer pool name: ",
            bufferPoolName,
            " doesn't exist in the bufferPool map.");
      }
      portPg->setBufferPoolConfig(bufferPoolCfg);
    }
  }
}

shared_ptr<Port> ThriftConfigApplier::updatePort(
    const shared_ptr<Port>& orig,
    const cfg::Port* portConf) {
  CHECK_EQ(orig->getID(), *portConf->logicalID_ref());

  auto vlans = portVlans_[orig->getID()];

  std::vector<cfg::PortQueue> cfgPortQueues;
  if (auto portQueueConfigName = portConf->portQueueConfigName_ref()) {
    auto it = cfg_->portQueueConfigs_ref()->find(*portQueueConfigName);
    if (it == cfg_->portQueueConfigs_ref()->end()) {
      throw FbossError(
          "Port queue config name: ",
          *portQueueConfigName,
          " does not exist in PortQueueConfig map");
    }
    cfgPortQueues = it->second;
  }

  const auto& oldIngressMirror = orig->getIngressMirror();
  const auto& oldEgressMirror = orig->getEgressMirror();
  auto newIngressMirror = std::optional<std::string>();
  auto newEgressMirror = std::optional<std::string>();
  if (auto ingressMirror = portConf->ingressMirror_ref()) {
    newIngressMirror = *ingressMirror;
  }
  if (auto egressMirror = portConf->egressMirror_ref()) {
    newEgressMirror = *egressMirror;
  }
  bool mirrorsUnChanged = (oldIngressMirror == newIngressMirror) &&
      (oldEgressMirror == newEgressMirror);

  auto newQosPolicy = std::optional<std::string>();
  if (auto dataPlaneTrafficPolicy = cfg_->dataPlaneTrafficPolicy_ref()) {
    if (auto defaultDataPlaneQosPolicy =
            dataPlaneTrafficPolicy->defaultQosPolicy_ref()) {
      newQosPolicy = *defaultDataPlaneQosPolicy;
    }
    if (auto portIdToQosPolicy =
            dataPlaneTrafficPolicy->portIdToQosPolicy_ref()) {
      auto qosPolicyItr = portIdToQosPolicy->find(*portConf->logicalID_ref());
      if (qosPolicyItr != portIdToQosPolicy->end()) {
        newQosPolicy = qosPolicyItr->second;
      }
    }
  }

  std::optional<cfg::QosMap> qosMap;
  if (newQosPolicy) {
    bool qosPolicyFound = false;
    for (auto qosPolicy : *cfg_->qosPolicies_ref()) {
      if (qosPolicyFound) {
        break;
      }
      qosPolicyFound = (*qosPolicy.name_ref() == newQosPolicy.value());
      if (qosPolicyFound && qosPolicy.qosMap_ref()) {
        qosMap = qosPolicy.qosMap_ref().value();
      }
    }
    if (!qosPolicyFound) {
      throw FbossError("qos policy ", newQosPolicy.value(), " not found");
    }
  }

  // For now, we only support update unicast port queues for ports
  QueueConfig portQueues;
  for (auto streamType : platform_->getAsic()->getQueueStreamTypes(false)) {
    auto maxQueues =
        platform_->getAsic()->getDefaultNumPortQueues(streamType, false);
    auto tmpPortQueues = updatePortQueues(
        orig->getPortQueues(), cfgPortQueues, maxQueues, streamType, qosMap);
    portQueues.insert(
        portQueues.begin(), tmpPortQueues.begin(), tmpPortQueues.end());
  }
  bool queuesUnchanged = portQueues.size() == orig->getPortQueues().size();
  for (int i = 0; i < portQueues.size() && queuesUnchanged; i++) {
    if (*(portQueues.at(i)) != *(orig->getPortQueues().at(i))) {
      queuesUnchanged = false;
      break;
    }
  }

  auto newSampleDest = std::optional<cfg::SampleDestination>();
  if (portConf->sampleDest_ref()) {
    newSampleDest = portConf->sampleDest_ref().value();

    if (newSampleDest.value() == cfg::SampleDestination::MIRROR &&
        *portConf->sFlowEgressRate_ref() > 0) {
      throw FbossError(
          "Port ",
          orig->getID(),
          ": Egress sampling to mirror destination is unsupported");
    }
  }

  auto newPfc = std::optional<cfg::PortPfc>();
  auto newPfcPriorities = std::optional<std::vector<PfcPriority>>();
  std::optional<PortPgConfigs> portPgCfgs;
  // lets compare the portPgConfigs
  bool portPgConfigUnchanged = true;
  if (portConf->pfc_ref().has_value()) {
    newPfc = portConf->pfc_ref().value();

    auto pause = portConf->pause_ref().value();
    bool pfc_rx = *newPfc->rx_ref();
    bool pfc_tx = *newPfc->tx_ref();
    bool pause_rx = *pause.rx_ref();
    bool pause_tx = *pause.tx_ref();

    if (pfc_rx || pfc_tx) {
      if (!platform_->getAsic()->isSupported(HwAsic::Feature::PFC)) {
        throw FbossError(
            "Port ",
            orig->getID(),
            " has PFC enabled, but its not supported feature for this platform");
      }
      if (pause_rx || pause_tx) {
        throw FbossError(
            "Port ",
            orig->getID(),
            " PAUSE and PFC cannot be enabled on the same port");
      }
    }

    auto portPgConfigName = newPfc->portPgConfigName_ref();
    if (newPfc->watchdog_ref().has_value() && (*portPgConfigName).empty()) {
      throw FbossError(
          "Port ",
          orig->getID(),
          " Priority group must be associated with port "
          "when PFC watchdog is configured");
    }
    if (auto portPgConfigs = cfg_->portPgConfigs_ref()) {
      auto it = portPgConfigs->find(*portPgConfigName);
      if (it == portPgConfigs->end()) {
        throw FbossError(
            "Port ",
            orig->getID(),
            " pg name",
            *portPgConfigName,
            "does not exist in portPgConfig map");
      }
      portPgCfgs = updatePortPgConfigs(it->second, orig);
      // validate that the given pg profile points to valid
      // buffer pool
      validateUpdatePgBufferPoolName(
          portPgCfgs.value(), orig, *portPgConfigName);

      /*
       * Keep track of enabled pfcPriorities which are 1:1
       * mapped to PG id.
       */
      newPfcPriorities = findEnabledPfcPriorities(portPgCfgs.value());
    } else if (!(*portPgConfigName).empty()) {
      throw FbossError(
          "Port: ",
          orig->getID(),
          " pg name: ",
          *portPgConfigName,
          " exist but not the portPgConfig map");
    }
  }
  portPgConfigUnchanged = isPgConfigUnchanged(portPgCfgs, orig);
  /*
   * The list of lookup classes would be different when first enabling the
   * feature and if we had to change the number of lookup classes (unlikely)
   * or disable the queue-per-host feature (for emergency).
   *
   * To avoid unncessary shuffling when only the order of lookup classes
   * changes, (e.g. due to config change), sort and compare. This requires a
   * deep copy and sorting, but in practice, the list of lookup classes would
   * be small (< 10).
   */
  auto origLookupClasses{orig->getLookupClassesToDistributeTrafficOn()};
  auto newLookupClasses{*portConf->lookupClasses_ref()};
  sort(origLookupClasses.begin(), origLookupClasses.end());
  sort(newLookupClasses.begin(), newLookupClasses.end());
  auto lookupClassesUnchanged = (origLookupClasses == newLookupClasses);

  // Prepare the new profileConfig
  const auto& portProfileCfg =
      platform_->getPlatformPort(orig->getID())
          ->getPortProfileConfig(*portConf->profileID_ref());
  if (*portConf->state_ref() == cfg::PortState::ENABLED &&
      *portProfileCfg.speed_ref() != *portConf->speed_ref()) {
    throw FbossError(
        orig->getName(),
        " has mismatched speed on profile:",
        apache::thrift::util::enumNameSafe(*portConf->profileID_ref()),
        " and config:",
        apache::thrift::util::enumNameSafe(*portConf->speed_ref()));
  }
  auto newProfileConfigRef = portProfileCfg.iphy_ref();
  auto profileConfigUnchanged =
      (*newProfileConfigRef == orig->getProfileConfig());
  XLOG_IF(DBG2, !profileConfigUnchanged)
      << orig->getName()
      << " has profileConfigUnchanged:" << profileConfigUnchanged;

  // Ensure portConf has actually changed, before applying
  if (*portConf->state_ref() == orig->getAdminState() &&
      VlanID(*portConf->ingressVlan_ref()) == orig->getIngressVlan() &&
      *portConf->speed_ref() == orig->getSpeed() &&
      *portConf->profileID_ref() == orig->getProfileID() &&
      *portConf->pause_ref() == orig->getPause() && newPfc == orig->getPfc() &&
      newPfcPriorities == orig->getPfcPriorities() &&
      *portConf->sFlowIngressRate_ref() == orig->getSflowIngressRate() &&
      *portConf->sFlowEgressRate_ref() == orig->getSflowEgressRate() &&
      newSampleDest == orig->getSampleDestination() &&
      portConf->name_ref().value_or({}) == orig->getName() &&
      portConf->description_ref().value_or({}) == orig->getDescription() &&
      vlans == orig->getVlans() && queuesUnchanged && portPgConfigUnchanged &&
      *portConf->loopbackMode_ref() == orig->getLoopbackMode() &&
      mirrorsUnChanged && newQosPolicy == orig->getQosPolicy() &&
      *portConf->expectedLLDPValues_ref() == orig->getLLDPValidations() &&
      *portConf->maxFrameSize_ref() == orig->getMaxFrameSize() &&
      lookupClassesUnchanged && profileConfigUnchanged) {
    return nullptr;
  }

  auto newPort = orig->clone();

  auto lldpmap = newPort->getLLDPValidations();
  for (const auto& tag : *portConf->expectedLLDPValues_ref()) {
    lldpmap[tag.first] = tag.second;
  }

  newPort->setAdminState(*portConf->state_ref());
  newPort->setIngressVlan(VlanID(*portConf->ingressVlan_ref()));
  newPort->setVlans(vlans);
  newPort->setSpeed(*portConf->speed_ref());
  newPort->setProfileId(*portConf->profileID_ref());
  newPort->setPause(*portConf->pause_ref());
  newPort->setSflowIngressRate(*portConf->sFlowIngressRate_ref());
  newPort->setSflowEgressRate(*portConf->sFlowEgressRate_ref());
  newPort->setSampleDestination(newSampleDest);
  newPort->setName(portConf->name_ref().value_or({}));
  newPort->setDescription(portConf->description_ref().value_or({}));
  newPort->setLoopbackMode(*portConf->loopbackMode_ref());
  newPort->resetPortQueues(portQueues);
  newPort->setIngressMirror(newIngressMirror);
  newPort->setEgressMirror(newEgressMirror);
  newPort->setQosPolicy(newQosPolicy);
  newPort->setExpectedLLDPValues(lldpmap);
  newPort->setLookupClassesToDistributeTrafficOn(
      *portConf->lookupClasses_ref());
  newPort->setMaxFrameSize(*portConf->maxFrameSize_ref());
  newPort->setPfc(newPfc);
  newPort->setPfcPriorities(newPfcPriorities);
  newPort->resetPgConfigs(portPgCfgs);
  newPort->setProfileConfig(*newProfileConfigRef);
  return newPort;
}

shared_ptr<AggregatePortMap> ThriftConfigApplier::updateAggregatePorts() {
  auto origAggPorts = orig_->getAggregatePorts();
  AggregatePortMap::NodeContainer newAggPorts;
  bool changed = false;

  size_t numExistingProcessed = 0;
  for (const auto& portCfg : *cfg_->aggregatePorts_ref()) {
    AggregatePortID id(*portCfg.key_ref());
    auto origAggPort = origAggPorts->getAggregatePortIf(id);

    shared_ptr<AggregatePort> newAggPort;
    if (origAggPort) {
      newAggPort = updateAggPort(origAggPort, portCfg);
      ++numExistingProcessed;
    } else {
      newAggPort = createAggPort(portCfg);
    }

    changed |= updateMap(&newAggPorts, origAggPort, newAggPort);
  }

  if (numExistingProcessed != origAggPorts->size()) {
    // Some existing aggregate ports were removed.
    CHECK_LE(numExistingProcessed, origAggPorts->size());
    changed = true;
  }

  if (!changed) {
    return nullptr;
  }

  return origAggPorts->clone(newAggPorts);
}

shared_ptr<AggregatePort> ThriftConfigApplier::updateAggPort(
    const shared_ptr<AggregatePort>& origAggPort,
    const cfg::AggregatePort& cfg) {
  CHECK_EQ(origAggPort->getID(), *cfg.key_ref());

  auto cfgSubports = getSubportsSorted(cfg);
  auto origSubports = origAggPort->sortedSubports();

  uint16_t cfgSystemPriority;
  folly::MacAddress cfgSystemID;
  std::tie(cfgSystemID, cfgSystemPriority) = getSystemLacpConfig();

  auto cfgMinLinkCount = computeMinimumLinkCount(cfg);

  if (origAggPort->getName() == *cfg.name_ref() &&
      origAggPort->getDescription() == *cfg.description_ref() &&
      origAggPort->getSystemPriority() == cfgSystemPriority &&
      origAggPort->getSystemID() == cfgSystemID &&
      origAggPort->getMinimumLinkCount() == cfgMinLinkCount &&
      std::equal(
          origSubports.begin(), origSubports.end(), cfgSubports.begin())) {
    return nullptr;
  }

  auto newAggPort = origAggPort->clone();
  newAggPort->setName(*cfg.name_ref());
  newAggPort->setDescription(*cfg.description_ref());
  newAggPort->setSystemPriority(cfgSystemPriority);
  newAggPort->setSystemID(cfgSystemID);
  newAggPort->setMinimumLinkCount(cfgMinLinkCount);
  newAggPort->setSubports(folly::range(cfgSubports.begin(), cfgSubports.end()));

  return newAggPort;
}

shared_ptr<AggregatePort> ThriftConfigApplier::createAggPort(
    const cfg::AggregatePort& cfg) {
  auto subports = getSubportsSorted(cfg);

  uint16_t cfgSystemPriority;
  folly::MacAddress cfgSystemID;
  std::tie(cfgSystemID, cfgSystemPriority) = getSystemLacpConfig();

  auto cfgMinLinkCount = computeMinimumLinkCount(cfg);

  return AggregatePort::fromSubportRange(
      AggregatePortID(*cfg.key_ref()),
      *cfg.name_ref(),
      *cfg.description_ref(),
      cfgSystemPriority,
      cfgSystemID,
      cfgMinLinkCount,
      folly::range(subports.begin(), subports.end()));
}

std::vector<AggregatePort::Subport> ThriftConfigApplier::getSubportsSorted(
    const cfg::AggregatePort& cfg) {
  std::vector<AggregatePort::Subport> subports(std::distance(
      cfg.memberPorts_ref()->begin(), cfg.memberPorts_ref()->end()));

  for (int i = 0; i < subports.size(); ++i) {
    if (*cfg.memberPorts_ref()[i].priority_ref() < 0 ||
        *cfg.memberPorts_ref()[i].priority_ref() >= 1 << 16) {
      throw FbossError("Member port ", i, " has priority outside of [0, 2^16)");
    }

    auto id = PortID(*cfg.memberPorts_ref()[i].memberPortID_ref());
    auto priority =
        static_cast<uint16_t>(*cfg.memberPorts_ref()[i].priority_ref());
    auto rate = *cfg.memberPorts_ref()[i].rate_ref();
    auto activity = *cfg.memberPorts_ref()[i].activity_ref();
    auto multiplier = *cfg.memberPorts_ref()[i].holdTimerMultiplier_ref();

    subports[i] =
        AggregatePort::Subport(id, priority, rate, activity, multiplier);
  }

  std::sort(subports.begin(), subports.end());

  return subports;
}

std::pair<folly::MacAddress, uint16_t>
ThriftConfigApplier::getSystemLacpConfig() {
  folly::MacAddress systemID;
  uint16_t systemPriority;

  if (auto lacp = cfg_->lacp_ref()) {
    systemID = MacAddress(*lacp->systemID_ref());
    systemPriority = *lacp->systemPriority_ref();
  } else {
    // If the system LACP configuration parameters were not specified,
    // we fall back to default parameters. Since the default system ID
    // is not a compile-time constant (it is derived from the CPU mac),
    // the default value is defined here, instead of, say,
    // AggregatePortFields::kDefaultSystemID.
    systemID = platform_->getLocalMac();
    systemPriority = kDefaultSystemPriority;
  }

  return std::make_pair(systemID, systemPriority);
}

uint8_t ThriftConfigApplier::computeMinimumLinkCount(
    const cfg::AggregatePort& cfg) {
  uint8_t minLinkCount = 1;

  auto minCapacity = *cfg.minimumCapacity_ref();
  switch (minCapacity.getType()) {
    case cfg::MinimumCapacity::Type::linkCount:
      // Thrift's byte type is an int8_t
      CHECK_GE(minCapacity.get_linkCount(), 1);

      minLinkCount = minCapacity.get_linkCount();
      break;
    case cfg::MinimumCapacity::Type::linkPercentage:
      CHECK_GT(minCapacity.get_linkPercentage(), 0);
      CHECK_LE(minCapacity.get_linkPercentage(), 1);

      minLinkCount = std::ceil(
          minCapacity.get_linkPercentage() *
          std::distance(
              cfg.memberPorts_ref()->begin(), cfg.memberPorts_ref()->end()));
      if (std::distance(
              cfg.memberPorts_ref()->begin(), cfg.memberPorts_ref()->end()) !=
          0) {
        CHECK_GE(minLinkCount, 1);
      }

      break;
    case cfg::MinimumCapacity::Type::__EMPTY__:
    // needed to handle error from -Werror=switch
    default:
      folly::assume_unreachable();
      break;
  }

  return minLinkCount;
}

shared_ptr<VlanMap> ThriftConfigApplier::updateVlans() {
  auto origVlans = orig_->getVlans();
  VlanMap::NodeContainer newVlans;
  bool changed = false;

  // Process all supplied VLAN configs
  size_t numExistingProcessed = 0;
  for (const auto& vlanCfg : *cfg_->vlans_ref()) {
    VlanID id(*vlanCfg.id_ref());
    auto origVlan = origVlans->getVlanIf(id);
    shared_ptr<Vlan> newVlan;
    if (origVlan) {
      newVlan = updateVlan(origVlan, &vlanCfg);
      ++numExistingProcessed;
    } else {
      newVlan = createVlan(&vlanCfg);
    }
    changed |= updateMap(&newVlans, origVlan, newVlan);
  }

  if (numExistingProcessed != origVlans->size()) {
    // Some existing VLANs were removed.
    CHECK_LT(numExistingProcessed, origVlans->size());
    changed = true;
  }

  if (!changed) {
    return nullptr;
  }

  return origVlans->clone(std::move(newVlans));
}

shared_ptr<Vlan> ThriftConfigApplier::createVlan(const cfg::Vlan* config) {
  const auto& ports = vlanPorts_[VlanID(*config->id_ref())];
  auto vlan = make_shared<Vlan>(config, ports);
  updateNeighborResponseTables(vlan.get(), config);
  updateDhcpOverrides(vlan.get(), config);

  /* TODO t7153326: Following code is added for backward compatibility
  Remove it once coop generates config with */
  if (auto intfID = config->intfID_ref()) {
    vlan->setInterfaceID(InterfaceID(*intfID));
  } else {
    auto& entry = vlanInterfaces_[VlanID(*config->id_ref())];
    if (!entry.interfaces.empty()) {
      vlan->setInterfaceID(*(entry.interfaces.begin()));
    }
  }
  return vlan;
}

shared_ptr<Vlan> ThriftConfigApplier::updateVlan(
    const shared_ptr<Vlan>& orig,
    const cfg::Vlan* config) {
  CHECK_EQ(orig->getID(), *config->id_ref());
  const auto& ports = vlanPorts_[orig->getID()];

  auto newVlan = orig->clone();
  bool changed_neighbor_table =
      updateNeighborResponseTables(newVlan.get(), config);
  bool changed_dhcp_overrides = updateDhcpOverrides(newVlan.get(), config);
  auto oldDhcpV4Relay = orig->getDhcpV4Relay();
  auto newDhcpV4Relay = config->dhcpRelayAddressV4_ref()
      ? IPAddressV4(*config->dhcpRelayAddressV4_ref())
      : IPAddressV4();

  auto oldDhcpV6Relay = orig->getDhcpV6Relay();
  auto newDhcpV6Relay = config->dhcpRelayAddressV6_ref()
      ? IPAddressV6(*config->dhcpRelayAddressV6_ref())
      : IPAddressV6("::");
  /* TODO t7153326: Following code is added for backward compatibility
  Remove it once coop generates config with intfID */
  auto oldIntfID = orig->getInterfaceID();
  auto newIntfID = InterfaceID(0);
  if (auto intfID = config->intfID_ref()) {
    newIntfID = InterfaceID(*intfID);
  } else {
    auto& entry = vlanInterfaces_[VlanID(*config->id_ref())];
    if (!entry.interfaces.empty()) {
      newIntfID = *(entry.interfaces.begin());
    }
  }

  if (orig->getName() == *config->name_ref() && oldIntfID == newIntfID &&
      orig->getPorts() == ports && oldDhcpV4Relay == newDhcpV4Relay &&
      oldDhcpV6Relay == newDhcpV6Relay && !changed_neighbor_table &&
      !changed_dhcp_overrides) {
    return nullptr;
  }

  newVlan->setName(*config->name_ref());
  newVlan->setInterfaceID(newIntfID);
  newVlan->setPorts(ports);
  newVlan->setDhcpV4Relay(newDhcpV4Relay);
  newVlan->setDhcpV6Relay(newDhcpV6Relay);
  return newVlan;
}

std::shared_ptr<QosPolicyMap> ThriftConfigApplier::updateQosPolicies() {
  QosPolicyMap::NodeContainer newQosPolicies;
  bool changed = false;
  int numExistingProcessed = 0;
  auto defaultDataPlaneQosPolicyName = getDefaultDataPlaneQosPolicyName();

  auto qosPolicies = *cfg_->qosPolicies_ref();
  for (auto& qosPolicy : qosPolicies) {
    if (defaultDataPlaneQosPolicyName.has_value() &&
        defaultDataPlaneQosPolicyName.value() == *qosPolicy.name_ref()) {
      // skip default QosPolicy as it will be maintained in switch state
      continue;
    }
    auto newQosPolicy =
        updateQosPolicy(qosPolicy, &numExistingProcessed, &changed);
    if (!newQosPolicies.emplace(*qosPolicy.name_ref(), newQosPolicy).second) {
      throw FbossError(
          "Invalid config: Qos Policy \"",
          *qosPolicy.name_ref(),
          "\" already exists");
    }
  }
  if (numExistingProcessed != orig_->getQosPolicies()->size()) {
    // Some existing Qos Policies were removed.
    changed = true;
  }
  if (!changed) {
    return nullptr;
  }
  return orig_->getQosPolicies()->clone(std::move(newQosPolicies));
}

std::shared_ptr<QosPolicy> ThriftConfigApplier::updateQosPolicy(
    cfg::QosPolicy& qosPolicy,
    int* numExistingProcessed,
    bool* changed) {
  auto origQosPolicy =
      orig_->getQosPolicies()->getQosPolicyIf(*qosPolicy.name_ref());
  auto newQosPolicy = createQosPolicy(qosPolicy);
  if (origQosPolicy) {
    ++(*numExistingProcessed);
    if (*origQosPolicy == *newQosPolicy) {
      return origQosPolicy;
    }
  }
  *changed = true;
  return newQosPolicy;
}

std::optional<std::string>
ThriftConfigApplier::getDefaultDataPlaneQosPolicyName() const {
  if (auto dataPlaneTrafficPolicy = cfg_->dataPlaneTrafficPolicy_ref()) {
    if (auto defaultDataPlaneQosPolicy =
            dataPlaneTrafficPolicy->defaultQosPolicy_ref()) {
      return *defaultDataPlaneQosPolicy;
    }
  }
  return std::nullopt;
}

std::shared_ptr<QosPolicy>
ThriftConfigApplier::updateDataplaneDefaultQosPolicy() {
  auto defaultDataPlaneQosPolicyName = getDefaultDataPlaneQosPolicyName();
  if (!defaultDataPlaneQosPolicyName) {
    return nullptr;
  }
  std::shared_ptr<QosPolicy> newQosPolicy = nullptr;
  auto qosPolicies = *cfg_->qosPolicies_ref();
  for (auto& qosPolicy : qosPolicies) {
    if (defaultDataPlaneQosPolicyName == *qosPolicy.name_ref()) {
      newQosPolicy = createQosPolicy(qosPolicy);
      if (newQosPolicy->getExpMap().empty()) {
        // if exp map is not provided, set some default mapping
        // TODO(pshaikh): remove this once default config for switches will
        // always have EXP maps
        auto expMap = TrafficClassToQosAttributeMap<EXP>();
        for (auto i = 0; i < 8; i++) {
          expMap.addToEntry(static_cast<TrafficClass>(i), static_cast<EXP>(i));
          expMap.addFromEntry(
              static_cast<TrafficClass>(i), static_cast<EXP>(i));
        }
        newQosPolicy->setExpMap(ExpMap(expMap));
      }
      break;
    }
  }
  auto oldQosPolicy = orig_->getDefaultDataPlaneQosPolicy();
  if (oldQosPolicy && newQosPolicy && *oldQosPolicy == *newQosPolicy) {
    return oldQosPolicy;
  }
  return newQosPolicy;
}

shared_ptr<QosPolicy> ThriftConfigApplier::createQosPolicy(
    const cfg::QosPolicy& qosPolicy) {
  if (qosPolicy.rules_ref()->empty() == !qosPolicy.qosMap_ref().has_value()) {
    XLOG(WARN) << "both qos rules and qos map are provided in qos policy "
               << *qosPolicy.name_ref()
               << "! dscp map if present in qos map, will override qos rules";
  }

  DscpMap ingressDscpMap;
  for (const auto& qosRule : *qosPolicy.rules_ref()) {
    if (qosRule.dscp_ref()->empty()) {
      throw FbossError("Invalid config: qosPolicy: empty dscp list");
    }
    for (const auto& dscpValue : *qosRule.dscp_ref()) {
      if (dscpValue < 0 || dscpValue > 63) {
        throw FbossError("dscp value is invalid (must be [0, 63])");
      }
      ingressDscpMap.addFromEntry(
          static_cast<TrafficClass>(*qosRule.queueId_ref()),
          static_cast<DSCP>(dscpValue));
    }
  }

  if (auto qosMap = qosPolicy.qosMap_ref()) {
    DscpMap dscpMap(*qosMap->dscpMaps_ref());
    ExpMap expMap(*qosMap->expMaps_ref());
    QosPolicy::TrafficClassToQueueId trafficClassToQueueId;
    std::optional<QosPolicy::PfcPriorityToQueueId> pfcPriorityToQueueId;

    for (auto cfgTrafficClassToQueueId : *qosMap->trafficClassToQueueId_ref()) {
      trafficClassToQueueId.emplace(
          cfgTrafficClassToQueueId.first, cfgTrafficClassToQueueId.second);
    }

    if (const auto& pfcPriorityMap = qosMap->pfcPriorityToQueueId_ref()) {
      QosPolicy::PfcPriorityToQueueId tmpMap;
      for (const auto& pfcPriorityEntry : *pfcPriorityMap) {
        if (pfcPriorityEntry.first >
            cfg::switch_config_constants::PFC_PRIORITY_VALUE_MAX()) {
          throw FbossError(
              "Invalid pfc priority value. Valid range is 0 to: ",
              cfg::switch_config_constants::PFC_PRIORITY_VALUE_MAX());
        }
        tmpMap.emplace(pfcPriorityEntry.first, pfcPriorityEntry.second);
      }
      pfcPriorityToQueueId = tmpMap;
    }

    auto qosPolicyNew = make_shared<QosPolicy>(
        *qosPolicy.name_ref(),
        dscpMap.empty() ? ingressDscpMap : dscpMap,
        expMap,
        trafficClassToQueueId);

    if (pfcPriorityToQueueId) {
      qosPolicyNew->setPfcPriorityToQueueIdMap(
          std::move(pfcPriorityToQueueId.value()));
    }

    if (const auto& tc2PgIdMap = qosMap->trafficClassToPgId_ref()) {
      QosPolicy::TrafficClassToPgId tc2PgIdTmp;
      for (auto tc2PgIdEntry : *tc2PgIdMap) {
        if (tc2PgIdEntry.second >
            cfg::switch_config_constants::PORT_PG_VALUE_MAX()) {
          throw FbossError(
              "Invalid pg id. Valid range is 0 to: ",
              cfg::switch_config_constants::PORT_PG_VALUE_MAX());
        }
        tc2PgIdTmp.emplace(tc2PgIdEntry.first, tc2PgIdEntry.second);
      }
      qosPolicyNew->setTrafficClassToPgIdMap(std::move(tc2PgIdTmp));
    }

    if (const auto& pfcPriority2PgIdMap = qosMap->pfcPriorityToPgId_ref()) {
      QosPolicy::PfcPriorityToPgId pfcPri2PgId;
      for (const auto& entry : *pfcPriority2PgIdMap) {
        if (entry.first >
            cfg::switch_config_constants::PFC_PRIORITY_VALUE_MAX()) {
          throw FbossError(
              "Invalid pfc priority value. Valid range is 0 to: ",
              cfg::switch_config_constants::PFC_PRIORITY_VALUE_MAX());
        }
        if (entry.second > cfg::switch_config_constants::PORT_PG_VALUE_MAX()) {
          throw FbossError(
              "Invalid pg id. Valid range is 0 to: ",
              cfg::switch_config_constants::PORT_PG_VALUE_MAX());
        }
        pfcPri2PgId.emplace(entry.first, entry.second);
      }
      qosPolicyNew->setPfcPriorityToPgIdMap(std::move(pfcPri2PgId));
    }

    return qosPolicyNew;
  }
  return make_shared<QosPolicy>(
      *qosPolicy.name_ref(),
      ingressDscpMap,
      ExpMap(TrafficClassToQosAttributeMap<EXP>()),
      QosPolicy::TrafficClassToQueueId());
}

std::shared_ptr<AclTableGroupMap> ThriftConfigApplier::updateAclTableGroups() {
  auto origAclTableGroups = orig_->getAclTableGroups();
  AclTableGroupMap::NodeContainer newAclTableGroups;

  if (!cfg_->aclTableGroup_ref()) {
    throw FbossError(
        "ACL Table Group must be specified if Multiple ACL Table support is enabled");
  }

  if (cfg_->aclTableGroup_ref()->stage_ref() != cfg::AclStage::INGRESS) {
    throw FbossError("Only ACL Stage INGRESS is supported");
  }

  auto origAclTableGroup =
      origAclTableGroups->getAclTableGroupIf(cfg::AclStage::INGRESS);

  auto newAclTableGroup = updateAclTableGroup(
      *cfg_->aclTableGroup_ref()->stage_ref(), origAclTableGroup);
  auto changed =
      updateMap(&newAclTableGroups, origAclTableGroup, newAclTableGroup);

  if (!changed) {
    return nullptr;
  }

  return origAclTableGroups->clone(std::move(newAclTableGroups));
}

std::shared_ptr<AclTableGroup> ThriftConfigApplier::updateAclTableGroup(
    cfg::AclStage aclStage,
    const std::shared_ptr<AclTableGroup>& origAclTableGroup) {
  auto newAclTableMap = std::make_shared<AclTableMap>();
  bool changed = false;
  int numExistingTablesProcessed = 0;

  auto aclByName = getAllAclsByName();
  // Check for controlPlane traffic acls
  if (cfg_->cpuTrafficPolicy_ref() &&
      cfg_->cpuTrafficPolicy_ref()->trafficPolicy_ref()) {
    checkTrafficPolicyAclsExistInConfig(
        *cfg_->cpuTrafficPolicy_ref()->trafficPolicy_ref(), aclByName);
  }
  // Check for dataPlane traffic acls
  if (auto dataPlaneTrafficPolicy = cfg_->dataPlaneTrafficPolicy_ref()) {
    checkTrafficPolicyAclsExistInConfig(*dataPlaneTrafficPolicy, aclByName);
  }

  // For each table in the config, update the table entries and priority
  for (const auto& aclTable : *cfg_->aclTableGroup_ref()->aclTables_ref()) {
    auto newTable =
        updateAclTable(aclStage, aclTable, &numExistingTablesProcessed);
    if (newTable) {
      changed = true;
      newAclTableMap->addTable(newTable);
    } else {
      newAclTableMap->addTable(orig_->getAclTableGroups()
                                   ->getAclTableGroup(aclStage)
                                   ->getAclTableMap()
                                   ->getTableIf(*(aclTable.name_ref())));
    }
  }

  if (!origAclTableGroup ||
      (origAclTableGroup->getAclTableMap() &&
       (numExistingTablesProcessed !=
        origAclTableGroup->getAclTableMap()->numTables()))) {
    changed = true;
  }

  if (!changed) {
    return nullptr;
  }

  auto newAclTableGroup =
      std::make_shared<AclTableGroup>(*cfg_->aclTableGroup_ref()->stage_ref());
  newAclTableGroup->setAclTableMap(newAclTableMap);
  newAclTableGroup->setName(*cfg_->aclTableGroup_ref()->name_ref());

  return newAclTableGroup;
}

flat_map<std::string, const cfg::AclEntry*>
ThriftConfigApplier::getAllAclsByName() {
  flat_map<std::string, const cfg::AclEntry*> aclByName;
  for (const auto& aclTable : *cfg_->aclTableGroup_ref()->aclTables_ref()) {
    auto aclEntries = *(aclTable.aclEntries_ref());
    folly::gen::from(aclEntries) |
        folly::gen::map([](const cfg::AclEntry& acl) {
          return std::make_pair(*acl.name_ref(), &acl);
        }) |
        folly::gen::appendTo(aclByName);
  }
  return aclByName;
}

void ThriftConfigApplier::checkTrafficPolicyAclsExistInConfig(
    const cfg::TrafficPolicyConfig& policy,
    flat_map<std::string, const cfg::AclEntry*> aclByName) {
  for (const auto& mta : *policy.matchToAction_ref()) {
    auto a = aclByName.find(*mta.matcher_ref());
    if (a == aclByName.end()) {
      throw FbossError(
          "Invalid config: No acl named ", *mta.matcher_ref(), " found.");
    }
  }
}

std::shared_ptr<AclTable> ThriftConfigApplier::updateAclTable(
    cfg::AclStage aclStage,
    const cfg::AclTable& configTable,
    int* numExistingTablesProcessed) {
  auto tableName = *configTable.name_ref();
  std::shared_ptr<AclTable> origTable;
  if (orig_->getAclTableGroups() &&
      orig_->getAclTableGroups()->getAclTableGroupIf(aclStage) &&
      orig_->getAclTableGroups()
          ->getAclTableGroup(aclStage)
          ->getAclTableMap()) {
    origTable = orig_->getAclTableGroups()
                    ->getAclTableGroup(aclStage)
                    ->getAclTableMap()
                    ->getTableIf(tableName);
  }

  auto newTableEntries = updateAcls(
      aclStage, *(configTable.aclEntries_ref()), std::make_optional(tableName));
  auto newTablePriority = *configTable.priority_ref();
  std::vector<cfg::AclTableActionType> newActionTypes =
      *configTable.actionTypes_ref();
  std::vector<cfg::AclTableQualifier> newQualifiers =
      *configTable.qualifiers_ref();

  if (origTable) {
    ++(*numExistingTablesProcessed);
    if (!newTableEntries && newTablePriority == origTable->getPriority() &&
        newActionTypes == origTable->getActionTypes() &&
        newQualifiers == origTable->getQualifiers()) {
      // Original table exists with same attributes.
      return nullptr;
    }
  }

  auto newTable = std::make_shared<AclTable>(newTablePriority, tableName);
  if (newTableEntries) {
    // Entries changed from original table or original table does not exist
    newTable->setAclMap(newTableEntries);
  } else if (origTable) {
    // entries are unchanged from original table
    newTable->setAclMap(origTable->getAclMap());
  } else {
    // original table does not exist, and new table is empty
    newTable->setAclMap(std::make_shared<AclMap>());
  }

  newTable->setActionTypes(newActionTypes);
  newTable->setQualifiers(newQualifiers);

  return newTable;
}

std::shared_ptr<AclMap> ThriftConfigApplier::updateAcls(
    cfg::AclStage aclStage,
    std::vector<cfg::AclEntry> configEntries,
    std::optional<std::string> tableName) {
  AclMap::NodeContainer newAcls;
  bool changed = false;
  int numExistingProcessed = 0;
  int priority = kAclStartPriority;
  int cpuPriority = 1;

  // Start with the DROP acls, these should have highest priority
  auto acls = folly::gen::from(configEntries) |
      folly::gen::filter([](const cfg::AclEntry& entry) {
                return *entry.actionType_ref() == cfg::AclActionType::DENY;
              }) |
      folly::gen::map([&](const cfg::AclEntry& entry) {
                auto acl = updateAcl(
                    aclStage,
                    entry,
                    priority++,
                    &numExistingProcessed,
                    &changed,
                    tableName);
                return std::make_pair(acl->getID(), acl);
              }) |
      folly::gen::appendTo(newAcls);

  // Let's get a map of acls to name so we don't have to search the acl list
  // for every new use
  flat_map<std::string, const cfg::AclEntry*> aclByName;
  folly::gen::from(configEntries) |
      folly::gen::map([](const cfg::AclEntry& acl) {
        return std::make_pair(*acl.name_ref(), &acl);
      }) |
      folly::gen::appendTo(aclByName);

  flat_map<std::string, const cfg::TrafficCounter*> counterByName;
  folly::gen::from(*cfg_->trafficCounters_ref()) |
      folly::gen::map([](const cfg::TrafficCounter& counter) {
        return std::make_pair(*counter.name_ref(), &counter);
      }) |
      folly::gen::appendTo(counterByName);

  // Generates new acls from template
  auto addToAcls = [&](const cfg::TrafficPolicyConfig& policy,
                       bool isCoppAcl = false)
      -> const std::vector<std::pair<std::string, std::shared_ptr<AclEntry>>> {
    std::vector<std::pair<std::string, std::shared_ptr<AclEntry>>> entries;
    for (const auto& mta : *policy.matchToAction_ref()) {
      auto a = aclByName.find(*mta.matcher_ref());
      if (a != aclByName.end()) {
        auto aclCfg = *(a->second);

        // We've already added any DENY acls
        if (*aclCfg.actionType_ref() == cfg::AclActionType::DENY) {
          continue;
        }

        // Here is sending to regular port queue action
        MatchAction matchAction = MatchAction();
        if (auto sendToQueue = mta.action_ref()->sendToQueue_ref()) {
          matchAction.setSendToQueue(std::make_pair(*sendToQueue, isCoppAcl));
        }
        if (auto actionCounter = mta.action_ref()->counter_ref()) {
          auto counter = counterByName.find(*actionCounter);
          if (counter == counterByName.end()) {
            throw FbossError(
                "Invalid config: No counter named ",
                *mta.action_ref()->counter_ref(),
                " found.");
          }
          matchAction.setTrafficCounter(*(counter->second));
        }
        if (auto setDscp = mta.action_ref()->setDscp_ref()) {
          matchAction.setSetDscp(*setDscp);
        }
        if (auto ingressMirror = mta.action_ref()->ingressMirror_ref()) {
          matchAction.setIngressMirror(*ingressMirror);
        }
        if (auto egressMirror = mta.action_ref()->egressMirror_ref()) {
          matchAction.setEgressMirror(*egressMirror);
        }
        if (auto toCpuAction = mta.action_ref()->toCpuAction_ref()) {
          matchAction.setToCpuAction(*toCpuAction);
        }

        auto acl = updateAcl(
            aclStage,
            aclCfg,
            isCoppAcl ? cpuPriority++ : priority++,
            &numExistingProcessed,
            &changed,
            tableName,
            &matchAction);

        if (acl->getAclAction().has_value()) {
          const auto& inMirror = acl->getAclAction().value().getIngressMirror();
          const auto& egMirror = acl->getAclAction().value().getIngressMirror();
          if (inMirror.has_value() &&
              !new_->getMirrors()->getMirrorIf(inMirror.value())) {
            throw FbossError("Mirror ", inMirror.value(), " is undefined");
          }
          if (egMirror.has_value() &&
              !new_->getMirrors()->getMirrorIf(egMirror.value())) {
            throw FbossError("Mirror ", egMirror.value(), " is undefined");
          }
        }
        entries.push_back(std::make_pair(acl->getID(), acl));
      }
    }
    return entries;
  };

  // Add controlPlane traffic acls
  if (cfg_->cpuTrafficPolicy_ref() &&
      cfg_->cpuTrafficPolicy_ref()->trafficPolicy_ref()) {
    folly::gen::from(
        addToAcls(*cfg_->cpuTrafficPolicy_ref()->trafficPolicy_ref(), true)) |
        folly::gen::appendTo(newAcls);
  }

  // Add dataPlane traffic acls
  if (auto dataPlaneTrafficPolicy = cfg_->dataPlaneTrafficPolicy_ref()) {
    folly::gen::from(addToAcls(*dataPlaneTrafficPolicy)) |
        folly::gen::appendTo(newAcls);
  }

  if (FLAGS_enable_acl_table_group) {
    if (orig_->getAclsForTable(aclStage, tableName.value()) &&
        numExistingProcessed !=
            orig_->getAclsForTable(aclStage, tableName.value())->size()) {
      // Some existing ACLs were removed from the table (multiple acl tables
      // implementation).
      changed = true;
    }
  } else {
    if (numExistingProcessed != orig_->getAcls()->size()) {
      // Some existing ACLs were removed (single acl table implementation).
      changed = true;
    }
  }

  if (!changed) {
    return nullptr;
  }

  if (tableName.has_value() &&
      orig_->getAclsForTable(aclStage, tableName.value())) {
    return orig_->getAclsForTable(aclStage, tableName.value())
        ->clone(std::move(newAcls));
  }

  return orig_->getAcls()->clone(std::move(newAcls));
}

std::shared_ptr<AclEntry> ThriftConfigApplier::updateAcl(
    cfg::AclStage aclStage,
    const cfg::AclEntry& acl,
    int priority,
    int* numExistingProcessed,
    bool* changed,
    std::optional<std::string> tableName,
    const MatchAction* action) {
  std::shared_ptr<AclEntry> origAcl;

  if (FLAGS_enable_acl_table_group) { // multiple acl tables implementation
    CHECK(tableName.has_value());

    if (orig_->getAclsForTable(aclStage, tableName.value())) {
      origAcl = orig_->getAclsForTable(aclStage, tableName.value())
                    ->getEntryIf(*acl.name_ref());
    }
  } else { // single acl table implementation
    CHECK(!tableName.has_value());
    origAcl = orig_->getAcls()->getEntryIf(
        *acl.name_ref()); // orig_ empty in coldboot, or comes from
                          // follydynamic in warmboot
  }

  auto newAcl =
      createAcl(&acl, priority, action); // new always comes from config

  if (origAcl) {
    ++(*numExistingProcessed);
    if (*origAcl == *newAcl) {
      return origAcl;
    }
  }

  *changed = true;
  return newAcl;
}

void ThriftConfigApplier::checkAcl(const cfg::AclEntry* config) const {
  // check l4 port
  if (auto l4SrcPort = config->l4SrcPort_ref()) {
    if (*l4SrcPort < 0 || *l4SrcPort > AclEntryFields::kMaxL4Port) {
      throw FbossError(
          "L4 source port must be between 0 and ",
          std::to_string(AclEntryFields::kMaxL4Port));
    }
  }
  if (auto l4DstPort = config->l4DstPort_ref()) {
    if (*l4DstPort < 0 || *l4DstPort > AclEntryFields::kMaxL4Port) {
      throw FbossError(
          "L4 destination port must be between 0 and ",
          std::to_string(AclEntryFields::kMaxL4Port));
    }
  }
  if (config->icmpCode_ref() && !config->icmpType_ref()) {
    throw FbossError("icmp type must be set when icmp code is set");
  }
  if (auto icmpType = config->icmpType_ref()) {
    if (*icmpType < 0 || *icmpType > AclEntryFields::kMaxIcmpType) {
      throw FbossError(
          "icmp type value must be between 0 and ",
          std::to_string(AclEntryFields::kMaxIcmpType));
    }
  }
  if (auto icmpCode = config->icmpCode_ref()) {
    if (*icmpCode < 0 || *icmpCode > AclEntryFields::kMaxIcmpCode) {
      throw FbossError(
          "icmp type value must be between 0 and ",
          std::to_string(AclEntryFields::kMaxIcmpCode));
    }
  }
  if (config->icmpType_ref() &&
      (!config->proto_ref() ||
       !(*config->proto_ref() == AclEntryFields::kProtoIcmp ||
         *config->proto_ref() == AclEntryFields::kProtoIcmpv6))) {
    throw FbossError(
        "proto must be either icmp or icmpv6 ", "if icmp type is set");
  }
  if (auto ttl = config->ttl_ref()) {
    if (auto ttlValue = *ttl->value_ref()) {
      if (ttlValue > 255) {
        throw FbossError("ttl value is larger than 255");
      }
      if (ttlValue < 0) {
        throw FbossError("ttl value is less than 0");
      }
    }
    if (auto ttlMask = *ttl->mask_ref()) {
      if (ttlMask > 255) {
        throw FbossError("ttl mask is larger than 255");
      }
      if (ttlMask < 0) {
        throw FbossError("ttl mask is less than 0");
      }
    }
  }
}

shared_ptr<AclEntry> ThriftConfigApplier::createAcl(
    const cfg::AclEntry* config,
    int priority,
    const MatchAction* action) {
  checkAcl(config);
  auto newAcl = make_shared<AclEntry>(priority, *config->name_ref());
  newAcl->setActionType(*config->actionType_ref());
  if (action) {
    newAcl->setAclAction(*action);
  }
  if (auto srcIp = config->srcIp_ref()) {
    newAcl->setSrcIp(IPAddress::createNetwork(*srcIp));
  }
  if (auto dstIp = config->dstIp_ref()) {
    newAcl->setDstIp(IPAddress::createNetwork(*dstIp));
  }
  if (auto proto = config->proto_ref()) {
    newAcl->setProto(*proto);
  }
  if (auto tcpFlagsBitMap = config->tcpFlagsBitMap_ref()) {
    newAcl->setTcpFlagsBitMap(*tcpFlagsBitMap);
  }
  if (auto srcPort = config->srcPort_ref()) {
    newAcl->setSrcPort(*srcPort);
  }
  if (auto dstPort = config->dstPort_ref()) {
    newAcl->setDstPort(*dstPort);
  }
  if (auto l4SrcPort = config->l4SrcPort_ref()) {
    newAcl->setL4SrcPort(*l4SrcPort);
  }
  if (auto l4DstPort = config->l4DstPort_ref()) {
    newAcl->setL4DstPort(*l4DstPort);
  }
  if (auto ipFrag = config->ipFrag_ref()) {
    newAcl->setIpFrag(*ipFrag);
  }
  if (auto icmpType = config->icmpType_ref()) {
    newAcl->setIcmpType(*icmpType);
  }
  if (auto icmpCode = config->icmpCode_ref()) {
    newAcl->setIcmpCode(*icmpCode);
  }
  if (auto dscp = config->dscp_ref()) {
    newAcl->setDscp(*dscp);
  }
  if (auto dstMac = config->dstMac_ref()) {
    newAcl->setDstMac(MacAddress(*dstMac));
  }
  if (auto ipType = config->ipType_ref()) {
    newAcl->setIpType(*ipType);
  }
  if (auto ttl = config->ttl_ref()) {
    newAcl->setTtl(AclTtl(*ttl->value_ref(), *ttl->mask_ref()));
  }
  if (auto lookupClassL2 = config->lookupClassL2_ref()) {
    newAcl->setLookupClassL2(*lookupClassL2);
  }
  if (auto lookupClassNeighbor = config->lookupClassNeighbor_ref()) {
    newAcl->setLookupClassNeighbor(*lookupClassNeighbor);
  }
  if (auto lookupClassRoute = config->lookupClassRoute_ref()) {
    newAcl->setLookupClassRoute(*lookupClassRoute);
  }
  if (auto packetLookupResult = config->packetLookupResult_ref()) {
    newAcl->setPacketLookupResult(*packetLookupResult);
  }
  return newAcl;
}

bool ThriftConfigApplier::updateDhcpOverrides(
    Vlan* vlan,
    const cfg::Vlan* config) {
  DhcpV4OverrideMap newDhcpV4OverrideMap;
  if (config->dhcpRelayOverridesV4_ref()) {
    for (const auto& pair : *config->dhcpRelayOverridesV4_ref()) {
      try {
        newDhcpV4OverrideMap[MacAddress(pair.first)] = IPAddressV4(pair.second);
      } catch (const IPAddressFormatException& ex) {
        throw FbossError(
            "Invalid IPv4 address in DHCPv4 relay override map: ", ex.what());
      }
    }
  }

  DhcpV6OverrideMap newDhcpV6OverrideMap;
  if (config->dhcpRelayOverridesV6_ref()) {
    for (const auto& pair : *config->dhcpRelayOverridesV6_ref()) {
      try {
        newDhcpV6OverrideMap[MacAddress(pair.first)] = IPAddressV6(pair.second);
      } catch (const IPAddressFormatException& ex) {
        throw FbossError(
            "Invalid IPv4 address in DHCPv4 relay override map: ", ex.what());
      }
    }
  }

  bool changed = false;
  auto oldDhcpV4OverrideMap = vlan->getDhcpV4RelayOverrides();
  if (oldDhcpV4OverrideMap != newDhcpV4OverrideMap) {
    vlan->setDhcpV4RelayOverrides(newDhcpV4OverrideMap);
    changed = true;
  }
  auto oldDhcpV6OverrideMap = vlan->getDhcpV6RelayOverrides();
  if (oldDhcpV6OverrideMap != newDhcpV6OverrideMap) {
    vlan->setDhcpV6RelayOverrides(newDhcpV6OverrideMap);
    changed = true;
  }
  return changed;
}

bool ThriftConfigApplier::updateNeighborResponseTables(
    Vlan* vlan,
    const cfg::Vlan* config) {
  auto origArp = vlan->getArpResponseTable();
  auto origNdp = vlan->getNdpResponseTable();
  ArpResponseTable::Table arpTable;
  NdpResponseTable::Table ndpTable;

  VlanID vlanID(*config->id_ref());
  auto it = vlanInterfaces_.find(vlanID);
  if (it != vlanInterfaces_.end()) {
    for (const auto& addrInfo : it->second.addresses) {
      NeighborResponseEntry entry(
          addrInfo.second.mac, addrInfo.second.interfaceID);
      if (addrInfo.first.isV4()) {
        arpTable.insert(std::make_pair(addrInfo.first.asV4(), entry));
      } else {
        ndpTable.insert(std::make_pair(addrInfo.first.asV6(), entry));
      }
    }
  }

  bool changed = false;
  if (origArp->getTable() != arpTable) {
    changed = true;
    vlan->setArpResponseTable(origArp->clone(std::move(arpTable)));
  }
  if (origNdp->getTable() != ndpTable) {
    changed = true;
    vlan->setNdpResponseTable(origNdp->clone(std::move(ndpTable)));
  }
  return changed;
}

std::shared_ptr<InterfaceMap> ThriftConfigApplier::updateInterfaces() {
  auto origIntfs = orig_->getInterfaces();
  InterfaceMap::NodeContainer newIntfs;
  bool changed = false;

  // Process all supplied interface configs
  size_t numExistingProcessed = 0;

  for (const auto& interfaceCfg : *cfg_->interfaces_ref()) {
    InterfaceID id(*interfaceCfg.intfID_ref());
    auto origIntf = origIntfs->getInterfaceIf(id);
    shared_ptr<Interface> newIntf;
    auto newAddrs = getInterfaceAddresses(&interfaceCfg);
    if (origIntf) {
      newIntf = updateInterface(origIntf, &interfaceCfg, newAddrs);
      ++numExistingProcessed;
    } else {
      newIntf = createInterface(&interfaceCfg, newAddrs);
    }
    updateVlanInterfaces(newIntf ? newIntf.get() : origIntf.get());
    changed |= updateMap(&newIntfs, origIntf, newIntf);
  }

  if (numExistingProcessed != origIntfs->size()) {
    // Some existing interfaces were removed.
    CHECK_LT(numExistingProcessed, origIntfs->size());
    changed = true;
  }

  if (!changed) {
    return nullptr;
  }

  return origIntfs->clone(std::move(newIntfs));
}

shared_ptr<Interface> ThriftConfigApplier::createInterface(
    const cfg::Interface* config,
    const Interface::Addresses& addrs) {
  auto name = getInterfaceName(config);
  auto mac = getInterfaceMac(config);
  auto mtu = config->mtu_ref().value_or(Interface::kDefaultMtu);
  auto intf = make_shared<Interface>(
      InterfaceID(*config->intfID_ref()),
      RouterID(*config->routerID_ref()),
      VlanID(*config->vlanID_ref()),
      name,
      mac,
      mtu,
      *config->isVirtual_ref(),
      *config->isStateSyncDisabled_ref());
  intf->setAddresses(addrs);
  if (auto ndp = config->ndp_ref()) {
    intf->setNdpConfig(*ndp);
  }
  return intf;
}

shared_ptr<SflowCollectorMap> ThriftConfigApplier::updateSflowCollectors() {
  auto origCollectors = orig_->getSflowCollectors();
  SflowCollectorMap::NodeContainer newCollectors;
  bool changed = false;

  // Process all supplied collectors
  size_t numExistingProcessed = 0;
  for (const auto& collector : *cfg_->sFlowCollectors_ref()) {
    folly::IPAddress address(*collector.ip_ref());
    auto id = address.toFullyQualified() + ':' +
        folly::to<std::string>(*collector.port_ref());
    auto origCollector = origCollectors->getNodeIf(id);
    shared_ptr<SflowCollector> newCollector;

    if (origCollector) {
      newCollector = updateSflowCollector(origCollector, &collector);
      ++numExistingProcessed;
    } else {
      newCollector = createSflowCollector(&collector);
    }
    changed |= updateMap(&newCollectors, origCollector, newCollector);
  }

  if (numExistingProcessed != origCollectors->size()) {
    // Some existing SflowCollectors were removed.
    CHECK_LT(numExistingProcessed, origCollectors->size());
    changed = true;
  }

  if (!changed) {
    return nullptr;
  }

  return origCollectors->clone(std::move(newCollectors));
}

shared_ptr<SflowCollector> ThriftConfigApplier::createSflowCollector(
    const cfg::SflowCollector* config) {
  return make_shared<SflowCollector>(*config->ip_ref(), *config->port_ref());
}

shared_ptr<SflowCollector> ThriftConfigApplier::updateSflowCollector(
    const shared_ptr<SflowCollector>& orig,
    const cfg::SflowCollector* config) {
  auto newCollector = createSflowCollector(config);

  if (orig->getAddress() == newCollector->getAddress()) {
    return nullptr;
  }

  return newCollector;
}

shared_ptr<Interface> ThriftConfigApplier::updateInterface(
    const shared_ptr<Interface>& orig,
    const cfg::Interface* config,
    const Interface::Addresses& addrs) {
  CHECK_EQ(orig->getID(), *config->intfID_ref());

  cfg::NdpConfig ndp;
  if (config->ndp_ref()) {
    ndp = *config->ndp_ref();
  }
  auto name = getInterfaceName(config);
  auto mac = getInterfaceMac(config);
  auto mtu = config->mtu_ref().value_or(Interface::kDefaultMtu);
  if (orig->getRouterID() == RouterID(*config->routerID_ref()) &&
      orig->getVlanID() == VlanID(*config->vlanID_ref()) &&
      orig->getName() == name && orig->getMac() == mac &&
      orig->getAddresses() == addrs && orig->getNdpConfig() == ndp &&
      orig->getMtu() == mtu && orig->isVirtual() == *config->isVirtual_ref() &&
      orig->isStateSyncDisabled() == *config->isStateSyncDisabled_ref()) {
    // No change
    return nullptr;
  }

  auto newIntf = orig->clone();
  newIntf->setRouterID(RouterID(*config->routerID_ref()));
  newIntf->setVlanID(VlanID(*config->vlanID_ref()));
  newIntf->setName(name);
  newIntf->setMac(mac);
  newIntf->setAddresses(addrs);
  newIntf->setNdpConfig(ndp);
  newIntf->setMtu(mtu);
  newIntf->setIsVirtual(*config->isVirtual_ref());
  newIntf->setIsStateSyncDisabled(*config->isStateSyncDisabled_ref());
  return newIntf;
}

shared_ptr<QcmCfg> ThriftConfigApplier::createQcmCfg(
    const cfg::QcmConfig& config) {
  auto newQcmCfg = make_shared<QcmCfg>();

  WeightMap newWeightMap;
  for (const auto& weights : *config.flowWeights_ref()) {
    newWeightMap.emplace(static_cast<int>(weights.first), weights.second);
  }
  newQcmCfg->setFlowWeightMap(newWeightMap);

  Port2QosQueueIdMap port2QosQueueIds;
  for (const auto& perPortQosQueueIds : *config.port2QosQueueIds_ref()) {
    std::set<int> queueIds;
    for (const auto& queueId : perPortQosQueueIds.second) {
      queueIds.insert(queueId);
    }
    port2QosQueueIds[perPortQosQueueIds.first] = queueIds;
  }
  newQcmCfg->setPort2QosQueueIdMap(port2QosQueueIds);

  newQcmCfg->setCollectorSrcPort(*config.collectorSrcPort_ref());
  newQcmCfg->setNumFlowSamplesPerView(*config.numFlowSamplesPerView_ref());
  newQcmCfg->setFlowLimit(*config.flowLimit_ref());
  newQcmCfg->setNumFlowsClear(*config.numFlowsClear_ref());
  newQcmCfg->setScanIntervalInUsecs(*config.scanIntervalInUsecs_ref());
  newQcmCfg->setExportThreshold(*config.exportThreshold_ref());
  newQcmCfg->setAgingInterval(*config.agingIntervalInMsecs_ref());
  newQcmCfg->setCollectorDstIp(
      IPAddress::createNetwork(*config.collectorDstIp_ref()));
  newQcmCfg->setCollectorDstPort(*config.collectorDstPort_ref());
  newQcmCfg->setMonitorQcmCfgPortsOnly(*config.monitorQcmCfgPortsOnly_ref());
  if (auto dscp = config.collectorDscp_ref()) {
    newQcmCfg->setCollectorDscp(*dscp);
  }
  if (auto ppsToQcm = config.ppsToQcm_ref()) {
    newQcmCfg->setPpsToQcm(*ppsToQcm);
  }
  newQcmCfg->setCollectorSrcIp(
      IPAddress::createNetwork(*config.collectorSrcIp_ref()));
  newQcmCfg->setMonitorQcmPortList(*config.monitorQcmPortList_ref());
  return newQcmCfg;
}

shared_ptr<BufferPoolCfgMap> ThriftConfigApplier::updateBufferPoolConfigs(
    bool* changed) {
  *changed = false;
  auto origBufferPoolConfigs = orig_->getBufferPoolCfgs();
  BufferPoolCfgMap::NodeContainer newBufferPoolConfigMap;
  auto newCfgedBufferPools = cfg_->bufferPoolConfigs_ref();

  if (!newCfgedBufferPools && !origBufferPoolConfigs) {
    return nullptr;
  }

  if (!newCfgedBufferPools && origBufferPoolConfigs) {
    // old cfg eixists but new one doesn't
    *changed = true;
    return nullptr;
  }

  if (newCfgedBufferPools && !origBufferPoolConfigs) {
    *changed = true;
  }

  // if old/new cfgs are present, compare size
  if (origBufferPoolConfigs &&
      (*origBufferPoolConfigs).size() != (*newCfgedBufferPools).size()) {
    *changed = true;
  }

  // origBufferPoolConfigs, newBufferPoolConfigs both are configured
  // and with with same size
  // check if there is any upate on it when compared
  // with last one
  for (auto& bufferPoolConfig : *newCfgedBufferPools) {
    auto newBufferPoolConfig =
        createBufferPoolConfig(bufferPoolConfig.first, bufferPoolConfig.second);
    if (origBufferPoolConfigs) {
      // if buffer pool cfg map exist, check if the specific buffer pool cfg
      // exists or not
      auto origBufferPoolConfig =
          origBufferPoolConfigs->getNodeIf(bufferPoolConfig.first);
      if (!origBufferPoolConfig ||
          (*origBufferPoolConfig != *newBufferPoolConfig)) {
        /* new entry added or existing entries do not match */
        *changed = true;
      }
    }
    newBufferPoolConfigMap.emplace(
        std::make_pair(bufferPoolConfig.first, newBufferPoolConfig));
  }

  if (*changed) {
    return origBufferPoolConfigs
        ? origBufferPoolConfigs->clone(std::move(newBufferPoolConfigMap))
        : std::make_shared<BufferPoolCfgMap>(std::move(newBufferPoolConfigMap));
  }
  return nullptr;
}

std::shared_ptr<BufferPoolCfg> ThriftConfigApplier::createBufferPoolConfig(
    const std::string& id,
    const cfg::BufferPoolConfig& bufferPoolConfig) {
  auto bufferPoolCfg = std::make_shared<BufferPoolCfg>(id);
  bufferPoolCfg->setSharedBytes(*bufferPoolConfig.sharedBytes_ref());
  bufferPoolCfg->setHeadroomBytes(*bufferPoolConfig.headroomBytes_ref());
  return bufferPoolCfg;
}

shared_ptr<QcmCfg> ThriftConfigApplier::updateQcmCfg(bool* changed) {
  auto origQcmConfig = orig_->getQcmCfg();
  if (!cfg_->qcmConfig_ref()) {
    if (origQcmConfig) {
      // going from cfg to empty
      *changed = true;
    }
    return nullptr;
  }
  auto newQcmCfg = createQcmCfg(*cfg_->qcmConfig_ref());
  if (origQcmConfig && (*origQcmConfig == *newQcmCfg)) {
    return nullptr;
  }
  *changed = true;
  return newQcmCfg;
}

shared_ptr<SwitchSettings> ThriftConfigApplier::updateSwitchSettings() {
  auto origSwitchSettings = orig_->getSwitchSettings();
  bool switchSettingsChange = false;
  auto newSwitchSettings = origSwitchSettings->clone();

  if (origSwitchSettings->getL2LearningMode() !=
      *cfg_->switchSettings_ref()->l2LearningMode_ref()) {
    newSwitchSettings->setL2LearningMode(
        *cfg_->switchSettings_ref()->l2LearningMode_ref());
    switchSettingsChange = true;
  }

  if (origSwitchSettings->isQcmEnable() !=
      *cfg_->switchSettings_ref()->qcmEnable_ref()) {
    newSwitchSettings->setQcmEnable(
        *cfg_->switchSettings_ref()->qcmEnable_ref());
    switchSettingsChange = true;
  }

  if (origSwitchSettings->isPtpTcEnable() !=
      *cfg_->switchSettings_ref()->ptpTcEnable_ref()) {
    newSwitchSettings->setPtpTcEnable(
        *cfg_->switchSettings_ref()->ptpTcEnable_ref());
    switchSettingsChange = true;
  }

  if (origSwitchSettings->getL2AgeTimerSeconds() !=
      *cfg_->switchSettings_ref()->l2AgeTimerSeconds_ref()) {
    newSwitchSettings->setL2AgeTimerSeconds(
        *cfg_->switchSettings_ref()->l2AgeTimerSeconds_ref());
    switchSettingsChange = true;
  }

  if (origSwitchSettings->getMaxRouteCounterIDs() !=
      *cfg_->switchSettings_ref()->maxRouteCounterIDs_ref()) {
    newSwitchSettings->setMaxRouteCounterIDs(
        *cfg_->switchSettings_ref()->maxRouteCounterIDs_ref());
    switchSettingsChange = true;
  }

  std::vector<std::pair<VlanID, folly::IPAddress>> cfgBlockNeighbors;
  for (const auto& blockNeighbor :
       *cfg_->switchSettings_ref()->blockNeighbors_ref()) {
    cfgBlockNeighbors.emplace_back(
        VlanID(*blockNeighbor.vlanID_ref()),
        folly::IPAddress(*blockNeighbor.ipAddress_ref()));
  }
  if (origSwitchSettings->getBlockNeighbors() != cfgBlockNeighbors) {
    newSwitchSettings->setBlockNeighbors(cfgBlockNeighbors);
    switchSettingsChange = true;
  }

  return switchSettingsChange ? newSwitchSettings : nullptr;
}

shared_ptr<ControlPlane> ThriftConfigApplier::updateControlPlane() {
  auto origCPU = orig_->getControlPlane();
  std::optional<std::string> qosPolicy;
  ControlPlane::RxReasonToQueue newRxReasonToQueue;
  bool rxReasonToQueueUnchanged = true;
  if (auto cpuTrafficPolicy = cfg_->cpuTrafficPolicy_ref()) {
    if (auto trafficPolicy = cpuTrafficPolicy->trafficPolicy_ref()) {
      if (auto defaultQosPolicy = trafficPolicy->defaultQosPolicy_ref()) {
        qosPolicy = *defaultQosPolicy;
      }
    }
    if (const auto rxReasonToQueue =
            cpuTrafficPolicy->rxReasonToQueueOrderedList_ref()) {
      for (auto rxEntry : *rxReasonToQueue) {
        newRxReasonToQueue.push_back(rxEntry);
      }
      if (newRxReasonToQueue != origCPU->getRxReasonToQueue()) {
        rxReasonToQueueUnchanged = false;
      }
    } else if (
        const auto rxReasonToQueue =
            cpuTrafficPolicy->rxReasonToCPUQueue_ref()) {
      // TODO(pgardideh): the map version of reason to queue is deprecated.
      // Remove
      // this read when it is safe to do so.
      for (auto rxEntry : *rxReasonToQueue) {
        newRxReasonToQueue.push_back(ControlPlane::makeRxReasonToQueueEntry(
            rxEntry.first, rxEntry.second));
      }
      if (newRxReasonToQueue != origCPU->getRxReasonToQueue()) {
        rxReasonToQueueUnchanged = false;
      }
    }
  } else {
    /*
     * if cpuTrafficPolicy is not configured default to dataPlaneTrafficPolicy
     * default i.e. with regards to QoS map configuration, treat CPU port like
     * any front panel port.
     */
    if (auto dataPlaneTrafficPolicy = cfg_->dataPlaneTrafficPolicy_ref()) {
      if (auto defaultDataPlaneQosPolicy =
              dataPlaneTrafficPolicy->defaultQosPolicy_ref()) {
        qosPolicy = *defaultDataPlaneQosPolicy;
      }
    }
  }

  bool qosPolicyUnchanged = qosPolicy == origCPU->getQosPolicy();

  std::optional<cfg::QosMap> qosMap;
  if (qosPolicy) {
    bool qosPolicyFound = false;
    for (auto policy : *cfg_->qosPolicies_ref()) {
      if (qosPolicyFound) {
        break;
      }
      qosPolicyFound = (*policy.name_ref() == qosPolicy.value());
      if (qosPolicyFound && policy.qosMap_ref()) {
        qosMap = policy.qosMap_ref().value();
      }
    }
    if (!qosPolicyFound) {
      throw FbossError("qos policy ", qosPolicy.value(), " not found");
    }
  }

  // check whether queue setting changed
  QueueConfig newQueues;
  for (auto streamType : platform_->getAsic()->getQueueStreamTypes(true)) {
    auto tmpPortQueues = updatePortQueues(
        origCPU->getQueues(),
        *cfg_->cpuQueues_ref(),
        origCPU->getQueues().size(),
        streamType,
        qosMap);
    newQueues.insert(
        newQueues.begin(), tmpPortQueues.begin(), tmpPortQueues.end());
  }
  bool queuesUnchanged = newQueues.size() == origCPU->getQueues().size();
  for (int i = 0; i < newQueues.size() && queuesUnchanged; i++) {
    if (*(newQueues.at(i)) != *(origCPU->getQueues().at(i))) {
      queuesUnchanged = false;
      break;
    }
  }

  if (queuesUnchanged && qosPolicyUnchanged && rxReasonToQueueUnchanged) {
    return nullptr;
  }

  auto newCPU = origCPU->clone();
  newCPU->resetQueues(newQueues);
  newCPU->resetQosPolicy(qosPolicy);
  newCPU->resetRxReasonToQueue(newRxReasonToQueue);
  return newCPU;
}

std::string ThriftConfigApplier::getInterfaceName(
    const cfg::Interface* config) {
  if (auto name = config->name_ref()) {
    return *name;
  }
  return folly::to<std::string>("Interface ", *config->intfID_ref());
}

MacAddress ThriftConfigApplier::getInterfaceMac(const cfg::Interface* config) {
  if (auto mac = config->mac_ref()) {
    return MacAddress(*mac);
  }
  return platform_->getLocalMac();
}

Interface::Addresses ThriftConfigApplier::getInterfaceAddresses(
    const cfg::Interface* config) {
  Interface::Addresses addrs;

  // Assign auto-generate v6 link-local address to interface. Config can
  // have more link-local addresses if needed.
  folly::MacAddress macAddr;
  if (auto mac = config->mac_ref()) {
    macAddr = folly::MacAddress(*mac);
  } else {
    macAddr = platform_->getLocalMac();
  }
  const folly::IPAddressV6 v6llAddr(folly::IPAddressV6::LINK_LOCAL, macAddr);
  addrs.emplace(v6llAddr, kV6LinkLocalAddrMask);

  // Add all interface addresses from config
  for (const auto& addr : *config->ipAddresses_ref()) {
    auto intfAddr = IPAddress::createNetwork(addr, -1, false);
    auto ret = addrs.insert(intfAddr);
    if (!ret.second) {
      throw FbossError(
          "Duplicate network IP address ",
          addr,
          " in interface ",
          *config->intfID_ref());
    }

    // NOTE: We do not want to leak link-local address into intfRouteTables_
    // TODO: For now we are allowing v4 LLs to be programmed because they are
    // used within Galaxy for LL routing. This hack should go away once we
    // move BGP sessions over non LL addresses
    if (intfAddr.first.isV6() and intfAddr.first.isLinkLocal()) {
      continue;
    }
    auto ret2 = intfRouteTables_[RouterID(*config->routerID_ref())].emplace(
        IPAddress::createNetwork(addr),
        std::make_pair(InterfaceID(*config->intfID_ref()), intfAddr.first));
    if (!ret2.second) {
      // we get same network, only allow it if that is from the same interface
      auto other = ret2.first->second.first;
      if (other != InterfaceID(*config->intfID_ref())) {
        throw FbossError(
            "Duplicate network address ",
            addr,
            " of interface ",
            *config->intfID_ref(),
            " as interface ",
            other,
            " in VRF ",
            *config->routerID_ref());
      }
      // For consistency with interface routes as added by RouteUpdater,
      // use the last address we see rather than the first. Otherwise,
      // we see pointless route updates on syncFib()
      *ret2.first = std::make_pair(
          IPAddress::createNetwork(addr),
          std::make_pair(InterfaceID(*config->intfID_ref()), intfAddr.first));
    }
  }

  return addrs;
}

std::shared_ptr<MirrorMap> ThriftConfigApplier::updateMirrors() {
  const auto& origMirrors = orig_->getMirrors();
  MirrorMap::NodeContainer newMirrors;
  bool changed = false;

  size_t numExistingProcessed = 0;
  int sflowMirrorCount = 0;
  for (const auto& mirrorCfg : *cfg_->mirrors_ref()) {
    auto origMirror = origMirrors->getMirrorIf(*mirrorCfg.name_ref());
    std::shared_ptr<Mirror> newMirror;
    if (origMirror) {
      newMirror = updateMirror(origMirror, &mirrorCfg);
      ++numExistingProcessed;
    } else {
      newMirror = createMirror(&mirrorCfg);
    }
    if (newMirror) {
      sflowMirrorCount += newMirror->type() == Mirror::Type::SFLOW ? 1 : 0;
    } else {
      sflowMirrorCount += origMirror->type() == Mirror::Type::SFLOW ? 1 : 0;
    }
    if (sflowMirrorCount > 1) {
      throw FbossError("More than one sflow mirrors configured");
    }
    changed |= updateMap(&newMirrors, origMirror, newMirror);
  }

  if (numExistingProcessed != origMirrors->size()) {
    // Some existing Mirrors were removed.
    CHECK_LT(numExistingProcessed, origMirrors->size());
    changed = true;
  }

  for (auto& port : *(new_->getPorts())) {
    auto portInMirror = port->getIngressMirror();
    auto portEgMirror = port->getEgressMirror();
    if (portInMirror.has_value()) {
      auto inMirrorMapEntry = newMirrors.find(portInMirror.value());
      if (inMirrorMapEntry == newMirrors.end()) {
        throw FbossError(
            "Mirror ", portInMirror.value(), " for port is not found");
      }
      if (port->getSampleDestination() &&
          port->getSampleDestination().value() ==
              cfg::SampleDestination::MIRROR &&
          inMirrorMapEntry->second->type() != Mirror::Type::SFLOW) {
        throw FbossError(
            "Ingress mirror ",
            portInMirror.value(),
            " for sampled port ",
            port->getID(),
            " not sflow");
      }
    }
    if (portEgMirror.has_value() &&
        newMirrors.find(portEgMirror.value()) == newMirrors.end()) {
      throw FbossError(
          "Mirror ", portEgMirror.value(), " for port is not found");
    }
  }

  if (!changed) {
    return nullptr;
  }

  return origMirrors->clone(std::move(newMirrors));
}

std::shared_ptr<Mirror> ThriftConfigApplier::createMirror(
    const cfg::Mirror* mirrorConfig) {
  if (!mirrorConfig->destination_ref()->egressPort_ref() &&
      !mirrorConfig->destination_ref()->tunnel_ref()) {
    /*
     * At least one of the egress port or tunnel is needed.
     */
    throw FbossError(
        "Must provide either egressPort or tunnel with endpoint ip for mirror");
  }

  std::optional<PortID> mirrorEgressPort;
  std::optional<folly::IPAddress> destinationIp;
  std::optional<folly::IPAddress> srcIp;
  std::optional<TunnelUdpPorts> udpPorts;

  if (auto egressPort = mirrorConfig->destination_ref()->egressPort_ref()) {
    std::shared_ptr<Port> mirrorToPort{nullptr};
    switch (egressPort->getType()) {
      case cfg::MirrorEgressPort::Type::name:
        for (auto& port : *(new_->getPorts())) {
          if (port->getName() == egressPort->get_name()) {
            mirrorToPort = port;
            break;
          }
        }
        break;
      case cfg::MirrorEgressPort::Type::logicalID:
        mirrorToPort =
            new_->getPorts()->getPortIf(PortID(egressPort->get_logicalID()));
        break;
      case cfg::MirrorEgressPort::Type::__EMPTY__:
        throw FbossError(
            "Must set either name or logicalID for MirrorEgressPort");
    }
    if (mirrorToPort &&
        mirrorToPort->getIngressMirror() != *mirrorConfig->name_ref() &&
        mirrorToPort->getEgressMirror() != *mirrorConfig->name_ref()) {
      mirrorEgressPort = PortID(mirrorToPort->getID());
    } else {
      throw FbossError("Invalid port name or ID");
    }
  }

  if (auto tunnel = mirrorConfig->destination_ref()->tunnel_ref()) {
    if (auto sflowTunnel = tunnel->sflowTunnel_ref()) {
      destinationIp = folly::IPAddress(*sflowTunnel->ip_ref());
      if (!sflowTunnel->udpSrcPort_ref() || !sflowTunnel->udpDstPort_ref()) {
        throw FbossError(
            "Both UDP source and UDP destination ports must be provided for \
            sFlow tunneling.");
      }
      if (destinationIp->isV6() &&
          !platform_->getAsic()->isSupported(HwAsic::Feature::SFLOWv6)) {
        throw FbossError("SFLOWv6 is not supported on this platform");
      }
      udpPorts = TunnelUdpPorts(
          *sflowTunnel->udpSrcPort_ref(), *sflowTunnel->udpDstPort_ref());
    } else if (auto greTunnel = tunnel->greTunnel_ref()) {
      destinationIp = folly::IPAddress(*greTunnel->ip_ref());
      if (destinationIp->isV6() &&
          !platform_->getAsic()->isSupported(HwAsic::Feature::ERSPANv6)) {
        throw FbossError("ERSPANv6 is not supported on this platform");
      }
    }

    if (auto srcIpRef = tunnel->srcIp_ref()) {
      srcIp = folly::IPAddress(*srcIpRef);
    }
  }

  uint8_t dscpMark = mirrorConfig->get_dscp();
  bool truncate = mirrorConfig->get_truncate();

  auto mirror = make_shared<Mirror>(
      *mirrorConfig->name_ref(),
      mirrorEgressPort,
      destinationIp,
      srcIp,
      udpPorts,
      dscpMark,
      truncate);
  return mirror;
}

std::shared_ptr<Mirror> ThriftConfigApplier::updateMirror(
    const std::shared_ptr<Mirror>& orig,
    const cfg::Mirror* mirrorConfig) {
  auto newMirror = createMirror(mirrorConfig);
  if (newMirror->getDestinationIp() == orig->getDestinationIp() &&
      newMirror->getSrcIp() == orig->getSrcIp() &&
      newMirror->getTunnelUdpPorts() == orig->getTunnelUdpPorts() &&
      newMirror->getTruncate() == orig->getTruncate() &&
      (!newMirror->configHasEgressPort() ||
       newMirror->getEgressPort() == orig->getEgressPort())) {
    if (orig->getMirrorTunnel()) {
      newMirror->setMirrorTunnel(orig->getMirrorTunnel().value());
    }
    if (orig->getEgressPort()) {
      newMirror->setEgressPort(orig->getEgressPort().value());
    }
  }
  if (*newMirror == *orig) {
    return nullptr;
  }
  return newMirror;
}

std::shared_ptr<ForwardingInformationBaseMap>
ThriftConfigApplier::updateForwardingInformationBaseContainers() {
  auto origForwardingInformationBaseMap = orig_->getFibs();
  ForwardingInformationBaseMap::NodeContainer newFibContainers;
  bool changed = false;

  std::size_t numExistingProcessed = 0;

  for (const auto& interfaceCfg : *cfg_->interfaces_ref()) {
    RouterID vrf(*interfaceCfg.routerID_ref());
    if (newFibContainers.find(vrf) != newFibContainers.end()) {
      continue;
    }

    auto origFibContainer = orig_->getFibs()->getFibContainerIf(vrf);

    std::shared_ptr<ForwardingInformationBaseContainer> newFibContainer{
        nullptr};
    if (origFibContainer) {
      newFibContainer = origFibContainer;
      ++numExistingProcessed;
    } else {
      newFibContainer =
          std::make_shared<ForwardingInformationBaseContainer>(vrf);
    }

    changed |= updateMap(&newFibContainers, origFibContainer, newFibContainer);
  }

  if (numExistingProcessed != orig_->getFibs()->size()) {
    CHECK_LE(numExistingProcessed, orig_->getFibs()->size());
    changed = true;
  }

  if (!changed) {
    return nullptr;
  }

  return origForwardingInformationBaseMap->clone(newFibContainers);
}

std::optional<cfg::PfcWatchdogRecoveryAction>
ThriftConfigApplier::getPfcWatchdogRecoveryAction() {
  std::shared_ptr<Port> firstPort;
  std::optional<cfg::PfcWatchdogRecoveryAction> recoveryAction{};
  for (const auto& port : *new_->getPorts()) {
    if (port->getPfc().has_value() &&
        port->getPfc()->watchdog_ref().has_value()) {
      auto pfcWd = port->getPfc()->watchdog_ref().value();
      if (!recoveryAction.has_value()) {
        recoveryAction = *pfcWd.recoveryAction_ref();
        firstPort = port;
        XLOG(DBG2) << "PFC watchdog recovery action initialized to "
                   << (int)*pfcWd.recoveryAction_ref();
      } else if (*recoveryAction != *pfcWd.recoveryAction_ref()) {
        // Error: All ports should have the same recovery action configured
        throw FbossError(
            "PFC watchdog deadlock recovery action ",
            *pfcWd.recoveryAction_ref(),
            " on ",
            port->getName(),
            " conflicting with ",
            firstPort->getName());
      }
    }
  }
  return recoveryAction;
}

LabelNextHopEntry ThriftConfigApplier::getStaticLabelNextHopEntry(
    LabelNextHopEntry::Action action,
    LabelNextHopSet nexthops) {
  switch (action) {
    case LabelNextHopEntry::Action::DROP:
      return LabelNextHopEntry(
          LabelNextHopEntry::Action::DROP, AdminDistance::STATIC_ROUTE);

    case LabelNextHopEntry::Action::TO_CPU:
      return LabelNextHopEntry(
          LabelNextHopEntry::Action::TO_CPU, AdminDistance::STATIC_ROUTE);

    case LabelNextHopEntry::Action::NEXTHOPS:
      return LabelNextHopEntry(nexthops, AdminDistance::STATIC_ROUTE);
  }
  throw FbossError("invalid label forwarding action ", action);
}

std::shared_ptr<LabelForwardingEntry>
ThriftConfigApplier::createLabelForwardingEntry(
    MplsLabel label,
    LabelNextHopEntry::Action action,
    LabelNextHopSet nexthops) {
  return std::make_shared<LabelForwardingEntry>(
      label,
      ClientID::STATIC_ROUTE,
      getStaticLabelNextHopEntry(action, nexthops));
}

std::shared_ptr<LabelForwardingInformationBase>
ThriftConfigApplier::updateStaticMplsRoutes(
    const std::vector<cfg::StaticMplsRouteWithNextHops>&
        staticMplsRoutesWithNhops,
    const std::vector<cfg::StaticMplsRouteNoNextHops>& staticMplsRoutesToNull,
    const std::vector<cfg::StaticMplsRouteNoNextHops>& staticMplsRoutesToCPU) {
  if (staticMplsRoutesWithNhops.empty() && staticMplsRoutesToNull.empty() &&
      staticMplsRoutesToCPU.empty()) {
    return nullptr;
  }
  auto labelFib = new_->getLabelForwardingInformationBase()->clone();
  for (auto& staticMplsRouteEntry : staticMplsRoutesWithNhops) {
    RouteNextHopSet resolvedNextHops{};
    // resolve next hops if any next hop is unresolved.
    for (auto nexthop : staticMplsRouteEntry.get_nexthops()) {
      auto nhop = util::fromThrift(nexthop);
      if (!nhop.labelForwardingAction()) {
        throw FbossError(
            "static mpls route for label ",
            staticMplsRouteEntry.get_ingressLabel(),
            " has next hop without label action");
      }
      folly::IPAddress nhopAddress(nhop.addr());
      if (nhopAddress.isLinkLocal() && !nhop.isResolved()) {
        throw FbossError(
            "static mpls route for label ",
            staticMplsRouteEntry.get_ingressLabel(),
            " has link local next hop without interface");
      }
      if (nhop.isResolved() ||
          nhop.labelForwardingAction()->type() ==
              MplsActionCode::POP_AND_LOOKUP) {
        resolvedNextHops.insert(nhop);
        continue;
      }
      // check if nhopAddress is in in one of the interface subnets
      // look up in interfaces of default router (RouterID(0))
      auto intfAddrToReach =
          new_->getInterfaces()->getIntfAddrToReach(RouterID(0), nhopAddress);
      if (!intfAddrToReach.intf) {
        throw FbossError(
            "static mpls route for label ",
            staticMplsRouteEntry.get_ingressLabel(),
            " has nexthop ",
            nhopAddress.str(),
            " out of interface subnets");
      }
      resolvedNextHops.insert(ResolvedNextHop(
          nhop.addr(),
          intfAddrToReach.intf->getID(),
          nhop.weight(),
          nhop.labelForwardingAction()));
    }
    auto entry = labelFib->getLabelForwardingEntryIf(
        staticMplsRouteEntry.get_ingressLabel());
    if (!entry) {
      labelFib->addNode(createLabelForwardingEntry(
          staticMplsRouteEntry.get_ingressLabel(),
          LabelNextHopEntry::Action::NEXTHOPS,
          resolvedNextHops));
    } else {
      entry = entry->clone();
      entry->update(
          ClientID::STATIC_ROUTE,
          getStaticLabelNextHopEntry(
              LabelNextHopEntry::Action::NEXTHOPS, resolvedNextHops));
    }
  }

  for (auto& staticMplsRouteEntry : staticMplsRoutesToNull) {
    auto entry = labelFib->getLabelForwardingEntryIf(
        staticMplsRouteEntry.get_ingressLabel());
    if (!entry) {
      labelFib->addNode(createLabelForwardingEntry(
          staticMplsRouteEntry.get_ingressLabel(),
          LabelNextHopEntry::Action::DROP,
          {}));
    } else {
      entry = entry->clone();
      entry->update(
          ClientID::STATIC_ROUTE,
          getStaticLabelNextHopEntry(LabelNextHopEntry::Action::DROP, {}));
    }
  }

  for (auto& staticMplsRouteEntry : staticMplsRoutesToCPU) {
    auto entry = labelFib->getLabelForwardingEntryIf(
        staticMplsRouteEntry.get_ingressLabel());
    if (!entry) {
      labelFib->addNode(createLabelForwardingEntry(
          staticMplsRouteEntry.get_ingressLabel(),
          LabelNextHopEntry::Action::TO_CPU,
          {}));
    } else {
      entry = entry->clone();
      entry->update(
          ClientID::STATIC_ROUTE,
          getStaticLabelNextHopEntry(LabelNextHopEntry::Action::TO_CPU, {}));
    }
  }
  return labelFib;
}

shared_ptr<SwitchState> applyThriftConfig(
    const shared_ptr<SwitchState>& state,
    const cfg::SwitchConfig* config,
    const Platform* platform,
    RoutingInformationBase* rib) {
  cfg::SwitchConfig emptyConfig;
  return ThriftConfigApplier(state, config, platform, rib).run();
}
shared_ptr<SwitchState> applyThriftConfig(
    const shared_ptr<SwitchState>& state,
    const cfg::SwitchConfig* config,
    const Platform* platform,
    RouteUpdateWrapper* routeUpdater) {
  cfg::SwitchConfig emptyConfig;
  return ThriftConfigApplier(state, config, platform, routeUpdater).run();
}

} // namespace facebook::fboss
