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

#include <fboss/thrift_cow/nodes/ThriftMapNode-inl.h>
#include <folly/FileUtil.h>
#include <folly/gen/Base.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>
#include <memory>
#include <optional>
#include <string>

#include "fboss/agent/AclNexthopHandler.h"

#include "fboss/agent/FbossError.h"
#include "fboss/agent/LacpTypes.h"
#include "fboss/agent/LoadBalancerConfigApplier.h"
#include "fboss/agent/Platform.h"
#include "fboss/agent/RouteUpdateWrapper.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/SwitchIdScopeResolver.h"
#include "fboss/agent/SwitchInfoUtils.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
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
#include "fboss/agent/state/FlowletSwitchingConfig.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/InterfaceMap.h"
#include "fboss/agent/state/IpTunnel.h"
#include "fboss/agent/state/IpTunnelMap.h"
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
#include "fboss/lib/config/PlatformConfigUtils.h"

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

namespace {

const uint8_t kV6LinkLocalAddrMask{64};

facebook::fboss::VlanID kPseudoVlanID(0);

// Only one buffer pool is supported systemwide. Variable to track the name
// and validate during a config change.
std::optional<std::string> sharedBufferPoolName;

std::shared_ptr<facebook::fboss::SwitchState> updateFibFromConfig(
    facebook::fboss::RouterID vrf,
    const facebook::fboss::IPv4NetworkToRouteMap& v4NetworkToRoute,
    const facebook::fboss::IPv6NetworkToRouteMap& v6NetworkToRoute,
    const facebook::fboss::LabelToRouteMap& labelToRoute,
    void* cookie) {
  facebook::fboss::ForwardingInformationBaseUpdater fibUpdater(
      vrf, v4NetworkToRoute, v6NetworkToRoute, labelToRoute);

  auto nextStatePtr =
      static_cast<std::shared_ptr<facebook::fboss::SwitchState>*>(cookie);

  fibUpdater(*nextStatePtr);
  return *nextStatePtr;
}

template <typename MultiMap, typename Map>
std::shared_ptr<MultiMap> toMnpuMap(
    const std::shared_ptr<Map>& map,
    const facebook::fboss::SwitchIdScopeResolver& resolver) {
  auto multiMap = std::make_shared<MultiMap>();
  for (const auto& idAndNode : *map) {
    multiMap->addNode(idAndNode.second, resolver.scope(idAndNode.second));
  }
  return multiMap;
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
      RoutingInformationBase* rib,
      AclNexthopHandler* aclNexthopHandler,
      const PlatformMapping* platformMapping)
      : orig_(orig),
        cfg_(config),
        platform_(platform),
        rib_(rib),
        aclNexthopHandler_(aclNexthopHandler),
        scopeResolver_(*config->switchSettings()->switchIdToSwitchInfo()),
        platformMapping_(platformMapping) {}

  ThriftConfigApplier(
      const std::shared_ptr<SwitchState>& orig,
      const cfg::SwitchConfig* config,
      const Platform* platform,
      RouteUpdateWrapper* routeUpdater,
      AclNexthopHandler* aclNexthopHandler,
      const PlatformMapping* platformMapping)
      : orig_(orig),
        cfg_(config),
        platform_(platform),
        routeUpdater_(routeUpdater),
        aclNexthopHandler_(aclNexthopHandler),
        scopeResolver_(*config->switchSettings()->switchIdToSwitchInfo()),
        platformMapping_(platformMapping) {}

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

  template <typename Node, typename NodeMap>
  bool updateThriftMapNode(
      NodeMap* map,
      std::shared_ptr<Node> origNode,
      std::shared_ptr<Node> newNode) {
    auto node = (newNode != nullptr) ? newNode : origNode;
    auto key = node->getID();
    auto ret = map->insert(key, std::move(node));
    if (!ret.second) {
      throw FbossError("duplicate entry ", key);
    }
    return newNode != nullptr;
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
  std::shared_ptr<PortMap> updatePorts(
      const std::shared_ptr<TransceiverMap>& transceiverMap);
  std::shared_ptr<SystemPortMap> updateSystemPorts(
      const std::shared_ptr<PortMap>& ports,
      const std::shared_ptr<SwitchSettings>& switchSettings);
  std::shared_ptr<Port> updatePort(
      const std::shared_ptr<Port>& orig,
      const cfg::Port* cfg,
      const std::shared_ptr<TransceiverSpec>& transceiver);
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
  std::optional<std::vector<int16_t>> findEnabledPfcPriorities(
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
  std::vector<int32_t> getAggregatePortInterfaceIDs(
      const std::vector<AggregatePort::Subport>& subports);
  std::pair<folly::MacAddress, uint16_t> getSystemLacpConfig();
  uint8_t computeMinimumLinkCount(const cfg::AggregatePort& cfg);
  std::shared_ptr<VlanMap> updateVlans();
  std::shared_ptr<VlanMap> updatePseudoVlans();
  shared_ptr<Vlan> createPseudoVlan();
  shared_ptr<Vlan> updatePseudoVlan(const shared_ptr<Vlan>& orig);
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
      const MatchAction* action = nullptr,
      bool enable = true);
  std::shared_ptr<AclEntry> updateAcl(
      cfg::AclStage aclStage,
      const cfg::AclEntry& acl,
      int priority,
      int* numExistingProcessed,
      bool* changed,
      std::optional<std::string> tableName,
      const MatchAction* action = nullptr,
      bool enable = true);
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
  struct InterfaceIpInfo;
  template <typename NeighborResponseEntry, typename IPAddr>
  std::shared_ptr<NeighborResponseEntry> updateNeighborResponseEntry(
      const std::shared_ptr<NeighborResponseEntry>& orig,
      IPAddr ip,
      InterfaceIpInfo addrInfo);
  bool updateNeighborResponseTables(Vlan* vlan, const cfg::Vlan* config);
  bool updateNbrResponseTablesFromAllIntfCfg(Vlan* vlan);
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

  shared_ptr<IpTunnel> createIpInIpTunnel(const cfg::IpInIpTunnel& config);
  shared_ptr<IpTunnel> updateIpInIpTunnel(
      const std::shared_ptr<IpTunnel>& orig,
      const cfg::IpInIpTunnel* config);
  std::shared_ptr<IpTunnelMap> updateIpInIpTunnels();
  std::shared_ptr<DsfNodeMap> updateDsfNodes();
  void processUpdatedDsfNodes();
  void validateUdfConfig(const UdfConfig& newUdfConfig);
  std::shared_ptr<UdfConfig> updateUdfConfig(bool* changed);

  void processInterfaceForPortForNonVoqSwitches(int64_t switchId);
  void processInterfaceForPortForVoqSwitches(int64_t switchId);
  void processInterfaceForPort();

  shared_ptr<FlowletSwitchingConfig> updateFlowletSwitchingConfig(
      bool* changed);
  shared_ptr<FlowletSwitchingConfig> createFlowletSwitchingConfig(
      const cfg::FlowletSwitchingConfig& config);

  std::shared_ptr<SwitchState> orig_;
  std::shared_ptr<SwitchState> new_;
  const cfg::SwitchConfig* cfg_{nullptr};
  const Platform* platform_{nullptr};
  RoutingInformationBase* rib_{nullptr};
  RouteUpdateWrapper* routeUpdater_{nullptr};
  AclNexthopHandler* aclNexthopHandler_{nullptr};
  SwitchIdScopeResolver scopeResolver_;
  const PlatformMapping* platformMapping_{nullptr};

  struct InterfaceIpInfo {
    InterfaceIpInfo(uint8_t mask, MacAddress mac, InterfaceID intf)
        : mask(mask), mac(mac), interfaceID(intf) {}

    uint8_t mask;
    MacAddress mac;
    InterfaceID interfaceID;
  };
  struct InterfaceInfo {
    RouterID routerID{0};
    flat_set<InterfaceID> interfaces;
    flat_map<IPAddress, InterfaceIpInfo> addresses;
  };

  flat_map<PortID, Port::VlanMembership> portVlans_;
  flat_map<VlanID, Vlan::MemberPorts> vlanPorts_;
  flat_map<VlanID, InterfaceInfo> vlanInterfaces_;
  flat_map<PortID, std::vector<int32_t>> port2InterfaceId_;
};

shared_ptr<SwitchState> ThriftConfigApplier::run() {
  new_ = orig_->clone();
  bool changed = false;

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
    auto newSwitchSettings = updateSwitchSettings();
    if (newSwitchSettings) {
      if ((newSwitchSettings->getSwitchIdToSwitchInfo() !=
           orig_->getSwitchSettings()->getSwitchIdToSwitchInfo())) {
        new_->resetSystemPorts(
            updateSystemPorts(new_->getPorts(), newSwitchSettings));
      }
      new_->resetSwitchSettings(std::move(newSwitchSettings));
      changed = true;
    }
  }

  processInterfaceForPort();

