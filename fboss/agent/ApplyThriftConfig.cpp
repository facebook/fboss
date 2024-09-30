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
#include <sstream>
#include <string>

#include "fboss/agent/AclNexthopHandler.h"

#include "fboss/agent/AgentFeatures.h"
#include "fboss/agent/AsicUtils.h"
#include "fboss/agent/FbossError.h"
#include "fboss/agent/HwAsicTable.h"
#include "fboss/agent/LacpTypes.h"
#include "fboss/agent/LoadBalancerConfigApplier.h"
#include "fboss/agent/LoadBalancerUtils.h"
#include "fboss/agent/RouteUpdateWrapper.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/SwitchIdScopeResolver.h"
#include "fboss/agent/SwitchInfoUtils.h"
#include "fboss/agent/Utils.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/if/gen-cpp2/mpls_types.h"
#include "fboss/agent/platforms/common/PlatformMapping.h"
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
#include "fboss/agent/state/PortFlowletConfig.h"
#include "fboss/agent/state/PortFlowletConfigMap.h"
#include "fboss/agent/state/PortMap.h"
#include "fboss/agent/state/PortPgConfig.h"
#include "fboss/agent/state/PortQueue.h"
#include "fboss/agent/state/QcmConfig.h"
#include "fboss/agent/state/QosPolicyMap.h"
#include "fboss/agent/state/Route.h"
#include "fboss/agent/state/RouteTypes.h"
#include "fboss/agent/state/SflowCollector.h"
#include "fboss/agent/state/SflowCollectorMap.h"
#include "fboss/agent/state/StateUtils.h"
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

DECLARE_bool(intf_nbr_tables);

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

// Only one buffer pool is supported systemwide. Variable to track the name
// and validate during a config change.
std::optional<std::string> sharedBufferPoolName;

std::shared_ptr<facebook::fboss::SwitchState> updateFibFromConfig(
    const facebook::fboss::SwitchIdScopeResolver* resolver,
    facebook::fboss::RouterID vrf,
    const facebook::fboss::IPv4NetworkToRouteMap& v4NetworkToRoute,
    const facebook::fboss::IPv6NetworkToRouteMap& v6NetworkToRoute,
    const facebook::fboss::LabelToRouteMap& labelToRoute,
    void* cookie) {
  facebook::fboss::ForwardingInformationBaseUpdater fibUpdater(
      resolver, vrf, v4NetworkToRoute, v6NetworkToRoute, labelToRoute);

  auto nextStatePtr =
      static_cast<std::shared_ptr<facebook::fboss::SwitchState>*>(cookie);

  fibUpdater(*nextStatePtr);
  return *nextStatePtr;
}

template <typename MultiMap, typename EntryT>
std::shared_ptr<MultiMap> toMultiSwitchMap(
    const std::shared_ptr<EntryT>& entry,
    const facebook::fboss::SwitchIdScopeResolver& resolver) {
  auto multiMap = std::make_shared<MultiMap>();
  for (const auto& idAndNode : *entry) {
    multiMap->addNode(idAndNode.second, resolver.scope(idAndNode.second));
  }
  return multiMap;
}
template <typename MultiMap, typename Map>
std::shared_ptr<MultiMap> toMultiSwitchMap(
    const std::shared_ptr<Map>& map,
    const facebook::fboss::cfg::SwitchConfig& cfg,
    const facebook::fboss::SwitchIdScopeResolver& resolver) {
  auto multiMap = std::make_shared<MultiMap>();
  for (const auto& idAndNode : *map) {
    multiMap->addNode(idAndNode.second, resolver.scope(idAndNode.second, cfg));
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
      bool supportsAddRemovePort,
      RoutingInformationBase* rib,
      AclNexthopHandler* aclNexthopHandler,
      const PlatformMapping* platformMapping,
      const HwAsicTable* hwAsicTable)
      : orig_(orig),
        cfg_(config),
        supportsAddRemovePort_(supportsAddRemovePort),
        rib_(rib),
        aclNexthopHandler_(aclNexthopHandler),
        scopeResolver_(getSwitchInfoFromConfig(config)),
        platformMapping_(platformMapping),
        hwAsicTable_(hwAsicTable) {}

  ThriftConfigApplier(
      const std::shared_ptr<SwitchState>& orig,
      const cfg::SwitchConfig* config,
      bool supportsAddRemovePort,
      RouteUpdateWrapper* routeUpdater,
      AclNexthopHandler* aclNexthopHandler,
      const PlatformMapping* platformMapping,
      const HwAsicTable* hwAsicTable)
      : orig_(orig),
        cfg_(config),
        supportsAddRemovePort_(supportsAddRemovePort),
        routeUpdater_(routeUpdater),
        aclNexthopHandler_(aclNexthopHandler),
        scopeResolver_(getSwitchInfoFromConfig(config)),
        platformMapping_(platformMapping),
        hwAsicTable_(hwAsicTable) {}

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
      const std::shared_ptr<MultiSwitchTransceiverMap>& transceiverMap);
  std::shared_ptr<SystemPortMap> updateSystemPorts(
      const std::shared_ptr<MultiSwitchPortMap>& ports,
      const std::shared_ptr<MultiSwitchSettings>& multiSwitchSettings);
  std::shared_ptr<MultiSwitchSystemPortMap> updateRemoteSystemPorts(
      const std::shared_ptr<MultiSwitchSystemPortMap>& systemPorts);

  std::shared_ptr<Port> updatePort(
      const std::shared_ptr<Port>& orig,
      const cfg::Port* cfg,
      const std::shared_ptr<TransceiverSpec>& transceiver);
  QueueConfig updatePortQueues(
      const std::vector<std::shared_ptr<PortQueue>>& origPortQueues,
      const std::vector<cfg::PortQueue>& cfgPortQueues,
      uint16_t maxQueues,
      cfg::StreamType streamType,
      std::optional<cfg::QosMap> qosMap = std::nullopt,
      bool resetDefaultQueue = true);
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
  bool isPortFlowletConfigUnchanged(
      std::shared_ptr<PortFlowletCfg> newPortFlowletCfg,
      const shared_ptr<Port>& port);
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
  bool updateMacTable(
      std::shared_ptr<Vlan>& newVlan,
      const std::set<PortDescriptor>& portDescs);
  std::map<uint32_t, std::set<PortDescriptor>>
  getMapOfVlanToPortsNeedingMacClear(
      const std::shared_ptr<MultiSwitchPortMap> newPorts,
      const std::shared_ptr<MultiSwitchPortMap> origPorts);
  std::shared_ptr<Vlan> createVlan(const cfg::Vlan* config);
  std::shared_ptr<Vlan> updateVlan(
      const std::shared_ptr<Vlan>& orig,
      const cfg::Vlan* config,
      const std::set<PortDescriptor>& portDescs);
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
  std::shared_ptr<AclMap> updateAclsImpl(
      cfg::AclStage aclStage,
      std::vector<cfg::AclEntry> configEntries,
      std::optional<std::string> tableName = std::nullopt);
  std::shared_ptr<AclMap> updateAcls(
      cfg::AclStage aclStage,
      std::vector<cfg::AclEntry> configEntries);
  std::shared_ptr<AclMap> updateAclsForTable(
      cfg::AclStage aclStage,
      std::vector<cfg::AclEntry> configEntries,
      std::optional<std::string> tableName = std::nullopt);
  std::shared_ptr<AclEntry> createAcl(
      const cfg::AclEntry* config,
      int priority,
      const MatchAction* action = nullptr,
      bool enable = true);
  void checkUdfAcl(const std::vector<std::string>& udfGroups) const;
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
  template <typename VlanOrIntfT, typename CfgVlanOrIntfT>
  bool updateDhcpOverrides(
      VlanOrIntfT* vlanOrIntf,
      const CfgVlanOrIntfT* config);
  std::shared_ptr<InterfaceMap> updateInterfaces();
  shared_ptr<Interface> createInterface(
      const cfg::Interface* config,
      const Interface::Addresses& addrs);
  shared_ptr<Interface> updateInterface(
      const shared_ptr<Interface>& orig,
      const cfg::Interface* config,
      const Interface::Addresses& addrs);
  bool updateNeighborResponseTablesForIntfs(
      Interface* intf,
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
  shared_ptr<MultiSwitchSettings> updateMultiSwitchSettings();
  shared_ptr<SwitchSettings> updateSwitchSettings(
      HwSwitchMatcher matcher,
      const std::shared_ptr<MultiSwitchSettings>& origSwitchSettings);

  // bufferPool specific configs
  shared_ptr<MultiSwitchBufferPoolCfgMap> updateBufferPoolConfigs(
      bool* changed);
  std::shared_ptr<BufferPoolCfg> createBufferPoolConfig(
      const std::string& id,
      const cfg::BufferPoolConfig& config);
  shared_ptr<QcmCfg> updateQcmCfg(bool* changed);
  shared_ptr<QcmCfg> createQcmCfg(const cfg::QcmConfig& config);
  shared_ptr<MultiControlPlane> updateControlPlane();
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

  std::shared_ptr<MultiLabelForwardingInformationBase> updateStaticMplsRoutes(
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
  void processReachabilityGroup(
      const std::vector<SwitchID>& localFabricSwitchIds);
  void validateUdfConfig(const UdfConfig& newUdfConfig);
  std::shared_ptr<UdfConfig> updateUdfConfig(bool* changed);

  void processInterfaceForPortForNonVoqSwitches(int64_t switchId);
  void processInterfaceForPortForVoqSwitches(int64_t switchId);
  void processInterfaceForPort();

  shared_ptr<FlowletSwitchingConfig> updateFlowletSwitchingConfig(
      bool* changed);
  shared_ptr<FlowletSwitchingConfig> createFlowletSwitchingConfig(
      const cfg::FlowletSwitchingConfig& config);

  shared_ptr<MultiSwitchPortFlowletCfgMap> updatePortFlowletConfigs(
      bool* changed);
  std::shared_ptr<PortFlowletCfg> createPortFlowletConfig(
      const std::string& id,
      const cfg::PortFlowletConfig& config);

  uint32_t generateDeterministicSeed(cfg::LoadBalancerID id);

  folly::MacAddress getLocalMac(SwitchID switchId) const;
  SwitchID getSwitchId(const cfg::Interface& intfConfig) const;
  void addRemoteIntfRoute();
  std::optional<SwitchID> getAnyVoqSwitchId();
  std::vector<SwitchID> getLocalFabricSwitchIds() const;
  std::optional<QueueConfig> getDefaultVoqConfigIfChanged(
      std::shared_ptr<SwitchSettings> switchSettings);
  QueueConfig getVoqConfig(PortID portId);

  std::shared_ptr<SwitchState> orig_;
  std::shared_ptr<SwitchState> new_;
  const cfg::SwitchConfig* cfg_{nullptr};
  bool supportsAddRemovePort_{false};
  RoutingInformationBase* rib_{nullptr};
  RouteUpdateWrapper* routeUpdater_{nullptr};
  AclNexthopHandler* aclNexthopHandler_{nullptr};
  SwitchIdScopeResolver scopeResolver_;
  const PlatformMapping* platformMapping_{nullptr};
  const HwAsicTable* hwAsicTable_{nullptr};

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
    bool portFlowletConfigChanged = false;
    auto newPortFlowletCfg =
        updatePortFlowletConfigs(&portFlowletConfigChanged);
    if (portFlowletConfigChanged) {
      new_->resetPortFlowletCfgs(newPortFlowletCfg);
      changed = true;
    }
  }

  auto newMultiSwitchSettings = updateMultiSwitchSettings();
  if (newMultiSwitchSettings) {
    new_->resetSwitchSettings(newMultiSwitchSettings);
    changed = true;
  }

  processInterfaceForPort();

  {
    auto newPorts = updatePorts(new_->getTransceivers());
    if (newPorts) {
      new_->resetPorts(
          toMultiSwitchMap<MultiSwitchPortMap>(newPorts, scopeResolver_));
      new_->resetSystemPorts(toMultiSwitchMap<MultiSwitchSystemPortMap>(
          updateSystemPorts(new_->getPorts(), new_->getSwitchSettings()),
          scopeResolver_));
      new_->resetRemoteSystemPorts(
          updateRemoteSystemPorts(new_->getSystemPorts()));
      changed = true;
    }
  }

  {
    auto newAggPorts = updateAggregatePorts();
    if (newAggPorts) {
      new_->resetAggregatePorts(toMultiSwitchMap<MultiSwitchAggregatePortMap>(
          newAggPorts, scopeResolver_));
      changed = true;
    }
  }

  // updateMirrors must be called after updatePorts, mirror needs ports!
  {
    auto newMirrors = updateMirrors();
    if (newMirrors) {
      new_->resetMirrors(
          toMultiSwitchMap<MultiSwitchMirrorMap>(newMirrors, scopeResolver_));
      changed = true;
    }
  }

  // updateAcls must be called after updateMirrors, acls may need mirror!
  {
    if (FLAGS_enable_acl_table_group) {
      auto newAclTableGroups = updateAclTableGroups();
      if (newAclTableGroups) {
        new_->resetAclTableGroups(toMultiSwitchMap<MultiSwitchAclTableGroupMap>(
            newAclTableGroups, scopeResolver_));
        changed = true;
      }
    } else {
      auto newAcls = updateAcls(cfg::AclStage::INGRESS, *cfg_->acls());
      if (newAcls) {
        new_->resetAcls(toMultiSwitchMap<MultiSwitchAclMap>(
            std::move(newAcls), scopeResolver_));
        changed = true;
      }
    }
  }

  {
    auto newQosPolicies = updateQosPolicies();
    if (newQosPolicies) {
      new_->resetQosPolicies(toMultiSwitchMap<MultiSwitchQosPolicyMap>(
          newQosPolicies, scopeResolver_));
      changed = true;
    }
  }

  {
    auto newIntfs = updateInterfaces();
    if (newIntfs) {
      new_->resetIntfs(toMultiSwitchMap<MultiSwitchInterfaceMap>(
          std::move(newIntfs), *cfg_, scopeResolver_));
      changed = true;
    }
  }

  // Note: updateInterfaces() must be called before updateVlans(),
  // as updateInterfaces() populates the vlanInterfaces_ data structure.
  {
    auto newVlans = updateVlans();
    if (newVlans) {
      new_->resetVlans(
          toMultiSwitchMap<MultiSwitchVlanMap>(newVlans, scopeResolver_));
      changed = true;
    }
  }

  // Add remote interface routes to route table.
  addRemoteIntfRoute();

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
      new_->resetForwardingInformationBases(
          toMultiSwitchMap<MultiSwitchForwardingInformationBaseMap>(
              newFibs, scopeResolver_));
      changed = true;
    }

    rib_->reconfigure(
        &scopeResolver_,
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
    if (newVlans->getNodeIf(dfltVlan) == nullptr) {
      throw FbossError("Default VLAN ", dfltVlan, " does not exist");
    }
  }

  // Make sure all interfaces refer to valid VLANs.
  for (const auto& vlanInfo : vlanInterfaces_) {
    if (newVlans->getNodeIf(vlanInfo.first) == nullptr) {
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
      new_->resetSflowCollectors(toMultiSwitchMap<MultiSwitchSflowCollectorMap>(
          newCollectors, scopeResolver_));
      changed = true;
    }
  }

  {
    LoadBalancerConfigApplier loadBalancerConfigApplier(
        orig_->getLoadBalancers(), cfg_->get_loadBalancers());
    auto newLoadBalancers = loadBalancerConfigApplier.updateLoadBalancers(
        generateDeterministicSeed(cfg::LoadBalancerID::ECMP),
        generateDeterministicSeed(cfg::LoadBalancerID::AGGREGATE_PORT));
    if (newLoadBalancers) {
      new_->resetLoadBalancers(toMultiSwitchMap<MultiSwitchLoadBalancerMap>(
          newLoadBalancers, scopeResolver_));
      changed = true;
    }
  }

  {
    auto newTunnels = updateIpInIpTunnels();
    if (newTunnels) {
      new_->resetTunnels(
          toMultiSwitchMap<MultiSwitchIpTunnelMap>(newTunnels, scopeResolver_));
      changed = true;
    }
  }

  {
    auto voqSwitchId = getAnyVoqSwitchId();
    std::shared_ptr<SwitchSettings> origSwitchSettings{};
    if (voqSwitchId.has_value()) {
      auto matcher =
          HwSwitchMatcher(std::unordered_set<SwitchID>({*voqSwitchId}));
      origSwitchSettings =
          orig_->getSwitchSettings()->getNodeIf(matcher.matcherString());
    }

    auto newDsfNodes = updateDsfNodes();
    if (newDsfNodes) {
      new_->resetDsfNodes(
          toMultiSwitchMap<MultiSwitchDsfNodeMap>(newDsfNodes, scopeResolver_));
      processUpdatedDsfNodes();
    }
    // defaultVoqConfig is used to program VoQ (which is tied to system
    // port) and hence needs updateSystemPorts() to be invoked if this
    // changes in addition to DsfNodes itself.
    if (newDsfNodes ||
        getDefaultVoqConfigIfChanged(origSwitchSettings).has_value()) {
      new_->resetSystemPorts(toMultiSwitchMap<MultiSwitchSystemPortMap>(
          updateSystemPorts(new_->getPorts(), new_->getSwitchSettings()),
          scopeResolver_));
      changed = true;
    }
  }

  {
    // Update reachability group setting for input balanced
    auto localFabricSwitchIds = getLocalFabricSwitchIds();
    if (FLAGS_enable_balanced_intput_mode && !localFabricSwitchIds.empty()) {
      processReachabilityGroup(localFabricSwitchIds);
    }
  }

  if (!changed) {
    return nullptr;
  }
  return new_;
}

