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
#include "fboss/agent/FbossError.h"
#include "fboss/agent/Platform.h"
#include "fboss/agent/state/AclEntry.h"
#include "fboss/agent/state/AclMap.h"
#include "fboss/agent/state/ArpResponseTable.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/InterfaceMap.h"
#include "fboss/agent/state/NdpResponseTable.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/PortMap.h"
#include "fboss/agent/state/RouteTypes.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/Vlan.h"
#include "fboss/agent/state/VlanMap.h"
#include "fboss/agent/state/Route.h"
#include "fboss/agent/state/RouteTableRib.h"
#include "fboss/agent/state/RouteTable.h"
#include "fboss/agent/state/RouteTableMap.h"
#include "fboss/agent/state/RouteUpdater.h"

#include <boost/container/flat_set.hpp>

using boost::container::flat_map;
using boost::container::flat_set;
using folly::IPAddress;
using folly::IPAddressV6;
using folly::IPAddressV4;
using folly::IPAddressFormatException;
using folly::MacAddress;
using folly::StringPiece;
using folly::CIDRNetwork;
using std::make_shared;
using std::shared_ptr;


namespace facebook { namespace fboss {

/*
 * A class for implementing applyThriftConfig().
 *
 * This implements a procedural function.  It is defined as a class purely as a
 * convenience for the implementation, to allow easily sharing state between
 * internal helper methods.
 */
class ThriftConfigApplier {
 public:
  ThriftConfigApplier(const std::shared_ptr<SwitchState>& orig,
                      const cfg::SwitchConfig* config,
                      const Platform* platform,
                      const cfg::SwitchConfig* prevCfg)
    : orig_(orig),
      cfg_(config),
      platform_(platform),
      prevCfg_(prevCfg) {}

  std::shared_ptr<SwitchState> run();

 private:
  // Forbidden copy constructor and assignment operator
  ThriftConfigApplier(ThriftConfigApplier const &) = delete;
  ThriftConfigApplier& operator=(ThriftConfigApplier const &) = delete;

  template<typename Node, typename NodeMap>
  bool updateMap(NodeMap* map, std::shared_ptr<Node> origNode,
                 std::shared_ptr<Node> newNode) {
    if (newNode) {
      auto ret = map->insert(std::make_pair(newNode->getID(), newNode));
      if (!ret.second) {
        throw FbossError("duplicate entry ", newNode->getID());
      }
      return true;
    } else {
      auto ret = map->insert(std::make_pair(origNode->getID(), origNode));
      if (!ret.second) {
        throw FbossError("duplicate entry ", origNode->getID());
      }
      return false;
    }
  }

  // Interface route prefix. IPAddress has mask applied
  typedef std::pair<folly::IPAddress, uint8_t> Prefix;
  typedef std::pair<InterfaceID, folly::IPAddress> IntfAddress;
  typedef boost::container::flat_map<Prefix, IntfAddress> IntfRoute;
  typedef boost::container::flat_map<RouterID, IntfRoute> IntfRouteTable;
  IntfRouteTable intfRouteTables_;

  void processVlanPorts();
  void updateVlanInterfaces(const Interface* intf);
  std::shared_ptr<PortMap> updatePorts();
  std::shared_ptr<Port> updatePort(const std::shared_ptr<Port>& orig,
                                   const cfg::Port* cfg);
  std::shared_ptr<VlanMap> updateVlans();
  std::shared_ptr<Vlan> createVlan(const cfg::Vlan* config);
  std::shared_ptr<Vlan> updateVlan(const std::shared_ptr<Vlan>& orig,
                                   const cfg::Vlan* config);
  std::shared_ptr<AclMap> updateAcls();
  std::shared_ptr<AclEntry> createAcl(const cfg::AclEntry* config);
  std::shared_ptr<AclEntry> updateAcl(const std::shared_ptr<AclEntry>& orig,
                                      const cfg::AclEntry* config);
  bool updateNeighborResponseTables(Vlan* vlan, const cfg::Vlan* config);
  bool updateDhcpOverrides(Vlan* vlan, const cfg::Vlan* config);
  std::shared_ptr<InterfaceMap> updateInterfaces();
  std::shared_ptr<RouteTableMap> updateInterfaceRoutes();
  std::shared_ptr<RouteTableMap> updateStaticRoutes(
      const std::shared_ptr<RouteTableMap>& curRoutingTables);
  shared_ptr<Interface> createInterface(const cfg::Interface* config,
                                        const Interface::Addresses& addrs);
  shared_ptr<Interface> updateInterface(const shared_ptr<Interface>& orig,
                                        const cfg::Interface* config,
                                        const Interface::Addresses& addrs);
  std::string getInterfaceName(const cfg::Interface* config);
  folly::MacAddress getInterfaceMac(const cfg::Interface* config);
  Interface::Addresses getInterfaceAddresses(const cfg::Interface* config);

