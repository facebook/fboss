/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/state/SwitchState.h"
#include <memory>
#include <tuple>

#include "DsfNodeMap.h"
#include "fboss/agent/FbossError.h"
#include "fboss/agent/state/AclEntry.h"
#include "fboss/agent/state/AclMap.h"
#include "fboss/agent/state/AclTableGroupMap.h"
#include "fboss/agent/state/AggregatePort.h"
#include "fboss/agent/state/AggregatePortMap.h"
#include "fboss/agent/state/BufferPoolConfig.h"
#include "fboss/agent/state/BufferPoolConfigMap.h"
#include "fboss/agent/state/ControlPlane.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/InterfaceMap.h"
#include "fboss/agent/state/IpTunnel.h"
#include "fboss/agent/state/IpTunnelMap.h"
#include "fboss/agent/state/LabelForwardingInformationBase.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/PortMap.h"
#include "fboss/agent/state/QosPolicyMap.h"
#include "fboss/agent/state/SflowCollectorMap.h"
#include "fboss/agent/state/SwitchSettings.h"
#include "fboss/agent/state/TeFlowEntry.h"
#include "fboss/agent/state/TeFlowTable.h"
#include "fboss/agent/state/Transceiver.h"
#include "fboss/agent/state/TransceiverMap.h"
#include "fboss/agent/state/Vlan.h"
#include "fboss/agent/state/VlanMap.h"

#include "fboss/agent/state/NodeBase-defs.h"
#include "folly/IPAddress.h"
#include "folly/IPAddressV4.h"

using std::make_shared;
using std::shared_ptr;
using std::chrono::seconds;

// TODO: it might be worth splitting up limits for ecmp/ucmp
DEFINE_uint32(
    ecmp_width,
    64,
    "Max ecmp width. Also implies ucmp normalization factor");

DEFINE_bool(
    enable_acl_table_group,
    false,
    "Allow multiple acl tables (acl table group)");

/*
 * VOQ switches require that the packets are not tagged with VLAN.
 * We are gradually enhancing the wedge_agent to handle tagged as well as
 * untagged packets.
 * As part of these changes, neighbor tables will move to Interfaces instead of
 * VLANs. This allows for the same neighbor table implementation for VOQ as
 * well as non-VOQ switches.
 *
 * When this flag is TRUE: use neighbor tables from Interfaces.
 * When this flag is FALSE: use neighbor tables from VLANs.
 *
 * Once we have completely migrated to using neighbor tables from Interfaces,
 * this flag will be removed.
 */

DEFINE_bool(
    intf_nbr_tables,
    false,
    "Use Neighbor Tables from Interfaces instead of VLANs");