  {
    auto newPorts = updatePorts(new_->getTransceivers());
    if (newPorts) {
      new_->resetPorts(std::move(newPorts));
      new_->resetSystemPorts(
          updateSystemPorts(new_->getPorts(), new_->getSwitchSettings()));
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
      new_->resetMirrors(toMnpuMap<MultiMirrorMap>(newMirrors, scopeResolver_));
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
      auto newAcls = updateAcls(cfg::AclStage::INGRESS, *cfg_->acls());
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
        *cfg_->staticRoutesWithNhops(),
        *cfg_->staticRoutesToNull(),
        *cfg_->staticRoutesToCPU(),
        *cfg_->staticIp2MplsRoutes(),
        *cfg_->staticMplsRoutesWithNhops(),
        *cfg_->staticMplsRoutesToNull(),
        *cfg_->staticMplsRoutesToCPU());
  } else if (rib_) {
    auto newFibs = updateForwardingInformationBaseContainers();
    if (newFibs) {
      new_->resetForwardingInformationBases(newFibs);
      changed = true;
    }

    rib_->reconfigure(
        intfRouteTables_,
        *cfg_->staticRoutesWithNhops(),
        *cfg_->staticRoutesToNull(),
        *cfg_->staticRoutesToCPU(),
        *cfg_->staticIp2MplsRoutes(),
        *cfg_->staticMplsRoutesWithNhops(),
        *cfg_->staticMplsRoutesToNull(),
        *cfg_->staticMplsRoutesToCPU(),
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
      *cfg_->staticMplsRoutesWithNhops(),
      *cfg_->staticMplsRoutesToNull(),
      *cfg_->staticMplsRoutesToNull());
  if (labelFib) {
    new_->resetLabelForwardingInformationBase(labelFib);
    changed = true;
  }

  auto newVlans = new_->getVlans();
  VlanID dfltVlan(*cfg_->defaultVlan());
  if (orig_->getDefaultVlan() != dfltVlan) {
    if (newVlans->getVlanIf(dfltVlan) == nullptr) {
      throw FbossError("Default VLAN ", dfltVlan, " does not exist");
    }
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

  {
    auto newTunnels = updateIpInIpTunnels();
    if (newTunnels) {
      new_->resetTunnels(std::move(newTunnels));
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

  {
    auto newDsfNodes = updateDsfNodes();
    if (newDsfNodes) {
      new_->resetDsfNodes(
          toMnpuMap<MultiDsfNodeMap>(newDsfNodes, scopeResolver_));
      processUpdatedDsfNodes();
      new_->resetSystemPorts(
          updateSystemPorts(new_->getPorts(), new_->getSwitchSettings()));
      changed = true;
    }
  }

  {
    // VOQ/Fabric switches require that the packets are not tagged with any
    // VLAN. We are gradually enhancing wedge_agent to handle tagged as well as
    // untagged packets. During this transition, we will use VlanID 0 as
    // "pseudoVlan" to populate SwitchState/Neighbor cache etc. data structures.
    // Once the wedge_agent changes are complete, we will no longer need
    // pseudoVlan notion.
    if (!new_->getSwitchSettings()->vlansSupported()) {
      auto pseudoVlans = updatePseudoVlans();
      if (pseudoVlans) {
        new_->resetVlans(std::move(pseudoVlans));
        changed = true;
      }
    }
  }

  if (!changed) {
    return nullptr;
  }
  return new_;
}

void ThriftConfigApplier::processUpdatedDsfNodes() {
  auto localSwitchIds = new_->getSwitchSettings()->getSwitchIds();
  for (auto localSwitchId : localSwitchIds) {
    auto origDsfNode = orig_->getDsfNodes()->getNodeIf(localSwitchId);
    if (origDsfNode &&
        origDsfNode->getType() !=
            new_->getDsfNodes()->getNodeIf(localSwitchId)->getType()) {
      throw FbossError("Change in DSF node type is not supported");
    }
  }

  if (new_->getSwitchSettings()
          ->getSwitchIdsOfType(cfg::SwitchType::VOQ)
          .size() == 0) {
    // DSF node processing only needed on VOQ Switches
    return;
  }
  auto delta = StateDelta(orig_, new_).getDsfNodesDelta();
  auto getRecyclePortId = [](const std::shared_ptr<DsfNode>& node) {
    CHECK(node->getSystemPortRange().has_value());
    return *node->getSystemPortRange()->minimum() + 1;
  };
  auto isLocal = [localSwitchIds](const std::shared_ptr<DsfNode>& node) {
    return localSwitchIds.find(node->getSwitchId()) != localSwitchIds.end();
  };
  auto isInterfaceNode = [](const std::shared_ptr<DsfNode>& node) {
    return node->getType() == cfg::DsfNodeType::INTERFACE_NODE;
  };
  auto getDsfNodeAsic =
      [&isInterfaceNode](const std::shared_ptr<DsfNode>& node) {
        CHECK(isInterfaceNode(node))
            << " Only expect to be called for Interface nodes";
        return HwAsic::makeAsic(
            node->getAsicType(),
            cfg::SwitchType::VOQ,
            static_cast<int64_t>(node->getSwitchId()),
            node->getSystemPortRange());
      };
  auto processLoopbacks = [&](const std::shared_ptr<DsfNode>& node,
                              const HwAsic* dsfNodeAsic) {
    auto recyclePortId = getRecyclePortId(node);
    InterfaceID intfID(recyclePortId);
    auto intfs = isLocal(node) ? new_->getInterfaces()->modify(&new_)
                               : new_->getRemoteInterfaces()->modify(&new_);
    auto intf = intfs->getInterface(intfID)->clone();
    Interface::Addresses addresses;
    // THRIFT_COPY: evaluate if getting entire thrift table is needed.
    auto arpTable = intf->getArpTable()->toThrift();
    auto ndpTable = intf->getNdpTable()->toThrift();
    std::optional<int64_t> encapIdx;
    if (dsfNodeAsic->isSupported(HwAsic::Feature::RESERVED_ENCAP_INDEX_RANGE)) {
      encapIdx = *dsfNodeAsic->getReservedEncapIndexRange().minimum();
    }
    for (const auto& network : node->getLoopbackIpsSorted()) {
      addresses.insert(network);
      state::NeighborEntryFields neighbor;
      neighbor.ipaddress() = network.first.str();
      CHECK(node->getMac().has_value());
      neighbor.mac() = node->getMac()->toString();
      neighbor.portId()->portType() = cfg::PortDescriptorType::SystemPort;
      neighbor.portId()->portId() = recyclePortId;
      neighbor.interfaceId() = recyclePortId;
      neighbor.state() = state::NeighborState::Reachable;
      if (encapIdx) {
        neighbor.encapIndex() = *encapIdx;
        *encapIdx = *encapIdx + 1;
      }
      neighbor.isLocal() = isLocal(node);
      neighbor.type() = state::NeighborEntryType::STATIC_ENTRY;
      if (network.first.isV6()) {
        ndpTable.insert({*neighbor.ipaddress(), neighbor});
      } else {
        arpTable.insert({*neighbor.ipaddress(), neighbor});
      }
    }
    intf->setAddresses(std::move(addresses));
    if (dsfNodeAsic->isSupported(HwAsic::Feature::RESERVED_ENCAP_INDEX_RANGE)) {
      intf->setArpTable(std::move(arpTable));
      intf->setNdpTable(std::move(ndpTable));
    }
    intfs->updateNode(intf);
  };
  auto addDsfNode = [&](const std::shared_ptr<DsfNode>& node) {
    if (!isInterfaceNode(node)) {
      // Only create recycle ports for INs
      return;
    }
    auto dsfNodeAsic = getDsfNodeAsic(node);
    if (!dsfNodeAsic->isSupported(HwAsic::Feature::RECYCLE_PORTS)) {
      return;
    }
    if (isLocal(node)) {
      processLoopbacks(node, dsfNodeAsic.get());
      // For local asic recycle port and sys port information will come
      // via local config. So only process need to process loopback IPs here.
      return;
    }
    auto recyclePortId = getRecyclePortId(node);
    auto sysPort = std::make_shared<SystemPort>(SystemPortID(recyclePortId));
    sysPort->setSwitchId(node->getSwitchId());
    sysPort->setPortName(folly::sformat("{}:rcy1/1/1", node->getName()));
    const auto& recyclePortInfo = dsfNodeAsic->getRecyclePortInfo();
    sysPort->setCoreIndex(recyclePortInfo.coreId);
    sysPort->setCorePortIndex(recyclePortInfo.corePortIndex);
    sysPort->setSpeedMbps(recyclePortInfo.speedMbps); // 10G
    sysPort->setNumVoqs(8);
    sysPort->setEnabled(true);
    auto sysPorts = new_->getRemoteSystemPorts()->clone();
    sysPorts->addNode(sysPort);
    new_->resetRemoteSystemPorts(sysPorts);
    CHECK(node->getMac().has_value());
    auto intf = std::make_shared<Interface>(
        InterfaceID(recyclePortId),
        RouterID(0),
        std::optional<VlanID>(std::nullopt),
        folly::StringPiece(sysPort->getPortName()),
        *node->getMac(),
        9000,
        true,
        true,
        cfg::InterfaceType::SYSTEM_PORT);
    auto intfs = new_->getRemoteInterfaces()->clone();
    intfs->addNode(intf);
    new_->resetRemoteIntfs(intfs);
    processLoopbacks(node, dsfNodeAsic.get());
  };
  auto rmDsfNode = [&](const std::shared_ptr<DsfNode>& node) {
    if (!isInterfaceNode(node)) {
      return;
    }
    if (!isLocal(node)) {
      auto recyclePortId = getRecyclePortId(node);
      auto sysPorts = new_->getRemoteSystemPorts()->clone();
      sysPorts->removeNodeIf(SystemPortID(recyclePortId));
      new_->resetRemoteSystemPorts(sysPorts);
      auto intfs = new_->getRemoteInterfaces()->clone();
      intfs->removeNodeIf(InterfaceID(recyclePortId));
      new_->resetRemoteIntfs(intfs);
    } else {
      // Local DSF node removal should be accompanied by
      // recycle port and intf removal in config. That will
      // cleanup any neighbor entries on intf removarl
    }
  };

  DeltaFunctions::forEachChanged(
      delta,
      [&](auto oldNode, auto newNode) {
        rmDsfNode(oldNode);
        addDsfNode(newNode);
      },
      [&](auto newNode) { addDsfNode(newNode); },
      [&](auto oldNode) { rmDsfNode(oldNode); });
}

void ThriftConfigApplier::validateUdfConfig(const UdfConfig& newUdfConfig) {
  auto udfGroupMap = newUdfConfig.getUdfGroupMap();
  if (udfGroupMap == nullptr) {
    return;
  }

  for (const auto& loadBalancerConfig : cfg_->get_loadBalancers()) {
    auto loadBalancerId = loadBalancerConfig.get_id();
    auto udfGroups = loadBalancerConfig.get_fieldSelection().udfGroups();
    for (auto& udfGroupName : *udfGroups) {
      if (udfGroupMap->find(udfGroupName) == udfGroupMap->end()) {
        throw FbossError(
            "Configuration does not exist for UdfGroup: ",
            udfGroupName,
            " but exists in UdfGroupList for LoadBalancer ",
            loadBalancerId);
      }
    }
  }

  for (const auto& udfGroupEntry : *udfGroupMap) {
    const auto& udfGroupName = udfGroupEntry.first;
    const auto& udfPacketMatchersList =
        udfGroupEntry.second->getUdfPacketMatcherIds();
    for (const auto& matcherMapName : udfPacketMatchersList) {
      const auto& udfPacketMatchers = newUdfConfig.getUdfPacketMatcherMap();
      if (udfPacketMatchers->find(matcherMapName) == udfPacketMatchers->end()) {
        throw FbossError(
            "Configuration does not exist for UdfPacketMatcherMap: ",
            matcherMapName,
            " but exists in packetMatcherList for UdfGroup ",
            udfGroupName);
      }
    }
  }
}

std::shared_ptr<UdfConfig> ThriftConfigApplier::updateUdfConfig(bool* changed) {
  *changed = false;
  auto origUdfConfig = orig_->getUdfConfig();
  auto newUdfConfig = std::make_shared<UdfConfig>();

  if (!cfg_->udfConfig()) {
    // cfg field is optional whereas state field is not. As a result
    // check if its populated or not to ascertain if there
    // are any changes in object
    if (origUdfConfig->isUdfConfigPopulated()) {
      *changed = true;
    }
    return newUdfConfig;
  }

  // new cfg exists
  newUdfConfig->fromThrift(*cfg_->udfConfig());
  // validate cfg
  validateUdfConfig(*newUdfConfig);
  // ThriftStructNode does deep comparison internally
  if (*origUdfConfig != *newUdfConfig) {
    *changed = true;
    return newUdfConfig;
  }

  return newUdfConfig;
}

std::shared_ptr<DsfNodeMap> ThriftConfigApplier::updateDsfNodes() {
  auto origNodes = orig_->getDsfNodes();
  auto newNodes = std::make_shared<DsfNodeMap>();
  newNodes->fromThrift(*cfg_->dsfNodes());
  bool changed = false;
  for (const auto& idAndNode : *newNodes) {
    auto newNode = idAndNode.second;
    auto origNode = origNodes->getNodeIf(newNode->getID());
    if (!origNode || *origNode != *newNode) {
      changed |= true;
    } else {
      newNodes->updateNode(origNode);
    }
  }
  // We accounted for all nodes in config, now
  // account of any old nodes that may not be
  // present in config. For this just comparing
  // size is enough, since
  // a. If a node got removed, we would see a delta in size
  // b. If size remained the same and nodes got updated we would
  // see a delta in the loop above
  changed |= (origNodes->numNodes() != newNodes->size());
  if (changed) {
    return newNodes;
  }
  return nullptr;
}
void ThriftConfigApplier::processVlanPorts() {
  // Build the Port --> Vlan mappings
  //
  // The config file has a separate list for this data,
  // but it is stored in the state tree as part of both the PortMap and the
  // VlanMap.
  for (const auto& vp : *cfg_->vlanPorts()) {
    PortID portID(*vp.logicalPort());
    VlanID vlanID(*vp.vlanID());
    auto ret1 = portVlans_[portID].insert(
        std::make_pair(vlanID, Port::VlanInfo(*vp.emitTags())));
    if (!ret1.second) {
      throw FbossError(
          "duplicate VlanPort for port ", portID, ", vlan ", vlanID);
    }
    auto ret2 =
        vlanPorts_[vlanID].insert(std::make_pair(portID, *vp.emitTags()));
    if (!ret2.second) {
      // This should never fail if the first insert succeeded above.
      throw FbossError(
          "duplicate VlanPort for vlan ", vlanID, ", port ", portID);
    }
  }
}

void ThriftConfigApplier::processInterfaceForPortForVoqSwitches(
    int64_t switchId) {
  // TODO - only look at ports corresponding to the passed in switchId
  auto dsfNodeItr = cfg_->dsfNodes()->find(switchId);
  CHECK(dsfNodeItr != cfg_->dsfNodes()->end());
  CHECK(dsfNodeItr->second.systemPortRange().has_value());
  auto systemPortRange = dsfNodeItr->second.systemPortRange();
  for (const auto& portCfg : *cfg_->ports()) {
    auto portType = *portCfg.portType();
    auto portID = PortID(*portCfg.logicalID());

    switch (portType) {
      case cfg::PortType::INTERFACE_PORT:
      case cfg::PortType::RECYCLE_PORT: {
        // system port is 1:1 with every interface and recycle port.
        // interface is 1:1 with system port.
        // InterfaceID is chosen to be the same as systemPortID. Thus:
        auto interfaceID = SystemPortID{*systemPortRange->minimum() + portID};
        port2InterfaceId_[portID].push_back(interfaceID);
      } break;
      case cfg::PortType::FABRIC_PORT:
      case cfg::PortType::CPU_PORT:
        // no interface for fabric/cpu port
        break;
    }
  }
}

void ThriftConfigApplier::processInterfaceForPortForNonVoqSwitches(
    int64_t /*switchId*/) {
  // TODO - only look at ports corresponding to the passed in switchId
  flat_map<VlanID, InterfaceID> vlan2InterfaceId;
  for (const auto& interfaceCfg : *cfg_->interfaces()) {
    vlan2InterfaceId[VlanID(*interfaceCfg.vlanID())] =
        InterfaceID(*interfaceCfg.intfID());
  }

  for (const auto& portCfg : *cfg_->ports()) {
    auto portID = PortID(*portCfg.logicalID());

    for (const auto& [vlanID, vlanInfo] : portVlans_[portID]) {
      auto it = vlan2InterfaceId.find(vlanID);
      // Skip if vlan has no interface && port is not enabled
      if (it == vlan2InterfaceId.end()) {
        if (*portCfg.state() != cfg::PortState::ENABLED) {
          continue;
        }
        throw FbossError(
            "VLAN ",
            vlanID,
            " has no interface, even when corresp port ",
            portID,
            " is enabled");
      }
      port2InterfaceId_[portID].push_back(it->second);
    }
  }
}

void ThriftConfigApplier::processInterfaceForPort() {
  // Build Port -> interface mappings in port2InterfaceId_
  for (const auto& switchIdAndInfo :
       new_->getSwitchSettings()->getSwitchIdToSwitchInfo()) {
    auto switchId = switchIdAndInfo.first;
    auto switchType = *switchIdAndInfo.second.switchType();
    switch (switchType) {
      case cfg::SwitchType::VOQ:
        processInterfaceForPortForVoqSwitches(switchId);
        break;
      case cfg::SwitchType::NPU:
        processInterfaceForPortForNonVoqSwitches(switchId);
        break;
      case cfg::SwitchType::FABRIC:
      case cfg::SwitchType::PHY:
        // No interface on FABRIC or PHY switch types
        break;
    }
  }
}

void ThriftConfigApplier::updateVlanInterfaces(const Interface* intf) {
  if (intf->getType() != cfg::InterfaceType::VLAN) {
    return;
  }
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

  for (auto iter : std::as_const(*intf->getAddresses())) {
    auto ipMask =
        std::make_pair(folly::IPAddress(iter.first), iter.second->cref());
    InterfaceIpInfo info(ipMask.second, intf->getMac(), intf->getID());
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
  InterfaceIpInfo linkLocalInfo(64, intf->getMac(), intf->getID());
  entry.addresses.emplace(IPAddress(linkLocalAddr), linkLocalInfo);
}

shared_ptr<SystemPortMap> ThriftConfigApplier::updateSystemPorts(
    const std::shared_ptr<PortMap>& ports,
    const std::shared_ptr<SwitchSettings>& switchSettings) {
  const auto kNumVoqs = 8;

  static const std::set<cfg::PortType> kCreateSysPortsFor = {
      cfg::PortType::INTERFACE_PORT, cfg::PortType::RECYCLE_PORT};
  QueueConfig systemPortQueues;
  if (cfg_->defaultVoqConfig()->size()) {
    systemPortQueues = updatePortQueues(
        QueueConfig(),
        *cfg_->defaultVoqConfig(),
        kNumVoqs,
        cfg::StreamType::UNICAST);
  }
  auto sysPorts = std::make_shared<SystemPortMap>();
  for (const auto& switchIdAndInfo :
       switchSettings->getSwitchIdToSwitchInfo()) {
    if (switchIdAndInfo.second.switchType() != cfg::SwitchType::VOQ) {
      continue;
    }

    auto switchId = switchIdAndInfo.first;
    auto dsfNode = cfg_->dsfNodes()->find(switchId)->second;
    auto nodeName = *dsfNode.name();
    CHECK(dsfNode.systemPortRange());
    auto systemPortRange = *dsfNode.systemPortRange();

    for (const auto& port : std::as_const(*ports)) {
      if (kCreateSysPortsFor.find(port.second->getPortType()) ==
          kCreateSysPortsFor.end()) {
        continue;
      }
      auto sysPort = std::make_shared<SystemPort>(
          SystemPortID{*systemPortRange.minimum() + port.second->getID()});
      sysPort->setSwitchId(SwitchID(switchId));
      sysPort->setPortName(
          folly::sformat("{}:{}", nodeName, port.second->getName()));
      auto platformPort =
          platformMapping_->getPlatformPort(port.second->getID());
      CHECK(platformPort.mapping()->attachedCoreId().has_value());
      CHECK(platformPort.mapping()->attachedCorePortIndex().has_value());
      sysPort->setCoreIndex(platformPort.mapping()->attachedCoreId().value());
      sysPort->setCorePortIndex(
          platformPort.mapping()->attachedCorePortIndex().value());
      sysPort->setSpeedMbps(static_cast<int>(port.second->getSpeed()));
      sysPort->setNumVoqs(kNumVoqs);
      sysPort->setEnabled(port.second->isEnabled());
      sysPort->setQosPolicy(port.second->getQosPolicy());
      sysPort->resetPortQueues(systemPortQueues);
      sysPorts->addSystemPort(std::move(sysPort));
    }
  }
  return sysPorts;
}

shared_ptr<PortMap> ThriftConfigApplier::updatePorts(
    const std::shared_ptr<TransceiverMap>& transceiverMap) {
  const auto origPorts = orig_->getPorts();
  PortMap::NodeContainer newPorts;
  bool changed = false;

  sharedBufferPoolName.reset();

  // Process all supplied port configs
  for (const auto& portCfg : *cfg_->ports()) {
    PortID id(*portCfg.logicalID());
    auto origPort = origPorts->getPortIf(id);
    std::shared_ptr<Port> newPort;
    // Find present Transceiver if it exists in TransceiverMap
    std::shared_ptr<TransceiverSpec> transceiver;
    auto platformPort = platformMapping_->getPlatformPort(id);
    const auto& chips = platformMapping_->getChips();
    if (auto tcvrID = utility::getTransceiverId(platformPort, chips)) {
      transceiver = transceiverMap->getTransceiverIf(*tcvrID);
    }
    if (!origPort) {
      state::PortFields portFields;
      portFields.portId() = *portCfg.logicalID();
      portFields.portName() = portCfg.name().value_or({});
      auto port = std::make_shared<Port>(std::move(portFields));
      newPort = updatePort(port, &portCfg, transceiver);
    } else {
      newPort = updatePort(origPort, &portCfg, transceiver);
    }
    changed |= updateMap(&newPorts, origPort, newPort);
  }

  for (const auto& origPort : std::as_const(*origPorts)) {
    // This port was listed in the config, and has already been configured
    if (newPorts.find(origPort.second->getID()) != newPorts.end()) {
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
          "New config is missing configuration for port ",
          origPort.second->getID());
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
    if (aqm.detection()->getType() ==
        cfg::QueueCongestionDetection::Type::__EMPTY__) {
      throw FbossError(
          "Active Queue Management must specify a congestion detection method");
    }
    if (behaviors.find(*aqm.behavior()) != behaviors.end()) {
      throw FbossError("Same Active Queue Management behavior already exists");
    }
    behaviors.insert(*aqm.behavior());
  }
}

std::shared_ptr<PortQueue> ThriftConfigApplier::updatePortQueue(
    const std::shared_ptr<PortQueue>& orig,
    const cfg::PortQueue* cfg,
    std::optional<TrafficClass> trafficClass,
    std::optional<std::set<PfcPriority>> pfcPriorities) {
  CHECK_EQ(orig->getID(), *cfg->id());

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
  auto queue = std::make_shared<PortQueue>(static_cast<uint8_t>(*cfg->id()));
  queue->setStreamType(*cfg->streamType());
  queue->setScheduling(*cfg->scheduling());
  if (auto weight = cfg->weight()) {
    queue->setWeight(*weight);
  }
  if (auto reservedBytes = cfg->reservedBytes()) {
    queue->setReservedBytes(*reservedBytes);
  }
  if (auto scalingFactor = cfg->scalingFactor()) {
    queue->setScalingFactor(*scalingFactor);
  }
  if (auto aqms = cfg->aqms()) {
    checkPortQueueAQMValid(*aqms);
    queue->resetAqms(*aqms);
  }
  if (auto shareBytes = cfg->sharedBytes()) {
    queue->setSharedBytes(*shareBytes);
  }
  if (auto name = cfg->name()) {
    queue->setName(*name);
  }
  if (auto portQueueRate = cfg->portQueueRate()) {
    queue->setPortQueueRate(*portQueueRate);
  }
  if (auto bandwidthBurstMinKbits = cfg->bandwidthBurstMinKbits()) {
    queue->setBandwidthBurstMinKbits(*bandwidthBurstMinKbits);
  }
  if (auto bandwidthBurstMaxKbits = cfg->bandwidthBurstMaxKbits()) {
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
  auto pgCfg = std::make_shared<PortPgConfig>(static_cast<uint8_t>(*cfg->id()));
  if (const auto scalingFactor = cfg->scalingFactor()) {
    pgCfg->setScalingFactor(*scalingFactor);
  }

  if (const auto name = cfg->name()) {
    pgCfg->setName(*name);
  }

  pgCfg->setMinLimitBytes(*cfg->minLimitBytes());

  if (const auto headroom = cfg->headroomLimitBytes()) {
    pgCfg->setHeadroomLimitBytes(*headroom);
  }
  if (const auto resumeOffsetBytes = cfg->resumeOffsetBytes()) {
    pgCfg->setResumeOffsetBytes(*resumeOffsetBytes);
  }
  pgCfg->setBufferPoolName(*cfg->bufferPoolName());
  return pgCfg;
}

bool ThriftConfigApplier::isPgConfigUnchanged(
    std::optional<PortPgConfigs> newPortPgCfgs,
    const shared_ptr<Port>& orig) {
  std::map<int, std::shared_ptr<PortPgConfig>> newPortPgConfigMap;
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
  if ((*newPortPgCfgs).size() != origPortPgConfig->size()) {
    return false;
  }

  // come here only if we have both orig, and new port pg cfg and have same size
  for (const auto& portPg : *newPortPgCfgs) {
    newPortPgConfigMap[portPg->getID()] = portPg;
  }

  for (const auto& origPg : *origPortPgConfig) {
    auto newPortPgConfigIter =
        newPortPgConfigMap.find(origPg->cref<switch_state_tags::id>()->cref());
    // THRIFT_COPY - no need to compare buffer pool since toThrift() will
    // compare thrift fields for buffer pool as well.
    if ((newPortPgConfigIter == newPortPgConfigMap.end()) ||
        ((newPortPgConfigIter->second->toThrift() != origPg->toThrift()))) {
      // pg id in the original cfg, is no longer there is new one
      // or the contents of the PG doesn't match
      return false;
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

std::optional<std::vector<int16_t>>
ThriftConfigApplier::findEnabledPfcPriorities(PortPgConfigs& portPgCfgs) {
  if (portPgCfgs.empty()) {
    return std::nullopt;
  }

  std::vector<int16_t> tmpPfcPri;
  for (auto& portPgCfg : portPgCfgs) {
    // We have a 1:1 mapping between PG id and PFC priority
    tmpPfcPri.push_back(static_cast<int16_t>(portPgCfg->getID()));
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
    if (streamType == queue.streamType()) {
      newQueues.emplace(std::make_pair(*queue.id(), &queue));
    }
  }

  if (newQueues.empty()) {
    for (const auto& queue : *cfg_->defaultPortQueues()) {
      if (streamType == queue.streamType()) {
        newQueues.emplace(std::make_pair(*queue.id(), &queue));
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
        for (auto entry : *qosMap->trafficClassToQueueId()) {
          if (entry.second == queueId) {
            trafficClass = static_cast<TrafficClass>(entry.first);
            break;
          }
        }

        if (const auto& pfcPri2QueueIdMap = qosMap->pfcPriorityToQueueId()) {
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
      if (streamType == cfg::StreamType::FABRIC_TX) {
        newPortQueue->setScheduling(cfg::QueueScheduling::INTERNAL);
      }
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
    const cfg::Port* portConf,
    const shared_ptr<TransceiverSpec>& transceiver) {
  CHECK_EQ(orig->getID(), PortID(*portConf->logicalID()));

  auto vlans = portVlans_[orig->getID()];

  std::vector<cfg::PortQueue> cfgPortQueues;
  if (auto portQueueConfigName = portConf->portQueueConfigName()) {
    auto it = cfg_->portQueueConfigs()->find(*portQueueConfigName);
    if (it == cfg_->portQueueConfigs()->end()) {
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
  if (auto ingressMirror = portConf->ingressMirror()) {
    newIngressMirror = *ingressMirror;
  }
  if (auto egressMirror = portConf->egressMirror()) {
    newEgressMirror = *egressMirror;
  }
  bool mirrorsUnChanged = (oldIngressMirror == newIngressMirror) &&
      (oldEgressMirror == newEgressMirror);

  auto newQosPolicy = std::optional<std::string>();
  if (auto dataPlaneTrafficPolicy = cfg_->dataPlaneTrafficPolicy()) {
    if (auto defaultDataPlaneQosPolicy =
            dataPlaneTrafficPolicy->defaultQosPolicy()) {
      newQosPolicy = *defaultDataPlaneQosPolicy;
    }
    if (auto portIdToQosPolicy = dataPlaneTrafficPolicy->portIdToQosPolicy()) {
      auto qosPolicyItr = portIdToQosPolicy->find(*portConf->logicalID());
      if (qosPolicyItr != portIdToQosPolicy->end()) {
        newQosPolicy = qosPolicyItr->second;
      }
    }
  }

  std::optional<cfg::QosMap> qosMap;
  if (newQosPolicy) {
    bool qosPolicyFound = false;
    for (auto qosPolicy : *cfg_->qosPolicies()) {
      if (qosPolicyFound) {
        break;
      }
      qosPolicyFound = (*qosPolicy.name() == newQosPolicy.value());
      if (qosPolicyFound && qosPolicy.qosMap()) {
        qosMap = qosPolicy.qosMap().value();
      }
    }
    if (!qosPolicyFound) {
      throw FbossError("qos policy ", newQosPolicy.value(), " not found");
    }
  }

  // For now, we only support update unicast port queues for ports
  QueueConfig portQueues;
  for (auto streamType :
       platform_->getAsic()->getQueueStreamTypes(*portConf->portType())) {
    auto maxQueues =
        platform_->getAsic()->getDefaultNumPortQueues(streamType, false);
    auto tmpPortQueues = updatePortQueues(
        orig->getPortQueues()->impl(),
        cfgPortQueues,
        maxQueues,
        streamType,
        qosMap);
    portQueues.insert(
        portQueues.begin(), tmpPortQueues.begin(), tmpPortQueues.end());
  }
  bool queuesUnchanged = portQueues.size() == orig->getPortQueues()->size();
  for (int i = 0; i < portQueues.size() && queuesUnchanged; i++) {
    if (*(portQueues.at(i)) != *(orig->getPortQueues()->at(i))) {
      queuesUnchanged = false;
      break;
    }
  }

  auto newSampleDest = std::optional<cfg::SampleDestination>();
  if (portConf->sampleDest()) {
    newSampleDest = portConf->sampleDest().value();

    if (newSampleDest.value() == cfg::SampleDestination::MIRROR &&
        *portConf->sFlowEgressRate() > 0) {
      throw FbossError(
          "Port ",
          orig->getID(),
          ": Egress sampling to mirror destination is unsupported");
    }
  }

  auto newPfc = std::optional<cfg::PortPfc>();
  auto newPfcPriorities = std::optional<std::vector<int16_t>>();
  std::optional<PortPgConfigs> portPgCfgs;
  // lets compare the portPgConfigs
  bool portPgConfigUnchanged = true;
  if (portConf->pfc().has_value()) {
    newPfc = portConf->pfc().value();

    auto pause = portConf->pause().value();
    bool pfc_rx = *newPfc->rx();
    bool pfc_tx = *newPfc->tx();
    bool pause_rx = *pause.rx();
    bool pause_tx = *pause.tx();

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

    auto portPgConfigName = newPfc->portPgConfigName();
    if (newPfc->watchdog().has_value() && (*portPgConfigName).empty()) {
      throw FbossError(
          "Port ",
          orig->getID(),
          " Priority group must be associated with port "
          "when PFC watchdog is configured");
    }
    if (auto portPgConfigs = cfg_->portPgConfigs()) {
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
  auto newLookupClasses{*portConf->lookupClasses()};
  sort(origLookupClasses.begin(), origLookupClasses.end());
  sort(newLookupClasses.begin(), newLookupClasses.end());
  auto lookupClassesUnchanged = (origLookupClasses == newLookupClasses);

  // Now use TransceiverMap as the source of truth to build matcher
  // Prepare the new profileConfig
  std::optional<cfg::PlatformPortConfigOverrideFactor> factor;
  if (transceiver != nullptr) {
    factor = transceiver->toPlatformPortConfigOverrideFactor();
  }
  platform_->getPlatformMapping()->customizePlatformPortConfigOverrideFactor(
      factor);
  PlatformPortProfileConfigMatcher matcher{
      *portConf->profileID(), orig->getID(), factor};

  auto portProfileCfg = platform_->getPortProfileConfig(matcher);
  if (!portProfileCfg) {
    throw FbossError(
        "No port profile config found with matcher:", matcher.toString());
  }
  if (*portConf->state() == cfg::PortState::ENABLED &&
      *portProfileCfg->speed() != *portConf->speed()) {
    throw FbossError(
        orig->getName(),
        " has mismatched speed on profile:",
        apache::thrift::util::enumNameSafe(*portConf->profileID()),
        " and config:",
        apache::thrift::util::enumNameSafe(*portConf->speed()));
  }
  auto newProfileConfigRef = portProfileCfg->iphy();
  auto profileConfigUnchanged =
      (*newProfileConfigRef == orig->getProfileConfig());

  const auto& oldPfcPriorities = orig->getPfcPriorities();
  auto pfcPrioritiesUnchanged = !newPfcPriorities && !oldPfcPriorities;
  if (newPfcPriorities && oldPfcPriorities &&
      (*newPfcPriorities).size() == oldPfcPriorities->size()) {
    pfcPrioritiesUnchanged = true;
    for (int i = 0; i < (*newPfcPriorities).size(); ++i) {
      if ((*newPfcPriorities).at(i) != oldPfcPriorities->at(i)->cref()) {
        pfcPrioritiesUnchanged = false;
        break;
      }
    }
  }

  const auto& newPinConfigs =
      platform_->getPlatformMapping()->getPortIphyPinConfigs(matcher);
  auto pinConfigsUnchanged = (newPinConfigs == orig->getPinConfigs());

  XLOG_IF(DBG2, !profileConfigUnchanged || !pinConfigsUnchanged)
      << orig->getName() << " has profileConfig: "
      << (profileConfigUnchanged ? "UNCHANGED" : "CHANGED")
      << ", pinConfigs: " << (pinConfigsUnchanged ? "UNCHANGED" : "CHANGED")
      << ", with matcher:" << matcher.toString();

  // Port drain is applicable to only fabric ports.
  if (*portConf->drainState() == cfg::PortDrainState::DRAINED &&
      orig->getPortType() != cfg::PortType::FABRIC_PORT) {
    throw FbossError(
        "Port ",
        orig->getID(),
        " cannot be drained as it's NOT a DSF fabric port");
  }

  // Ensure portConf has actually changed, before applying
  if (*portConf->state() == orig->getAdminState() &&
      VlanID(*portConf->ingressVlan()) == orig->getIngressVlan() &&
      *portConf->speed() == orig->getSpeed() &&
      *portConf->profileID() == orig->getProfileID() &&
      *portConf->pause() == orig->getPause() && newPfc == orig->getPfc() &&
      pfcPrioritiesUnchanged &&
      *portConf->sFlowIngressRate() == orig->getSflowIngressRate() &&
      *portConf->sFlowEgressRate() == orig->getSflowEgressRate() &&
      newSampleDest == orig->getSampleDestination() &&
      portConf->name().value_or({}) == orig->getName() &&
      portConf->description().value_or({}) == orig->getDescription() &&
      vlans == orig->getVlans() && queuesUnchanged && portPgConfigUnchanged &&
      *portConf->loopbackMode() == orig->getLoopbackMode() &&
      mirrorsUnChanged && newQosPolicy == orig->getQosPolicy() &&
      *portConf->expectedLLDPValues() == orig->getLLDPValidations() &&
      *portConf->expectedNeighborReachability() ==
          orig->getExpectedNeighborValues()->toThrift() &&
      *portConf->maxFrameSize() == orig->getMaxFrameSize() &&
      lookupClassesUnchanged && profileConfigUnchanged && pinConfigsUnchanged &&
      *portConf->portType() == orig->getPortType() &&
      *portConf->drainState() == orig->getPortDrainState()) {
    return nullptr;
  }

  auto newPort = orig->clone();

  auto lldpmap = newPort->getLLDPValidations();
  for (const auto& tag : *portConf->expectedLLDPValues()) {
    lldpmap[tag.first] = tag.second;
  }

  newPort->setAdminState(*portConf->state());
  newPort->setIngressVlan(VlanID(*portConf->ingressVlan()));
  newPort->setVlans(vlans);
  newPort->setSpeed(*portConf->speed());
  newPort->setProfileId(*portConf->profileID());
  newPort->setPause(*portConf->pause());
  newPort->setSflowIngressRate(*portConf->sFlowIngressRate());
  newPort->setSflowEgressRate(*portConf->sFlowEgressRate());
  newPort->setSampleDestination(newSampleDest);
  newPort->setName(portConf->name().value_or({}));
  newPort->setDescription(portConf->description().value_or({}));
  newPort->setLoopbackMode(*portConf->loopbackMode());
  newPort->resetPortQueues(portQueues);
  newPort->setIngressMirror(newIngressMirror);
  newPort->setEgressMirror(newEgressMirror);
  newPort->setQosPolicy(newQosPolicy);
  newPort->setExpectedLLDPValues(lldpmap);
  newPort->setLookupClassesToDistributeTrafficOn(*portConf->lookupClasses());
  newPort->setMaxFrameSize(*portConf->maxFrameSize());
  newPort->setPfc(newPfc);
  newPort->setPfcPriorities(newPfcPriorities);
  newPort->resetPgConfigs(portPgCfgs);
  newPort->setProfileConfig(*newProfileConfigRef);
  newPort->resetPinConfigs(newPinConfigs);
  newPort->setPortType(*portConf->portType());
  newPort->setInterfaceIDs(port2InterfaceId_[orig->getID()]);
  newPort->setExpectedNeighborReachability(
      *portConf->expectedNeighborReachability());
  newPort->setPortDrainState(*portConf->drainState());
  return newPort;
}

shared_ptr<AggregatePortMap> ThriftConfigApplier::updateAggregatePorts() {
  auto origAggPorts = orig_->getAggregatePorts();
  AggregatePortMap::NodeContainer newAggPorts;
  bool changed = false;

  size_t numExistingProcessed = 0;
  for (const auto& portCfg : *cfg_->aggregatePorts()) {
    AggregatePortID id(*portCfg.key());
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
  CHECK_EQ(origAggPort->getID(), AggregatePortID(*cfg.key()));

  auto cfgSubports = getSubportsSorted(cfg);
  auto origSubports = origAggPort->sortedSubports();

  auto cfgAggregatePortInterfaceIDs = getAggregatePortInterfaceIDs(cfgSubports);

  uint16_t cfgSystemPriority;
  folly::MacAddress cfgSystemID;
  std::tie(cfgSystemID, cfgSystemPriority) = getSystemLacpConfig();

  auto cfgMinLinkCount = computeMinimumLinkCount(cfg);

  if (origAggPort->getName() == *cfg.name() &&
      origAggPort->getDescription() == *cfg.description() &&
      origAggPort->getSystemPriority() == cfgSystemPriority &&
      origAggPort->getSystemID() == cfgSystemID &&
      origAggPort->getMinimumLinkCount() == cfgMinLinkCount &&
      std::equal(
          origSubports.begin(), origSubports.end(), cfgSubports.begin()) &&
      std::equal(
          origAggPort->getInterfaceIDs()->begin(),
          origAggPort->getInterfaceIDs()->end(),
          cfgAggregatePortInterfaceIDs.begin())) {
    return nullptr;
  }

  auto newAggPort = origAggPort->clone();
  newAggPort->setName(*cfg.name());
  newAggPort->setDescription(*cfg.description());
  newAggPort->setSystemPriority(cfgSystemPriority);
  newAggPort->setSystemID(cfgSystemID);
  newAggPort->setMinimumLinkCount(cfgMinLinkCount);
  newAggPort->setSubports(folly::range(cfgSubports.begin(), cfgSubports.end()));
  newAggPort->setInterfaceIDs(cfgAggregatePortInterfaceIDs);

  return newAggPort;
}

shared_ptr<AggregatePort> ThriftConfigApplier::createAggPort(
    const cfg::AggregatePort& cfg) {
  auto subports = getSubportsSorted(cfg);
  auto aggregatePortInterfaceIDs = getAggregatePortInterfaceIDs(subports);

  uint16_t cfgSystemPriority;
  folly::MacAddress cfgSystemID;
  std::tie(cfgSystemID, cfgSystemPriority) = getSystemLacpConfig();

  auto cfgMinLinkCount = computeMinimumLinkCount(cfg);

  return AggregatePort::fromSubportRange(
      AggregatePortID(*cfg.key()),
      *cfg.name(),
      *cfg.description(),
      cfgSystemPriority,
      cfgSystemID,
      cfgMinLinkCount,
      folly::range(subports.begin(), subports.end()),
      aggregatePortInterfaceIDs);
}

std::vector<AggregatePort::Subport> ThriftConfigApplier::getSubportsSorted(
    const cfg::AggregatePort& cfg) {
  if ((*cfg.memberPorts()).size() >
      platform_->getAsic()->getMaxLagMemberSize()) {
    throw FbossError(
        "Trying to set ",
        (*cfg.memberPorts()).size(),
        " lag members, ",
        "which is greater than the hardware limit ",
        platform_->getAsic()->getMaxLagMemberSize());
  }
  std::vector<AggregatePort::Subport> subports(
      std::distance(cfg.memberPorts()->begin(), cfg.memberPorts()->end()));

  for (int i = 0; i < subports.size(); ++i) {
    if (*cfg.memberPorts()[i].priority() < 0 ||
        *cfg.memberPorts()[i].priority() >= 1 << 16) {
      throw FbossError("Member port ", i, " has priority outside of [0, 2^16)");
    }

    auto id = PortID(*cfg.memberPorts()[i].memberPortID());
    auto priority = static_cast<uint16_t>(*cfg.memberPorts()[i].priority());
    auto rate = *cfg.memberPorts()[i].rate();
    auto activity = *cfg.memberPorts()[i].activity();
    auto multiplier = *cfg.memberPorts()[i].holdTimerMultiplier();

    subports[i] =
        AggregatePort::Subport(id, priority, rate, activity, multiplier);
  }

  std::sort(subports.begin(), subports.end());

  return subports;
}

std::vector<int32_t> ThriftConfigApplier::getAggregatePortInterfaceIDs(
    const std::vector<AggregatePort::Subport>& subports) {
  if (subports.size() > 0) {
    // all Aggregate member ports always belong to the same interface(s). Thus,
    // pick the interface for any member port
    auto subport = subports.front();
    return port2InterfaceId_[subport.portID];
  }

  return {};
}

std::pair<folly::MacAddress, uint16_t>
ThriftConfigApplier::getSystemLacpConfig() {
  folly::MacAddress systemID;
  uint16_t systemPriority;

  if (auto lacp = cfg_->lacp()) {
    systemID = MacAddress(*lacp->systemID());
    systemPriority = *lacp->systemPriority();
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

  auto minCapacity = *cfg.minimumCapacity();
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
          std::distance(cfg.memberPorts()->begin(), cfg.memberPorts()->end()));
      if (std::distance(cfg.memberPorts()->begin(), cfg.memberPorts()->end()) !=
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
  // TODO(skhare)
  // VOQ/Fabric switches require that the packets are not tagged with any
  // VLAN. We are gradually enhancing wedge_agent to handle tagged as well as
  // untagged packets. During this transition, we will use PseudoVlan (VlanID
  // 0) to populate SwitchState/Neighbor cache etc. data structures.
  // Thus, for VOQ/Fabric switches, cfg_ will not carry vlan, but after
  // updatePseudoVlan() runs, origVlans will carry pseudoVlan which will be
  // removed by updateVlans() processig.
  // Avoid it by skipping updateVlans() for VOQ/Fabric switches.
  // Once wedge_agent changes are complete, we can remove this check as
  // cfg_->vlans and origVlans will always be empty for VOQ/Fabric switches and
  // then this function will be a no-op
  if (!new_->getSwitchSettings()->vlansSupported()) {
    return nullptr;
  }

  auto origVlans = orig_->getVlans();
  VlanMap::NodeContainer newVlans;
  bool changed = false;

  // Process all supplied VLAN configs
  size_t numExistingProcessed = 0;
  for (const auto& vlanCfg : *cfg_->vlans()) {
    VlanID id(*vlanCfg.id());
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
  const auto& ports = vlanPorts_[VlanID(*config->id())];
  auto vlan = make_shared<Vlan>(config, ports);
  updateNeighborResponseTables(vlan.get(), config);
  updateDhcpOverrides(vlan.get(), config);

  /* TODO t7153326: Following code is added for backward compatibility
  Remove it once coop generates config with */
  if (auto intfID = config->intfID()) {
    vlan->setInterfaceID(InterfaceID(*intfID));
  } else {
    auto& entry = vlanInterfaces_[VlanID(*config->id())];
    if (!entry.interfaces.empty()) {
      vlan->setInterfaceID(*(entry.interfaces.begin()));
    }
  }
  return vlan;
}

shared_ptr<Vlan> ThriftConfigApplier::updateVlan(
    const shared_ptr<Vlan>& orig,
    const cfg::Vlan* config) {
  CHECK_EQ(orig->getID(), VlanID(*config->id()));
  const auto& ports = vlanPorts_[orig->getID()];

  auto newVlan = orig->clone();
  bool changed_neighbor_table =
      updateNeighborResponseTables(newVlan.get(), config);
  bool changed_dhcp_overrides = updateDhcpOverrides(newVlan.get(), config);
  auto oldDhcpV4Relay = orig->getDhcpV4Relay();
  auto newDhcpV4Relay = config->dhcpRelayAddressV4()
      ? IPAddressV4(*config->dhcpRelayAddressV4())
      : IPAddressV4();

  auto oldDhcpV6Relay = orig->getDhcpV6Relay();
  auto newDhcpV6Relay = config->dhcpRelayAddressV6()
      ? IPAddressV6(*config->dhcpRelayAddressV6())
      : IPAddressV6("::");
  /* TODO t7153326: Following code is added for backward compatibility
  Remove it once coop generates config with intfID */
  auto oldIntfID = orig->getInterfaceID();
  auto newIntfID = InterfaceID(0);
  if (auto intfID = config->intfID()) {
    newIntfID = InterfaceID(*intfID);
  } else {
    auto& entry = vlanInterfaces_[VlanID(*config->id())];
    if (!entry.interfaces.empty()) {
      newIntfID = *(entry.interfaces.begin());
    }
  }

  if (orig->getName() == *config->name() && oldIntfID == newIntfID &&
      orig->getPorts() == ports && oldDhcpV4Relay == newDhcpV4Relay &&
      oldDhcpV6Relay == newDhcpV6Relay && !changed_neighbor_table &&
      !changed_dhcp_overrides) {
    return nullptr;
  }

  newVlan->setName(*config->name());
  newVlan->setInterfaceID(newIntfID);
  newVlan->setPorts(ports);
  newVlan->setDhcpV4Relay(newDhcpV4Relay);
  newVlan->setDhcpV6Relay(newDhcpV6Relay);
  return newVlan;
}

shared_ptr<VlanMap> ThriftConfigApplier::updatePseudoVlans() {
  // Pseudo vlan only case of non vlan supporting configs
  CHECK(!new_->getSwitchSettings()->vlansSupported());

  auto origVlans = orig_->getVlans();
  auto origVlan = origVlans->getVlanIf(kPseudoVlanID);
  VlanMap::NodeContainer newVlans;
  bool changed = false;

  shared_ptr<Vlan> newVlan;
  if (origVlan) {
    // update pseudo VLAN
    newVlan = updatePseudoVlan(origVlan);
  } else {
    newVlan = createPseudoVlan();
  }

  changed |= updateMap(&newVlans, origVlan, newVlan);

  if (!changed) {
    return nullptr;
  }

  return origVlans->clone(std::move(newVlans));
}

shared_ptr<Vlan> ThriftConfigApplier::updatePseudoVlan(
    const shared_ptr<Vlan>& orig) {
  auto newVlan = orig->clone();
  bool changed_neighbor_table =
      updateNbrResponseTablesFromAllIntfCfg(newVlan.get());

  if (!changed_neighbor_table) {
    return nullptr;
  }

  return newVlan;
}

shared_ptr<Vlan> ThriftConfigApplier::createPseudoVlan() {
  auto vlan = make_shared<Vlan>(kPseudoVlanID, std::string("pseudoVlan"));
  updateNbrResponseTablesFromAllIntfCfg(vlan.get());

  return vlan;
}

std::shared_ptr<QosPolicyMap> ThriftConfigApplier::updateQosPolicies() {
  QosPolicyMap::NodeContainer newQosPolicies;
  bool changed = false;
  int numExistingProcessed = 0;
  auto defaultDataPlaneQosPolicyName = getDefaultDataPlaneQosPolicyName();

  auto qosPolicies = *cfg_->qosPolicies();
  for (auto& qosPolicy : qosPolicies) {
    if (defaultDataPlaneQosPolicyName.has_value() &&
        defaultDataPlaneQosPolicyName.value() == *qosPolicy.name()) {
      // skip default QosPolicy as it will be maintained in switch state
      continue;
    }
    auto newQosPolicy =
        updateQosPolicy(qosPolicy, &numExistingProcessed, &changed);
    if (!newQosPolicies.emplace(*qosPolicy.name(), newQosPolicy).second) {
      throw FbossError(
          "Invalid config: Qos Policy \"",
          *qosPolicy.name(),
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
      orig_->getQosPolicies()->getQosPolicyIf(*qosPolicy.name());
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
  if (auto dataPlaneTrafficPolicy = cfg_->dataPlaneTrafficPolicy()) {
    if (auto defaultDataPlaneQosPolicy =
            dataPlaneTrafficPolicy->defaultQosPolicy()) {
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
  auto qosPolicies = *cfg_->qosPolicies();
  for (auto& qosPolicy : qosPolicies) {
    if (defaultDataPlaneQosPolicyName == *qosPolicy.name()) {
      newQosPolicy = createQosPolicy(qosPolicy);
      if (newQosPolicy->getExpMap()->get<switch_state_tags::from>()->empty() &&
          newQosPolicy->getExpMap()->get<switch_state_tags::to>()->empty()) {
        // if exp map is not provided, set some default mapping
        // TODO(pshaikh): remove this once default config for switches will
        // always have EXP maps
        auto expMap = ExpMap();
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
  if (qosPolicy.rules()->empty() == !qosPolicy.qosMap().has_value()) {
    XLOG(WARN) << "both qos rules and qos map are provided in qos policy "
               << *qosPolicy.name()
               << "! dscp map if present in qos map, will override qos rules";
  }

  DscpMap ingressDscpMap;
  for (const auto& qosRule : *qosPolicy.rules()) {
    if (qosRule.dscp()->empty()) {
      throw FbossError("Invalid config: qosPolicy: empty dscp list");
    }
    for (const auto& dscpValue : *qosRule.dscp()) {
      if (dscpValue < 0 || dscpValue > 63) {
        throw FbossError("dscp value is invalid (must be [0, 63])");
      }
      ingressDscpMap.addFromEntry(
          static_cast<TrafficClass>(*qosRule.queueId()),
          static_cast<DSCP>(dscpValue));
    }
  }

  if (auto qosMap = qosPolicy.qosMap()) {
    DscpMap dscpMap(*qosMap->dscpMaps());
    ExpMap expMap(*qosMap->expMaps());

    if (const auto& pfcPriorityMap = qosMap->pfcPriorityToQueueId()) {
      for (const auto& pfcPriorityEntry : *pfcPriorityMap) {
        if (pfcPriorityEntry.first >
            cfg::switch_config_constants::PFC_PRIORITY_VALUE_MAX()) {
          throw FbossError(
              "Invalid pfc priority value. Valid range is 0 to: ",
              cfg::switch_config_constants::PFC_PRIORITY_VALUE_MAX());
        }
      }
    }

    auto qosPolicyNew = make_shared<QosPolicy>(
        *qosPolicy.name(),
        dscpMap.empty() ? ingressDscpMap : dscpMap,
        expMap,
        *qosMap->trafficClassToQueueId());

    if (qosMap->pfcPriorityToQueueId().has_value()) {
      qosPolicyNew->setPfcPriorityToQueueIdMap(*qosMap->pfcPriorityToQueueId());
    }

    if (const auto& tc2PgIdMap = qosMap->trafficClassToPgId()) {
      for (auto tc2PgIdEntry : *tc2PgIdMap) {
        if (tc2PgIdEntry.second >
            cfg::switch_config_constants::PORT_PG_VALUE_MAX()) {
          throw FbossError(
              "Invalid pg id. Valid range is 0 to: ",
              cfg::switch_config_constants::PORT_PG_VALUE_MAX());
        }
      }
      qosPolicyNew->setTrafficClassToPgIdMap(*tc2PgIdMap);
    }

    if (const auto& pfcPriority2PgIdMap = qosMap->pfcPriorityToPgId()) {
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
      }
      qosPolicyNew->setPfcPriorityToPgIdMap(*pfcPriority2PgIdMap);
    }

    return qosPolicyNew;
  }
  return make_shared<QosPolicy>(*qosPolicy.name(), ingressDscpMap);
}

std::shared_ptr<AclTableGroupMap> ThriftConfigApplier::updateAclTableGroups() {
  auto origAclTableGroups = orig_->getAclTableGroups();
  AclTableGroupMap::NodeContainer newAclTableGroups;

  if (!cfg_->aclTableGroup()) {
    /*
     * While we are transitioning from Non multi Acl to multi Acl, its possible
     * for cfg to not contain AclTableGroup updates. In those cases, return
     * nullptr to signify no changes in Acls. Since we dont support acl config
     * changes during the transition, returning nullptr is fine
     * TODO(Elangovan): Remove once multi Acl is fully rolled out
     */
    XLOG(ERR) << "AclTableGroup missing from the config";
    return nullptr;
  }

  if (cfg_->aclTableGroup()->stage() != cfg::AclStage::INGRESS) {
    throw FbossError("Only ACL Stage INGRESS is supported");
  }

  auto origAclTableGroup =
      origAclTableGroups->getAclTableGroupIf(cfg::AclStage::INGRESS);

  auto newAclTableGroup =
      updateAclTableGroup(*cfg_->aclTableGroup()->stage(), origAclTableGroup);
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
  if (cfg_->cpuTrafficPolicy() && cfg_->cpuTrafficPolicy()->trafficPolicy()) {
    checkTrafficPolicyAclsExistInConfig(
        *cfg_->cpuTrafficPolicy()->trafficPolicy(), aclByName);
  }
  // Check for dataPlane traffic acls
  if (auto dataPlaneTrafficPolicy = cfg_->dataPlaneTrafficPolicy()) {
    checkTrafficPolicyAclsExistInConfig(*dataPlaneTrafficPolicy, aclByName);
  }

  // For each table in the config, update the table entries and priority
  for (const auto& aclTable : *cfg_->aclTableGroup()->aclTables()) {
    auto newTable =
        updateAclTable(aclStage, aclTable, &numExistingTablesProcessed);
    if (newTable) {
      changed = true;
      newAclTableMap->addTable(newTable);
    } else {
      newAclTableMap->addTable(orig_->getAclTableGroups()
                                   ->getAclTableGroup(aclStage)
                                   ->getAclTableMap()
                                   ->getTableIf(*(aclTable.name())));
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
      std::make_shared<AclTableGroup>(*cfg_->aclTableGroup()->stage());
  newAclTableGroup->setAclTableMap(newAclTableMap);
  newAclTableGroup->setName(*cfg_->aclTableGroup()->name());

  return newAclTableGroup;
}

flat_map<std::string, const cfg::AclEntry*>
ThriftConfigApplier::getAllAclsByName() {
  flat_map<std::string, const cfg::AclEntry*> aclByName;
  for (const auto& aclTable : *cfg_->aclTableGroup()->aclTables()) {
    auto aclEntries = *(aclTable.aclEntries());
    folly::gen::from(aclEntries) |
        folly::gen::map([](const cfg::AclEntry& acl) {
          return std::make_pair(*acl.name(), &acl);
        }) |
        folly::gen::appendTo(aclByName);
  }
  return aclByName;
}

void ThriftConfigApplier::checkTrafficPolicyAclsExistInConfig(
    const cfg::TrafficPolicyConfig& policy,
    flat_map<std::string, const cfg::AclEntry*> aclByName) {
  for (const auto& mta : *policy.matchToAction()) {
    auto a = aclByName.find(*mta.matcher());
    if (a == aclByName.end()) {
      throw FbossError(
          "Invalid config: No acl named ", *mta.matcher(), " found.");
    }
  }
}

std::shared_ptr<AclTable> ThriftConfigApplier::updateAclTable(
    cfg::AclStage aclStage,
    const cfg::AclTable& configTable,
    int* numExistingTablesProcessed) {
  auto tableName = *configTable.name();
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
      aclStage, *(configTable.aclEntries()), std::make_optional(tableName));
  auto newTablePriority = *configTable.priority();
  std::vector<cfg::AclTableActionType> newActionTypes =
      *configTable.actionTypes();
  std::vector<cfg::AclTableQualifier> newQualifiers = *configTable.qualifiers();

  if (origTable) {
    ++(*numExistingTablesProcessed);
    if (!newTableEntries && newTablePriority == origTable->getPriority() &&
        newActionTypes == origTable->getActionTypes() &&
        newQualifiers == origTable->getQualifiers()) {
      // Original table exists with same attributes.
      return nullptr;
    }
  }

  state::AclTableFields aclTableFields{};
  aclTableFields.id() = tableName;
  aclTableFields.priority() = newTablePriority;
  auto newTable = std::make_shared<AclTable>(std::move(aclTableFields));
  if (newTableEntries) {
    // Entries changed from original table or original table does not exist
    newTable->setAclMap(newTableEntries);
  } else if (origTable) {
    // entries are unchanged from original table
    newTable->setAclMap(origTable->getAclMap()->clone());
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
  int priority = AclTable::kDataplaneAclMaxPriority;
  int cpuPriority = 1;

  // Start with the DROP acls, these should have highest priority
  auto acls = folly::gen::from(configEntries) |
      folly::gen::filter([](const cfg::AclEntry& entry) {
                return *entry.actionType() == cfg::AclActionType::DENY;
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
        return std::make_pair(*acl.name(), &acl);
      }) |
      folly::gen::appendTo(aclByName);

  flat_map<std::string, const cfg::TrafficCounter*> counterByName;
  folly::gen::from(*cfg_->trafficCounters()) |
      folly::gen::map([](const cfg::TrafficCounter& counter) {
        return std::make_pair(*counter.name(), &counter);
      }) |
      folly::gen::appendTo(counterByName);

  // Generates new acls from template
  auto addToAcls = [&](const cfg::TrafficPolicyConfig& policy,
                       bool isCoppAcl = false)
      -> const std::vector<std::pair<std::string, std::shared_ptr<AclEntry>>> {
    std::vector<std::pair<std::string, std::shared_ptr<AclEntry>>> entries;
    for (const auto& mta : *policy.matchToAction()) {
      auto a = aclByName.find(*mta.matcher());
      if (a != aclByName.end()) {
        auto aclCfg = *(a->second);

        // We've already added any DENY acls
        if (*aclCfg.actionType() == cfg::AclActionType::DENY) {
          continue;
        }

        // Here is sending to regular port queue action
        MatchAction matchAction = MatchAction();
        if (auto sendToQueue = mta.action()->sendToQueue()) {
          matchAction.setSendToQueue(std::make_pair(*sendToQueue, isCoppAcl));
        }
        if (auto actionCounter = mta.action()->counter()) {
          auto counter = counterByName.find(*actionCounter);
          if (counter == counterByName.end()) {
            throw FbossError(
                "Invalid config: No counter named ",
                *mta.action()->counter(),
                " found.");
          }
          matchAction.setTrafficCounter(*(counter->second));
        }
        if (auto setDscp = mta.action()->setDscp()) {
          matchAction.setSetDscp(*setDscp);
        }
        if (auto ingressMirror = mta.action()->ingressMirror()) {
          matchAction.setIngressMirror(*ingressMirror);
        }
        if (auto egressMirror = mta.action()->egressMirror()) {
          matchAction.setEgressMirror(*egressMirror);
        }
        if (auto toCpuAction = mta.action()->toCpuAction()) {
          matchAction.setToCpuAction(*toCpuAction);
        }
        bool enableAcl = true;
        if (auto redirectToNextHop = mta.action()->redirectToNextHop()) {
          matchAction.setRedirectToNextHop(
              std::make_pair(*redirectToNextHop, MatchAction::NextHopSet()));
          if (aclNexthopHandler_) {
            aclNexthopHandler_->resolveActionNexthops(matchAction);
          }
          if (!matchAction.getRedirectToNextHop().value().second.size()) {
            XLOG(DBG2)
                << "Setting newly configured ACL as disabled since no nexthops are available";
            enableAcl = false;
          }
        }

        auto acl = updateAcl(
            aclStage,
            aclCfg,
            isCoppAcl ? cpuPriority++ : priority++,
            &numExistingProcessed,
            &changed,
            tableName,
            &matchAction,
            enableAcl);

        if (const auto& aclAction = acl->getAclAction()) {
          const auto& inMirror =
              aclAction->cref<switch_state_tags::ingressMirror>();
          const auto& egMirror =
              aclAction->cref<switch_state_tags::egressMirror>();
          if (inMirror && !new_->getMirrors()->getNodeIf(inMirror->cref())) {
            throw FbossError("Mirror ", inMirror->cref(), " is undefined");
          }
          if (egMirror && !new_->getMirrors()->getNodeIf(egMirror->cref())) {
            throw FbossError("Mirror ", egMirror->cref(), " is undefined");
          }
        }
        entries.push_back(std::make_pair(acl->getID(), acl));
      }
    }
    return entries;
  };

  // Add controlPlane traffic acls
  if (cfg_->cpuTrafficPolicy() && cfg_->cpuTrafficPolicy()->trafficPolicy()) {
    folly::gen::from(
        addToAcls(*cfg_->cpuTrafficPolicy()->trafficPolicy(), true)) |
        folly::gen::appendTo(newAcls);
  }

  // Add dataPlane traffic acls
  if (auto dataPlaneTrafficPolicy = cfg_->dataPlaneTrafficPolicy()) {
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
    const MatchAction* action,
    bool enable) {
  std::shared_ptr<AclEntry> origAcl;

  if (FLAGS_enable_acl_table_group) { // multiple acl tables implementation
    CHECK(tableName.has_value());

    if (orig_->getAclsForTable(aclStage, tableName.value())) {
      origAcl = orig_->getAclsForTable(aclStage, tableName.value())
                    ->getEntryIf(*acl.name());
    }
  } else { // single acl table implementation
    CHECK(!tableName.has_value());
    origAcl = orig_->getAcls()->getEntryIf(
        *acl.name()); // orig_ empty in coldboot, or comes from
                      // follydynamic in warmboot
  }

  auto newAcl =
      createAcl(&acl, priority, action, enable); // new always comes from config

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
  if (auto l4SrcPort = config->l4SrcPort()) {
    if (*l4SrcPort < 0 || *l4SrcPort > AclEntry::kMaxL4Port) {
      throw FbossError(
          "L4 source port must be between 0 and ",
          std::to_string(AclEntry::kMaxL4Port));
    }
  }
  if (auto l4DstPort = config->l4DstPort()) {
    if (*l4DstPort < 0 || *l4DstPort > AclEntry::kMaxL4Port) {
      throw FbossError(
          "L4 destination port must be between 0 and ",
          std::to_string(AclEntry::kMaxL4Port));
    }
  }
  if (config->icmpCode() && !config->icmpType()) {
    throw FbossError("icmp type must be set when icmp code is set");
  }
  if (auto icmpType = config->icmpType()) {
    if (*icmpType < 0 || *icmpType > AclEntry::kMaxIcmpType) {
      throw FbossError(
          "icmp type value must be between 0 and ",
          std::to_string(AclEntry::kMaxIcmpType));
    }
  }
  if (auto icmpCode = config->icmpCode()) {
    if (*icmpCode < 0 || *icmpCode > AclEntry::kMaxIcmpCode) {
      throw FbossError(
          "icmp type value must be between 0 and ",
          std::to_string(AclEntry::kMaxIcmpCode));
    }
  }
  if (config->icmpType() &&
      (!config->proto() ||
       !(*config->proto() == AclEntry::kProtoIcmp ||
         *config->proto() == AclEntry::kProtoIcmpv6))) {
    throw FbossError(
        "proto must be either icmp or icmpv6 ", "if icmp type is set");
  }
  if (auto ttl = config->ttl()) {
    if (auto ttlValue = *ttl->value()) {
      if (ttlValue > 255) {
        throw FbossError("ttl value is larger than 255");
      }
      if (ttlValue < 0) {
        throw FbossError("ttl value is less than 0");
      }
    }
    if (auto ttlMask = *ttl->mask()) {
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
    const MatchAction* action,
    bool enable) {
  checkAcl(config);
  auto newAcl = make_shared<AclEntry>(priority, *config->name());
  newAcl->setActionType(*config->actionType());
  if (action) {
    newAcl->setAclAction(*action);
  }
  if (auto srcIp = config->srcIp()) {
    newAcl->setSrcIp(IPAddress::createNetwork(*srcIp));
  }
  if (auto dstIp = config->dstIp()) {
    newAcl->setDstIp(IPAddress::createNetwork(*dstIp));
  }
  if (auto proto = config->proto()) {
    newAcl->setProto(*proto);
  }
  if (auto tcpFlagsBitMap = config->tcpFlagsBitMap()) {
    newAcl->setTcpFlagsBitMap(*tcpFlagsBitMap);
  }
  if (auto srcPort = config->srcPort()) {
    newAcl->setSrcPort(*srcPort);
  }
  if (auto dstPort = config->dstPort()) {
    newAcl->setDstPort(*dstPort);
  }
  if (auto l4SrcPort = config->l4SrcPort()) {
    newAcl->setL4SrcPort(*l4SrcPort);
  }
  if (auto l4DstPort = config->l4DstPort()) {
    newAcl->setL4DstPort(*l4DstPort);
  }
  if (auto ipFrag = config->ipFrag()) {
    newAcl->setIpFrag(*ipFrag);
  }
  if (auto icmpType = config->icmpType()) {
    newAcl->setIcmpType(*icmpType);
  }
  if (auto icmpCode = config->icmpCode()) {
    newAcl->setIcmpCode(*icmpCode);
  }
  if (auto dscp = config->dscp()) {
    newAcl->setDscp(*dscp);
  }
  if (auto dstMac = config->dstMac()) {
    newAcl->setDstMac(MacAddress(*dstMac));
  }
  if (auto ipType = config->ipType()) {
    newAcl->setIpType(*ipType);
  }
  if (auto etherType = config->etherType()) {
    newAcl->setEtherType(*etherType);
  }
  if (auto ttl = config->ttl()) {
    newAcl->setTtl(AclTtl(*ttl->value(), *ttl->mask()));
  }
  if (auto lookupClassL2 = config->lookupClassL2()) {
    newAcl->setLookupClassL2(*lookupClassL2);
  }
  if (auto lookupClassNeighbor = config->lookupClassNeighbor()) {
    newAcl->setLookupClassNeighbor(*lookupClassNeighbor);
  }
  if (auto lookupClassRoute = config->lookupClassRoute()) {
    newAcl->setLookupClassRoute(*lookupClassRoute);
  }
  if (auto packetLookupResult = config->packetLookupResult()) {
    newAcl->setPacketLookupResult(*packetLookupResult);
  }
  if (auto vlanID = config->vlanID()) {
    newAcl->setVlanID(*vlanID);
  }
  newAcl->setEnabled(enable);
  return newAcl;
}

bool ThriftConfigApplier::updateDhcpOverrides(
    Vlan* vlan,
    const cfg::Vlan* config) {
  DhcpV4OverrideMap newDhcpV4OverrideMap;
  if (config->dhcpRelayOverridesV4()) {
    for (const auto& pair : *config->dhcpRelayOverridesV4()) {
      try {
        newDhcpV4OverrideMap[MacAddress(pair.first)] = IPAddressV4(pair.second);
      } catch (const IPAddressFormatException& ex) {
        throw FbossError(
            "Invalid IPv4 address in DHCPv4 relay override map: ", ex.what());
      }
    }
  }

  DhcpV6OverrideMap newDhcpV6OverrideMap;
  if (config->dhcpRelayOverridesV6()) {
    for (const auto& pair : *config->dhcpRelayOverridesV6()) {
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

template <typename NeighborResponseEntry, typename IPAddr>
std::shared_ptr<NeighborResponseEntry>
ThriftConfigApplier::updateNeighborResponseEntry(
    const std::shared_ptr<NeighborResponseEntry>& orig,
    IPAddr ip,
    ThriftConfigApplier::InterfaceIpInfo addrInfo) {
  if (orig && orig->getMac() == addrInfo.mac &&
      orig->getInterfaceID() == addrInfo.interfaceID) {
    return nullptr;
  } else {
    return std::make_shared<NeighborResponseEntry>(
        ip, addrInfo.mac, addrInfo.interfaceID);
  }
}

bool ThriftConfigApplier::updateNbrResponseTablesFromAllIntfCfg(Vlan* vlan) {
  auto arpChanged = false, ndpChanged = false;
  auto origArp = vlan->getArpResponseTable();
  auto origNdp = vlan->getNdpResponseTable();
  ArpResponseTable::NodeContainer arpTable;
  NdpResponseTable::NodeContainer ndpTable;

  std::set<std::pair<folly::IPAddress, uint8_t>> ipAddrsAdded;

  // Add every interface IP address to neighbor response table for pseudo vlan.
  for (const auto& interfaceCfg : *cfg_->interfaces()) {
    auto mac = getInterfaceMac(&interfaceCfg);
    auto intfID = InterfaceID(*interfaceCfg.intfID());
    auto addresses = getInterfaceAddresses(&interfaceCfg);

    for (const auto& [ip, mask] : addresses) {
      auto ipAndMask = std::make_pair(ip, mask);
      auto it = ipAddrsAdded.find(ipAndMask);
      if (it != ipAddrsAdded.end()) {
        continue;
      }
      ipAddrsAdded.insert(ipAndMask);

      if (ip.isV4()) {
        auto origNode = origArp->getEntry(ip.asV4());
        auto newNode = updateNeighborResponseEntry(
            origNode,
            ip.asV4(),
            ThriftConfigApplier::InterfaceIpInfo{mask, mac, intfID});
        arpChanged |= updateMap(&arpTable, origNode, newNode);
      } else {
        auto origNode = origNdp->getEntry(ip.asV6());
        auto newNode = updateNeighborResponseEntry(
            origNode,
            ip.asV6(),
            ThriftConfigApplier::InterfaceIpInfo{mask, mac, intfID});
        ndpChanged |= updateMap(&ndpTable, origNode, newNode);
      }
    }
  }

  arpChanged |= origArp->size() != arpTable.size();
  ndpChanged |= origNdp->size() != ndpTable.size();

  if (arpChanged) {
    vlan->setArpResponseTable(origArp->clone(std::move(arpTable)));
  }
  if (ndpChanged) {
    vlan->setNdpResponseTable(origNdp->clone(std::move(ndpTable)));
  }

  return arpChanged || ndpChanged;
}

bool ThriftConfigApplier::updateNeighborResponseTables(
    Vlan* vlan,
    const cfg::Vlan* config) {
  auto arpChanged = false, ndpChanged = false;
  auto origArp = vlan->getArpResponseTable();
  auto origNdp = vlan->getNdpResponseTable();
  ArpResponseTable::NodeContainer arpTable;
  NdpResponseTable::NodeContainer ndpTable;

  VlanID vlanID(*config->id());
  auto it = vlanInterfaces_.find(vlanID);
  if (it != vlanInterfaces_.end()) {
    for (const auto& [ip, addrInfo] : it->second.addresses) {
      if (ip.isV4()) {
        auto origNode = origArp->getEntry(ip.asV4());
        auto newNode =
            updateNeighborResponseEntry(origNode, ip.asV4(), addrInfo);
        arpChanged |= updateMap(&arpTable, origNode, newNode);

      } else {
        auto origNode = origNdp->getEntry(ip.asV6());
        auto newNode =
            updateNeighborResponseEntry(origNode, ip.asV6(), addrInfo);
        ndpChanged |= updateMap(&ndpTable, origNode, newNode);
      }
    }
  }

  arpChanged |= origArp->size() != arpTable.size();
  ndpChanged |= origNdp->size() != ndpTable.size();

  if (arpChanged) {
    vlan->setArpResponseTable(origArp->clone(std::move(arpTable)));
  }
  if (ndpChanged) {
    vlan->setNdpResponseTable(origNdp->clone(std::move(ndpTable)));
  }
  return arpChanged || ndpChanged;
}

std::shared_ptr<InterfaceMap> ThriftConfigApplier::updateInterfaces() {
  auto origIntfs = orig_->getInterfaces();
  InterfaceMap::NodeContainer newIntfs;
  bool changed = false;

  // Process all supplied interface configs
  size_t numExistingProcessed = 0;

  for (const auto& interfaceCfg : *cfg_->interfaces()) {
    InterfaceID id(*interfaceCfg.intfID());
    auto origIntf = origIntfs->getInterfaceIf(id);
    shared_ptr<Interface> newIntf;
    auto newAddrs = getInterfaceAddresses(&interfaceCfg);
    if (interfaceCfg.type() == cfg::InterfaceType::SYSTEM_PORT) {
      auto sysPort = new_->getSystemPorts()->getSystemPort(
          SystemPortID(*interfaceCfg.intfID()));
      auto dsfNode = cfg_->dsfNodes()->find(sysPort->getSwitchId())->second;
      auto sysPortRange = dsfNode.systemPortRange();
      CHECK(sysPortRange.has_value());
      if (interfaceCfg.intfID() < sysPortRange->minimum() ||
          interfaceCfg.intfID() > sysPortRange->maximum()) {
        throw FbossError(
            "Interface intfID :",
            *interfaceCfg.intfID(),
            "is out of range for corresponding VOQ switch.",
            "sys port range,->min: ",
            *sysPortRange->minimum(),
            "->max: ",
            *sysPortRange->maximum());
      }
    }
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
  auto mtu = config->mtu().value_or(Interface::kDefaultMtu);
  auto intf = make_shared<Interface>(
      InterfaceID(*config->intfID()),
      RouterID(*config->routerID()),
      std::optional<VlanID>(*config->vlanID()),
      folly::StringPiece(name),
      mac,
      mtu,
      *config->isVirtual(),
      *config->isStateSyncDisabled(),
      *config->type());
  intf->setAddresses(addrs);
  if (auto ndp = config->ndp()) {
    if (ndp->routerAddress() &&
        !intf->hasAddress(folly::IPAddress(*ndp->routerAddress()))) {
      throw FbossError(
          "Router address: ",
          *ndp->routerAddress(),
          " does not match any interface address");
    }
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
  for (const auto& collector : *cfg_->sFlowCollectors()) {
    folly::IPAddress address(*collector.ip());
    auto id = address.toFullyQualified() + ':' +
        folly::to<std::string>(*collector.port());
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
  return make_shared<SflowCollector>(
      *config->ip(), static_cast<uint16_t>(*config->port()));
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
  CHECK_EQ(orig->getID(), InterfaceID(*config->intfID()));

  cfg::NdpConfig ndp;
  if (config->ndp()) {
    ndp = *config->ndp();
  }
  auto name = getInterfaceName(config);
  auto mac = getInterfaceMac(config);
  auto mtu = config->mtu().value_or(Interface::kDefaultMtu);
  if (orig->getRouterID() == RouterID(*config->routerID()) &&
      (!orig->getVlanIDIf().has_value() ||
       orig->getVlanIDIf().value() == VlanID(*config->vlanID())) &&
      orig->getName() == name && orig->getMac() == mac &&
      orig->getAddressesCopy() == addrs &&
      orig->getNdpConfig()->toThrift() == ndp && orig->getMtu() == mtu &&
      orig->isVirtual() == *config->isVirtual() &&
      orig->isStateSyncDisabled() == *config->isStateSyncDisabled() &&
      orig->getType() == *config->type()) {
    // No change
    return nullptr;
  }

  auto newIntf = orig->clone();
  newIntf->setRouterID(RouterID(*config->routerID()));
  newIntf->setType(*config->type());
  if (newIntf->getType() == cfg::InterfaceType::VLAN) {
    newIntf->setVlanID(VlanID(*config->vlanID()));
  }
  newIntf->setName(name);
  newIntf->setMac(mac);
  newIntf->setAddresses(addrs);
  newIntf->setNdpConfig(ndp);
  newIntf->setMtu(mtu);
  newIntf->setIsVirtual(*config->isVirtual());
  newIntf->setIsStateSyncDisabled(*config->isStateSyncDisabled());
  return newIntf;
}

shared_ptr<QcmCfg> ThriftConfigApplier::createQcmCfg(
    const cfg::QcmConfig& config) {
  auto newQcmCfg = make_shared<QcmCfg>();

  WeightMap newWeightMap;
  for (const auto& weights : *config.flowWeights()) {
    newWeightMap.emplace(static_cast<int>(weights.first), weights.second);
  }
  newQcmCfg->setFlowWeightMap(newWeightMap);

  Port2QosQueueIdMap port2QosQueueIds;
  for (const auto& perPortQosQueueIds : *config.port2QosQueueIds()) {
    std::set<int> queueIds;
    for (const auto& queueId : perPortQosQueueIds.second) {
      queueIds.insert(queueId);
    }
    port2QosQueueIds[perPortQosQueueIds.first] = queueIds;
  }
  newQcmCfg->setPort2QosQueueIdMap(port2QosQueueIds);

  newQcmCfg->setCollectorSrcPort(*config.collectorSrcPort());
  newQcmCfg->setNumFlowSamplesPerView(*config.numFlowSamplesPerView());
  newQcmCfg->setFlowLimit(*config.flowLimit());
  newQcmCfg->setNumFlowsClear(*config.numFlowsClear());
  newQcmCfg->setScanIntervalInUsecs(*config.scanIntervalInUsecs());
  newQcmCfg->setExportThreshold(*config.exportThreshold());
  newQcmCfg->setAgingInterval(*config.agingIntervalInMsecs());
  newQcmCfg->setCollectorDstIp(
      IPAddress::createNetwork(*config.collectorDstIp()));
  newQcmCfg->setCollectorDstPort(*config.collectorDstPort());
  newQcmCfg->setMonitorQcmCfgPortsOnly(*config.monitorQcmCfgPortsOnly());
  if (auto dscp = config.collectorDscp()) {
    newQcmCfg->setCollectorDscp(*dscp);
  }
  if (auto ppsToQcm = config.ppsToQcm()) {
    newQcmCfg->setPpsToQcm(*ppsToQcm);
  }
  newQcmCfg->setCollectorSrcIp(
      IPAddress::createNetwork(*config.collectorSrcIp()));
  newQcmCfg->setMonitorQcmPortList(*config.monitorQcmPortList());
  return newQcmCfg;
}

shared_ptr<FlowletSwitchingConfig>
ThriftConfigApplier::createFlowletSwitchingConfig(
    const cfg::FlowletSwitchingConfig& config) {
  auto newFlowletSwitchingConfig = make_shared<FlowletSwitchingConfig>();

  newFlowletSwitchingConfig->setInactivityIntervalUsecs(
      *config.inactivityIntervalUsecs());
  newFlowletSwitchingConfig->setFlowletTableSize(*config.flowletTableSize());
  newFlowletSwitchingConfig->setDynamicEgressLoadExponent(
      *config.dynamicEgressLoadExponent());
  newFlowletSwitchingConfig->setDynamicQueueExponent(
      *config.dynamicQueueExponent());
  newFlowletSwitchingConfig->setDynamicQueueMinThresholdBytes(
      *config.dynamicQueueMinThresholdBytes());
  newFlowletSwitchingConfig->setDynamicQueueMaxThresholdBytes(
      *config.dynamicQueueMaxThresholdBytes());
  newFlowletSwitchingConfig->setDynamicSampleRate(*config.dynamicSampleRate());
  if (auto portScalingFactor = config.portScalingFactor()) {
    newFlowletSwitchingConfig->setPortScalingFactor(*portScalingFactor);
  }
  if (auto portLoadWeight = config.portLoadWeight()) {
    newFlowletSwitchingConfig->setPortLoadWeight(*portLoadWeight);
  }
  if (auto portQueueWeight = config.portQueueWeight()) {
    newFlowletSwitchingConfig->setPortQueueWeight(*portQueueWeight);
  }
  return newFlowletSwitchingConfig;
}

shared_ptr<BufferPoolCfgMap> ThriftConfigApplier::updateBufferPoolConfigs(
    bool* changed) {
  *changed = false;
  auto origBufferPoolConfigs = orig_->getBufferPoolCfgs();
  BufferPoolCfgMap::NodeContainer newBufferPoolConfigMap;
  auto newCfgedBufferPools = cfg_->bufferPoolConfigs();

  if (!newCfgedBufferPools && origBufferPoolConfigs->empty()) {
    return nullptr;
  }

  if (!newCfgedBufferPools && !origBufferPoolConfigs->empty()) {
    // old cfg eixists but new one doesn't
    *changed = true;
    return nullptr;
  }

  if (newCfgedBufferPools && origBufferPoolConfigs->empty()) {
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
  bufferPoolCfg->setSharedBytes(*bufferPoolConfig.sharedBytes());
  bufferPoolCfg->setHeadroomBytes(*bufferPoolConfig.headroomBytes());
  return bufferPoolCfg;
}

shared_ptr<QcmCfg> ThriftConfigApplier::updateQcmCfg(bool* changed) {
  auto origQcmConfig = orig_->getQcmCfg();
  if (!cfg_->qcmConfig()) {
    if (origQcmConfig) {
      // going from cfg to empty
      *changed = true;
    }
    return nullptr;
  }
  auto newQcmCfg = createQcmCfg(*cfg_->qcmConfig());
  if (origQcmConfig && (*origQcmConfig == *newQcmCfg)) {
    return nullptr;
  }
  *changed = true;
  return newQcmCfg;
}

shared_ptr<FlowletSwitchingConfig>
ThriftConfigApplier::updateFlowletSwitchingConfig(bool* changed) {
  auto origFlowletSwitchingConfig = orig_->getFlowletSwitchingConfig();
  if (!cfg_->flowletSwitchingConfig()) {
    if (origFlowletSwitchingConfig) {
      // going from cfg to empty
      *changed = true;
    }
    return nullptr;
  }
  auto newFlowletSwitchingConfig =
      createFlowletSwitchingConfig(*cfg_->flowletSwitchingConfig());
  if (origFlowletSwitchingConfig &&
      (*origFlowletSwitchingConfig == *newFlowletSwitchingConfig)) {
    return nullptr;
  }
  *changed = true;
  return newFlowletSwitchingConfig;
}

shared_ptr<SwitchSettings> ThriftConfigApplier::updateSwitchSettings() {
  auto origSwitchSettings = orig_->getSwitchSettings();
  bool switchSettingsChange = false;
  auto newSwitchSettings = origSwitchSettings->clone();

  if (origSwitchSettings->getL2LearningMode() !=
      *cfg_->switchSettings()->l2LearningMode()) {
    newSwitchSettings->setL2LearningMode(
        *cfg_->switchSettings()->l2LearningMode());
    switchSettingsChange = true;
  }

  if (origSwitchSettings->isQcmEnable() !=
      *cfg_->switchSettings()->qcmEnable()) {
    newSwitchSettings->setQcmEnable(*cfg_->switchSettings()->qcmEnable());
    switchSettingsChange = true;
  }

  if (origSwitchSettings->isPtpTcEnable() !=
      *cfg_->switchSettings()->ptpTcEnable()) {
    newSwitchSettings->setPtpTcEnable(*cfg_->switchSettings()->ptpTcEnable());
    switchSettingsChange = true;
  }

  if (origSwitchSettings->getL2AgeTimerSeconds() !=
      *cfg_->switchSettings()->l2AgeTimerSeconds()) {
    newSwitchSettings->setL2AgeTimerSeconds(
        *cfg_->switchSettings()->l2AgeTimerSeconds());
    switchSettingsChange = true;
  }

  if (origSwitchSettings->getMaxRouteCounterIDs() !=
      *cfg_->switchSettings()->maxRouteCounterIDs()) {
    newSwitchSettings->setMaxRouteCounterIDs(
        *cfg_->switchSettings()->maxRouteCounterIDs());
    switchSettingsChange = true;
  }

  std::vector<state::BlockedNeighbor> cfgBlockNeighbors;
  for (const auto& blockNeighbor : *cfg_->switchSettings()->blockNeighbors()) {
    state::BlockedNeighbor neighbor{};
    neighbor.blockNeighborVlanID_ref() = *blockNeighbor.vlanID();
    neighbor.blockNeighborIP_ref() =
        network::toBinaryAddress(folly::IPAddress(*blockNeighbor.ipAddress()));
    cfgBlockNeighbors.emplace_back(neighbor);
  }
  // THRIFT_COPY
  if (origSwitchSettings->getBlockNeighbors()->toThrift() !=
      cfgBlockNeighbors) {
    newSwitchSettings->setBlockNeighbors(cfgBlockNeighbors);
    switchSettingsChange = true;
  }

  std::vector<std::pair<VlanID, folly::MacAddress>> cfgMacAddrsToBlock;
  for (const auto& macAddrToBlock :
       *cfg_->switchSettings()->macAddrsToBlock()) {
    cfgMacAddrsToBlock.emplace_back(
        VlanID(*macAddrToBlock.vlanID()),
        folly::MacAddress(*macAddrToBlock.macAddress()));
  }
  if (origSwitchSettings->getMacAddrsToBlock_DEPRECATED() !=
      cfgMacAddrsToBlock) {
    newSwitchSettings->setMacAddrsToBlock(cfgMacAddrsToBlock);
    switchSettingsChange = true;
  }

  // TODO - Disallow changing any switchInfo parameter after first
  // config apply. Currently we check only switchId and SwitchType
  // This is to allow rollout of new parameters - portIdRange and
  // switchIndex without breaking warmboot
  auto validateSwitchInfoChange = [](const auto& oldSwitchInfo,
                                     const auto& newSwitchInfo) {
    if (oldSwitchInfo.size() != newSwitchInfo.size()) {
      return false;
    }
    for (const auto& switchIdAndInfo : newSwitchInfo) {
      const auto switchId = switchIdAndInfo.first;
      const auto& switchInfo = switchIdAndInfo.second;
      // Disallow SwitchId and SwitchType changes
      if (oldSwitchInfo.find(switchId) == oldSwitchInfo.end() ||
          switchInfo.switchType() != oldSwitchInfo.at(switchId).switchType()) {
        return false;
      }
    }
    return true;
  };

  SwitchIdToSwitchInfo switchIdtoSwitchInfo =
      getSwitchInfoFromConfig(cfg_, platform_);
  if (origSwitchSettings->getSwitchIdToSwitchInfo() != switchIdtoSwitchInfo) {
    if (origSwitchSettings->getSwitchIdToSwitchInfo().size() &&
        !validateSwitchInfoChange(
            origSwitchSettings->getSwitchIdToSwitchInfo(),
            switchIdtoSwitchInfo)) {
      throw FbossError(
          "SwitchId and SwitchInfo type cannot be changed on the fly");
    }
    newSwitchSettings->setSwitchIdToSwitchInfo(switchIdtoSwitchInfo);
    switchSettingsChange = true;
  }

  if (origSwitchSettings->getSwitchDrainState() !=
      *cfg_->switchSettings()->switchDrainState()) {
    if (newSwitchSettings->getSwitchIdsOfType(cfg::SwitchType::FABRIC).size() ==
        0) {
      throw FbossError("Switch drain/isolate is supported only on FDSW");
    }
    newSwitchSettings->setSwitchDrainState(
        *cfg_->switchSettings()->switchDrainState());
    switchSettingsChange = true;
  }

  auto originalExactMatchTableConfig =
      origSwitchSettings->getExactMatchTableConfig();
  // THRIFT_COPY
  if (originalExactMatchTableConfig->toThrift() !=
      *cfg_->switchSettings()->exactMatchTableConfigs()) {
    if (cfg_->switchSettings()->exactMatchTableConfigs()->size() > 1) {
      throw FbossError("Multiple EM tables not supported yet");
    }
    newSwitchSettings->setExactMatchTableConfig(
        *cfg_->switchSettings()->exactMatchTableConfigs());
    switchSettingsChange = true;
  }
  for (auto& switchId : newSwitchSettings->getSwitchIds()) {
    if (newSwitchSettings->getSwitchType(static_cast<int64_t>(switchId)) ==
        cfg::SwitchType::VOQ) {
      auto dsfItr = *cfg_->dsfNodes()->find(static_cast<int64_t>(switchId));
      if (dsfItr == *cfg_->dsfNodes()->end()) {
        throw FbossError(
            "Missing dsf config for switch id: ",
            static_cast<int64_t>(switchId));
      }
      auto localNode = dsfItr.second;
      CHECK(localNode.systemPortRange().has_value());
      auto origLocalNode = orig_->getDsfNodes()->getNodeIf(switchId);
      if (!origLocalNode ||
          origLocalNode->getSystemPortRange() != *localNode.systemPortRange()) {
        switchSettingsChange = true;
      }
    }
  }

  VlanID defaultVlan(*cfg_->defaultVlan());
  if (orig_->getDefaultVlan() != defaultVlan) {
    newSwitchSettings->setDefaultVlan(defaultVlan);
    switchSettingsChange = true;
  }

  std::chrono::seconds arpTimeout(*cfg_->arpTimeoutSeconds());
  if (orig_->getArpTimeout() != arpTimeout) {
    newSwitchSettings->setArpTimeout(arpTimeout);

    // TODO: add ndpTimeout field to SwitchConfig. For now use the same
    // timeout for both ARP and NDP
    newSwitchSettings->setNdpTimeout(arpTimeout);
    switchSettingsChange = true;
  }

  std::chrono::seconds staleEntryInterval(*cfg_->staleEntryInterval());
  if (orig_->getStaleEntryInterval() != staleEntryInterval) {
    newSwitchSettings->setStaleEntryInterval(staleEntryInterval);
    switchSettingsChange = true;
  }

  std::chrono::seconds arpAgerInterval(*cfg_->arpAgerInterval());
  if (orig_->getArpAgerInterval() != arpAgerInterval) {
    newSwitchSettings->setArpAgerInterval(arpAgerInterval);
    switchSettingsChange = true;
  }

  uint32_t maxNeighborProbes(*cfg_->maxNeighborProbes());
  if (orig_->getMaxNeighborProbes() != maxNeighborProbes) {
    newSwitchSettings->setMaxNeighborProbes(maxNeighborProbes);
    switchSettingsChange = true;
  }

  auto oldDhcpV4RelaySrc = orig_->getDhcpV4RelaySrc();
  auto newDhcpV4RelaySrc = cfg_->dhcpRelaySrcOverrideV4()
      ? IPAddressV4(*cfg_->dhcpRelaySrcOverrideV4())
      : IPAddressV4();
  if (oldDhcpV4RelaySrc != newDhcpV4RelaySrc) {
    newSwitchSettings->setDhcpV4RelaySrc(newDhcpV4RelaySrc);
    switchSettingsChange = true;
  }

  auto oldDhcpV6RelaySrc = orig_->getDhcpV6RelaySrc();
  auto newDhcpV6RelaySrc = cfg_->dhcpRelaySrcOverrideV6()
      ? IPAddressV6(*cfg_->dhcpRelaySrcOverrideV6())
      : IPAddressV6("::");
  if (oldDhcpV6RelaySrc != newDhcpV6RelaySrc) {
    newSwitchSettings->setDhcpV6RelaySrc(newDhcpV6RelaySrc);
    switchSettingsChange = true;
  }

  auto oldDhcpV4ReplySrc = orig_->getDhcpV4ReplySrc();
  auto newDhcpV4ReplySrc = cfg_->dhcpReplySrcOverrideV4()
      ? IPAddressV4(*cfg_->dhcpReplySrcOverrideV4())
      : IPAddressV4();
  if (oldDhcpV4ReplySrc != newDhcpV4ReplySrc) {
    newSwitchSettings->setDhcpV4ReplySrc(newDhcpV4ReplySrc);
    switchSettingsChange = true;
  }

  auto oldDhcpV6ReplySrc = orig_->getDhcpV6ReplySrc();
  auto newDhcpV6ReplySrc = cfg_->dhcpReplySrcOverrideV6()
      ? IPAddressV6(*cfg_->dhcpReplySrcOverrideV6())
      : IPAddressV6("::");
  if (oldDhcpV6ReplySrc != newDhcpV6ReplySrc) {
    newSwitchSettings->setDhcpV6ReplySrc(newDhcpV6ReplySrc);
    switchSettingsChange = true;
  }

  {
    bool qcmChanged = false;
    auto newQcmConfig = updateQcmCfg(&qcmChanged);
    if (qcmChanged) {
      newSwitchSettings->setQcmCfg(newQcmConfig);
      switchSettingsChange = true;
    }
  }

  {
    auto newDefaultQosPolicy = updateDataplaneDefaultQosPolicy();
    if (new_->getDefaultDataPlaneQosPolicy() != newDefaultQosPolicy) {
      newSwitchSettings->setDefaultDataPlaneQosPolicy(newDefaultQosPolicy);
      switchSettingsChange = true;
    }
  }

  {
    bool udfCfgChanged = false;
    auto newUdfCfg = updateUdfConfig(&udfCfgChanged);
    if (udfCfgChanged) {
      newSwitchSettings->setUdfConfig(std::move(newUdfCfg));
      switchSettingsChange = true;
    }
  }

  {
    bool flowletSwitchingChanged = false;
    auto newFlowletSwitchingConfig =
        updateFlowletSwitchingConfig(&flowletSwitchingChanged);
    if (flowletSwitchingChanged) {
      newSwitchSettings->setFlowletSwitchingConfig(
          std::move(newFlowletSwitchingConfig));
      switchSettingsChange = true;
    }
  }

  return switchSettingsChange ? newSwitchSettings : nullptr;
}

shared_ptr<ControlPlane> ThriftConfigApplier::updateControlPlane() {
  if (!platform_->getAsic()->isSupported(HwAsic::Feature::CPU_PORT)) {
    if (cfg_->cpuTrafficPolicy()) {
      throw FbossError(
          platform_->getAsic()->getAsicTypeStr(),
          " ASIC does not support CPU port");
    }
    return nullptr;
  }
  auto origCPU = orig_->getControlPlane();
  std::optional<std::string> qosPolicy;
  ControlPlane::RxReasonToQueue newRxReasonToQueue;
  bool rxReasonToQueueUnchanged = true;
  if (auto cpuTrafficPolicy = cfg_->cpuTrafficPolicy()) {
    if (auto trafficPolicy = cpuTrafficPolicy->trafficPolicy()) {
      if (auto defaultQosPolicy = trafficPolicy->defaultQosPolicy()) {
        qosPolicy = *defaultQosPolicy;
      }
    }
    if (const auto rxReasonToQueue =
            cpuTrafficPolicy->rxReasonToQueueOrderedList()) {
      for (auto rxEntry : *rxReasonToQueue) {
        newRxReasonToQueue.push_back(rxEntry);
      }
      // THRIFT_COPY
      if (newRxReasonToQueue != origCPU->getRxReasonToQueue()->toThrift()) {
        rxReasonToQueueUnchanged = false;
      }
    } else if (
        const auto rxReasonToQueue = cpuTrafficPolicy->rxReasonToCPUQueue()) {
      // TODO(pgardideh): the map version of reason to queue is deprecated.
      // Remove
      // this read when it is safe to do so.
      for (auto rxEntry : *rxReasonToQueue) {
        newRxReasonToQueue.push_back(ControlPlane::makeRxReasonToQueueEntry(
            rxEntry.first, rxEntry.second));
      }
      // THRIFT_COPY
      if (newRxReasonToQueue != origCPU->getRxReasonToQueue()->toThrift()) {
        rxReasonToQueueUnchanged = false;
      }
    }
  } else {
    /*
     * if cpuTrafficPolicy is not configured default to dataPlaneTrafficPolicy
     * default i.e. with regards to QoS map configuration, treat CPU port like
     * any front panel port.
     */
    if (auto dataPlaneTrafficPolicy = cfg_->dataPlaneTrafficPolicy()) {
      if (auto defaultDataPlaneQosPolicy =
              dataPlaneTrafficPolicy->defaultQosPolicy()) {
        qosPolicy = *defaultDataPlaneQosPolicy;
      }
    }
  }

  bool qosPolicyUnchanged = qosPolicy == origCPU->getQosPolicy();

  std::optional<cfg::QosMap> qosMap;
  if (qosPolicy) {
    bool qosPolicyFound = false;
    for (auto policy : *cfg_->qosPolicies()) {
      if (qosPolicyFound) {
        break;
      }
      qosPolicyFound = (*policy.name() == qosPolicy.value());
      if (qosPolicyFound && policy.qosMap()) {
        qosMap = policy.qosMap().value();
      }
    }
    if (!qosPolicyFound) {
      throw FbossError("qos policy ", qosPolicy.value(), " not found");
    }
  }

  // check whether queue setting changed
  QueueConfig newQueues;
  for (auto streamType :
       platform_->getAsic()->getQueueStreamTypes(cfg::PortType::CPU_PORT)) {
    auto tmpPortQueues = updatePortQueues(
        origCPU->getQueuesConfig(),
        *cfg_->cpuQueues(),
        origCPU->getQueues()->size(),
        streamType,
        qosMap);
    newQueues.insert(
        newQueues.begin(), tmpPortQueues.begin(), tmpPortQueues.end());
  }
  bool queuesUnchanged = newQueues.size() == origCPU->getQueues()->size();
  for (int i = 0; i < newQueues.size() && queuesUnchanged; i++) {
    if (*(newQueues.at(i)) != *(origCPU->getQueues()->at(i))) {
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
  if (auto name = config->name()) {
    return *name;
  }
  return folly::to<std::string>("Interface ", *config->intfID());
}

MacAddress ThriftConfigApplier::getInterfaceMac(const cfg::Interface* config) {
  if (auto mac = config->mac()) {
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
  if (auto mac = config->mac()) {
    macAddr = folly::MacAddress(*mac);
  } else {
    macAddr = platform_->getLocalMac();
  }
  const folly::IPAddressV6 v6llAddr(folly::IPAddressV6::LINK_LOCAL, macAddr);
  addrs.emplace(v6llAddr, kV6LinkLocalAddrMask);

  // Add all interface addresses from config
  for (const auto& addr : *config->ipAddresses()) {
    auto intfAddr = IPAddress::createNetwork(addr, -1, false);
    auto ret = addrs.insert(intfAddr);
    if (!ret.second) {
      throw FbossError(
          "Duplicate network IP address ",
          addr,
          " in interface ",
          *config->intfID());
    }

    // NOTE: We do not want to leak link-local address into intfRouteTables_
    // TODO: For now we are allowing v4 LLs to be programmed because they are
    // used within Galaxy for LL routing. This hack should go away once we
    // move BGP sessions over non LL addresses
    if (intfAddr.first.isV6() and intfAddr.first.isLinkLocal()) {
      continue;
    }
    auto ret2 = intfRouteTables_[RouterID(*config->routerID())].emplace(
        IPAddress::createNetwork(addr),
        std::make_pair(InterfaceID(*config->intfID()), intfAddr.first));
    if (!ret2.second) {
      // we get same network, only allow it if that is from the same interface
      auto other = ret2.first->second.first;
      if (other != InterfaceID(*config->intfID())) {
        throw FbossError(
            "Duplicate network address ",
            addr,
            " of interface ",
            *config->intfID(),
            " as interface ",
            other,
            " in VRF ",
            *config->routerID());
      }
      // For consistency with interface routes as added by RouteUpdater,
      // use the last address we see rather than the first. Otherwise,
      // we see pointless route updates on syncFib()
      *ret2.first = std::make_pair(
          IPAddress::createNetwork(addr),
          std::make_pair(InterfaceID(*config->intfID()), intfAddr.first));
    }
  }

  return addrs;
}

std::shared_ptr<MirrorMap> ThriftConfigApplier::updateMirrors() {
  const auto& origMirrors = orig_->getMirrors();
  auto newMirrors = std::make_shared<MirrorMap>();

  bool changed = false;

  size_t numExistingProcessed = 0;
  int sflowMirrorCount = 0;
  for (const auto& mirrorCfg : *cfg_->mirrors()) {
    auto origMirror = origMirrors->getNodeIf(*mirrorCfg.name());
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
    changed |= updateThriftMapNode(newMirrors.get(), origMirror, newMirror);
  }

  if (numExistingProcessed != origMirrors->numNodes()) {
    // Some existing Mirrors were removed.
    CHECK_LT(numExistingProcessed, origMirrors->numNodes());
    changed = true;
  }

  for (auto& port : std::as_const(*(new_->getPorts()))) {
    auto portInMirror = port.second->getIngressMirror();
    auto portEgMirror = port.second->getEgressMirror();
    if (portInMirror.has_value()) {
      auto inMirrorMapEntry = newMirrors->find(portInMirror.value());
      if (inMirrorMapEntry == newMirrors->end()) {
        throw FbossError(
            "Mirror ", portInMirror.value(), " for port is not found");
      }
      if (port.second->getSampleDestination() &&
          port.second->getSampleDestination().value() ==
              cfg::SampleDestination::MIRROR &&
          inMirrorMapEntry->second->type() != Mirror::Type::SFLOW) {
        throw FbossError(
            "Ingress mirror ",
            portInMirror.value(),
            " for sampled port ",
            port.second->getID(),
            " not sflow");
      }
    }
    if (portEgMirror.has_value() &&
        newMirrors->find(portEgMirror.value()) == newMirrors->end()) {
      throw FbossError(
          "Mirror ", portEgMirror.value(), " for port is not found");
    }
  }

  if (!changed) {
    return nullptr;
  }

  return newMirrors;
}

std::shared_ptr<Mirror> ThriftConfigApplier::createMirror(
    const cfg::Mirror* mirrorConfig) {
  if (!mirrorConfig->destination()->egressPort() &&
      !mirrorConfig->destination()->tunnel()) {
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

  if (auto egressPort = mirrorConfig->destination()->egressPort()) {
    std::shared_ptr<Port> mirrorToPort{nullptr};
    switch (egressPort->getType()) {
      case cfg::MirrorEgressPort::Type::name:
        for (auto& port : std::as_const(*(new_->getPorts()))) {
          if (port.second->getName() == egressPort->get_name()) {
            mirrorToPort = port.second;
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
        mirrorToPort->getIngressMirror() != *mirrorConfig->name() &&
        mirrorToPort->getEgressMirror() != *mirrorConfig->name()) {
      mirrorEgressPort = PortID(mirrorToPort->getID());
    } else {
      throw FbossError("Invalid port name or ID");
    }
  }

  if (auto tunnel = mirrorConfig->destination()->tunnel()) {
    if (auto sflowTunnel = tunnel->sflowTunnel()) {
      destinationIp = folly::IPAddress(*sflowTunnel->ip());
      if (!sflowTunnel->udpSrcPort() || !sflowTunnel->udpDstPort()) {
        throw FbossError(
            "Both UDP source and UDP destination ports must be provided for \
            sFlow tunneling.");
      }
      if (destinationIp->isV6() &&
          !platform_->getAsic()->isSupported(HwAsic::Feature::SFLOWv6)) {
        throw FbossError("SFLOWv6 is not supported on this platform");
      }
      udpPorts = TunnelUdpPorts(
          *sflowTunnel->udpSrcPort(), *sflowTunnel->udpDstPort());
    } else if (auto greTunnel = tunnel->greTunnel()) {
      destinationIp = folly::IPAddress(*greTunnel->ip());
      if (destinationIp->isV6() &&
          !platform_->getAsic()->isSupported(HwAsic::Feature::ERSPANv6)) {
        throw FbossError("ERSPANv6 is not supported on this platform");
      }
    }

    if (auto srcIpRef = tunnel->srcIp()) {
      srcIp = folly::IPAddress(*srcIpRef);
    }
  }

  uint8_t dscpMark = mirrorConfig->get_dscp();
  bool truncate = mirrorConfig->get_truncate();

  auto mirror = make_shared<Mirror>(
      *mirrorConfig->name(),
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

  for (const auto& interfaceCfg : *cfg_->interfaces()) {
    RouterID vrf(*interfaceCfg.routerID());
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
      LabelForwardingEntry::makeThrift(
          label,
          ClientID::STATIC_ROUTE,
          getStaticLabelNextHopEntry(action, nexthops)));
}

std::shared_ptr<IpTunnelMap> ThriftConfigApplier::updateIpInIpTunnels() {
  const auto& origTunnels = orig_->getTunnels();
  IpTunnelMap::NodeContainer newTunnels;
  bool changed = false;
  size_t numExistingProcessed = 0;
  if (!cfg_->ipInIpTunnels().has_value()) {
    return nullptr;
  }
  for (const auto& tunnelCfg : cfg_->ipInIpTunnels().value()) {
    auto origTunnel = origTunnels->getTunnelIf(*tunnelCfg.ipInIpTunnelId());
    std::shared_ptr<IpTunnel> newTunnel;
    if (origTunnel) {
      newTunnel = updateIpInIpTunnel(origTunnel, &tunnelCfg);
      ++numExistingProcessed;
    } else {
      newTunnel = createIpInIpTunnel(tunnelCfg);
    }

    changed |= updateMap(&newTunnels, origTunnel, newTunnel);
  }

  if (numExistingProcessed != origTunnels->size()) {
    // Some existing Tunnels were removed.
    CHECK_LT(numExistingProcessed, origTunnels->size());
    changed = true;
  }

  if (!changed) {
    return nullptr;
  }
  return origTunnels->clone(std::move(newTunnels));
}

shared_ptr<IpTunnel> ThriftConfigApplier::updateIpInIpTunnel(
    const std::shared_ptr<IpTunnel>& orig,
    const cfg::IpInIpTunnel* config) {
  // the original tunnel has the same name as the new one from config
  auto newTunnel = createIpInIpTunnel(*config);
  if (*newTunnel == *orig) {
    return nullptr;
  }
  return newTunnel;
}

shared_ptr<IpTunnel> ThriftConfigApplier::createIpInIpTunnel(
    const cfg::IpInIpTunnel& config) {
  auto tunnel = make_shared<IpTunnel>(*config.ipInIpTunnelId());
  tunnel->setType(cfg::TunnelType::IP_IN_IP);
  if (config.tunnelType().has_value()) {
    tunnel->setType(*config.tunnelType());
  }
  tunnel->setTunnelTermType(cfg::TunnelTerminationType::P2MP);
  if (config.tunnelTermType().has_value()) {
    tunnel->setTunnelTermType(*config.tunnelTermType());
  }
  tunnel->setUnderlayIntfId(InterfaceID(*config.underlayIntfID()));
  if (auto ttl = config.ttlMode()) {
    tunnel->setTTLMode(static_cast<cfg::IpTunnelMode>(*ttl));
  } else {
    tunnel->setTTLMode(cfg::IpTunnelMode::UNIFORM);
  }
  if (auto dscp = config.dscpMode()) {
    tunnel->setDscpMode(static_cast<cfg::IpTunnelMode>(*dscp));
  } else {
    tunnel->setDscpMode(cfg::IpTunnelMode::UNIFORM);
  }
  if (auto ecn = config.ecnMode()) {
    tunnel->setEcnMode(static_cast<cfg::IpTunnelMode>(*ecn));
  } else {
    tunnel->setEcnMode(cfg::IpTunnelMode::UNIFORM);
  }
  // IP in IP tunnel decap: dst ip is the src of Tunnel state
  // (state default: encap)
  tunnel->setSrcIP(folly::IPAddressV6(*config.dstIp()));
  if (config.srcIp().has_value()) {
    tunnel->setDstIP(folly::IPAddressV6(*config.srcIp()));
  }
  if (config.dstIpMask().has_value()) {
    tunnel->setSrcIPMask(folly::IPAddressV6(*config.dstIpMask()));
  }
  if (config.srcIpMask().has_value()) {
    tunnel->setDstIPMask(folly::IPAddressV6(*config.srcIpMask()));
  }

  return tunnel;
}

std::shared_ptr<LabelForwardingInformationBase>
ThriftConfigApplier::updateStaticMplsRoutes(
    const std::vector<cfg::StaticMplsRouteWithNextHops>&
        staticMplsRoutesWithNhops,
    const std::vector<cfg::StaticMplsRouteNoNextHops>& staticMplsRoutesToNull,
    const std::vector<cfg::StaticMplsRouteNoNextHops>& staticMplsRoutesToCPU) {
  if (FLAGS_mpls_rib) {
    return nullptr;
  }
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
      auto inftToReach =
          new_->getInterfaces()->getIntfToReach(RouterID(0), nhopAddress);
      if (!inftToReach) {
        throw FbossError(
            "static mpls route for label ",
            staticMplsRouteEntry.get_ingressLabel(),
            " has nexthop ",
            nhopAddress.str(),
            " out of interface subnets");
      }
      resolvedNextHops.insert(ResolvedNextHop(
          nhop.addr(),
          inftToReach->getID(),
          nhop.weight(),
          nhop.labelForwardingAction()));
    }
    auto entry = labelFib->getLabelForwardingEntryIf(
        staticMplsRouteEntry.get_ingressLabel());
    if (!entry) {
      auto node = createLabelForwardingEntry(
          staticMplsRouteEntry.get_ingressLabel(),
          LabelNextHopEntry::Action::NEXTHOPS,
          resolvedNextHops);
      LabelForwardingInformationBase::resolve(node);
      labelFib->addNode(node);
    } else {
      auto entryToUpdate = labelFib->cloneLabelEntry(entry);
      entryToUpdate->update(
          ClientID::STATIC_ROUTE,
          getStaticLabelNextHopEntry(
              LabelNextHopEntry::Action::NEXTHOPS, resolvedNextHops));
      LabelForwardingInformationBase::resolve(entryToUpdate);
      labelFib->updateNode(entryToUpdate);
    }
  }

  for (auto& staticMplsRouteEntry : staticMplsRoutesToNull) {
    auto entry = labelFib->getLabelForwardingEntryIf(
        staticMplsRouteEntry.get_ingressLabel());
    if (!entry) {
      auto node = createLabelForwardingEntry(
          staticMplsRouteEntry.get_ingressLabel(),
          LabelNextHopEntry::Action::DROP,
          LabelNextHopSet());
      LabelForwardingInformationBase::resolve(node);
      labelFib->addNode(node);
    } else {
      auto entryToUpdate = labelFib->cloneLabelEntry(entry);
      entryToUpdate->update(
          ClientID::STATIC_ROUTE,
          getStaticLabelNextHopEntry(
              LabelNextHopEntry::Action::DROP, LabelNextHopSet()));
      LabelForwardingInformationBase::resolve(entryToUpdate);
      labelFib->updateNode(entryToUpdate);
    }
  }

  for (auto& staticMplsRouteEntry : staticMplsRoutesToCPU) {
    auto entry = labelFib->getLabelForwardingEntryIf(
        staticMplsRouteEntry.get_ingressLabel());
    if (!entry) {
      auto node = createLabelForwardingEntry(
          staticMplsRouteEntry.get_ingressLabel(),
          LabelNextHopEntry::Action::TO_CPU,
          LabelNextHopSet());
      LabelForwardingInformationBase::resolve(node);
      labelFib->addNode(node);
    } else {
      auto entryToUpdate = labelFib->cloneLabelEntry(entry);
      entryToUpdate->update(
          ClientID::STATIC_ROUTE,
          getStaticLabelNextHopEntry(
              LabelNextHopEntry::Action::TO_CPU, LabelNextHopSet()));
      LabelForwardingInformationBase::resolve(entryToUpdate);
      labelFib->updateNode(entryToUpdate);
    }
  }
  return labelFib;
}

shared_ptr<SwitchState> applyThriftConfig(
    const shared_ptr<SwitchState>& state,
    const cfg::SwitchConfig* config,
    const Platform* platform,
    RoutingInformationBase* rib,
    AclNexthopHandler* aclNexthopHandler,
    const PlatformMapping* platformMapping) {
  cfg::SwitchConfig emptyConfig;
  return ThriftConfigApplier(
             state, config, platform, rib, aclNexthopHandler, platformMapping)
      .run();
}
shared_ptr<SwitchState> applyThriftConfig(
    const shared_ptr<SwitchState>& state,
    const cfg::SwitchConfig* config,
    const Platform* platform,
    RouteUpdateWrapper* routeUpdater,
    AclNexthopHandler* aclNexthopHandler,
    const PlatformMapping* platformMapping) {
  cfg::SwitchConfig emptyConfig;
  return ThriftConfigApplier(
             state,
             config,
             platform,
             routeUpdater,
             aclNexthopHandler,
             platformMapping)
      .run();
}

} // namespace facebook::fboss