std::optional<SwitchID> ThriftConfigApplier::getAnyVoqSwitchId() {
  std::optional<SwitchID> switchId;
  for (const auto& switchIdAndSwitchInfo :
       *cfg_->switchSettings()->switchIdToSwitchInfo()) {
    if (switchIdAndSwitchInfo.second.switchType() == cfg::SwitchType::VOQ) {
      switchId = static_cast<SwitchID>(switchIdAndSwitchInfo.first);
      break;
    }
  }
  // Returns a switchId only if we have a VoQ switch in config!
  return switchId;
}

std::vector<SwitchID> ThriftConfigApplier::getLocalFabricSwitchIds() const {
  std::vector<SwitchID> localFabricSwitchIds;
  for (const auto& switchIdAndSwitchInfo :
       *cfg_->switchSettings()->switchIdToSwitchInfo()) {
    if (switchIdAndSwitchInfo.second.switchType() == cfg::SwitchType::FABRIC) {
      localFabricSwitchIds.push_back(
          static_cast<SwitchID>(switchIdAndSwitchInfo.first));
    }
  }
  return localFabricSwitchIds;
}

// Return the new defaultVoqConfig if it is different from the
// original config.
std::optional<QueueConfig> ThriftConfigApplier::getDefaultVoqConfigIfChanged(
    std::shared_ptr<SwitchSettings> origSwitchSettings) {
  if (cfg_->defaultVoqConfig()->size()) {
    // TODO(daiweix): do not hardcode 8
    const auto kNumVoqs = 8;
    auto origPortQueues = QueueConfig();
    if (origSwitchSettings &&
        !origSwitchSettings->getDefaultVoqConfig().empty()) {
      origPortQueues = origSwitchSettings->getDefaultVoqConfig();
    }
    std::optional<QueueConfig> defaultVoqConfig = updatePortQueues(
        origPortQueues,
        *cfg_->defaultVoqConfig(),
        kNumVoqs,
        cfg::StreamType::UNICAST,
        std::nullopt,
        false);
    if (!origSwitchSettings ||
        (origSwitchSettings->getDefaultVoqConfig() != *defaultVoqConfig)) {
      return defaultVoqConfig;
    }
  }
  return std::nullopt;
}

QueueConfig ThriftConfigApplier::getVoqConfig(PortID portId) {
  for (const auto& portCfg : *cfg_->ports()) {
    if (PortID(*portCfg.logicalID()) == portId) {
      if (auto portVoqConfigName = portCfg.portVoqConfigName()) {
        auto it = cfg_->portQueueConfigs()->find(*portVoqConfigName);
        if (it == cfg_->portQueueConfigs()->end()) {
          throw FbossError(
              "Port voq config name: ",
              *portVoqConfigName,
              " does not exist in PortQueueConfig map");
        }
        std::vector<cfg::PortQueue> cfgPortVoqs = it->second;
        QueueConfig voqs;
        // TODO(daiweix): do not hardcode 8
        const auto kNumVoqs = 8;
        return updatePortQueues(
            voqs,
            cfgPortVoqs,
            kNumVoqs,
            cfg::StreamType::UNICAST,
            std::nullopt,
            false);
      } else {
        break;
      }
    }
  }
  // use default voq config if not specified
  return utility::getFirstNodeIf(new_->getSwitchSettings())
      ->getDefaultVoqConfig();
}