  std::shared_ptr<SwitchState> orig_;
  const cfg::SwitchConfig* cfg_{nullptr};
  const Platform* platform_{nullptr};
  const cfg::SwitchConfig* prevCfg_{nullptr};

  struct VlanIpInfo {
    VlanIpInfo(uint8_t mask, MacAddress mac, InterfaceID intf)
      : mask(mask),
        mac(mac),
        interfaceID(intf) {}

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
  auto newState = orig_->clone();
  bool changed = false;

  processVlanPorts();

  {
    auto newPorts = updatePorts();
    if (newPorts) {
      newState->resetPorts(std::move(newPorts));
      changed = true;
    }
  }

  {
    auto newIntfs = updateInterfaces();
    if (newIntfs) {
      newState->resetIntfs(std::move(newIntfs));
      changed = true;
    }
  }

  // Note: updateInterfaces() must be called before updateVlans(),
  // as updateInterfaces() populates the vlanInterfaces_ data structure.
  {
    auto newVlans = updateVlans();
    if (newVlans) {
      newState->resetVlans(std::move(newVlans));
      changed = true;
    }
  }

  // Note: updateInterfaces() must be called before updateInterfaceRoutes(),
  // as updateInterfaces() populates the intfRouteTables_ data structure.
  {
    auto newTables = updateInterfaceRoutes();
    if (newTables) {
      newState->resetRouteTables(newTables);
      changed = true;
    }
    auto newerTables = updateStaticRoutes(newTables ? newTables :
        orig_->getRouteTables());
    if (newerTables) {
      newState->resetRouteTables(std::move(newerTables));
      changed = true;
    }
  }

  auto newVlans = newState->getVlans();
  VlanID dfltVlan(cfg_->defaultVlan);
  if (orig_->getDefaultVlan() != dfltVlan) {
    if (newVlans->getVlanIf(dfltVlan) == nullptr) {
      throw FbossError("Default VLAN ", dfltVlan, " does not exist");
    }
    newState->setDefaultVlan(dfltVlan);
    changed = true;
  }

  // Make sure all interfaces refer to valid VLANs.
  for (const auto& vlanInfo : vlanInterfaces_) {
    if (newVlans->getVlanIf(vlanInfo.first) == nullptr) {
      throw FbossError("Interface ",
                       *(vlanInfo.second.interfaces.begin()),
                       " refers to non-existent VLAN ", vlanInfo.first);
    }
    // Make sure there is a one-to-one map between vlan and interface
    // Remove this sanity check if multiple interfaces are allowed per vlans
    auto& entry = vlanInterfaces_[vlanInfo.first];
    if (entry.interfaces.size() != 1) {
      auto cpu_vlan = newState->getDefaultVlan();
      if (vlanInfo.first != cpu_vlan) {
        throw FbossError("Vlan ", vlanInfo.first, " refers to ",
                       entry.interfaces.size(), " interfaces ");
      }
   }
  }

  {
    auto newAcls = updateAcls();
    if (newAcls) {
      newState->resetAcls(std::move(newAcls));
      changed = true;
    }
  }