namespace facebook::fboss {

template <typename MultiMapName, typename Map>
void SwitchState::resetDefaultMap(std::shared_ptr<Map> map) {
  const auto& matcher = HwSwitchMatcher::defaultHwSwitchMatcher();
  resetMap<MultiMapName, Map>(map, matcher);
}
template <typename MultiMapName, typename Map>
void SwitchState::resetMap(
    const std::shared_ptr<Map>& map,
    const HwSwitchMatcher& matcher) {
  auto multiMap = cref<MultiMapName>()->clone();
  if (!multiMap->getMapNodeIf(matcher)) {
    multiMap->addMapNode(map, matcher);
  } else {
    multiMap->updateMapNode(map, matcher);
  }
  ref<MultiMapName>() = multiMap;
}

template <typename MultiMapName, typename Map>
const std::shared_ptr<Map>& SwitchState::getDefaultMap() const {
  return getMap<MultiMapName, Map>(HwSwitchMatcher::defaultHwSwitchMatcher());
}

template <typename MultiMapName, typename Map>
const std::shared_ptr<Map>& SwitchState::getMap(
    const HwSwitchMatcher& matcher) const {
  if (safe_cref<MultiMapName>()->find(matcher.matcherString()) ==
      safe_cref<MultiMapName>()->end()) {
    std::string stateJson;
    apache::thrift::SimpleJSONSerializer::serialize(
        BaseT::toThrift(), &stateJson);
    XLOG(FATAL) << "Cannot find matcher " << matcher.matcherString()
                << " in multimap " << stateJson;
  }
  return safe_cref<MultiMapName>()->cref(matcher.matcherString());
}

template <typename MultiMapName, typename Map>
std::shared_ptr<Map>& SwitchState::getDefaultMap() {
  return getMap<MultiMapName, Map>(HwSwitchMatcher::defaultHwSwitchMatcher());
}

template <typename MultiMapName, typename Map>
std::shared_ptr<Map>& SwitchState::getMap(const HwSwitchMatcher& matcher) {
  if (safe_ref<MultiMapName>()->find(matcher.matcherString()) ==
      safe_ref<MultiMapName>()->end()) {
    std::string stateJson;
    apache::thrift::SimpleJSONSerializer::serialize(
        BaseT::toThrift(), &stateJson);
    XLOG(FATAL) << "Cannot find matcher " << matcher.matcherString()
                << " in multimap " << stateJson;
  }
  return safe_ref<MultiMapName>()->ref(matcher.matcherString());
}

SwitchState::SwitchState() {
  set<switch_state_tags::dhcpV4RelaySrc>(
      network::toBinaryAddress(folly::IPAddress("0.0.0.0")));
  set<switch_state_tags::dhcpV4ReplySrc>(
      network::toBinaryAddress(folly::IPAddress("0.0.0.0")));
  set<switch_state_tags::dhcpV6RelaySrc>(
      network::toBinaryAddress(folly::IPAddress("::")));
  set<switch_state_tags::dhcpV6ReplySrc>(
      network::toBinaryAddress(folly::IPAddress("::")));

  set<switch_state_tags::aclTableGroupMap>(
      std::map<cfg::AclStage, state::AclTableGroupFields>{});
  resetIntfs(std::make_shared<MultiSwitchInterfaceMap>());
  resetRemoteIntfs(std::make_shared<MultiSwitchInterfaceMap>());
  resetTransceivers(std::make_shared<MultiSwitchTransceiverMap>());
  resetControlPlane(std::make_shared<MultiControlPlane>());
  resetSwitchSettings(std::make_shared<MultiSwitchSettings>());
}

SwitchState::~SwitchState() {}

void SwitchState::modify(std::shared_ptr<SwitchState>* state) {
  if (!(*state)->isPublished()) {
    return;
  }
  *state = (*state)->clone();
}

std::shared_ptr<Port> SwitchState::getPort(PortID id) const {
  return getPorts()->getNode(id);
}

void SwitchState::resetPorts(std::shared_ptr<MultiSwitchPortMap> ports) {
  ref<switch_state_tags::portMaps>() = ports;
}

const std::shared_ptr<MultiSwitchPortMap>& SwitchState::getPorts() const {
  return safe_cref<switch_state_tags::portMaps>();
}

void SwitchState::resetVlans(std::shared_ptr<MultiSwitchVlanMap> vlans) {
  ref<switch_state_tags::vlanMaps>() = vlans;
}

const std::shared_ptr<MultiSwitchVlanMap>& SwitchState::getVlans() const {
  return safe_cref<switch_state_tags::vlanMaps>();
}

void SwitchState::resetIntfs(
    const std::shared_ptr<MultiSwitchInterfaceMap>& intfs) {
  ref<switch_state_tags::interfaceMaps>() = intfs;
}

const std::shared_ptr<MultiSwitchInterfaceMap>& SwitchState::getInterfaces()
    const {
  return safe_cref<switch_state_tags::interfaceMaps>();
}

void SwitchState::resetRemoteIntfs(
    const std::shared_ptr<MultiSwitchInterfaceMap>& intfs) {
  ref<switch_state_tags::remoteInterfaceMaps>() = intfs;
}

const std::shared_ptr<MultiSwitchInterfaceMap>&
SwitchState::getRemoteInterfaces() const {
  return safe_cref<switch_state_tags::remoteInterfaceMaps>();
}

const std::shared_ptr<MultiSwitchSystemPortMap>&
SwitchState::getRemoteSystemPorts() const {
  return safe_cref<switch_state_tags::remoteSystemPortMaps>();
}

std::shared_ptr<AclEntry> SwitchState::getAcl(const std::string& name) const {
  return getAcls()->getNodeIf(name);
}

void SwitchState::resetAcls(const std::shared_ptr<MultiSwitchAclMap>& acls) {
  ref<switch_state_tags::aclMaps>() = acls;
}

const std::shared_ptr<MultiSwitchAclMap>& SwitchState::getAcls() const {
  return safe_cref<switch_state_tags::aclMaps>();
}

void SwitchState::resetAclTableGroups(
    std::shared_ptr<MultiSwitchAclTableGroupMap> aclTableGroups) {
  ref<switch_state_tags::aclTableGroupMaps>() = aclTableGroups;
}

const std::shared_ptr<MultiSwitchAclTableGroupMap>&
SwitchState::getAclTableGroups() const {
  return safe_cref<switch_state_tags::aclTableGroupMaps>();
}

void SwitchState::resetAggregatePorts(
    std::shared_ptr<MultiSwitchAggregatePortMap> aggPorts) {
  ref<switch_state_tags::aggregatePortMaps>() = aggPorts;
}

const std::shared_ptr<MultiSwitchAggregatePortMap>&
SwitchState::getAggregatePorts() const {
  return safe_cref<switch_state_tags::aggregatePortMaps>();
}

void SwitchState::resetSflowCollectors(
    const std::shared_ptr<MultiSwitchSflowCollectorMap>& collectors) {
  ref<switch_state_tags::sflowCollectorMaps>() = collectors;
}

void SwitchState::resetQosPolicies(
    const std::shared_ptr<MultiSwitchQosPolicyMap>& qosPolicies) {
  ref<switch_state_tags::qosPolicyMaps>() = qosPolicies;
}

void SwitchState::resetControlPlane(
    std::shared_ptr<MultiControlPlane> controlPlane) {
  ref<switch_state_tags::controlPlaneMap>() = controlPlane;
}

void SwitchState::resetLoadBalancers(
    std::shared_ptr<MultiSwitchLoadBalancerMap> loadBalancers) {
  ref<switch_state_tags::loadBalancerMaps>() = loadBalancers;
}

void SwitchState::resetSwitchSettings(
    std::shared_ptr<MultiSwitchSettings> switchSettings) {
  ref<switch_state_tags::switchSettingsMap>() = switchSettings;
}

void SwitchState::resetBufferPoolCfgs(
    std::shared_ptr<MultiSwitchBufferPoolCfgMap> cfgs) {
  ref<switch_state_tags::bufferPoolCfgMaps>() = cfgs;
}

const std::shared_ptr<MultiSwitchBufferPoolCfgMap>
SwitchState::getBufferPoolCfgs() const {
  return safe_cref<switch_state_tags::bufferPoolCfgMaps>();
}

void SwitchState::resetPortFlowletCfgs(
    std::shared_ptr<MultiSwitchPortFlowletCfgMap> cfgs) {
  ref<switch_state_tags::portFlowletCfgMaps>() = cfgs;
}

const std::shared_ptr<MultiSwitchPortFlowletCfgMap>
SwitchState::getPortFlowletCfgs() const {
  return safe_cref<switch_state_tags::portFlowletCfgMaps>();
}

const std::shared_ptr<MultiSwitchLoadBalancerMap>&
SwitchState::getLoadBalancers() const {
  return safe_cref<switch_state_tags::loadBalancerMaps>();
}

void SwitchState::resetMirrors(
    const std::shared_ptr<MultiSwitchMirrorMap>& mirrors) {
  ref<switch_state_tags::mirrorMaps>() = mirrors;
}

const std::shared_ptr<MultiSwitchSflowCollectorMap>&
SwitchState::getSflowCollectors() const {
  return safe_cref<switch_state_tags::sflowCollectorMaps>();
}

const std::shared_ptr<MultiSwitchMirrorMap>& SwitchState::getMirrors() const {
  return safe_cref<switch_state_tags::mirrorMaps>();
}

const std::shared_ptr<MultiSwitchQosPolicyMap>& SwitchState::getQosPolicies()
    const {
  return safe_cref<switch_state_tags::qosPolicyMaps>();
}

const std::shared_ptr<MultiSwitchForwardingInformationBaseMap>&
SwitchState::getFibs() const {
  return safe_cref<switch_state_tags::fibsMap>();
}

const std::shared_ptr<MultiControlPlane>& SwitchState::getControlPlane() const {
  return safe_cref<switch_state_tags::controlPlaneMap>();
}

const std::shared_ptr<MultiLabelForwardingInformationBase>&
SwitchState::getLabelForwardingInformationBase() const {
  return safe_cref<switch_state_tags::labelFibMap>();
}

void SwitchState::resetLabelForwardingInformationBase(
    std::shared_ptr<MultiLabelForwardingInformationBase> labelFib) {
  ref<switch_state_tags::labelFibMap>() = labelFib;
}

void SwitchState::resetForwardingInformationBases(
    std::shared_ptr<MultiSwitchForwardingInformationBaseMap> fibs) {
  ref<switch_state_tags::fibsMap>() = fibs;
}

void SwitchState::resetTransceivers(
    std::shared_ptr<MultiSwitchTransceiverMap> transceivers) {
  ref<switch_state_tags::transceiverMaps>() = transceivers;
}

const std::shared_ptr<MultiSwitchTransceiverMap>& SwitchState::getTransceivers()
    const {
  return safe_cref<switch_state_tags::transceiverMaps>();
}

void SwitchState::resetSystemPorts(
    const std::shared_ptr<MultiSwitchSystemPortMap>& systemPorts) {
  ref<switch_state_tags::systemPortMaps>() = systemPorts;
}

const std::shared_ptr<MultiSwitchSystemPortMap>& SwitchState::getSystemPorts()
    const {
  return safe_cref<switch_state_tags::systemPortMaps>();
}

void SwitchState::resetTunnels(
    std::shared_ptr<MultiSwitchIpTunnelMap> tunnels) {
  ref<switch_state_tags::ipTunnelMaps>() = tunnels;
}

const std::shared_ptr<MultiSwitchIpTunnelMap>& SwitchState::getTunnels() const {
  return safe_cref<switch_state_tags::ipTunnelMaps>();
}

void SwitchState::resetTeFlowTable(
    std::shared_ptr<MultiTeFlowTable> flowTable) {
  ref<switch_state_tags::teFlowTables>() = flowTable;
}

const std::shared_ptr<MultiTeFlowTable>& SwitchState::getTeFlowTable() const {
  return safe_cref<switch_state_tags::teFlowTables>();
}

void SwitchState::resetDsfNodes(
    const std::shared_ptr<MultiSwitchDsfNodeMap>& dsfNodes) {
  ref<switch_state_tags::dsfNodesMap>() = dsfNodes;
}

const std::shared_ptr<MultiSwitchDsfNodeMap>& SwitchState::getDsfNodes() const {
  return safe_cref<switch_state_tags::dsfNodesMap>();
}

std::shared_ptr<const AclTableMap> SwitchState::getAclTablesForStage(
    cfg::AclStage aclStage) const {
  if (getAclTableGroups() && getAclTableGroups()->getNodeIf(aclStage) &&
      getAclTableGroups()->getNodeIf(aclStage)->getAclTableMap()) {
    return getAclTableGroups()->getNodeIf(aclStage)->getAclTableMap();
  }

  return nullptr;
}

std::shared_ptr<const AclMap> SwitchState::getAclsForTable(
    cfg::AclStage aclStage,
    const std::string& tableName) const {
  auto aclTableMap = getAclTablesForStage(aclStage);

  if (aclTableMap && aclTableMap->getTableIf(tableName)) {
    return aclTableMap->getTable(tableName)->getAclMap().unwrap();
  }

  return nullptr;
}

bool SwitchState::isLocalSwitchId(SwitchID switchId) const {
  for ([[maybe_unused]] const auto& [_, switchSettings] :
       std::as_const(*getSwitchSettings())) {
    auto localSwitchIds = switchSettings->getSwitchIds();
    if (localSwitchIds.find(switchId) != localSwitchIds.end()) {
      return true;
    }
  }
  return false;
}

std::shared_ptr<SystemPortMap> SwitchState::getSystemPorts(
    SwitchID switchId) const {
  auto mSwitchSysPorts =
      isLocalSwitchId(switchId) ? getSystemPorts() : getRemoteSystemPorts();
  auto toRet = std::make_shared<SystemPortMap>();
  for (const auto& [_, sysPorts] : std::as_const(*mSwitchSysPorts)) {
    for (const auto& idAndSysPort : std::as_const(*sysPorts)) {
      const auto& sysPort = idAndSysPort.second;
      if (sysPort->getSwitchId() == switchId) {
        toRet->addSystemPort(sysPort);
      }
    }
  };
  return toRet;
}

std::shared_ptr<InterfaceMap> SwitchState::getInterfaces(
    SwitchID switchId) const {
  bool isLocal = isLocalSwitchId(switchId);
  auto mSwitchIntfs = isLocal ? getInterfaces() : getRemoteInterfaces();
  auto toRet = std::make_shared<InterfaceMap>();
  auto sysPorts = getSystemPorts(switchId);
  for (const auto& [_, intfMap] : std::as_const(*mSwitchIntfs)) {
    for (const auto& [interfaceID, interface] : std::as_const(*intfMap)) {
      SystemPortID sysPortId(interfaceID);
      if (!isLocal && !sysPorts->getNodeIf(sysPortId)) {
        // Remote intfs must have a remote sys port corresponding to
        // the same switchId
        continue;
      }
      toRet->addNode(interface);
    }
  }
  return toRet;
}

const std::shared_ptr<MultiSwitchSettings>& SwitchState::getSwitchSettings()
    const {
  return safe_cref<switch_state_tags::switchSettingsMap>();
}

void SwitchState::revertNewTeFlowEntry(
    const std::shared_ptr<TeFlowEntry>& newTeFlowEntry,
    const std::shared_ptr<TeFlowEntry>& oldTeFlowEntry,
    std::shared_ptr<SwitchState>* appliedState) {
  auto clonedTeFlowTable =
      (*appliedState)->getTeFlowTable()->modify(appliedState);
  if (oldTeFlowEntry) {
    auto [node, matcher] =
        clonedTeFlowTable->getNodeAndScope(oldTeFlowEntry->getID());
    DCHECK_EQ(node.get(), oldTeFlowEntry.get());
    clonedTeFlowTable->updateNode(oldTeFlowEntry, matcher);
  } else {
    clonedTeFlowTable->removeNode(newTeFlowEntry->getID());
  }
}

/*
 * if a multi map is empty or has no node for default key matcher or default key
 * matcher is empty, populate it from corresponding map.
 *
 * if multi map has non empty and default key matcher map and corresponding map
 * is not empty then confirm they are same.
 */
template <typename MultiMapName, typename MapName>
void SwitchState::fromThrift(bool emptyMnpuMapOk) {
  const auto& matcher = HwSwitchMatcher::defaultHwSwitchMatcher();
  auto& map = this->ref<MapName>();
  auto& multiMap = this->ref<MultiMapName>();
  if (emptyMnpuMapOk && multiMap->empty() && map->empty()) {
    // emptyMnpuMapOk is set for maps that have been
    // migrated away from the assumption of always having
    // the default matcher entry in m-npu map
    return;
  }
  if (multiMap->empty() || !multiMap->getMapNodeIf(matcher)) {
    multiMap->addMapNode(map->clone(), matcher);
  } else if (auto matchedNode = multiMap->getMapNodeIf(matcher)) {
    if (matchedNode->empty()) {
      multiMap->updateMapNode(map->clone(), matcher);
    } else if (!map->empty()) {
      // if both multi map's default map and map are not empty
      // let map take precedence and set up multi-map's default map
      // this is because default map will contain relevant data
      // while multi will contain obsoleted data
      // THRIFT_COPY
      if (map->toThrift() != matchedNode->toThrift()) {
        multiMap->updateMapNode(map->clone(), matcher);
      }
    }
  }
  map->fromThrift({});
}

std::unique_ptr<SwitchState> SwitchState::uniquePtrFromThrift(
    const state::SwitchState& switchState) {
  auto state = std::make_unique<SwitchState>();
  state->BaseT::fromThrift(switchState);
  if (FLAGS_enable_acl_table_group) {
    // Use old map if valid else use mmap
    auto aclMap = state->cref<switch_state_tags::aclMap>();
    aclMap = aclMap->size()
        ? aclMap
        : state->cref<switch_state_tags::aclMaps>()->getAclMap();
    if (aclMap && aclMap->size()) {
      state->ref<switch_state_tags::aclTableGroupMap>() =
          AclTableGroupMap::createDefaultAclTableGroupMapFromThrift(
              aclMap->toThrift());
      state->resetAcls(std::make_shared<MultiSwitchAclMap>());
      state->ref<switch_state_tags::aclMap>()->clear();
    }
  }
  if (!FLAGS_enable_acl_table_group) {
    // check legacy table first
    auto aclMap = state->cref<switch_state_tags::aclTableGroupMap>()
        ? AclTableGroupMap::getDefaultAclTableGroupMap(
              state->cref<switch_state_tags::aclTableGroupMap>()->toThrift())
        : nullptr;
    aclMap = aclMap && aclMap->size()
        ? aclMap
        : state->cref<switch_state_tags::aclTableGroupMaps>()->getAclMap();
    if (aclMap && aclMap->size()) {
      state->set<switch_state_tags::aclMap>(aclMap->toThrift());
      state->ref<switch_state_tags::aclTableGroupMap>().reset();
    }
  }
  /* forward compatibility */
  state->fromThrift<
      switch_state_tags::qosPolicyMaps,
      switch_state_tags::qosPolicyMap>(true /*emptyMnpuMapOk*/);

  state
      ->fromThrift<switch_state_tags::labelFibMap, switch_state_tags::labelFib>(
          true /*emptyMnpuMapOk*/);
  state->fromThrift<
      switch_state_tags::sflowCollectorMaps,
      switch_state_tags::sflowCollectorMap>(true /*emptyMnpuMapOk*/);
  state->fromThrift<switch_state_tags::fibsMap, switch_state_tags::fibs>(
      true /* emptyMnpuMapOk */);
  state->fromThrift<
      switch_state_tags::ipTunnelMaps,
      switch_state_tags::ipTunnelMap>(true /*emptyMnpuMapOk*/);
  state->fromThrift<
      switch_state_tags::teFlowTables,
      switch_state_tags::teFlowTable>(true /*emptyMnpuMapOk*/);
  state->fromThrift<
      switch_state_tags::aggregatePortMaps,
      switch_state_tags::aggregatePortMap>(true /*emptyMnpuMapOk*/);
  state->fromThrift<
      switch_state_tags::loadBalancerMaps,
      switch_state_tags::loadBalancerMap>(true /*emptyMnpuMapOk*/);
  state->fromThrift<switch_state_tags::vlanMaps, switch_state_tags::vlanMap>(
      true /*emptyMnpuMapOk*/);
  state->fromThrift<switch_state_tags::portMaps, switch_state_tags::portMap>(
      true /*emptyMnpuMapOk*/);
  state->fromThrift<
      switch_state_tags::interfaceMaps,
      switch_state_tags::interfaceMap>(true /*emptyMnpuMapOk*/);
  if (state->cref<switch_state_tags::aclTableGroupMap>()) {
    // set multi map if acl table group map exists
    state->fromThrift<
        switch_state_tags::aclTableGroupMaps,
        switch_state_tags::aclTableGroupMap>(true /*emptyMnpuMapOk*/);
  }
  state
      ->fromThrift<switch_state_tags::dsfNodesMap, switch_state_tags::dsfNodes>(
          true /*emptyMnpuMapOk*/);
  state->fromThrift<
      switch_state_tags::remoteInterfaceMaps,
      switch_state_tags::remoteInterfaceMap>(true /*emptyMnpuMapOk*/);
  state->fromThrift<
      switch_state_tags::systemPortMaps,
      switch_state_tags::systemPortMap>(true /*emptyMnpuMapOk*/);
  state->fromThrift<
      switch_state_tags::remoteSystemPortMaps,
      switch_state_tags::remoteSystemPortMap>(true /*emptyMnpuMapOk*/);

  state->fromThrift<switch_state_tags::aclMaps, switch_state_tags::aclMap>(
      true /*emptyMnpuMapOk*/);
  return state;
}

VlanID SwitchState::getDefaultVlan() const {
  auto switchSettings = getSwitchSettings()->size()
      ? getSwitchSettings()->cbegin()->second
      : std::make_shared<SwitchSettings>();
  auto defaultVlan = switchSettings->getDefaultVlan();
  if (defaultVlan.has_value()) {
    return VlanID(defaultVlan.value());
  }
  return VlanID(cref<switch_state_tags::defaultVlan>()->toThrift());
}

SwitchID SwitchState::getAssociatedSwitchID(PortID portID) const {
  auto port = getPorts()->getNode(portID);
  if (port->getInterfaceIDs().size() != 1) {
    throw FbossError(
        "Unexpected number of interfaces associated with port: ",
        port->getName(),
        " expected: 1 got: ",
        port->getInterfaceIDs().size());
  }
  auto intf = getInterfaces()->getNodeIf(port->getInterfaceID());

  if (!intf || intf->getType() != cfg::InterfaceType::SYSTEM_PORT) {
    throw FbossError("TODO: figure out switch id for non VOQ switch ports");
  }
  auto systemPortID = intf->getSystemPortID();
  CHECK(systemPortID.has_value());
  return getSystemPorts()->getNode(*systemPortID)->getSwitchId();
}

std::optional<cfg::Range64> SwitchState::getAssociatedSystemPortRangeIf(
    InterfaceID intfID) const {
  auto intf = getInterfaces()->getNodeIf(intfID);
  if (!intf || intf->getType() != cfg::InterfaceType::SYSTEM_PORT) {
    return std::nullopt;
  }
  auto systemPortID = intf->getSystemPortID();
  CHECK(systemPortID.has_value());
  auto switchId = getSystemPorts()->getNode(*systemPortID)->getSwitchId();
  auto dsfNode = getDsfNodes()->getNodeIf(switchId);
  return dsfNode->getSystemPortRange();
}

std::optional<cfg::Range64> SwitchState::getAssociatedSystemPortRangeIf(
    PortID portID) const {
  auto port = getPorts()->getNodeIf(portID);
  if (!port || port->getInterfaceIDs().size() != 1) {
    return std::nullopt;
  }
  return getAssociatedSystemPortRangeIf(port->getInterfaceID());
}

InterfaceID SwitchState::getInterfaceIDForPort(PortID portID) const {
  auto port = getPorts()->getNode(portID);
  // On VOQ/Fabric switches, port and interface have 1:1 relation.
  // For non VOQ/Fabric switches, in practice, a port is always part of a
  // single VLAN (and thus single interface).
  return port->getInterfaceID();
}

std::shared_ptr<SwitchState> SwitchState::fromThrift(
    const state::SwitchState& data) {
  auto uniqState = uniquePtrFromThrift(data);
  std::shared_ptr<SwitchState> state = std::move(uniqState);
  return state;
}

template <typename MultiMapType, typename ThriftType>
std::optional<ThriftType> SwitchState::toThrift(
    const std::shared_ptr<MultiMapType>& multiMap) const {
  if (!multiMap || multiMap->empty()) {
    return std::nullopt;
  }
  const auto& key = HwSwitchMatcher::defaultHwSwitchMatcher();
  if (auto map = multiMap->getMapNodeIf(key)) {
    return map->toThrift();
  }
  return std::nullopt;
}

state::SwitchState SwitchState::toThrift() const {
  auto data = BaseT::toThrift();
  auto aclMaps = data.aclMaps();
  auto aclTableGroupMaps = data.aclTableGroupMaps();
  const auto& matcher = HwSwitchMatcher::defaultHwSwitchMatcherKey();
  if (FLAGS_enable_acl_table_group) {
    if ((aclMaps->find(matcher) != aclMaps->end()) &&
        !data.get_aclMaps().at(matcher).empty() &&
        (aclTableGroupMaps->empty() ||
         aclTableGroupMaps->find(matcher) == aclTableGroupMaps->end() ||
         aclTableGroupMaps->find(matcher)->second.empty())) {
      data.aclTableGroupMaps()->emplace(
          matcher,
          AclTableGroupMap::createDefaultAclTableGroupMapFromThrift(
              data.get_aclMaps().at(matcher))
              ->toThrift());
    }
    aclMaps->clear();
  } else {
    if (!aclTableGroupMaps->empty() &&
        (aclTableGroupMaps->find(matcher) != aclTableGroupMaps->end()) &&
        (aclMaps->empty() || aclMaps->find(matcher) == aclMaps->end() ||
         aclMaps->find(matcher)->second.empty())) {
      if (auto aclMapPtr = AclTableGroupMap::getDefaultAclTableGroupMap(
              aclTableGroupMaps[matcher])) {
        aclMaps->at(matcher) = aclMapPtr->toThrift();
      }
    }
    aclTableGroupMaps->clear();
  }

  // SwitchSettings need to restored before the transition logic
  // for new SwitchSettings members is executed
  auto multiSwitchSwitchSettings = cref<switch_state_tags::switchSettingsMap>();
  if (!multiSwitchSwitchSettings->empty()) {
    data.switchSettings() =
        multiSwitchSwitchSettings->cbegin()->second->toThrift();
  }

  // Write defaultVlan to switchSettings and old fields for transition
  if (data.switchSettings()->defaultVlan().has_value()) {
    data.defaultVlan() = data.switchSettings()->defaultVlan().value();
  } else {
    data.switchSettings()->defaultVlan() = data.defaultVlan().value();
  }
  // Write arpTimeout to switchSettings and old fields for transition
  if (data.switchSettings()->arpTimeout().has_value()) {
    data.arpTimeout() = data.switchSettings()->arpTimeout().value();
  } else {
    data.switchSettings()->arpTimeout() = data.arpTimeout().value();
  }
  // Write ndpTimeout to switchSettings and old fields for transition
  if (data.switchSettings()->ndpTimeout().has_value()) {
    data.ndpTimeout() = data.switchSettings()->ndpTimeout().value();
  } else {
    data.switchSettings()->ndpTimeout() = data.ndpTimeout().value();
  }
  // Write arpAgerInterval to switchSettings and old fields for transition
  if (data.switchSettings()->arpAgerInterval().has_value()) {
    data.arpAgerInterval() = data.switchSettings()->arpAgerInterval().value();
  } else {
    data.switchSettings()->arpAgerInterval() = data.arpAgerInterval().value();
  }
  // Write staleEntryInterval to switchSettings and old fields for transition
  if (data.switchSettings()->staleEntryInterval().has_value()) {
    data.staleEntryInterval() =
        data.switchSettings()->staleEntryInterval().value();
  } else {
    data.switchSettings()->staleEntryInterval() =
        data.staleEntryInterval().value();
  }
  // Write maxNeighborProbes to switchSettings and old fields for transition
  if (data.switchSettings()->maxNeighborProbes().has_value()) {
    data.maxNeighborProbes() =
        data.switchSettings()->maxNeighborProbes().value();
  } else {
    data.switchSettings()->maxNeighborProbes() =
        data.maxNeighborProbes().value();
  }
  // Write dhcp fields to switchSettings and old fields for transition
  if (data.switchSettings()->dhcpV4RelaySrc().has_value()) {
    data.dhcpV4RelaySrc() = data.switchSettings()->dhcpV4RelaySrc().value();
  } else {
    data.switchSettings()->dhcpV4RelaySrc() = data.dhcpV4RelaySrc().value();
  }
  if (data.switchSettings()->dhcpV6RelaySrc().has_value()) {
    data.dhcpV6RelaySrc() = data.switchSettings()->dhcpV6RelaySrc().value();
  } else {
    data.switchSettings()->dhcpV6RelaySrc() = data.dhcpV6RelaySrc().value();
  }
  if (data.switchSettings()->dhcpV4ReplySrc().has_value()) {
    data.dhcpV4ReplySrc() = data.switchSettings()->dhcpV4ReplySrc().value();
  } else {
    data.switchSettings()->dhcpV4ReplySrc() = data.dhcpV4ReplySrc().value();
  }
  if (data.switchSettings()->dhcpV6ReplySrc().has_value()) {
    data.dhcpV6ReplySrc() = data.switchSettings()->dhcpV6ReplySrc().value();
  } else {
    data.switchSettings()->dhcpV6ReplySrc() = data.dhcpV6ReplySrc().value();
  }
  // Write QcmCfg to switchSettings and old fields for transition
  if (data.switchSettings()->qcmCfg().has_value()) {
    data.qcmCfg() = data.switchSettings()->qcmCfg().value();
  } else if (data.qcmCfg().has_value()) {
    data.switchSettings()->qcmCfg() = data.qcmCfg().value();
  }
  // Write defaultQosPolicy to switchSettings and old fields for transition
  if (data.switchSettings()->defaultDataPlaneQosPolicy().has_value()) {
    data.defaultDataPlaneQosPolicy() =
        data.switchSettings()->defaultDataPlaneQosPolicy().value();
  } else if (data.defaultDataPlaneQosPolicy().has_value()) {
    data.switchSettings()->defaultDataPlaneQosPolicy() =
        data.defaultDataPlaneQosPolicy().value();
  }
  // Write udfConfig to switchSettings and old fields for transition
  if (data.switchSettings()->udfConfig().has_value()) {
    data.udfConfig() = data.switchSettings()->udfConfig().value();
  } else if (data.udfConfig().is_set()) {
    data.switchSettings()->udfConfig() = data.udfConfig().value();
  }
  // Write flowletSwitchingConfig to switchSettings and old fields for
  // transition
  if (data.switchSettings()->flowletSwitchingConfig().has_value()) {
    data.flowletSwitchingConfig() =
        data.switchSettings()->flowletSwitchingConfig().value();
  } else if (data.flowletSwitchingConfig().has_value()) {
    data.switchSettings()->flowletSwitchingConfig() =
        data.flowletSwitchingConfig().value();
  }
  /* backward compatibility */
  if (auto obj = toThrift(cref<switch_state_tags::fibsMap>())) {
    data.fibs() = *obj;
  }
  if (auto obj = toThrift(cref<switch_state_tags::labelFibMap>())) {
    data.labelFib() = *obj;
  }
  if (auto obj = toThrift(cref<switch_state_tags::sflowCollectorMaps>())) {
    data.sflowCollectorMap() = *obj;
  }
  if (!cref<switch_state_tags::qosPolicyMaps>()->empty()) {
    auto key = HwSwitchMatcher::defaultHwSwitchMatcher();
    if (auto qosPolicys =
            cref<switch_state_tags::qosPolicyMaps>()->getMapNodeIf(key)) {
      data.qosPolicyMap() = qosPolicys->toThrift();
    }
  }
  if (auto obj = toThrift(cref<switch_state_tags::ipTunnelMaps>())) {
    data.ipTunnelMap() = *obj;
  }
  if (auto obj = toThrift(cref<switch_state_tags::teFlowTables>())) {
    data.teFlowTable() = *obj;
  }
  if (auto obj = toThrift(cref<switch_state_tags::aggregatePortMaps>())) {
    data.aggregatePortMap() = *obj;
  }
  if (auto obj = toThrift(cref<switch_state_tags::loadBalancerMaps>())) {
    data.loadBalancerMap() = *obj;
  }
  if (auto obj = toThrift(cref<switch_state_tags::vlanMaps>())) {
    data.vlanMap() = *obj;
  }
  if (auto obj = toThrift(cref<switch_state_tags::portMaps>())) {
    data.portMap() = *obj;
  }
  if (auto obj = toThrift(cref<switch_state_tags::remoteSystemPortMaps>())) {
    data.remoteSystemPortMap() = *obj;
  }
  if (auto obj = toThrift(cref<switch_state_tags::interfaceMaps>())) {
    data.interfaceMap() = *obj;
  }
  if (auto obj = toThrift(cref<switch_state_tags::aclTableGroupMaps>())) {
    data.aclTableGroupMap() = *obj;
  }
  if (auto obj = toThrift(cref<switch_state_tags::dsfNodesMap>())) {
    data.dsfNodes() = *obj;
  }
  if (auto obj = toThrift(cref<switch_state_tags::remoteInterfaceMaps>())) {
    data.remoteInterfaceMap() = *obj;
  }
  if (auto obj = toThrift(cref<switch_state_tags::systemPortMaps>())) {
    data.systemPortMap() = *obj;
  }
  if (auto controlPlane =
          cref<switch_state_tags::controlPlaneMap>()->getControlPlane()) {
    data.controlPlane() = controlPlane->toThrift();
  }
  if (auto obj = toThrift(cref<switch_state_tags::aclMaps>())) {
    data.aclMap() = *obj;
  }
  // for backward compatibility
  if (const auto& pfcWatchdogRecoveryAction = getPfcWatchdogRecoveryAction()) {
    data.pfcWatchdogRecoveryAction() = pfcWatchdogRecoveryAction.value();
  }
  return data;
}

std::optional<cfg::PfcWatchdogRecoveryAction>
SwitchState::getPfcWatchdogRecoveryAction() const {
  std::optional<cfg::PfcWatchdogRecoveryAction> recoveryAction{};
  // TODO - support per port recovery action. Return first ports
  // recovery action till then
  for (const auto& portMap : std::as_const(*getPorts())) {
    for (const auto& port : std::as_const(*portMap.second)) {
      if (port.second->getPfc().has_value() &&
          port.second->getPfc()->watchdog().has_value()) {
        auto pfcWd = port.second->getPfc()->watchdog().value();
        if (!recoveryAction.has_value()) {
          return *pfcWd.recoveryAction();
        }
      }
    }
  }
  return recoveryAction;
}

// THRIFT_COPY
bool SwitchState::operator==(const SwitchState& other) const {
  return (toThrift() == other.toThrift());
}

template <typename Tag, typename Type>
Type* SwitchState::modify(std::shared_ptr<SwitchState>* state) {
  if ((*state)->isPublished()) {
    SwitchState::modify(state);
  }
  auto newMnpuMap = (*state)->cref<Tag>();
  if (newMnpuMap->isPublished()) {
    newMnpuMap = newMnpuMap->clone();
    (*state)->ref<Tag>() = newMnpuMap;
  }
  return newMnpuMap.get();
}

template MultiSwitchInterfaceMap* SwitchState::modify<
    switch_state_tags::interfaceMaps>(std::shared_ptr<SwitchState>*);
template MultiSwitchInterfaceMap* SwitchState::modify<
    switch_state_tags::remoteInterfaceMaps>(std::shared_ptr<SwitchState>*);
template MultiSwitchMirrorMap* SwitchState::modify<
    switch_state_tags::mirrorMaps>(std::shared_ptr<SwitchState>*);
template MultiSwitchIpTunnelMap* SwitchState::modify<
    switch_state_tags::ipTunnelMaps>(std::shared_ptr<SwitchState>*);
template MultiSwitchSystemPortMap* SwitchState::modify<
    switch_state_tags::systemPortMaps>(std::shared_ptr<SwitchState>*);
template MultiSwitchSystemPortMap* SwitchState::modify<
    switch_state_tags::remoteSystemPortMaps>(std::shared_ptr<SwitchState>*);
template MultiSwitchPortMap* SwitchState::modify<switch_state_tags::portMaps>(
    std::shared_ptr<SwitchState>*);
template MultiLabelForwardingInformationBase* SwitchState::modify<
    switch_state_tags::labelFibMap>(std::shared_ptr<SwitchState>*);
template MultiSwitchBufferPoolCfgMap* SwitchState::modify<
    switch_state_tags::bufferPoolCfgMaps>(std::shared_ptr<SwitchState>*);
template MultiSwitchQosPolicyMap* SwitchState::modify<
    switch_state_tags::qosPolicyMaps>(std::shared_ptr<SwitchState>*);
template MultiSwitchAggregatePortMap* SwitchState::modify<
    switch_state_tags::aggregatePortMaps>(std::shared_ptr<SwitchState>*);
template MultiSwitchVlanMap* SwitchState::modify<switch_state_tags::vlanMaps>(
    std::shared_ptr<SwitchState>*);
template MultiSwitchSflowCollectorMap* SwitchState::modify<
    switch_state_tags::sflowCollectorMaps>(std::shared_ptr<SwitchState>*);
template MultiSwitchTransceiverMap* SwitchState::modify<
    switch_state_tags::transceiverMaps>(std::shared_ptr<SwitchState>*);
template MultiSwitchPortFlowletCfgMap* SwitchState::modify<
    switch_state_tags::portFlowletCfgMaps>(std::shared_ptr<SwitchState>*);
template MultiSwitchDsfNodeMap* SwitchState::modify<
    switch_state_tags::dsfNodesMap>(std::shared_ptr<SwitchState>*);

template class ThriftStructNode<SwitchState, state::SwitchState>;

} // namespace facebook::fboss