void ThriftConfigApplier::processUpdatedDsfNodes() {
  bool hasVoqSwitch = false;
  std::unordered_set<SwitchID> localSwitchIds;

  for (auto& [matcherString, switchSettings] :
       std::as_const(*new_->getSwitchSettings())) {
    auto localSwitchId = HwSwitchMatcher(matcherString).switchId();
    localSwitchIds.insert(localSwitchId);

    auto origDsfNode = orig_->getDsfNodes()->getNodeIf(localSwitchId);
    if (origDsfNode &&
        origDsfNode->getType() !=
            new_->getDsfNodes()->getNodeIf(localSwitchId)->getType()) {
      throw FbossError("Change in DSF node type is not supported");
    }

    if (switchSettings->l3SwitchType()) {
      hasVoqSwitch |=
          (switchSettings->l3SwitchType().value() == cfg::SwitchType::VOQ);
    }
  }

  // On VOQ switches, DSF nodes in the SwitchState are consumed by the DSF
  // subscriber to subscribe to every other VOQ switch. Thus, process DSF
  // nodes if at least one of the switchIDs is for VOQ switch.
  if (!hasVoqSwitch) {
    return;
  }

  auto delta = StateDelta(orig_, new_).getDsfNodesDelta();
  auto switchIdToSwitchIndex =
      computeSwitchIdToSwitchIndex(new_->getDsfNodes());

  auto getRecyclePortId = [](const std::shared_ptr<DsfNode>& node) {
    CHECK(node->getSystemPortRange().has_value());
    return *node->getSystemPortRange()->minimum() + 1;
  };

  auto getRecyclePortName =
      [&switchIdToSwitchIndex](const std::shared_ptr<DsfNode>& node) {
        int asicCore;
        switch (node->getAsicType()) {
          case cfg::AsicType::ASIC_TYPE_MOCK:
          case cfg::AsicType::ASIC_TYPE_FAKE:
          case cfg::AsicType::ASIC_TYPE_JERICHO2:
            asicCore = 1;
            break;
          case cfg::AsicType::ASIC_TYPE_JERICHO3:
            asicCore = 441;
            break;
          case cfg::AsicType::ASIC_TYPE_TRIDENT2:
          case cfg::AsicType::ASIC_TYPE_TOMAHAWK:
          case cfg::AsicType::ASIC_TYPE_TOMAHAWK3:
          case cfg::AsicType::ASIC_TYPE_TOMAHAWK4:
          case cfg::AsicType::ASIC_TYPE_ELBERT_8DD:
          case cfg::AsicType::ASIC_TYPE_EBRO:
          case cfg::AsicType::ASIC_TYPE_GARONNE:
          case cfg::AsicType::ASIC_TYPE_SANDIA_PHY:
          case cfg::AsicType::ASIC_TYPE_RAMON:
          case cfg::AsicType::ASIC_TYPE_TOMAHAWK5:
          case cfg::AsicType::ASIC_TYPE_YUBA:
          case cfg::AsicType::ASIC_TYPE_RAMON3:
            throw FbossError(
                "Recycle ports are not applicable for AsicType: ",
                apache::thrift::util::enumNameSafe(node->getAsicType()));
        }

        auto iter = switchIdToSwitchIndex.find(node->getSwitchId());
        // switchIdToSwitchIndex is computed from DsfNode map. Thus, we should
        // always find the switchId
        CHECK(iter != switchIdToSwitchIndex.end());
        auto switchIndex = iter->second;

        // Recycle port name format:
        //    rcy<pim_id>/<npu_id>/<npu_core>
        // pmi_id is always 1 for Recycle port.
        // npu_id = switchIndex + 1 (switchIndex strarts at 0)
        return folly::sformat(
            "{}:rcy1/{}/{}", node->getName(), switchIndex + 1, asicCore);
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
        auto mac = node->getMac() ? *node->getMac() : folly::MacAddress();
        return HwAsic::makeAsic(
            node->getAsicType(),
            cfg::SwitchType::VOQ,
            static_cast<int64_t>(node->getSwitchId()),
            0, /* dummy switchIndex*/
            node->getSystemPortRange(),
            mac,
            std::nullopt);
      };
  auto processLoopbacks = [&](const std::shared_ptr<DsfNode>& node,
                              const HwAsic* dsfNodeAsic) {
    auto recyclePortId = getRecyclePortId(node);
    InterfaceID intfID(recyclePortId);
    auto intfs = isLocal(node) ? new_->getInterfaces()->modify(&new_)
                               : new_->getRemoteInterfaces()->modify(&new_);
    auto intf = intfs->getNode(intfID)->clone();
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
      neighbor.resolvedSince() = static_cast<int64_t>(std::time(nullptr));
      // For local loopback IPs we set noHostRoute to true
      // since we dont want to install the host/neighbor entry
      // in HW (the same /128, /32 is covered by ip2me routes).
      // However we still want to program the encap info (MAC,
      // encap ID) in HW, so we continue to send the entry to
      // send it to SDK, but with noHostRoute flag set.
      neighbor.noHostRoute() = isLocal(node);
      if (network.first.isV6()) {
        ndpTable.insert({*neighbor.ipaddress(), neighbor});
      } else {
        arpTable.insert({*neighbor.ipaddress(), neighbor});
      }
    }
    intf->setAddresses(std::move(addresses));
    intf->setArpTable(std::move(arpTable));
    intf->setNdpTable(std::move(ndpTable));
    intfs->updateNode(intf, scopeResolver_.scope(intf, new_));
  };
  auto addDsfNode = [&](const std::shared_ptr<DsfNode>& node) {
    if (!isInterfaceNode(node)) {
      // Only create recycle ports for INs
      return;
    }
    auto dsfNodeAsic = getDsfNodeAsic(node);
    if (isLocal(node)) {
      processLoopbacks(node, dsfNodeAsic.get());
      // For local asic recycle port and sys port information will come
      // via local config. So only process need to process loopback IPs here.
      return;
    }
    auto recyclePortId = getRecyclePortId(node);
    auto sysPort = std::make_shared<SystemPort>(
        SystemPortID(recyclePortId),
        std::make_optional(RemoteSystemPortType::STATIC_ENTRY));
    sysPort->setSwitchId(node->getSwitchId());
    sysPort->setName(getRecyclePortName(node));
    const auto& recyclePortInfo = dsfNodeAsic->getRecyclePortInfo();
    sysPort->setCoreIndex(recyclePortInfo.coreId);
    sysPort->setCorePortIndex(recyclePortInfo.corePortIndex);
    sysPort->setSpeedMbps(recyclePortInfo.speedMbps); // 10G
    // TODO(daiweix): do not hardcode 8
    sysPort->setNumVoqs(8);
    sysPort->setScope(cfg::Scope::GLOBAL);
    // TODO(daiweix): use voq config of rcy ports, hardcode rcy portID 1 for now
    sysPort->resetPortQueues(getVoqConfig(PortID(1)));
    if (auto cpuTrafficPolicy = cfg_->cpuTrafficPolicy()) {
      if (auto trafficPolicy = cpuTrafficPolicy->trafficPolicy()) {
        if (auto defaultQosPolicy = trafficPolicy->defaultQosPolicy()) {
          sysPort->setQosPolicy(*defaultQosPolicy);
        }
      }
    }
    auto sysPorts = new_->getRemoteSystemPorts()->modify(&new_);
    sysPorts->addNode(sysPort, scopeResolver_.scope(sysPort));
    CHECK(node->getMac().has_value());
    auto intf = std::make_shared<Interface>(
        InterfaceID(recyclePortId),
        RouterID(0),
        std::optional<VlanID>(std::nullopt),
        folly::StringPiece(sysPort->getName()),
        *node->getMac(),
        9000,
        true,
        true,
        cfg::InterfaceType::SYSTEM_PORT,
        isLocal(node) ? std::optional<RemoteInterfaceType>(std::nullopt)
                      : std::make_optional(RemoteInterfaceType::STATIC_ENTRY),
        std::optional<LivenessStatus>(std::nullopt),
        cfg::Scope::GLOBAL);
    auto intfs = new_->getRemoteInterfaces()->modify(&new_);
    intfs->addNode(intf, scopeResolver_.scope(intf, new_));
    processLoopbacks(node, dsfNodeAsic.get());
  };
  auto rmDsfNode = [&](const std::shared_ptr<DsfNode>& node) {
    if (!isInterfaceNode(node)) {
      return;
    }
    if (!isLocal(node)) {
      auto recyclePortId = getRecyclePortId(node);
      auto sysPorts = new_->getRemoteSystemPorts()->modify(&new_);
      sysPorts->removeNode(SystemPortID(recyclePortId));
      auto intfs = new_->getRemoteInterfaces()->modify(&new_);
      intfs->removeNode(InterfaceID(recyclePortId));
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

void ThriftConfigApplier::processReachabilityGroup(
    const std::vector<SwitchID>& localFabricSwitchIds) {
  std::unordered_map<std::string, std::vector<uint32_t>> switchNameToSwitchIds;

  // TODO(zecheng): Update to look at DsfNode layer config once available.

  // To determine if Dsf cluster is single stage:
  // If there's more than one clusterIds in dsfNode map, it's in multi-stage.
  std::unordered_set<int> clusterIds;
  for (const auto& [_, dsfNodeMap] : std::as_const(*new_->getDsfNodes())) {
    for (const auto& [_, dsfNode] : std::as_const(*dsfNodeMap)) {
      std::string nodeName = dsfNode->getName();
      auto iter = switchNameToSwitchIds.find(nodeName);
      if (iter != switchNameToSwitchIds.end()) {
        iter->second.push_back(dsfNode->getID());
      } else {
        switchNameToSwitchIds[nodeName] = {dsfNode->getID()};
      }
      if (auto clusterId = dsfNode->getClusterId()) {
        clusterIds.insert(*clusterId);
      }
    }
  }
  bool isSingleStageCluster = clusterIds.size() <= 1;

  auto updateReachabilityGroupListSize =
      [&](const auto fabricSwitchId, const auto reachabilityGroupListSize) {
        auto matcher = HwSwitchMatcher(std::unordered_set<SwitchID>(
            {static_cast<SwitchID>(fabricSwitchId)}));
        if (new_->getSwitchSettings()
                ->getNodeIf(matcher.matcherString())
                ->getReachabilityGroupListSize() != reachabilityGroupListSize) {
          auto newMultiSwitchSettings = new_->getSwitchSettings()->clone();
          auto newSwitchSettings =
              newMultiSwitchSettings->getNodeIf(matcher.matcherString())
                  ->clone();
          newSwitchSettings->setReachabilityGroupListSize(
              reachabilityGroupListSize);
          newMultiSwitchSettings->updateNode(
              matcher.matcherString(), newSwitchSettings);
          new_->resetSwitchSettings(newMultiSwitchSettings);
        }
      };

  bool parallelVoqLinks = haveParallelLinksToInterfaceNodes(
      cfg_, localFabricSwitchIds, switchNameToSwitchIds, scopeResolver_);

  if (!isSingleStageCluster || parallelVoqLinks) {
    auto newPortMap = new_->getPorts()->modify(&new_);
    for (const auto& fabricSwitchId : localFabricSwitchIds) {
      std::unordered_map<int, int> destinationId2ReachabilityGroup;
      for (const auto& portCfg : *cfg_->ports()) {
        if (scopeResolver_.scope(portCfg).has(SwitchID(fabricSwitchId)) &&
            portCfg.expectedNeighborReachability()->size() > 0) {
          auto neighborRemoteSwitchId =
              getRemoteSwitchID(cfg_, portCfg, switchNameToSwitchIds);
          const auto& neighborDsfNode =
              new_->getDsfNodes()->getNode(neighborRemoteSwitchId);

          int destinationId;
          // For parallel VOQ links, assign links reaching the same VOQ
          // switch.
          if (parallelVoqLinks &&
              neighborDsfNode->getType() == cfg::DsfNodeType::INTERFACE_NODE) {
            destinationId = neighborRemoteSwitchId;
          } else if (neighborDsfNode->getClusterId() == std::nullopt) {
            // Assign links based on cluster ID. Note that in dual stage,
            // FE2 has no cluster ID: use -1 for grouping purpose.
            CHECK_EQ(
                static_cast<int>(cfg::DsfNodeType::FABRIC_NODE),
                static_cast<int>(neighborDsfNode->getType()));
            destinationId = -1;
          } else {
            destinationId = neighborDsfNode->getClusterId().value();
          }

          auto [it, inserted] = destinationId2ReachabilityGroup.insert(
              {destinationId, destinationId2ReachabilityGroup.size() + 1});
          auto reachabilityGroupId = it->second;
          if (inserted) {
            XLOG(DBG2) << "Create new reachability group "
                       << reachabilityGroupId << " towards node "
                       << neighborDsfNode->getName() << " with switchId "
                       << neighborRemoteSwitchId;
          } else {
            XLOG(DBG2) << "Add node " << neighborDsfNode->getName()
                       << " with switchId " << neighborRemoteSwitchId
                       << " to existing reachability group "
                       << reachabilityGroupId;
          }

          auto newPort =
              newPortMap->getNode(PortID(*portCfg.logicalID()))->modify(&new_);
          newPort->setReachabilityGroupId(reachabilityGroupId);
        }
      }
      updateReachabilityGroupListSize(
          fabricSwitchId, destinationId2ReachabilityGroup.size());
    }
  }
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
  auto origUdfConfig = orig_->getUdfConfig() ? orig_->getUdfConfig()
                                             : std::make_shared<UdfConfig>();
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
  auto switchInfoItr =
      cfg_->switchSettings()->switchIdToSwitchInfo()->find(switchId);
  CHECK(switchInfoItr != cfg_->switchSettings()->switchIdToSwitchInfo()->end());
  CHECK(dsfNodeItr->second.systemPortRange().has_value());
  CHECK(switchInfoItr->second.portIdRange().has_value());
  auto systemPortRange = dsfNodeItr->second.systemPortRange();
  auto portIdRange = switchInfoItr->second.portIdRange();
  CHECK(systemPortRange);
  for (const auto& portCfg : *cfg_->ports()) {
    auto portType = *portCfg.portType();
    auto portID = PortID(*portCfg.logicalID());
    // Only process ports belonging to the passed switchId
    if (scopeResolver_.scope(portCfg).has(SwitchID(switchId))) {
      switch (portType) {
        case cfg::PortType::INTERFACE_PORT:
        case cfg::PortType::RECYCLE_PORT:
        case cfg::PortType::EVENTOR_PORT:
        case cfg::PortType::MANAGEMENT_PORT: {
          /*
           * System port is 1:1 with every interface and recycle port.
           * Interface is 1:1 with system port.
           * InterfaceID is chosen to be the same as systemPortID. Thus:
           * For multi ASIC switches, the the port ID range minimum must
           * taken into account while computing the interface ID.
           *
           * For eg:
           *
           * ----------------------------------------------
           * |   config        |   asic 0   |   asic 1    |
           * ----------------------------------------------
           * | sys port range  |   100-199  |  200-299    |
           * ----------------------------------------------
           * | port range      |   0-2047   | 2048-4096   |
           * ----------------------------------------------
           *
           * Interface of recycle port on asic 1 is 201.
           * Port ID of a recycle port in platform mapping will be 2049
           * Interface ID of recycle port = 200 + 2049 - 2048 = 201
           */
          auto interfaceID = SystemPortID{
              *systemPortRange->minimum() + portID - *portIdRange->minimum()};
          port2InterfaceId_[portID].push_back(interfaceID);
        } break;
        case cfg::PortType::FABRIC_PORT:
        case cfg::PortType::CPU_PORT:
          // no interface for fabric/cpu port
          break;
      }
    }
  }
}

void ThriftConfigApplier::processInterfaceForPortForNonVoqSwitches(
    int64_t switchId) {
  flat_map<VlanID, InterfaceID> vlan2InterfaceId;
  for (const auto& interfaceCfg : *cfg_->interfaces()) {
    vlan2InterfaceId[VlanID(*interfaceCfg.vlanID())] =
        InterfaceID(*interfaceCfg.intfID());
  }

  for (const auto& portCfg : *cfg_->ports()) {
    auto portID = PortID(*portCfg.logicalID());
    if (!scopeResolver_.scope(portCfg).has(SwitchID(switchId))) {
      continue;
    }

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
  const auto& switchSettings =
      utility::getFirstNodeIf(new_->getSwitchSettings());
  // Build Port -> interface mappings in port2InterfaceId_
  for (const auto& switchIdAndInfo :
       switchSettings->getSwitchIdToSwitchInfo()) {
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
    const std::shared_ptr<MultiSwitchPortMap>& ports,
    const std::shared_ptr<MultiSwitchSettings>& multiSwitchSettings) {
  // TODO(daiweix): do not hardcode 8
  const auto kNumVoqs = 8;

  static const std::set<cfg::PortType> kCreateSysPortsFor = {
      cfg::PortType::INTERFACE_PORT,
      cfg::PortType::RECYCLE_PORT,
      cfg::PortType::MANAGEMENT_PORT,
      cfg::PortType::EVENTOR_PORT};
  auto sysPorts = std::make_shared<SystemPortMap>();

  for (const auto& [matcherString, portMap] : std::as_const(*ports)) {
    auto switchId = HwSwitchMatcher(matcherString).switchId();
    auto switchSettings = multiSwitchSettings->getNodeIf(matcherString);
    if (switchSettings->l3SwitchType() != cfg::SwitchType::VOQ) {
      continue;
    }

    auto dsfNode = cfg_->dsfNodes()->find(switchId)->second;
    auto nodeName = *dsfNode.name();

    for (const auto& port : std::as_const(*portMap)) {
      if (kCreateSysPortsFor.find(port.second->getPortType()) ==
          kCreateSysPortsFor.end()) {
        continue;
      }
      auto sysPort = std::make_shared<SystemPort>(getSystemPortID(
          port.second->getID(),
          switchSettings->getSwitchIdToSwitchInfo(),
          switchId));
      sysPort->setSwitchId(SwitchID(switchId));
      sysPort->setName(
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
      sysPort->setQosPolicy(port.second->getQosPolicy());
      sysPort->resetPortQueues(getVoqConfig(port.second->getID()));
      // TODO(daiweix): remove this CHECK_EQ after verifying scope config is
      // always correct
      CHECK_EQ(
          (int)platformPort.mapping()->scope().value(),
          (int)port.second->getScope());
      sysPort->setScope(port.second->getScope());
      sysPorts->addSystemPort(std::move(sysPort));
    }
  }

  return sysPorts;
}

std::shared_ptr<MultiSwitchSystemPortMap>
ThriftConfigApplier::updateRemoteSystemPorts(
    const std::shared_ptr<MultiSwitchSystemPortMap>& systemPorts) {
  if (scopeResolver_.hasVoq() &&
      scopeResolver_.scope(cfg::SwitchType::VOQ).size() <= 1) {
    // remote system ports are applicable only for voq switches
    // remote system ports are updated on config only when more than voq
    // switches are configured on a given SwSwitch
    return orig_->getRemoteSystemPorts();
  }
  auto remoteSystemPorts = orig_->getRemoteSystemPorts()->clone();
  for (const auto& [matcherStr, singleSwitchIdSysPorts] :
       std::as_const(*systemPorts)) {
    auto matcher = HwSwitchMatcher(matcherStr);
    CHECK_EQ(matcher.switchIds().size(), 1);
    auto remoteSystemPortMapMatcher =
        scopeResolver_.scope(cfg::SwitchType::VOQ);
    remoteSystemPortMapMatcher.exclude(matcher.switchIds());
    if (remoteSystemPorts->getMapNodeIf(remoteSystemPortMapMatcher)) {
      remoteSystemPorts->updateMapNode(
          singleSwitchIdSysPorts, remoteSystemPortMapMatcher);
    } else {
      remoteSystemPorts->addMapNode(
          singleSwitchIdSysPorts, remoteSystemPortMapMatcher);
    }
  }
  return remoteSystemPorts;
}

shared_ptr<PortMap> ThriftConfigApplier::updatePorts(
    const std::shared_ptr<MultiSwitchTransceiverMap>& transceiverMap) {
  const auto origPorts = orig_->getPorts();
  PortMap::NodeContainer newPorts;
  bool changed = false;

  sharedBufferPoolName.reset();

  // Process all supplied port configs
  for (const auto& portCfg : *cfg_->ports()) {
    PortID id(*portCfg.logicalID());
    auto origPort = origPorts->getNodeIf(id);
    std::shared_ptr<Port> newPort;
    // Find present Transceiver if it exists in TransceiverMap
    std::shared_ptr<TransceiverSpec> transceiver;
    auto platformPort = platformMapping_->getPlatformPort(id);
    const auto& chips = platformMapping_->getChips();
    if (auto tcvrID = utility::getTransceiverId(platformPort, chips)) {
      transceiver = transceiverMap->getNodeIf(*tcvrID);
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

  for (const auto& origPortMap : std::as_const(*origPorts)) {
    for (const auto& origPort : std::as_const(*origPortMap.second)) {
      // This port was listed in the config, and has already been configured
      if (newPorts.find(origPort.second->getID()) != newPorts.end()) {
        continue;
      }

      // For platforms that support add/removing ports, we should leave the
      // ports without configs out of the switch state. For BCM tests + hardware
      // that doesn't allow add/remove, we need to leave the ports in the switch
      // state with a default (disabled) config.
      if (supportsAddRemovePort_) {
        changed = true;
      } else {
        throw FbossError(
            "New config is missing configuration for port ",
            origPort.second->getID());
      }
    }
  }

  if (!changed) {
    return nullptr;
  }

  auto ports = std::make_shared<PortMap>(std::move(newPorts));
  return ports;
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
  if (auto maxDynamicSharedBytes = cfg->maxDynamicSharedBytes()) {
    queue->setMaxDynamicSharedBytes(*maxDynamicSharedBytes);
  }
  if (auto bufferPoolName = cfg->bufferPoolName()) {
    auto bufferPoolCfgMap = new_->getBufferPoolCfgs();
    // bufferPool cfg is keyed on the buffer pool name
    auto bufferPoolCfg = bufferPoolCfgMap->getNodeIf(*bufferPoolName);
    if (!bufferPoolCfg) {
      throw FbossError(
          "Queue: ",
          queue->getID(),
          " buffer pool: ",
          *bufferPoolName,
          " doesn't exist in the bufferPool map.");
    }
    queue->setBufferPoolName(*bufferPoolName);
    queue->setBufferPoolConfig(bufferPoolCfg);
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
    // If we have non-zero value in headroom, then its a lossless PG
    if ((portPgCfg->getHeadroomLimitBytes().has_value() &&
         *portPgCfg->getHeadroomLimitBytes() != 0) ||
        FLAGS_allow_zero_headroom_for_lossless_pg) {
      tmpPfcPri.push_back(static_cast<int16_t>(portPgCfg->getID()));
    }
  }
  if (tmpPfcPri.empty()) {
    return std::nullopt;
  }

  return tmpPfcPri;
}

bool ThriftConfigApplier::isPortFlowletConfigUnchanged(
    std::shared_ptr<PortFlowletCfg> newPortFlowletCfg,
    const shared_ptr<Port>& port) {
  std::shared_ptr<PortFlowletCfg> oldPortFlowletCfg{nullptr};
  if (port->getPortFlowletConfig().has_value()) {
    oldPortFlowletCfg = port->getPortFlowletConfig().value();
  }
  // old port flowlet cfg exists and new one doesn't or vice versa
  if ((newPortFlowletCfg && !oldPortFlowletCfg) ||
      (!newPortFlowletCfg && oldPortFlowletCfg)) {
    return false;
  }
  // contents changed in the port flowlet cfg
  if (oldPortFlowletCfg && newPortFlowletCfg) {
    if (*oldPortFlowletCfg != *newPortFlowletCfg) {
      return false;
    }
  }
  return true;
}

QueueConfig ThriftConfigApplier::updatePortQueues(
    const QueueConfig& origPortQueues,
    const std::vector<cfg::PortQueue>& cfgPortQueues,
    uint16_t maxQueues,
    cfg::StreamType streamType,
    std::optional<cfg::QosMap> qosMap,
    bool resetDefaultQueue) {
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
      newPortQueues.push_back(newPortQueue);
    } else if (resetDefaultQueue) {
      // Resetting defaut queues are not applicable to VOQs - we only configure
      // the ones present in config.
      newPortQueue = std::make_shared<PortQueue>(static_cast<uint8_t>(queueId));
      newPortQueue->setStreamType(streamType);
      if (streamType == cfg::StreamType::FABRIC_TX) {
        newPortQueue->setScheduling(cfg::QueueScheduling::INTERNAL);
      }
      newPortQueues.push_back(newPortQueue);
    }
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
        maxQueues);
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

  const auto& switchSettings =
      utility::getFirstNodeIf(new_->getSwitchSettings());
  // For now, we only support update unicast port queues for ports
  auto switchIds =
      SwitchIdScopeResolver(switchSettings->getSwitchIdToSwitchInfo())
          .scope(orig->getID())
          .switchIds();
  CHECK_EQ(switchIds.size(), 1);
  auto asic = hwAsicTable_->getHwAsicIf(*switchIds.begin());
  CHECK(asic != nullptr);
  QueueConfig portQueues;
  for (auto streamType : asic->getQueueStreamTypes(*portConf->portType())) {
    auto maxQueues =
        asic->getDefaultNumPortQueues(streamType, *portConf->portType());
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
      if (!asic->isSupported(HwAsic::Feature::PFC)) {
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
  platformMapping_->customizePlatformPortConfigOverrideFactor(factor);
  PlatformPortProfileConfigMatcher matcher{
      *portConf->profileID(), orig->getID(), factor};

  auto portProfileCfg = platformMapping_->getPortProfileConfig(matcher);
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
  auto pfcPrioritiesUnchanged = !newPfcPriorities && oldPfcPriorities.empty();
  if (newPfcPriorities && !oldPfcPriorities.empty() &&
      (*newPfcPriorities).size() == oldPfcPriorities.size()) {
    pfcPrioritiesUnchanged = true;
    for (int i = 0; i < (*newPfcPriorities).size(); ++i) {
      if (static_cast<PfcPriority>((*newPfcPriorities).at(i)) !=
          oldPfcPriorities.at(i)) {
        pfcPrioritiesUnchanged = false;
        break;
      }
    }
  }

  const auto& newPinConfigs = platformMapping_->getPortIphyPinConfigs(matcher);
  auto pinConfigsUnchanged = (newPinConfigs == orig->getPinConfigs());

  XLOG_IF(DBG2, !profileConfigUnchanged || !pinConfigsUnchanged)
      << orig->getName() << " has profileConfig: "
      << (profileConfigUnchanged ? "UNCHANGED" : "CHANGED")
      << ", pinConfigs: " << (pinConfigsUnchanged ? "UNCHANGED" : "CHANGED")
      << ", with matcher:" << matcher.toString();

  // Port drain is applicable to only fabric ports.
  if (*portConf->drainState() == cfg::PortDrainState::DRAINED &&
      *portConf->portType() != cfg::PortType::FABRIC_PORT) {
    throw FbossError(
        "Port ",
        orig->getID(),
        " cannot be drained as it's NOT a DSF fabric port");
  }

  bool portFlowletConfigUnchanged = true;
  auto newFlowletConfigName = std::optional<cfg::PortFlowletConfigName>();
  std::shared_ptr<PortFlowletCfg> portFlowletCfg;
  if (portConf->flowletConfigName().has_value()) {
    newFlowletConfigName = portConf->flowletConfigName().value();
    if (auto portFlowletConfigs = cfg_->portFlowletConfigs()) {
      auto it = portFlowletConfigs->find(newFlowletConfigName.value());
      if (it == portFlowletConfigs->end()) {
        throw FbossError(
            "Port flowlet config name: ",
            *newFlowletConfigName,
            " does not exist in PortFlowletConfig map");
      }
    }
    auto portFlowletCfgMap = new_->getPortFlowletCfgs();
    portFlowletCfg = portFlowletCfgMap->getNodeIf(*newFlowletConfigName);
    if (!portFlowletCfg) {
      throw FbossError(
          "Port:",
          orig->getID(),
          " but flowlet config name: ",
          *newFlowletConfigName,
          " doesn't exist in the port flowlet config map.");
    }
    portFlowletConfigUnchanged =
        isPortFlowletConfigUnchanged(portFlowletCfg, orig);
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
      *portConf->drainState() == orig->getPortDrainState() &&
      portFlowletConfigUnchanged &&
      newFlowletConfigName == orig->getFlowletConfigName()) {
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
  newPort->setFlowletConfigName(newFlowletConfigName);
  newPort->setPortFlowletConfig(portFlowletCfg);
  newPort->setScope(*portConf->scope());
  return newPort;
}

shared_ptr<AggregatePortMap> ThriftConfigApplier::updateAggregatePorts() {
  auto origAggPorts = orig_->getAggregatePorts();
  AggregatePortMap::NodeContainer newAggPorts;
  bool changed = false;

  size_t numExistingProcessed = 0;
  for (const auto& portCfg : *cfg_->aggregatePorts()) {
    AggregatePortID id(*portCfg.key());
    auto origAggPort = origAggPorts->getNodeIf(id);

    shared_ptr<AggregatePort> newAggPort;
    if (origAggPort) {
      newAggPort = updateAggPort(origAggPort, portCfg);
      ++numExistingProcessed;
    } else {
      newAggPort = createAggPort(portCfg);
    }

    changed |= updateMap(&newAggPorts, origAggPort, newAggPort);
  }

  if (numExistingProcessed != origAggPorts->numNodes()) {
    // Some existing aggregate ports were removed.
    CHECK_LE(numExistingProcessed, origAggPorts->numNodes());
    changed = true;
  }

  if (!changed) {
    return nullptr;
  }

  return std::make_shared<AggregatePortMap>(newAggPorts);
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

  if (!subports.empty()) {
    auto switchIds = scopeResolver_.scope(cfg).switchIds();
    if (switchIds.size() > 1) {
      throw FbossError("Multi Switch LAG not supported ", *cfg.key());
    }
    auto asic = hwAsicTable_->getHwAsicIf(*switchIds.begin());
    if (subports.size() > asic->getMaxLagMemberSize()) {
      throw FbossError(
          "Trying to set ",
          (*cfg.memberPorts()).size(),
          " lag members, ",
          "which is greater than the hardware limit ",
          asic->getMaxLagMemberSize());
    }
  }
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
    systemID = getLocalMacAddress();
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

// If create only attributes or speed changes for a port, we
// delete the port and recreate it. In this sequence, before
// a port is deleted, we need to remove VLAN membership and
// before that, need to clear any MACs learnt on that port/
// VLAN. This API identifies VLAN id / ports where all MACs
// learnt need to be cleared.
std::map<uint32_t, std::set<PortDescriptor>>
ThriftConfigApplier::getMapOfVlanToPortsNeedingMacClear(
    const std::shared_ptr<MultiSwitchPortMap> newPorts,
    const std::shared_ptr<MultiSwitchPortMap> origPorts) {
  std::map<uint32_t, std::set<PortDescriptor>> vlanToPortMap;

  for (auto& portMap : std::as_const(*origPorts)) {
    for (auto& [oldPortId, oldPort] : std::as_const(*portMap.second)) {
      auto newPort = std::as_const(*newPorts).getNodeIf(oldPortId);
      if (!newPort || oldPort->getSpeed() != newPort->getSpeed()) {
        // If oldPort does not exist anymore or if the speed has changed,
        // we need to clear mac addresses learnt on the port for all vlans
        // before port removal is attempted.
        for (auto& [vlanId, _] : oldPort->getVlans()) {
          auto vlanIter = vlanToPortMap.find(vlanId);
          if (vlanIter == vlanToPortMap.end()) {
            vlanToPortMap[vlanId] =
                std::set<PortDescriptor>{PortDescriptor(oldPort->getID())};
          } else {
            vlanIter->second.insert(PortDescriptor(oldPort->getID()));
          }
        }
      }
    }
  }
  return vlanToPortMap;
}

shared_ptr<VlanMap> ThriftConfigApplier::updateVlans() {
  const auto& switchSettings =
      utility::getFirstNodeIf(new_->getSwitchSettings());
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
  if (!switchSettings->vlansSupported()) {
    return nullptr;
  }

  auto origVlans = orig_->getVlans();
  VlanMap::NodeContainer newVlans;
  bool changed = false;

  std::map<uint32_t, std::set<PortDescriptor>> vlanToPortMap{};
  if (new_ && orig_) {
    vlanToPortMap =
        getMapOfVlanToPortsNeedingMacClear(new_->getPorts(), orig_->getPorts());
  }
  // Process all supplied VLAN configs
  size_t numExistingProcessed = 0;
  for (const auto& vlanCfg : *cfg_->vlans()) {
    VlanID id(*vlanCfg.id());
    auto origVlan = origVlans->getNodeIf(id);
    shared_ptr<Vlan> newVlan;
    if (origVlan) {
      std::set<PortDescriptor> portDescs;
      auto vlanIter = vlanToPortMap.find(id);
      if (vlanIter != vlanToPortMap.end()) {
        portDescs = vlanIter->second;
      }
      newVlan = updateVlan(origVlan, &vlanCfg, portDescs);
      ++numExistingProcessed;
    } else {
      newVlan = createVlan(&vlanCfg);
    }
    changed |= updateMap(&newVlans, origVlan, newVlan);
  }

  if (numExistingProcessed != origVlans->numNodes()) {
    // Some existing VLANs were removed.
    CHECK_LT(numExistingProcessed, origVlans->numNodes());
    changed = true;
  }

  if (!changed) {
    return nullptr;
  }

  auto vlans = std::make_shared<VlanMap>(std::move(newVlans));
  return vlans;
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

  auto dhcpV4Relay = config->dhcpRelayAddressV4()
      ? IPAddressV4(*config->dhcpRelayAddressV4())
      : IPAddressV4();
  auto dhcpV6Relay = config->dhcpRelayAddressV6()
      ? IPAddressV6(*config->dhcpRelayAddressV6())
      : IPAddressV6("::");
  vlan->setDhcpV4Relay(dhcpV4Relay);
  vlan->setDhcpV6Relay(dhcpV6Relay);

  return vlan;
}

bool ThriftConfigApplier::updateMacTable(
    std::shared_ptr<Vlan>& newVlan,
    const std::set<PortDescriptor>& portDescs) {
  auto newMacTable{newVlan->getMacTable()->clone()};
  int macRemovedCount{0};
  for (auto& [_, macEntry] : std::as_const(*(newVlan->getMacTable()))) {
    if (portDescs.contains(macEntry->getPort())) {
      newMacTable->removeEntry(macEntry->getMac());
      macRemovedCount++;
    }
  }
  if (macRemovedCount) {
    newVlan->setMacTable(newMacTable);
    XLOG(DBG2) << "Removed " << macRemovedCount << " MAC addresses on VLAN "
               << newVlan->getID();
  }
  return macRemovedCount > 0;
}

shared_ptr<Vlan> ThriftConfigApplier::updateVlan(
    const shared_ptr<Vlan>& orig,
    const cfg::Vlan* config,
    const std::set<PortDescriptor>& portSet) {
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

  bool macChanged = updateMacTable(newVlan, portSet);
  if (orig->getName() == *config->name() && oldIntfID == newIntfID &&
      orig->getPorts() == ports && oldDhcpV4Relay == newDhcpV4Relay &&
      oldDhcpV6Relay == newDhcpV6Relay && !changed_neighbor_table &&
      !changed_dhcp_overrides && !macChanged) {
    return nullptr;
  }

  newVlan->setName(*config->name());
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
  if (numExistingProcessed != orig_->getQosPolicies()->numNodes()) {
    // Some existing Qos Policies were removed.
    changed = true;
  }
  if (!changed) {
    return nullptr;
  }

  return std::make_shared<QosPolicyMap>(std::move(newQosPolicies));
}

std::shared_ptr<QosPolicy> ThriftConfigApplier::updateQosPolicy(
    cfg::QosPolicy& qosPolicy,
    int* numExistingProcessed,
    bool* changed) {
  auto origQosPolicy = orig_->getQosPolicies()->getNodeIf(*qosPolicy.name());
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

    if (const auto& trafficClassToVoqIdMap = qosMap->trafficClassToVoqId()) {
      qosPolicyNew->setTrafficClassToVoqIdMap(*trafficClassToVoqIdMap);
    }

    return qosPolicyNew;
  }
  return make_shared<QosPolicy>(*qosPolicy.name(), ingressDscpMap);
}

std::shared_ptr<AclTableGroupMap> ThriftConfigApplier::updateAclTableGroups() {
  auto origAclTableGroups = orig_->getAclTableGroups();
  AclTableGroupMap::NodeContainer newAclTableGroups;

  if (!cfg_->aclTableGroup()) {
    throw FbossError(
        "ACL Table Group must be specified if Multiple ACL Table support is enabled");
  }

  if (cfg_->aclTableGroup()->stage() != cfg::AclStage::INGRESS) {
    throw FbossError("Only ACL Stage INGRESS is supported");
  }

  auto origAclTableGroup =
      origAclTableGroups->getNodeIf(cfg::AclStage::INGRESS);

  auto newAclTableGroup =
      updateAclTableGroup(*cfg_->aclTableGroup()->stage(), origAclTableGroup);
  auto changed =
      updateMap(&newAclTableGroups, origAclTableGroup, newAclTableGroup);

  if (!changed) {
    return nullptr;
  }

  return std::make_shared<AclTableGroupMap>(std::move(newAclTableGroups));
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
                                   ->getNodeIf(aclStage)
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
      orig_->getAclTableGroups()->getNodeIf(aclStage) &&
      orig_->getAclTableGroups()->getNodeIf(aclStage)->getAclTableMap()) {
    origTable = orig_->getAclTableGroups()
                    ->getNodeIf(aclStage)
                    ->getAclTableMap()
                    ->getTableIf(tableName);
  }

  auto newTableEntries = updateAclsForTable(
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
    std::vector<cfg::AclEntry> configEntries) {
  auto acls = updateAclsImpl(aclStage, configEntries);
  if (!acls) {
    return nullptr;
  }
  return acls;
}

std::shared_ptr<AclMap> ThriftConfigApplier::updateAclsForTable(
    cfg::AclStage aclStage,
    std::vector<cfg::AclEntry> configEntries,
    std::optional<std::string> tableName) {
  return updateAclsImpl(aclStage, configEntries, tableName);
}

std::shared_ptr<AclMap> ThriftConfigApplier::updateAclsImpl(
    cfg::AclStage aclStage,
    std::vector<cfg::AclEntry> configEntries,
    std::optional<std::string> tableName) {
  AclMap::NodeContainer newAcls;
  bool changed = false;
  int numExistingProcessed = 0;
  int dataPriority = AclTable::kDataplaneAclMaxPriority;
  int cpuPriority = 1;

  flat_map<std::string, const cfg::TrafficCounter*> counterByName;
  folly::gen::from(*cfg_->trafficCounters()) |
      folly::gen::map([](const cfg::TrafficCounter& counter) {
        return std::make_pair(*counter.name(), &counter);
      }) |
      folly::gen::appendTo(counterByName);

  // Let's get a map of traffic policies to name
  flat_map<std::string, const cfg::MatchToAction*> cpuPolicyByName;
  if (cfg_->cpuTrafficPolicy() && cfg_->cpuTrafficPolicy()->trafficPolicy()) {
    folly::gen::from(
        *cfg_->cpuTrafficPolicy()->trafficPolicy()->matchToAction()) |
        folly::gen::map([](const cfg::MatchToAction& mta) {
          return std::make_pair(*mta.matcher(), &mta);
        }) |
        folly::gen::appendTo(cpuPolicyByName);
  }

  flat_map<std::string, const cfg::MatchToAction*> dataPolicyByName;
  if (cfg_->dataPlaneTrafficPolicy()) {
    folly::gen::from(*cfg_->dataPlaneTrafficPolicy()->matchToAction()) |
        folly::gen::map([](const cfg::MatchToAction& mta) {
          return std::make_pair(*mta.matcher(), &mta);
        }) |
        folly::gen::appendTo(dataPolicyByName);
  }

  // Generates new acls from template
  auto addToAcls = [&]()
      -> const std::vector<std::pair<std::string, std::shared_ptr<AclEntry>>> {
    std::vector<std::pair<std::string, std::shared_ptr<AclEntry>>> entries;
    for (const auto& aclCfg : configEntries) {
      bool enableAcl = true;

      // The ACLs have to be processed in the order in which they are listed in
      // the config. The traffic policy for each ACL may either be in the data
      // or the cpu policy configuration or absent altogether.
      // The ACL has to be created always.
      // Find and use the traffic policy from one of the paths if present.
      const cfg::MatchToAction* matchToAction = nullptr;
      auto cpu = cpuPolicyByName.find(*aclCfg.name());
      auto data = dataPolicyByName.find(*aclCfg.name());
      bool isCoppAcl = false;
      if (cpu != cpuPolicyByName.end()) {
        matchToAction = (cpu->second);
        isCoppAcl = true;
      } else if (data != dataPolicyByName.end()) {
        matchToAction = (data->second);
      }

      // Here is sending to regular port queue action
      MatchAction* ma = nullptr;
      MatchAction matchAction = MatchAction();
      if (matchToAction) {
        const cfg::MatchToAction& mta = *matchToAction;
        if (auto sendToQueue = mta.action()->sendToQueue()) {
          matchAction.setSendToQueue(std::make_pair(*sendToQueue, isCoppAcl));
        }
        // TODO(daiweix): set setTc and userDefinedTrap actions only when
        // disruptive feature sai_user_defined_trap is enabled. Otherwise,
        // although these actions will not take effect and programmed ACL
        // won't change. Switch switch change will still trigger
        // SaiAclTableManager::changedAclEntry() to remove and re-program the
        // same ACL during warmboot. This is unnecessary and caused
        // programming issue on platforms like TH4. Avoiding this issue by
        // skip setting setTc and userDefinedTrap for now.
        if (FLAGS_sai_user_defined_trap) {
          if (auto setTc = mta.action()->setTc()) {
            matchAction.setSetTc(std::make_pair(*setTc, isCoppAcl));
          }
          if (auto userDefinedTrap = mta.action()->userDefinedTrap()) {
            matchAction.setUserDefinedTrap(*userDefinedTrap);
          }
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
        if (auto flowletAction = mta.action()->flowletAction()) {
          matchAction.setFlowletAction(*flowletAction);
        }
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
        ma = &matchAction;
      }

      auto acl = updateAcl(
          aclStage,
          aclCfg,
          isCoppAcl ? cpuPriority++ : dataPriority++,
          &numExistingProcessed,
          &changed,
          tableName,
          ma,
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
    return entries;
  };

  folly::gen::from(addToAcls()) | folly::gen::appendTo(newAcls);

  if (FLAGS_enable_acl_table_group) {
    if (orig_->getAclsForTable(aclStage, tableName.value()) &&
        numExistingProcessed !=
            orig_->getAclsForTable(aclStage, tableName.value())->size()) {
      // Some existing ACLs were removed from the table (multiple acl tables
      // implementation).
      changed = true;
    }
  } else {
    if (numExistingProcessed != orig_->getAcls()->numNodes()) {
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

  return std::make_shared<AclMap>(std::move(newAcls));
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
    origAcl = orig_->getAcls()->getNodeIf(
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

void ThriftConfigApplier::checkUdfAcl(
    const std::vector<std::string>& udfGroups) const {
  if ((udfGroups).size() == 0) {
    throw FbossError("udf group list is empty");
  }
  if (!cfg_->udfConfig()) {
    throw FbossError("No udf config exists");
  }
  auto newUdfConfig = std::make_shared<UdfConfig>();
  newUdfConfig->fromThrift(*cfg_->udfConfig());

  auto udfGroupMap = newUdfConfig->getUdfGroupMap();
  if (udfGroupMap == nullptr) {
    throw FbossError("Udf group map does not exist");
  }

  for (const auto& udfGroupName : udfGroups) {
    if (udfGroupMap->find(udfGroupName) == udfGroupMap->end()) {
      throw FbossError(
          "Udf group in the ACL entry: ",
          udfGroupName,
          " does not exist in the Udf group map");
    }
  }
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

  if (auto udfGroups = config->udfGroups()) {
    checkUdfAcl(*udfGroups);
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
  if (auto udfGroupList = config->udfGroups()) {
    newAcl->setUdfGroups(*udfGroupList);
  }
  if (auto roceOpcode = config->roceOpcode()) {
    newAcl->setRoceOpcode(*roceOpcode);
  }
  if (auto roceBytes = config->roceBytes()) {
    newAcl->setRoceBytes(*roceBytes);
  }
  if (auto roceMask = config->roceMask()) {
    newAcl->setRoceMask(*roceMask);
  }
  if (auto udfTable = config->udfTable()) {
    newAcl->setUdfTable(*udfTable);
  }
  newAcl->setEnabled(enable);
  return newAcl;
}

template <typename VlanOrIntfT, typename CfgVlanOrIntfT>
bool ThriftConfigApplier::updateDhcpOverrides(
    VlanOrIntfT* vlanOrIntf,
    const CfgVlanOrIntfT* config) {
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
  auto oldDhcpV4OverrideMap = vlanOrIntf->getDhcpV4RelayOverrides();
  if (oldDhcpV4OverrideMap != newDhcpV4OverrideMap) {
    vlanOrIntf->setDhcpV4RelayOverrides(newDhcpV4OverrideMap);
    changed = true;
  }
  auto oldDhcpV6OverrideMap = vlanOrIntf->getDhcpV6RelayOverrides();
  if (oldDhcpV6OverrideMap != newDhcpV6OverrideMap) {
    vlanOrIntf->setDhcpV6RelayOverrides(newDhcpV6OverrideMap);
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

bool ThriftConfigApplier::updateNeighborResponseTables(
    Vlan* vlan,
    const cfg::Vlan* config) {
  if (FLAGS_intf_nbr_tables) {
    // Neighbor response tables are consumed from Interface
    return false;
  }

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
    auto origIntf = origIntfs->getNodeIf(id);
    shared_ptr<Interface> newIntf;
    auto newAddrs = getInterfaceAddresses(&interfaceCfg);
    if (interfaceCfg.type() == cfg::InterfaceType::SYSTEM_PORT) {
      auto sysPort =
          new_->getSystemPorts()->getNode(SystemPortID(*interfaceCfg.intfID()));
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
      CHECK_EQ((int)sysPort->getScope(), (int)(*interfaceCfg.scope()));
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

  if (numExistingProcessed != origIntfs->numNodes()) {
    // Some existing interfaces were removed.
    CHECK_LT(numExistingProcessed, origIntfs->numNodes());
    changed = true;
  }

  if (!changed) {
    return nullptr;
  }

  return std::make_shared<InterfaceMap>(std::move(newIntfs));
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
      *config->type(),
      std::optional<RemoteInterfaceType>(std::nullopt),
      std::optional<LivenessStatus>(std::nullopt),
      *config->scope());
  updateNeighborResponseTablesForIntfs(intf.get(), addrs);
  updateDhcpOverrides(intf.get(), config);
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

  auto dhcpV4Relay = config->dhcpRelayAddressV4()
      ? IPAddressV4(*config->dhcpRelayAddressV4())
      : IPAddressV4();
  auto dhcpV6Relay = config->dhcpRelayAddressV6()
      ? IPAddressV6(*config->dhcpRelayAddressV6())
      : IPAddressV6("::");
  intf->setDhcpV4Relay(dhcpV4Relay);
  intf->setDhcpV6Relay(dhcpV6Relay);

  return intf;
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
  auto oldDhcpV4Relay = orig->getDhcpV4Relay();
  auto newDhcpV4Relay = config->dhcpRelayAddressV4()
      ? IPAddressV4(*config->dhcpRelayAddressV4())
      : IPAddressV4();
  auto oldDhcpV6Relay = orig->getDhcpV6Relay();
  auto newDhcpV6Relay = config->dhcpRelayAddressV6()
      ? IPAddressV6(*config->dhcpRelayAddressV6())
      : IPAddressV6("::");

  auto newIntf = orig->clone();
  bool changed_neighbor_table =
      updateNeighborResponseTablesForIntfs(newIntf.get(), addrs);
  bool changed_dhcp_overrides = updateDhcpOverrides(newIntf.get(), config);

  if (orig->getRouterID() == RouterID(*config->routerID()) &&
      (!orig->getVlanIDIf().has_value() ||
       orig->getVlanIDIf().value() == VlanID(*config->vlanID())) &&
      orig->getName() == name && orig->getMac() == mac &&
      orig->getAddressesCopy() == addrs &&
      orig->getNdpConfig()->toThrift() == ndp && orig->getMtu() == mtu &&
      orig->isVirtual() == *config->isVirtual() &&
      orig->isStateSyncDisabled() == *config->isStateSyncDisabled() &&
      orig->getType() == *config->type() && oldDhcpV4Relay == newDhcpV4Relay &&
      oldDhcpV6Relay == newDhcpV6Relay && !changed_neighbor_table &&
      !changed_dhcp_overrides) {
    // No change
    return nullptr;
  }

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
  newIntf->setDhcpV4Relay(newDhcpV4Relay);
  newIntf->setDhcpV6Relay(newDhcpV6Relay);
  newIntf->setScope(*config->scope());
  return newIntf;
}

bool ThriftConfigApplier::updateNeighborResponseTablesForIntfs(
    Interface* intf,
    const Interface::Addresses& addrs) {
  if (!FLAGS_intf_nbr_tables) {
    // Neighbor response tables are consumed from VLANs
    return false;
  }

  auto arpChanged = false, ndpChanged = false;
  auto origArp = intf->getArpResponseTable();
  auto origNdp = intf->getNdpResponseTable();
  ArpResponseTable::NodeContainer arpTable;
  NdpResponseTable::NodeContainer ndpTable;

  auto mac = intf->getMac();
  auto intfID = intf->getID();

  for (const auto& [ip, mask] : addrs) {
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

  arpChanged |= origArp->size() != arpTable.size();
  ndpChanged |= origNdp->size() != ndpTable.size();

  if (arpChanged) {
    intf->setArpResponseTable(origArp->clone(std::move(arpTable)));
  }
  if (ndpChanged) {
    intf->setNdpResponseTable(origNdp->clone(std::move(ndpTable)));
  }
  return arpChanged || ndpChanged;
}

shared_ptr<SflowCollectorMap> ThriftConfigApplier::updateSflowCollectors() {
  auto origCollectors = orig_->getSflowCollectors();
  auto newCollectors = std::make_shared<SflowCollectorMap>();
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
    changed |=
        updateThriftMapNode(newCollectors.get(), origCollector, newCollector);
  }

  if (numExistingProcessed != origCollectors->numNodes()) {
    // Some existing SflowCollectors were removed.
    CHECK_LT(numExistingProcessed, origCollectors->numNodes());
    changed = true;
  }

  if (!changed) {
    return nullptr;
  }

  return newCollectors;
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
  newFlowletSwitchingConfig->setDynamicEgressMinThresholdBytes(
      *config.dynamicEgressMinThresholdBytes());
  newFlowletSwitchingConfig->setDynamicEgressMaxThresholdBytes(
      *config.dynamicEgressMaxThresholdBytes());
  newFlowletSwitchingConfig->setDynamicPhysicalQueueExponent(
      *config.dynamicPhysicalQueueExponent());
  newFlowletSwitchingConfig->setMaxLinks(*config.maxLinks());
  newFlowletSwitchingConfig->setSwitchingMode(*config.switchingMode());
  return newFlowletSwitchingConfig;
}

shared_ptr<MultiSwitchPortFlowletCfgMap>
ThriftConfigApplier::updatePortFlowletConfigs(bool* changed) {
  *changed = false;
  auto origPortFlowletConfigs = orig_->getPortFlowletCfgs();
  PortFlowletCfgMap::NodeContainer newPortFlowletConfigMap;
  auto newCfgedPortFlowlets = cfg_->portFlowletConfigs();

  if (!newCfgedPortFlowlets && !origPortFlowletConfigs->numNodes()) {
    return nullptr;
  }

  if (!newCfgedPortFlowlets && origPortFlowletConfigs->numNodes()) {
    // old cfg eixists but new one doesn't
    *changed = true;
    return std::make_shared<MultiSwitchPortFlowletCfgMap>();
  }

  if (newCfgedPortFlowlets && !origPortFlowletConfigs->numNodes()) {
    *changed = true;
  }

  // if old/new cfgs are present, compare size
  if (origPortFlowletConfigs->numNodes() != (*newCfgedPortFlowlets).size()) {
    *changed = true;
  }

  // origPortFlowletConfigs, newPortFlowletConfigs both are configured
  // and with with same size
  // check if there is any upate on it when compared
  // with last one
  for (auto& portFlowletConfig : *newCfgedPortFlowlets) {
    auto newPortFlowletConfig = createPortFlowletConfig(
        portFlowletConfig.first, portFlowletConfig.second);
    // if port flowlet cfg map exist, check if the specific port flowlet cfg
    // exists or not
    auto origPortFlowletConfig =
        origPortFlowletConfigs->getNodeIf(portFlowletConfig.first);
    if (!origPortFlowletConfig ||
        (*origPortFlowletConfig != *newPortFlowletConfig)) {
      /* new entry added or existing entries do not match */
      *changed = true;
    }
    newPortFlowletConfigMap.emplace(
        std::make_pair(portFlowletConfig.first, newPortFlowletConfig));
  }

  if (*changed) {
    auto portFlowletConfigMap =
        std::make_shared<PortFlowletCfgMap>(std::move(newPortFlowletConfigMap));
    return toMultiSwitchMap<MultiSwitchPortFlowletCfgMap>(
        portFlowletConfigMap, scopeResolver_);
  }
  return nullptr;
}

std::shared_ptr<PortFlowletCfg> ThriftConfigApplier::createPortFlowletConfig(
    const std::string& id,
    const cfg::PortFlowletConfig& portFlowletConfig) {
  auto portFlowletCfg = std::make_shared<PortFlowletCfg>(id);
  portFlowletCfg->setScalingFactor(*portFlowletConfig.scalingFactor());
  portFlowletCfg->setLoadWeight(*portFlowletConfig.loadWeight());
  portFlowletCfg->setQueueWeight(*portFlowletConfig.queueWeight());
  return portFlowletCfg;
}

shared_ptr<MultiSwitchBufferPoolCfgMap>
ThriftConfigApplier::updateBufferPoolConfigs(bool* changed) {
  *changed = false;
  auto origBufferPoolConfigs = orig_->getBufferPoolCfgs();
  BufferPoolCfgMap::NodeContainer newBufferPoolConfigMap;
  auto newCfgedBufferPools = cfg_->bufferPoolConfigs();

  if (!newCfgedBufferPools && !origBufferPoolConfigs->numNodes()) {
    return nullptr;
  }

  if (!newCfgedBufferPools && origBufferPoolConfigs->numNodes()) {
    // old cfg eixists but new one doesn't
    *changed = true;
    return std::make_shared<MultiSwitchBufferPoolCfgMap>();
  }

  if (newCfgedBufferPools && !origBufferPoolConfigs->numNodes()) {
    *changed = true;
  }

  // if old/new cfgs are present, compare size
  if (origBufferPoolConfigs->numNodes() != (*newCfgedBufferPools).size()) {
    *changed = true;
  }

  // origBufferPoolConfigs, newBufferPoolConfigs both are configured
  // and with with same size
  // check if there is any upate on it when compared
  // with last one
  for (auto& bufferPoolConfig : *newCfgedBufferPools) {
    auto newBufferPoolConfig =
        createBufferPoolConfig(bufferPoolConfig.first, bufferPoolConfig.second);
    // if buffer pool cfg map exist, check if the specific buffer pool cfg
    // exists or not
    auto origBufferPoolConfig =
        origBufferPoolConfigs->getNodeIf(bufferPoolConfig.first);
    if (!origBufferPoolConfig ||
        (*origBufferPoolConfig != *newBufferPoolConfig)) {
      /* new entry added or existing entries do not match */
      *changed = true;
    }
    newBufferPoolConfigMap.emplace(
        std::make_pair(bufferPoolConfig.first, newBufferPoolConfig));
  }

  if (*changed) {
    auto bufferPoolConfigMap =
        std::make_shared<BufferPoolCfgMap>(std::move(newBufferPoolConfigMap));
    return toMultiSwitchMap<MultiSwitchBufferPoolCfgMap>(
        bufferPoolConfigMap, scopeResolver_);
  }
  return nullptr;
}

std::shared_ptr<BufferPoolCfg> ThriftConfigApplier::createBufferPoolConfig(
    const std::string& id,
    const cfg::BufferPoolConfig& bufferPoolConfig) {
  auto bufferPoolCfg = std::make_shared<BufferPoolCfg>(id);
  bufferPoolCfg->setSharedBytes(*bufferPoolConfig.sharedBytes());
  if (bufferPoolConfig.headroomBytes().has_value()) {
    bufferPoolCfg->setHeadroomBytes(*bufferPoolConfig.headroomBytes());
  }
  if (bufferPoolConfig.reservedBytes().has_value()) {
    bufferPoolCfg->setReservedBytes(*bufferPoolConfig.reservedBytes());
  }

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

shared_ptr<MultiSwitchSettings>
ThriftConfigApplier::updateMultiSwitchSettings() {
  bool multiSwitchSettingsChange = false;
  auto origMultiSwitchSettings = orig_->getSwitchSettings();
  auto newMultiSwitchSettings = origMultiSwitchSettings
      ? origMultiSwitchSettings->clone()
      : std::make_shared<MultiSwitchSettings>();

  origMultiSwitchSettings = origMultiSwitchSettings
      ? origMultiSwitchSettings
      : std::make_shared<MultiSwitchSettings>();

  if (scopeResolver_.switchIdToSwitchInfo().size() == 0) {
    throw FbossError("SwitchIdToSwitchInfo cannot be empty");
  }

  for (auto& switchIdAndSwitchInfo : scopeResolver_.switchIdToSwitchInfo()) {
    auto switchId = switchIdAndSwitchInfo.first;
    auto matcher = HwSwitchMatcher(
        std::unordered_set<SwitchID>({static_cast<SwitchID>(switchId)}));

    auto origSwitchSettings =
        origMultiSwitchSettings->getNodeIf(matcher.matcherString());

    // If origmultiSwitchSettings is already populated, and if config carries
    // switchId that is not present in origMultiSwitchSettings, throw error
    if (origMultiSwitchSettings->size() != 0) {
      if (!origSwitchSettings) {
        throw FbossError("SwitchId cannot be changed on the fly");
      }
    }

    auto newSwitchSettings =
        updateSwitchSettings(matcher, origMultiSwitchSettings);
    if (newSwitchSettings) {
      if (origSwitchSettings) {
        newMultiSwitchSettings->updateNode(
            matcher.matcherString(), newSwitchSettings);
      } else {
        newMultiSwitchSettings->addNode(
            matcher.matcherString(), newSwitchSettings);
      }
      multiSwitchSettingsChange = true;
    }
  }

  if (multiSwitchSettingsChange) {
    if (newMultiSwitchSettings->empty()) {
      throw FbossError("SwitchSettings cannot be empty");
    }
    return newMultiSwitchSettings;
  }

  return nullptr;
}

shared_ptr<SwitchSettings> ThriftConfigApplier::updateSwitchSettings(
    HwSwitchMatcher matcher,
    const std::shared_ptr<MultiSwitchSettings>& origMultiSwitchSettings) {
  auto origSwitchSettings =
      origMultiSwitchSettings->getNodeIf(matcher.matcherString());
  bool switchSettingsChange = false;
  auto newSwitchSettings = origSwitchSettings
      ? origSwitchSettings->clone()
      : std::make_shared<SwitchSettings>();

  origSwitchSettings = origSwitchSettings ? origSwitchSettings
                                          : std::make_shared<SwitchSettings>();

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

  if (origSwitchSettings->getVendorMacOuis()->toThrift() !=
      *cfg_->switchSettings()->vendorMacOuis()) {
    newSwitchSettings->setVendorMacOuis(
        *cfg_->switchSettings()->vendorMacOuis());
    switchSettingsChange = true;
  }
  if (origSwitchSettings->getMetaMacOuis()->toThrift() !=
      *cfg_->switchSettings()->metaMacOuis()) {
    newSwitchSettings->setMetaMacOuis(*cfg_->switchSettings()->metaMacOuis());
    switchSettingsChange = true;
  }

  auto defaultVoqConfig = getDefaultVoqConfigIfChanged(origSwitchSettings);
  if (defaultVoqConfig.has_value()) {
    newSwitchSettings->setDefaultVoqConfig(*defaultVoqConfig);
    switchSettingsChange = true;
  }

  // TODO - Disallow changing any switchInfo parameter after first
  // config apply. Currently we check only switchId and SwitchType
  // This is to allow rollout of new parameters - portIdRange and
  // switchIndex without breaking warmboot
  auto validateSwitchIdToSwitchInfoChange =
      [](const auto& oldSwitchIdToSwitchInfo,
         const auto& newSwitchIdToSwitchInfo) {
        if (oldSwitchIdToSwitchInfo.size() != newSwitchIdToSwitchInfo.size()) {
          return false;
        }
        for (const auto& switchIdAndInfo : newSwitchIdToSwitchInfo) {
          const auto switchId = switchIdAndInfo.first;
          const auto& switchInfo = switchIdAndInfo.second;
          // Disallow SwitchId and SwitchType changes
          if (oldSwitchIdToSwitchInfo.find(switchId) ==
                  oldSwitchIdToSwitchInfo.end() ||
              switchInfo.switchType() !=
                  oldSwitchIdToSwitchInfo.at(switchId).switchType()) {
            return false;
          }
        }
        return true;
      };

  SwitchIdToSwitchInfo switchIdToSwitchInfo = getSwitchInfoFromConfig(cfg_);
  if (origSwitchSettings->getSwitchIdToSwitchInfo() != switchIdToSwitchInfo) {
    if (origSwitchSettings->getSwitchIdToSwitchInfo().size() &&
        !validateSwitchIdToSwitchInfoChange(
            origSwitchSettings->getSwitchIdToSwitchInfo(),
            switchIdToSwitchInfo)) {
      throw FbossError(
          "SwitchId and SwitchInfo type cannot be changed on the fly");
    }
    newSwitchSettings->setSwitchIdToSwitchInfo(switchIdToSwitchInfo);
    switchSettingsChange = true;
  }

  // computeActualSwitchDrainState relies on minLinksToRemainInVOQDomain and
  // minLinksToJoinVOQDomain. Thus, setting these fields must precede call to
  // computeActualSwitchDrainState.
  std::optional<int32_t> newMinLinksToRemainInVOQDomain{std::nullopt};
  if (cfg_->switchSettings()->minLinksToRemainInVOQDomain()) {
    if (newSwitchSettings->getSwitchIdsOfType(cfg::SwitchType::VOQ).size() ==
        0) {
      throw FbossError(
          "Min links to remain in VOQ Domain is supported only for VOQ switches");
    }

    newMinLinksToRemainInVOQDomain =
        *cfg_->switchSettings()->minLinksToRemainInVOQDomain();
  }
  if (origSwitchSettings->getMinLinksToRemainInVOQDomain() !=
      newMinLinksToRemainInVOQDomain) {
    newSwitchSettings->setMinLinksToRemainInVOQDomain(
        newMinLinksToRemainInVOQDomain);
    switchSettingsChange = true;
  }

  std::optional<int32_t> newMinLinksToJoinVOQDomain{std::nullopt};
  if (cfg_->switchSettings()->minLinksToJoinVOQDomain()) {
    if (newSwitchSettings->getSwitchIdsOfType(cfg::SwitchType::VOQ).size() ==
        0) {
      throw FbossError(
          "Min links to join VOQ Domain is supported only for VOQ switches");
    }

    newMinLinksToJoinVOQDomain =
        *cfg_->switchSettings()->minLinksToJoinVOQDomain();
  }
  if (origSwitchSettings->getMinLinksToJoinVOQDomain() !=
      newMinLinksToJoinVOQDomain) {
    newSwitchSettings->setMinLinksToJoinVOQDomain(newMinLinksToJoinVOQDomain);
    switchSettingsChange = true;
  }

  if (origSwitchSettings->getSwitchDrainState() !=
      *cfg_->switchSettings()->switchDrainState()) {
    auto numVoqSwtitches =
        newSwitchSettings->getSwitchIdsOfType(cfg::SwitchType::VOQ).size();
    auto numFabSwtitches =
        newSwitchSettings->getSwitchIdsOfType(cfg::SwitchType::FABRIC).size();
    if (numFabSwtitches == 0 && numVoqSwtitches == 0) {
      throw FbossError(
          "Switch drain/isolate is supported only on VOQ, Fabric switches");
    }
    newSwitchSettings->setSwitchDrainState(
        *cfg_->switchSettings()->switchDrainState());
    switchSettingsChange = true;
  }

  auto newActualSwitchDrainState = computeActualSwitchDrainState(
      newSwitchSettings, getNumActiveFabricPorts(orig_, matcher));
  if (newActualSwitchDrainState !=
      origSwitchSettings->getActualSwitchDrainState()) {
    newSwitchSettings->setActualSwitchDrainState(newActualSwitchDrainState);
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

  auto oldIcmpV4UnavailableSrcAddress = orig_->getIcmpV4UnavailableSrcAddress();
  auto newIcmpV4UnavailableSrcAddress = cfg_->icmpV4UnavailableSrcAddress();
  if (newIcmpV4UnavailableSrcAddress.has_value()) {
    auto newIcmpV4Address = IPAddressV4(*newIcmpV4UnavailableSrcAddress);
    if (newIcmpV4Address != oldIcmpV4UnavailableSrcAddress) {
      newSwitchSettings->setIcmpV4UnavailableSrcAddress(newIcmpV4Address);
      switchSettingsChange = true;
    }
  }

  auto oldHostname = orig_->getHostname();
  auto newHostname = cfg_->hostname() && !(*cfg_->hostname()).empty()
      ? *cfg_->hostname()
      : getLocalHostname();
  if (oldHostname != newHostname) {
    newSwitchSettings->setHostname(newHostname);
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

  if (switchSettingsChange) {
    return newSwitchSettings;
  }

  return nullptr;
}

shared_ptr<MultiControlPlane> ThriftConfigApplier::updateControlPlane() {
  if (!hwAsicTable_->isFeatureSupportedOnAnyAsic(HwAsic::Feature::CPU_PORT)) {
    if (cfg_->cpuTrafficPolicy()) {
      throw FbossError(
          "No member of ASIC list supports CPU port ",
          folly::join<std::string, std::vector<std::string>>(
              " ", hwAsicTable_->asicNames()));
    }
    return nullptr;
  }
  auto multiSwitchControlPlane = orig_->getControlPlane();
  CHECK_LE(multiSwitchControlPlane->size(), 1);
  auto origCPU = multiSwitchControlPlane->size()
      ? multiSwitchControlPlane->cbegin()->second
      : std::make_shared<ControlPlane>();
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
  auto switchIds = scopeResolver_.scope(origCPU).switchIds();
  CHECK(scopeResolver_.hasL3());
  CHECK_GT(switchIds.size(), 0);
  // all switches on a given box will have same ASIC, so just pick the first
  auto asic = hwAsicTable_->getHwAsicIf(*switchIds.begin());
  for (auto streamType : hwAsicTable_->getCpuPortQueueStreamTypes()) {
    auto tmpPortQueues = updatePortQueues(
        origCPU->getQueuesConfig(),
        *cfg_->cpuQueues(),
        asic->getDefaultNumPortQueues(streamType, cfg::PortType::CPU_PORT),
        streamType,
        qosMap);
    newQueues.insert(
        newQueues.begin(), tmpPortQueues.begin(), tmpPortQueues.end());
  }
  bool queuesUnchanged = false;
  if (origCPU->getQueues()->size() > 0) {
    /* on cold boot original queues are 0 */
    queuesUnchanged = (newQueues.size() == origCPU->getQueues()->size());
    for (int i = 0; i < newQueues.size() && queuesUnchanged; i++) {
      if (*(newQueues.at(i)) != *(origCPU->getQueues()->at(i))) {
        queuesUnchanged = false;
        break;
      }
    }
  }

  if (queuesUnchanged && qosPolicyUnchanged && rxReasonToQueueUnchanged) {
    return nullptr;
  }

  auto newCPU = origCPU->clone();
  newCPU->resetQueues(newQueues);
  newCPU->resetQosPolicy(qosPolicy);
  newCPU->resetRxReasonToQueue(newRxReasonToQueue);
  auto newMultiSwitchControlPlane = std::make_shared<MultiControlPlane>();
  newMultiSwitchControlPlane->addNode(
      scopeResolver_.scope(origCPU).matcherString(), newCPU);
  return newMultiSwitchControlPlane;
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
  return getLocalMac(getSwitchId(*config));
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
    macAddr = getLocalMac(getSwitchId(*config));
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
  for (const auto& mirrorCfg : *cfg_->mirrors()) {
    auto origMirror = origMirrors->getNodeIf(*mirrorCfg.name());
    std::shared_ptr<Mirror> newMirror;
    if (origMirror) {
      newMirror = updateMirror(origMirror, &mirrorCfg);
      ++numExistingProcessed;
    } else {
      newMirror = createMirror(&mirrorCfg);
    }
    changed |= updateThriftMapNode(newMirrors.get(), origMirror, newMirror);
  }

  if (numExistingProcessed != origMirrors->numNodes()) {
    // Some existing Mirrors were removed.
    CHECK_LT(numExistingProcessed, origMirrors->numNodes());
    changed = true;
  }

  std::set<std::string> ingressMirrors;
  for (auto& portMap : std::as_const(*(new_->getPorts()))) {
    for (auto& port : std::as_const(*portMap.second)) {
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
        if (inMirrorMapEntry->second->type() == Mirror::Type::SFLOW) {
          ingressMirrors.insert(portInMirror.value());
        }
      }
      if (portEgMirror.has_value() &&
          newMirrors->find(portEgMirror.value()) == newMirrors->end()) {
        throw FbossError(
            "Mirror ", portEgMirror.value(), " for port is not found");
      }
    }
  }
  if (ingressMirrors.size() > 1) {
    throw FbossError(
        "Only one sflow mirror can be configured across all ports");
  }

  if (!changed) {
    return nullptr;
  }

  auto newMirrorWithSwitchIds = std::make_shared<MirrorMap>();
  for (auto& switchIdAndSwitchInfo :
       *cfg_->switchSettings()->switchIdToSwitchInfo()) {
    if (switchIdAndSwitchInfo.second.switchType() != cfg::SwitchType::VOQ &&
        switchIdAndSwitchInfo.second.switchType() != cfg::SwitchType::NPU) {
      continue;
    }
    auto switchId = switchIdAndSwitchInfo.first;
    for (auto& mirrorMapEntry : std::as_const(*newMirrors)) {
      auto newMirror = mirrorMapEntry.second->clone();
      newMirror->setSwitchId(SwitchID(switchId));
      // TODO: Mirror name is unique per switch, so we need
      // to append the switchId to the mirror name.
      newMirrorWithSwitchIds->insert(
          mirrorMapEntry.first, std::move(newMirror));
    }
  }

  return newMirrorWithSwitchIds;
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
        for (auto& portMap : std::as_const(*(new_->getPorts()))) {
          for (auto& port : std::as_const(*portMap.second)) {
            if (port.second->getName() == egressPort->get_name()) {
              mirrorToPort = port.second;
              break;
            }
          }
        }
        break;
      case cfg::MirrorEgressPort::Type::logicalID:
        mirrorToPort =
            new_->getPorts()->getNodeIf(PortID(egressPort->get_logicalID()));
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
      throw FbossError(
          "MirrorConfig ",
          *mirrorConfig->name(),
          " doesn't match ingress ",
          mirrorToPort && mirrorToPort->getIngressMirror().has_value()
              ? mirrorToPort->getIngressMirror().value()
              : "",
          " or egress mirror ",
          mirrorToPort && mirrorToPort->getEgressMirror().has_value()
              ? mirrorToPort->getEgressMirror().value()
              : "");
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
          !hwAsicTable_->isFeatureSupportedOnAnyAsic(
              HwAsic::Feature::SFLOWv6)) {
        throw FbossError(
            "SFLOWv6 is not supported on this platform for  ",
            *mirrorConfig->name());
      }
      udpPorts = TunnelUdpPorts(
          *sflowTunnel->udpSrcPort(), *sflowTunnel->udpDstPort());
    } else if (auto greTunnel = tunnel->greTunnel()) {
      destinationIp = folly::IPAddress(*greTunnel->ip());
      if (destinationIp->isV6() &&
          !hwAsicTable_->isFeatureSupportedOnAnyAsic(
              HwAsic::Feature::ERSPANv6)) {
        throw FbossError(
            "ERSPANv6 is not supported on this platform ",
            *mirrorConfig->name());
      }
    }

    if (auto srcIpRef = tunnel->srcIp()) {
      srcIp = folly::IPAddress(*srcIpRef);
    }
  }

  uint8_t dscpMark = mirrorConfig->get_dscp();
  bool truncate = mirrorConfig->get_truncate();

  std::optional<PortDescriptor> egressPortDesc;
  if (mirrorEgressPort.has_value()) {
    egressPortDesc = PortDescriptor(mirrorEgressPort.value());
  }

  std::optional<uint32_t> samplingRate;
  if (mirrorConfig->samplingRate().has_value()) {
    samplingRate = mirrorConfig->samplingRate().value();
  }

  auto mirror = make_shared<Mirror>(
      *mirrorConfig->name(),
      egressPortDesc,
      destinationIp,
      srcIp,
      udpPorts,
      dscpMark,
      truncate,
      samplingRate);
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
       newMirror->getEgressPort() == orig->getEgressPort() ||
       newMirror->getEgressPortDesc() == orig->getEgressPortDesc()) &&
      newMirror->getSamplingRate() == orig->getSamplingRate()) {
    if (orig->getMirrorTunnel()) {
      newMirror->setMirrorTunnel(orig->getMirrorTunnel().value());
    }
    if (orig->getEgressPort()) {
      newMirror->setEgressPortDesc(
          PortDescriptor(orig->getEgressPort().value()));
      newMirror->setEgressPort(orig->getEgressPort().value());
    }
    if (orig->getEgressPortDesc()) {
      newMirror->setEgressPortDesc(
          PortDescriptor(orig->getEgressPortDesc().value()));
      newMirror->setEgressPort(orig->getEgressPortDesc().value().phyPortID());
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

    auto origFibContainer = orig_->getFibs()->getNodeIf(vrf);

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

  if (numExistingProcessed != orig_->getFibs()->numNodes()) {
    CHECK_LE(numExistingProcessed, orig_->getFibs()->numNodes());
    changed = true;
  }

  if (!changed) {
    return nullptr;
  }

  return std::make_shared<ForwardingInformationBaseMap>(newFibContainers);
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
  auto newTunnels = std::make_shared<IpTunnelMap>();

  bool changed = false;
  size_t numExistingProcessed = 0;
  if (!cfg_->ipInIpTunnels().has_value()) {
    return nullptr;
  }
  for (const auto& tunnelCfg : cfg_->ipInIpTunnels().value()) {
    auto origTunnel = origTunnels->getNodeIf(*tunnelCfg.ipInIpTunnelId());
    std::shared_ptr<IpTunnel> newTunnel;
    if (origTunnel) {
      newTunnel = updateIpInIpTunnel(origTunnel, &tunnelCfg);
      ++numExistingProcessed;
    } else {
      newTunnel = createIpInIpTunnel(tunnelCfg);
    }

    changed |= updateThriftMapNode(newTunnels.get(), origTunnel, newTunnel);
  }

  if (numExistingProcessed != origTunnels->numNodes()) {
    // Some existing Tunnels were removed.
    CHECK_LT(numExistingProcessed, origTunnels->numNodes());
    changed = true;
  }

  if (!changed) {
    return nullptr;
  }
  return newTunnels;
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

std::shared_ptr<MultiLabelForwardingInformationBase>
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
    auto entry = labelFib->getNodeIf(staticMplsRouteEntry.get_ingressLabel());
    if (!entry) {
      auto node = createLabelForwardingEntry(
          staticMplsRouteEntry.get_ingressLabel(),
          LabelNextHopEntry::Action::NEXTHOPS,
          resolvedNextHops);
      MultiLabelForwardingInformationBase::resolve(node);
      labelFib->addNode(node, scopeResolver_.scope(node));
    } else {
      auto entryToUpdate = entry->clone();
      entryToUpdate->update(
          ClientID::STATIC_ROUTE,
          getStaticLabelNextHopEntry(
              LabelNextHopEntry::Action::NEXTHOPS, resolvedNextHops));
      MultiLabelForwardingInformationBase::resolve(entryToUpdate);
      labelFib->updateNode(entryToUpdate, scopeResolver_.scope(entryToUpdate));
    }
  }

  for (auto& staticMplsRouteEntry : staticMplsRoutesToNull) {
    auto entry = labelFib->getNodeIf(staticMplsRouteEntry.get_ingressLabel());
    if (!entry) {
      auto node = createLabelForwardingEntry(
          staticMplsRouteEntry.get_ingressLabel(),
          LabelNextHopEntry::Action::DROP,
          LabelNextHopSet());
      MultiLabelForwardingInformationBase::resolve(node);
      labelFib->addNode(node, scopeResolver_.scope(node));
    } else {
      auto entryToUpdate = entry->clone();
      entryToUpdate->update(
          ClientID::STATIC_ROUTE,
          getStaticLabelNextHopEntry(
              LabelNextHopEntry::Action::DROP, LabelNextHopSet()));
      MultiLabelForwardingInformationBase::resolve(entryToUpdate);
      labelFib->updateNode(entryToUpdate, scopeResolver_.scope(entryToUpdate));
    }
  }

  for (auto& staticMplsRouteEntry : staticMplsRoutesToCPU) {
    auto entry = labelFib->getNodeIf(staticMplsRouteEntry.get_ingressLabel());
    if (!entry) {
      auto node = createLabelForwardingEntry(
          staticMplsRouteEntry.get_ingressLabel(),
          LabelNextHopEntry::Action::TO_CPU,
          LabelNextHopSet());
      MultiLabelForwardingInformationBase::resolve(node);
      labelFib->addNode(node, scopeResolver_.scope(node));
    } else {
      auto entryToUpdate = entry->clone();
      entryToUpdate->update(
          ClientID::STATIC_ROUTE,
          getStaticLabelNextHopEntry(
              LabelNextHopEntry::Action::TO_CPU, LabelNextHopSet()));
      MultiLabelForwardingInformationBase::resolve(entryToUpdate);
      labelFib->updateNode(entryToUpdate, scopeResolver_.scope(entryToUpdate));
    }
  }
  return labelFib;
}

uint32_t ThriftConfigApplier::generateDeterministicSeed(
    cfg::LoadBalancerID id) {
  if (auto sdkVersion = cfg_->sdkVersion()) {
    if (sdkVersion->saiSdk()) {
      return utility::generateDeterministicSeed(id, getLocalMacAddress(), true);
    }
  }
  return utility::generateDeterministicSeed(id, getLocalMacAddress(), false);
}

SwitchID ThriftConfigApplier::getSwitchId(
    const cfg::Interface& intfConfig) const {
  auto scope = scopeResolver_.scope(
      *intfConfig.type(), InterfaceID(*intfConfig.intfID()), *cfg_);
  CHECK_EQ(scope.switchIds().size(), 1)
      << "Interface can belong to only one switch";
  return scope.switchId();
}

folly::MacAddress ThriftConfigApplier::getLocalMac(SwitchID switchId) const {
  const auto& info = scopeResolver_.switchIdToSwitchInfo();
  auto iter = info.find(switchId);
  if (iter != info.end()) {
    if (auto switchMac = iter->second.switchMac()) {
      return folly::MacAddress(*switchMac);
    }
  }
  XLOG(WARNING) << " No mac address found for switch " << switchId;
  return getLocalMacAddress();
}

void ThriftConfigApplier::addRemoteIntfRoute() {
  // In order to resolve ECMP members pointing to remote nexthops,
  // also treat remote Interfaces as directly connected route in rib.
  // HwSwitch will point remote nextHops as dropped such that switch does not
  // attract traffic for remote nexthops.
  for (const auto& remoteInterfaceMap :
       std::as_const(*orig_->getRemoteInterfaces())) {
    for (const auto& [_, remoteInterface] :
         std::as_const(*remoteInterfaceMap.second)) {
      for (const auto& [addr, mask] :
           std::as_const(*remoteInterface->getAddresses())) {
        intfRouteTables_[remoteInterface->getRouterID()].emplace(
            IPAddress::createNetwork(
                folly::to<std::string>(addr, "/", mask->toThrift())),
            std::make_pair(remoteInterface->getID(), addr));
      }
    }
  }
}

std::shared_ptr<SwitchState> applyThriftConfig(
    const std::shared_ptr<SwitchState>& state,
    const cfg::SwitchConfig* config,
    const bool supportsAddRemovePort,
    const PlatformMapping* platformMapping,
    const HwAsicTable* hwAsicTable,
    RoutingInformationBase* rib,
    AclNexthopHandler* aclNexthopHandler) {
  return ThriftConfigApplier(
             state,
             config,
             supportsAddRemovePort,
             rib,
             aclNexthopHandler,
             platformMapping,
             hwAsicTable)
      .run();
}

std::shared_ptr<SwitchState> applyThriftConfig(
    const std::shared_ptr<SwitchState>& state,
    const cfg::SwitchConfig* config,
    const bool supportsAddRemovePort,
    const PlatformMapping* platformMapping,
    const HwAsicTable* hwAsicTable,
    RouteUpdateWrapper* routeUpdater,
    AclNexthopHandler* aclNexthopHandler) {
  return ThriftConfigApplier(
             state,
             config,
             supportsAddRemovePort,
             routeUpdater,
             aclNexthopHandler,
             platformMapping,
             hwAsicTable)
      .run();
}

} // namespace facebook::fboss