  std::chrono::seconds arpAgerInterval(cfg_->arpAgerInterval);
  if (orig_->getArpAgerInterval() != arpAgerInterval) {
    newState->setArpAgerInterval(arpAgerInterval);
    changed = true;
  }

  std::chrono::seconds arpTimeout(cfg_->arpTimeoutSeconds);
  if (orig_->getArpTimeout() != arpTimeout) {
    newState->setArpTimeout(arpTimeout);

    // TODO(aeckert): add ndpTimeout field to SwitchConfig. For now use the same
    // timeout for both ARP and NDP
    newState->setNdpTimeout(arpTimeout);
    changed = true;
  }

  uint32_t maxNeighborProbes(cfg_->maxNeighborProbes);
  if (orig_->getMaxNeighborProbes() != maxNeighborProbes) {
    newState->setMaxNeighborProbes(maxNeighborProbes);
    changed = true;
  }

  std::chrono::seconds staleEntryInterval(cfg_->staleEntryInterval);
  if (orig_->getStaleEntryInterval() != staleEntryInterval) {
    newState->setStaleEntryInterval(staleEntryInterval);
    changed = true;
  }


  if (!changed) {
    return nullptr;
  }
  return newState;
}

void ThriftConfigApplier::processVlanPorts() {
  // Build the Port --> Vlan mappings
  //
  // The config file has a separate list for this data,
  // but it is stored in the state tree as part of both the PortMap and the
  // VlanMap.
  for (const auto& vp : cfg_->vlanPorts) {
    PortID portID(vp.logicalPort);
    VlanID vlanID(vp.vlanID);
    auto ret1 = portVlans_[portID].insert(
        std::make_pair(vlanID, Port::VlanInfo(vp.emitTags)));
    if (!ret1.second) {
      throw FbossError("duplicate VlanPort for port ", portID, ", vlan ",
                       vlanID);
    }
    auto ret2 = vlanPorts_[vlanID].insert(
      std::make_pair(portID, Vlan::PortInfo(vp.emitTags)));
    if (!ret2.second) {
      // This should never fail if the first insert succeeded above.
      throw FbossError("duplicate VlanPort for vlan ", vlanID, ", port ",
                       portID);
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
      throw FbossError("VLAN ", intf->getVlanID(), " configured in multiple "
                       "different virtual routers: ", entry.routerID, " and ",
                       intf->getRouterID());
    }
  }

  auto intfRet = entry.interfaces.insert(intf->getID());
  if (!intfRet.second) {
    // This shouldn't happen
    throw FbossError("interface ", intf->getID(), " processed twice for "
                     "VLAN ", intf->getVlanID());
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
      throw FbossError("VLAN ", intf->getVlanID(), " has IP ", ipMask.first,
                       " configured multiple times with different masks (",
                       oldInfo.mask, " and ", info.mask, ")");
    }
    if (oldInfo.mac != info.mac) {
      throw FbossError("VLAN ", intf->getVlanID(), " has IP ", ipMask.first,
                       " configured multiple times with different MACs (",
                       oldInfo.mac, " and ", info.mac, ")");
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

  // Process all supplied port configs
  for (const auto& portCfg : cfg_->ports) {
    PortID id(portCfg.logicalID);
    auto origPort = origPorts->getPortIf(id);
    if (!origPort) {
      throw FbossError("config listed for non-existent port ", id);
    }

    auto newPort = updatePort(origPort, &portCfg);
    changed |= updateMap(&newPorts, origPort, newPort);
  }

  // Find all ports that didn't have a config listed
  // and reset them to their default (disabled) state.
  for (const auto& origPort : *origPorts) {
    if (newPorts.find(origPort->getID()) != newPorts.end()) {
      // This port was listed in the config, and has already been configured
      continue;
    }
    cfg::Port defaultConfig;
    origPort->initDefaultConfig(&defaultConfig);
    auto newPort = updatePort(origPort, &defaultConfig);
    changed |= updateMap(&newPorts, origPort, newPort);
  }

  if (!changed) {
    return nullptr;
  }

  return origPorts->clone(newPorts);
}

shared_ptr<Port> ThriftConfigApplier::updatePort(const shared_ptr<Port>& orig,
                                                 const cfg::Port* cfg) {
  CHECK_EQ(orig->getID(), cfg->logicalID);

  auto vlans = portVlans_[orig->getID()];
  if (cfg->state == orig->getState() &&
      VlanID(cfg->ingressVlan) == orig->getIngressVlan() &&
      vlans == orig->getVlans() &&
      cfg->speed == orig->getSpeed() &&
      cfg->name == orig->getName() &&
      cfg->description == orig->getDescription()) {
    return nullptr;
  }

  auto newPort = orig->clone();
  newPort->setState(cfg->state);
  newPort->setIngressVlan(VlanID(cfg->ingressVlan));
  newPort->setVlans(vlans);
  newPort->setSpeed(cfg->speed);
  newPort->setName(cfg->name);
  newPort->setDescription(cfg->description);
  return newPort;
}

shared_ptr<VlanMap> ThriftConfigApplier::updateVlans() {
  auto origVlans = orig_->getVlans();
  VlanMap::NodeContainer newVlans;
  bool changed = false;

  // Process all supplied VLAN configs
  size_t numExistingProcessed = 0;
  for (const auto& vlanCfg : cfg_->vlans) {
    VlanID id(vlanCfg.id);
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
  const auto& ports = vlanPorts_[VlanID(config->id)];
  auto vlan = make_shared<Vlan>(config, ports);
  updateNeighborResponseTables(vlan.get(), config);
  updateDhcpOverrides(vlan.get(), config);

  /* TODO t7153326: Following code is added for backward compatibility
  Remove it once coop generates config with */
  if (config->__isset.intfID) {
    vlan->setInterfaceID(InterfaceID(config->intfID));
  } else {
    auto& entry = vlanInterfaces_[VlanID(config->id)];
    if (!entry.interfaces.empty()) {
      vlan->setInterfaceID(*(entry.interfaces.begin()));
    }
  }
  return vlan;
}

shared_ptr<Vlan> ThriftConfigApplier::updateVlan(const shared_ptr<Vlan>& orig,
                                                 const cfg::Vlan* config) {
  CHECK_EQ(orig->getID(), config->id);
  const auto& ports = vlanPorts_[orig->getID()];

  auto newVlan = orig->clone();
  bool changed_neighbor_table =
      updateNeighborResponseTables(newVlan.get(), config);
  bool changed_dhcp_overrides = updateDhcpOverrides(newVlan.get(), config);
  auto oldDhcpV4Relay = orig->getDhcpV4Relay();
  auto newDhcpV4Relay = config->__isset.dhcpRelayAddressV4 ?
    IPAddressV4(config->dhcpRelayAddressV4) : IPAddressV4();

  auto oldDhcpV6Relay = orig->getDhcpV6Relay();
  auto newDhcpV6Relay = config->__isset.dhcpRelayAddressV6 ?
    IPAddressV6(config->dhcpRelayAddressV6) : IPAddressV6("::");

  /* TODO t7153326: Following code is added for backward compatibility
  Remove it once coop generates config with intfID */
  auto oldIntfID = orig->getInterfaceID();
  auto newIntfID = InterfaceID(0);
  if (config->__isset.intfID) {
    newIntfID = InterfaceID(config->intfID);
  } else {
    auto& entry = vlanInterfaces_[VlanID(config->id)];
    if (!entry.interfaces.empty()) {
      newIntfID = *(entry.interfaces.begin());
    }
  }

  if (orig->getName() == config->name &&
      oldIntfID == newIntfID &&
      orig->getPorts() == ports &&
      oldDhcpV4Relay == newDhcpV4Relay &&
      oldDhcpV6Relay == newDhcpV6Relay &&
      !changed_neighbor_table && !changed_dhcp_overrides) {
    return nullptr;
  }

  newVlan->setName(config->name);
  newVlan->setInterfaceID(newIntfID);
  newVlan->setPorts(ports);
  newVlan->setDhcpV4Relay(newDhcpV4Relay);
  newVlan->setDhcpV6Relay(newDhcpV6Relay);
  return newVlan;
}

shared_ptr<AclMap> ThriftConfigApplier::updateAcls() {
  auto origAcls = orig_->getAcls();
  AclMap::NodeContainer newAcls;
  bool changed = false;

  // Process all supplied ACLs
  size_t numExistingProcessed = 0;
  for (int i = 0; i < cfg_->acls.size(); ++i) {
    const auto& acl = cfg_->acls[i];
    auto origAcl = origAcls->getEntryIf(AclEntryID(acl.id));
    shared_ptr<AclEntry> newAcl;
    if (origAcl) {
      newAcl = updateAcl(origAcl, &acl);
      ++numExistingProcessed;
    } else {
      newAcl = createAcl(&acl);
    }
    changed |= updateMap(&newAcls, origAcl, newAcl);
  }

  if (numExistingProcessed != origAcls->size()) {
    // Some existing ACLs were removed.
    CHECK_LT(numExistingProcessed, origAcls->size());
    changed = true;
  }

  if (!changed) {
    return nullptr;
  }

  return origAcls->clone(std::move(newAcls));
}

shared_ptr<AclEntry> ThriftConfigApplier::createAcl(
    const cfg::AclEntry* config) {
  auto newAcl = make_shared<AclEntry>(AclEntryID(config->id));
  newAcl->setAction(config->action);
  if (config->__isset.srcIp) {
    newAcl->setSrcIp(IPAddress::createNetwork(config->srcIp));
  }
  if (config->__isset.dstIp) {
    newAcl->setDstIp(IPAddress::createNetwork(config->dstIp));
  }
  if (config->__isset.l4SrcPort) {
    newAcl->setL4SrcPort(config->l4SrcPort);
  }
  if (config->__isset.l4DstPort) {
    newAcl->setL4DstPort(config->l4DstPort);
  }
  if (config->__isset.proto) {
    newAcl->setProto(config->proto);
  }
  if (config->__isset.tcpFlags) {
    newAcl->setTcpFlags(config->tcpFlags);
  }
  if (config->__isset.tcpFlagsMask) {
    newAcl->setTcpFlagsMask(config->tcpFlagsMask);
  }
  if (config->__isset.srcPort) {
    newAcl->setSrcPort(config->srcPort);
  }
  if (config->__isset.dstPort) {
    newAcl->setDstPort(config->dstPort);
  }
  return newAcl;
}


shared_ptr<AclEntry> ThriftConfigApplier::updateAcl(
    const shared_ptr<AclEntry>& orig,
    const cfg::AclEntry* config) {

  auto newAcl = createAcl(config);

  if (orig->getID() == newAcl->getID() &&
      orig->getAction() == newAcl->getAction() &&
      orig->getSrcIp() == newAcl->getSrcIp() &&
      orig->getDstIp() == newAcl->getDstIp() &&
      orig->getL4SrcPort() == newAcl->getL4SrcPort() &&
      orig->getL4DstPort() == newAcl->getL4DstPort() &&
      orig->getProto() == newAcl->getProto() &&
      orig->getTcpFlags() == newAcl->getTcpFlags() &&
      orig->getTcpFlagsMask() == newAcl->getTcpFlagsMask()) {
    return nullptr;
  }

  return newAcl;
}

bool ThriftConfigApplier::updateDhcpOverrides(Vlan* vlan,
                                              const cfg::Vlan* config) {
  DhcpV4OverrideMap newDhcpV4OverrideMap;
  for (const auto& pair : config->dhcpRelayOverridesV4) {
    try {
      newDhcpV4OverrideMap[MacAddress(pair.first)] = IPAddressV4(pair.second);
    } catch (const IPAddressFormatException& ex) {
      throw FbossError("Invalid IPv4 address in DHCPv4 relay override map: ",
                       ex.what());
    }
  }

  DhcpV6OverrideMap newDhcpV6OverrideMap;
  for (const auto& pair : config->dhcpRelayOverridesV6) {
    try {
      newDhcpV6OverrideMap[MacAddress(pair.first)] = IPAddressV6(pair.second);
    } catch (const IPAddressFormatException& ex) {
      throw FbossError("Invalid IPv4 address in DHCPv4 relay override map: ",
                       ex.what());
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

  VlanID vlanID(config->id);
  auto it = vlanInterfaces_.find(vlanID);
  if (it != vlanInterfaces_.end()) {
    for (const auto& addrInfo : it->second.addresses) {
      NeighborResponseEntry entry(addrInfo.second.mac,
                                  addrInfo.second.interfaceID);
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

shared_ptr<RouteTableMap> ThriftConfigApplier::updateInterfaceRoutes() {
  flat_set<RouterID> newToAddTables;
  flat_set<RouterID> oldToDeleteTables;
  RouteUpdater updater(orig_->getRouteTables());
  // add or update the interface routes
  for (const auto& table : intfRouteTables_) {
    for (const auto& entry : table.second) {
      auto intf = entry.second.first;
      auto& addr = entry.second.second;
      auto len = entry.first.second;
      updater.addRoute(table.first, intf, addr, len);
    }
    newToAddTables.insert(table.first);
  }
  // need to go through all existing connected routes and delete those
  // not there anymore
  for (const auto& intf : orig_->getInterfaces()->getAllNodes()) {
    auto id = intf.second->getRouterID();
    auto iter = intfRouteTables_.find(id);
    if (iter == intfRouteTables_.end()) {
      // if the old router ID does not exist any more, need to remove the
      // v6 link local route from it.
      oldToDeleteTables.insert(id);
    }
    for (const auto& addr : intf.second->getAddresses()) {
      auto prefix = std::make_pair(addr.first.mask(addr.second), addr.second);
      bool found = false;
      if (iter != intfRouteTables_.end()) {
        const auto& newAddrs = iter->second;
        auto iter2 = newAddrs.find(prefix);
        if (iter2 != newAddrs.end()) {
          found = true;
        }
      }
      if (!found) {
        updater.delRoute(id, addr.first, addr.second);
      }
    }
  }
  // delete v6 link route from no long existing router ID
  for (auto id : oldToDeleteTables) {
    updater.delLinkLocalRoutes(id);
  }
  // add v6 link route to the new router
  for (auto id : newToAddTables) {
    updater.addLinkLocalRoutes(id);
  }
  return updater.updateDone();
}


std::shared_ptr<RouteTableMap> ThriftConfigApplier::updateStaticRoutes(
    const std::shared_ptr<RouteTableMap>& curRoutingTables) {
  RouteUpdater updater(curRoutingTables);
  updater.updateStaticRoutes(*cfg_, *prevCfg_);
  return updater.updateDone();
}

std::shared_ptr<InterfaceMap> ThriftConfigApplier::updateInterfaces() {
  auto origIntfs = orig_->getInterfaces();
  InterfaceMap::NodeContainer newIntfs;
  bool changed = false;

  // Process all supplied interface configs
  size_t numExistingProcessed = 0;

  for (const auto& interfaceCfg : cfg_->interfaces) {
    InterfaceID id(interfaceCfg.intfID);
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
  auto mtu = config->__isset.mtu ? config->mtu : Interface::kDefaultMtu;
  auto intf = make_shared<Interface>(InterfaceID(config->intfID),
                                     RouterID(config->routerID),
                                     VlanID(config->vlanID),
                                     name,
                                     mac,
                                     mtu);
  intf->setAddresses(addrs);
  if (config->__isset.ndp) {
    intf->setNdpConfig(config->ndp);
  }
  return intf;
}

shared_ptr<Interface> ThriftConfigApplier::updateInterface(
    const shared_ptr<Interface>& orig,
    const cfg::Interface* config,
    const Interface::Addresses& addrs) {
  CHECK_EQ(orig->getID(), config->intfID);

  cfg::NdpConfig ndp;
  if (config->__isset.ndp) {
    ndp = config->ndp;
  }
  auto name = getInterfaceName(config);
  auto mac = getInterfaceMac(config);
  auto mtu = config->__isset.mtu ? config->mtu : Interface::kDefaultMtu;
  if (orig->getRouterID() == RouterID(config->routerID) &&
      orig->getVlanID() == VlanID(config->vlanID) &&
      orig->getName() == name &&
      orig->getMac() == mac &&
      orig->getAddresses() == addrs &&
      orig->getNdpConfig() == ndp &&
      orig->getMtu() == mtu) {
    // No change
    return nullptr;
  }

  auto newIntf = orig->clone();
  newIntf->setRouterID(RouterID(config->routerID));
  newIntf->setVlanID(VlanID(config->vlanID));
  newIntf->setName(name);
  newIntf->setMac(mac);
  newIntf->setAddresses(addrs);
  newIntf->setNdpConfig(ndp);
  newIntf->setMtu(mtu);
  return newIntf;
}

std::string
ThriftConfigApplier::getInterfaceName(const cfg::Interface* config) {
  if (config->__isset.name) {
    return config->name;
  }
  return folly::to<std::string>("Interface ", config->intfID);
}

MacAddress ThriftConfigApplier::getInterfaceMac(const cfg::Interface* config) {
  if (config->__isset.mac) {
    return MacAddress(config->mac);
  }
  return platform_->getLocalMac();
}

Interface::Addresses ThriftConfigApplier::getInterfaceAddresses(
    const cfg::Interface* config) {
  Interface::Addresses addrs;
  for (const auto& addr : config->ipAddresses) {
    auto intfAddr = IPAddress::createNetwork(addr, -1, false);
    auto ret = addrs.insert(intfAddr);
    if (!ret.second) {
      throw FbossError("Duplicate network IP address ", addr,
                       " in interface ", config->intfID);
    }
    auto ret2 = intfRouteTables_[RouterID(config->routerID)].emplace(
        IPAddress::createNetwork(addr),
        std::make_pair(InterfaceID(config->intfID), intfAddr.first));
    if (!ret2.second) {
      // we get same network, only allow it if that is from the same interface
      auto other = ret2.first->second.first;
      if (other != InterfaceID(config->intfID)) {
        throw FbossError("Duplicate network address ", addr, " of interface ",
                         config->intfID, " as interface ", other,
                         " in VRF ", config->routerID);
      }
      // For consistency with interface routes as added by RouteUpdater,
      // use the last address we see rather than the first. Otherwise,
      // we see pointless route updates on syncFib()
      *ret2.first = std::make_pair(
          IPAddress::createNetwork(addr),
          std::make_pair(InterfaceID(config->intfID), intfAddr.first));
    }
  }
  return addrs;
}

shared_ptr<SwitchState> applyThriftConfig(
    const shared_ptr<SwitchState>& state,
    const cfg::SwitchConfig* config,
    const Platform* platform,
    const cfg::SwitchConfig* prevConfig) {
  cfg::SwitchConfig emptyConfig;
  return ThriftConfigApplier(state, config, platform,
      prevConfig ? prevConfig : &emptyConfig).run();
}

std::pair<std::shared_ptr<SwitchState>, std::string> applyThriftConfigFile(
  const std::shared_ptr<SwitchState>& state,
  const folly::StringPiece path,
  const Platform* platform,
  const cfg::SwitchConfig* prevConfig) {
  //
  // This is basically what configerator's getConfigAndParse() code does,
  // except that we manually read the file from disk for now.
  // We may not be able to rely on the configerator infrastructure for
  // distributing the config files.
  cfg::SwitchConfig config;
  std::string configStr;
  if (!folly::readFile(path.toString().c_str(), configStr)) {
    throw FbossError("unable to read ", path);
  }
  config.readFromJson(configStr.c_str());

  return std::make_pair(
      applyThriftConfig(state, &config, platform, prevConfig), configStr);
}

}} // facebook::fboss
