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
  // default multi-map (for single npu) system
  resetForwardingInformationBases(
      std::make_shared<ForwardingInformationBaseMap>());
  resetSflowCollectors(std::make_shared<SflowCollectorMap>());
  resetLabelForwardingInformationBase(
      std::make_shared<LabelForwardingInformationBase>());
  resetQosPolicies(std::make_shared<QosPolicyMap>());
  resetAggregatePorts(std::make_shared<AggregatePortMap>());
  resetTransceivers(std::make_shared<TransceiverMap>());
  resetBufferPoolCfgs(std::make_shared<BufferPoolCfgMap>());
  resetVlans(std::make_shared<VlanMap>());
  resetPorts(std::make_shared<PortMap>());
  resetIntfs(std::make_shared<InterfaceMap>());
  resetAclTableGroups(std::make_shared<AclTableGroupMap>());
  resetRemoteIntfs(std::make_shared<InterfaceMap>());
  resetControlPlane(std::make_shared<ControlPlane>());
  resetSwitchSettings(std::make_shared<SwitchSettings>());
}

SwitchState::~SwitchState() {}

void SwitchState::modify(std::shared_ptr<SwitchState>* state) {
  if (!(*state)->isPublished()) {
    return;
  }
  *state = (*state)->clone();
}

std::shared_ptr<Port> SwitchState::getPort(PortID id) const {
  return getPorts()->getPort(id);
}

void SwitchState::registerPort(
    PortID id,
    const std::string& name,
    cfg::PortType portType) {
  getDefaultMap<switch_state_tags::portMaps>()->registerPort(
      id, name, portType);
}

void SwitchState::addPort(const std::shared_ptr<Port>& port) {
  getDefaultMap<switch_state_tags::portMaps>()->addPort(port);
}

void SwitchState::resetPorts(std::shared_ptr<PortMap> ports) {
  resetDefaultMap<switch_state_tags::portMaps>(ports);
}

void SwitchState::resetPorts(std::shared_ptr<MultiSwitchPortMap> ports) {
  ref<switch_state_tags::portMaps>() = ports;
}

const std::shared_ptr<PortMap>& SwitchState::getPorts() const {
  return getDefaultMap<switch_state_tags::portMaps>();
}

const std::shared_ptr<MultiSwitchPortMap>& SwitchState::getMultiSwitchPorts()
    const {
  return safe_cref<switch_state_tags::portMaps>();
}

void SwitchState::resetVlans(std::shared_ptr<VlanMap> vlans) {
  resetDefaultMap<switch_state_tags::vlanMaps>(vlans);
}

const std::shared_ptr<VlanMap>& SwitchState::getVlans() const {
  return getDefaultMap<switch_state_tags::vlanMaps>();
}

void SwitchState::addVlan(const std::shared_ptr<Vlan>& vlan) {
  if (getVlans()->isPublished()) {
    // For ease-of-use, automatically clone the VlanMap if we are still
    // pointing to a published map.
    auto vlans = getVlans()->clone();

    resetVlans(vlans);
  }
  getDefaultMap<switch_state_tags::vlanMaps>()->addVlan(vlan);
}

void SwitchState::resetIntfs(const std::shared_ptr<InterfaceMap>& intfs) {
  resetDefaultMap<switch_state_tags::interfaceMaps>(intfs);
}

void SwitchState::resetIntfs(
    const std::shared_ptr<MultiSwitchInterfaceMap>& intfs) {
  ref<switch_state_tags::interfaceMaps>() = intfs;
}

const std::shared_ptr<InterfaceMap>& SwitchState::getInterfaces() const {
  return getDefaultMap<switch_state_tags::interfaceMaps>();
}

const std::shared_ptr<MultiSwitchInterfaceMap>&
SwitchState::getMultiSwitchInterfaces() const {
  return safe_cref<switch_state_tags::interfaceMaps>();
}

void SwitchState::resetRemoteIntfs(std::shared_ptr<InterfaceMap> intfs) {
  resetDefaultMap<switch_state_tags::remoteInterfaceMaps>(intfs);
}

const std::shared_ptr<InterfaceMap>& SwitchState::getRemoteInterfaces() const {
  return getDefaultMap<switch_state_tags::remoteInterfaceMaps>();
}

const std::shared_ptr<MultiSwitchInterfaceMap>&
SwitchState::getMultiSwitchRemoteInterfaces() const {
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
    std::shared_ptr<AclTableGroupMap> aclTableGroups) {
  resetDefaultMap<switch_state_tags::aclTableGroupMaps>(aclTableGroups);
}

void SwitchState::resetAclTableGroups(
    std::shared_ptr<MultiSwitchAclTableGroupMap> aclTableGroups) {
  ref<switch_state_tags::aclTableGroupMaps>() = aclTableGroups;
}

const std::shared_ptr<AclTableGroupMap>& SwitchState::getAclTableGroups()
    const {
  return getDefaultMap<switch_state_tags::aclTableGroupMaps>();
}

const std::shared_ptr<MultiSwitchAclTableGroupMap>&
SwitchState::getMultiSwitchAclTableGroups() const {
  return safe_cref<switch_state_tags::aclTableGroupMaps>();
}

void SwitchState::resetAggregatePorts(
    std::shared_ptr<AggregatePortMap> aggPorts) {
  resetDefaultMap<switch_state_tags::aggregatePortMaps>(aggPorts);
}

const std::shared_ptr<AggregatePortMap>& SwitchState::getAggregatePorts()
    const {
  return getDefaultMap<switch_state_tags::aggregatePortMaps>();
}

void SwitchState::resetSflowCollectors(
    const std::shared_ptr<SflowCollectorMap>& sflowCollectors) {
  resetDefaultMap<switch_state_tags::sflowCollectorMaps>(sflowCollectors);
}

void SwitchState::resetQosPolicies(
    const std::shared_ptr<QosPolicyMap>& qosPolicies) {
  const auto& matcher = HwSwitchMatcher::defaultHwSwitchMatcher();
  auto qosPolicyMaps = cref<switch_state_tags::qosPolicyMaps>()->clone();
  if (!qosPolicyMaps->getMapNodeIf(matcher)) {
    qosPolicyMaps->addMapNode(qosPolicies, matcher);
  } else {
    qosPolicyMaps->updateMapNode(qosPolicies, matcher);
  }
  ref<switch_state_tags::qosPolicyMaps>() = qosPolicyMaps;
}

void SwitchState::resetControlPlane(
    std::shared_ptr<ControlPlane> controlPlane) {
  const auto& matcher = HwSwitchMatcher::defaultHwSwitchMatcherKey();
  auto controlPlaneMap = cref<switch_state_tags::controlPlaneMap>()->clone();
  if (!controlPlaneMap->getNodeIf(matcher)) {
    controlPlaneMap->addNode(matcher, controlPlane);
  } else {
    controlPlaneMap->updateNode(matcher, controlPlane);
  }
  ref<switch_state_tags::controlPlaneMap>() = controlPlaneMap;
}

void SwitchState::resetLoadBalancers(
    std::shared_ptr<MultiSwitchLoadBalancerMap> loadBalancers) {
  ref<switch_state_tags::loadBalancerMaps>() = loadBalancers;
}

void SwitchState::resetSwitchSettings(
    std::shared_ptr<SwitchSettings> switchSettings) {
  const auto& matcher = HwSwitchMatcher::defaultHwSwitchMatcherKey();
  auto switchSettingsMap =
      cref<switch_state_tags::switchSettingsMap>()->clone();
  if (!switchSettingsMap->getNodeIf(matcher)) {
    switchSettingsMap->addNode(matcher, switchSettings);
  } else {
    switchSettingsMap->updateNode(matcher, switchSettings);
  }
  ref<switch_state_tags::switchSettingsMap>() = switchSettingsMap;
}

void SwitchState::resetBufferPoolCfgs(std::shared_ptr<BufferPoolCfgMap> cfgs) {
  resetDefaultMap<switch_state_tags::bufferPoolCfgMaps>(cfgs);
}

const std::shared_ptr<BufferPoolCfgMap> SwitchState::getBufferPoolCfgs() const {
  return getDefaultMap<switch_state_tags::bufferPoolCfgMaps>();
}

const std::shared_ptr<MultiSwitchLoadBalancerMap>&
SwitchState::getLoadBalancers() const {
  return safe_cref<switch_state_tags::loadBalancerMaps>();
}

void SwitchState::resetMirrors(
    const std::shared_ptr<MultiSwitchMirrorMap>& mirrors) {
  ref<switch_state_tags::mirrorMaps>() = mirrors;
}

const std::shared_ptr<SflowCollectorMap>& SwitchState::getSflowCollectors()
    const {
  return getDefaultMap<switch_state_tags::sflowCollectorMaps>();
}

const std::shared_ptr<MultiSwitchMirrorMap>& SwitchState::getMirrors() const {
  return safe_cref<switch_state_tags::mirrorMaps>();
}

const std::shared_ptr<QosPolicyMap>& SwitchState::getQosPolicies() const {
  return cref<switch_state_tags::qosPolicyMaps>()->cref(
      HwSwitchMatcher::defaultHwSwitchMatcherKey());
}

const std::shared_ptr<ForwardingInformationBaseMap>& SwitchState::getFibs()
    const {
  return getDefaultMap<switch_state_tags::fibsMap>();
}

const std::shared_ptr<ControlPlane>& SwitchState::getControlPlane() const {
  return cref<switch_state_tags::controlPlaneMap>()->cref(
      HwSwitchMatcher::defaultHwSwitchMatcherKey());
}

const std::shared_ptr<LabelForwardingInformationBase>&
SwitchState::getLabelForwardingInformationBase() const {
  return getDefaultMap<switch_state_tags::labelFibMap>();
}

void SwitchState::resetLabelForwardingInformationBase(
    std::shared_ptr<LabelForwardingInformationBase> labelFib) {
  resetDefaultMap<switch_state_tags::labelFibMap>(labelFib);
}

void SwitchState::resetForwardingInformationBases(
    std::shared_ptr<ForwardingInformationBaseMap> fibs) {
  resetDefaultMap<switch_state_tags::fibsMap>(fibs);
}

void SwitchState::addTransceiver(
    const std::shared_ptr<TransceiverSpec>& transceiver) {
  // For ease-of-use, automatically clone the TransceiverMap if we are still
  // pointing to a published map.
  if (getTransceivers()->isPublished()) {
    auto xcvrs = getTransceivers()->clone();
    resetTransceivers(xcvrs);
  }
  getDefaultMap<switch_state_tags::transceiverMaps>()->addTransceiver(
      transceiver);
}

void SwitchState::resetTransceivers(
    std::shared_ptr<TransceiverMap> transceivers) {
  resetDefaultMap<switch_state_tags::transceiverMaps>(transceivers);
}

const std::shared_ptr<TransceiverMap>& SwitchState::getTransceivers() const {
  return getDefaultMap<switch_state_tags::transceiverMaps>();
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
  if (getAclTableGroups() &&
      getAclTableGroups()->getAclTableGroupIf(aclStage) &&
      getAclTableGroups()->getAclTableGroup(aclStage)->getAclTableMap()) {
    return getAclTableGroups()->getAclTableGroup(aclStage)->getAclTableMap();
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

std::shared_ptr<SwitchState> SwitchState::modifyTransceivers(
    const std::shared_ptr<SwitchState>& state,
    const std::unordered_map<TransceiverID, TransceiverInfo>& currentTcvrs) {
  auto origTcvrs = state->getTransceivers();
  TransceiverMap::NodeContainer newTcvrs;
  bool changed = false;
  for (const auto& tcvrInfo : currentTcvrs) {
    auto origTcvr = origTcvrs->getTransceiverIf(tcvrInfo.first);
    auto newTcvr = TransceiverSpec::createPresentTransceiver(tcvrInfo.second);
    if (!newTcvr) {
      // If the transceiver used to be present but now was removed
      changed |= (origTcvr != nullptr);
      continue;
    } else {
      if (origTcvr && *origTcvr == *newTcvr) {
        newTcvrs.emplace(origTcvr->getID(), origTcvr);
      } else {
        changed = true;
        newTcvrs.emplace(newTcvr->getID(), newTcvr);
      }
    }
  }

  if (changed) {
    XLOG(DBG2) << "New TransceiverMap has " << newTcvrs.size()
               << " present transceivers, original map has "
               << origTcvrs->size();
    auto newState = state->clone();
    newState->resetTransceivers(origTcvrs->clone(newTcvrs));
    return newState;
  } else {
    XLOG(DBG2)
        << "Current transceivers from QsfpCache has the same transceiver size:"
        << origTcvrs->size()
        << ", no need to reset TransceiverMap in current SwitchState";
    return nullptr;
  }
}

bool SwitchState::isLocalSwitchId(SwitchID switchId) const {
  auto localSwitchIds = getSwitchSettings()->getSwitchIds();
  return localSwitchIds.find(switchId) != localSwitchIds.end();
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
  if (isLocalSwitchId(switchId)) {
    return getInterfaces();
  }
  auto toRet = std::make_shared<InterfaceMap>();
  // For non local switch ids get rifs corresponding to
  // sysports on the passed in switch id
  auto sysPorts = getSystemPorts(switchId);
  for (const auto& [interfaceID, interface] :
       std::as_const(*getRemoteInterfaces())) {
    SystemPortID sysPortId(interfaceID);
    if (sysPorts->getNodeIf(sysPortId)) {
      toRet->addInterface(interface);
    }
  }
  return toRet;
}

const std::shared_ptr<SwitchSettings>& SwitchState::getSwitchSettings() const {
  return cref<switch_state_tags::switchSettingsMap>()->cref(
      HwSwitchMatcher::defaultHwSwitchMatcherKey());
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
      state->resetAclTableGroups(std::make_shared<AclTableGroupMap>());
    }
  }
  /* forward compatibility */
  auto& qosPolicyMap = state->cref<switch_state_tags::qosPolicyMap>();
  auto& multiQosPolicyMap = state->cref<switch_state_tags::qosPolicyMaps>();
  if (multiQosPolicyMap->empty() || state->getQosPolicies()->empty()) {
    // keep map for default npu
    state->resetQosPolicies(qosPolicyMap);
    // clear legacy mirror map
    state->set<switch_state_tags::qosPolicyMap>(
        std::map<std::string, state::QosPolicyFields>());
  }

  if (state->cref<switch_state_tags::controlPlaneMap>()->empty()) {
    // keep map for default npu
    state->resetControlPlane(state->cref<switch_state_tags::controlPlane>());
  }

  if (state->cref<switch_state_tags::switchSettingsMap>()->empty()) {
    // keep map for default npu
    state->resetSwitchSettings(
        state->cref<switch_state_tags::switchSettings>());
  }

  state->fromThrift<
      switch_state_tags::labelFibMap,
      switch_state_tags::labelFib>();
  state->fromThrift<
      switch_state_tags::sflowCollectorMaps,
      switch_state_tags::sflowCollectorMap>();
  state
      ->fromThrift<switch_state_tags::mirrorMaps, switch_state_tags::mirrorMap>(
          true /*emptyMnpuMapOk*/);
  state->fromThrift<switch_state_tags::fibsMap, switch_state_tags::fibs>();
  state->fromThrift<
      switch_state_tags::ipTunnelMaps,
      switch_state_tags::ipTunnelMap>(true /*emptyMnpuMapOk*/);
  state->fromThrift<
      switch_state_tags::teFlowTables,
      switch_state_tags::teFlowTable>(true /*emptyMnpuMapOk*/);
  state->fromThrift<
      switch_state_tags::aggregatePortMaps,
      switch_state_tags::aggregatePortMap>();
  state->fromThrift<
      switch_state_tags::loadBalancerMaps,
      switch_state_tags::loadBalancerMap>(true /*emptyMnpuMapOk*/);
  state->fromThrift<
      switch_state_tags::transceiverMaps,
      switch_state_tags::transceiverMap>();
  state->fromThrift<
      switch_state_tags::bufferPoolCfgMaps,
      switch_state_tags::bufferPoolCfgMap>();
  state->fromThrift<switch_state_tags::vlanMaps, switch_state_tags::vlanMap>();
  state->fromThrift<switch_state_tags::portMaps, switch_state_tags::portMap>();
  state->fromThrift<
      switch_state_tags::interfaceMaps,
      switch_state_tags::interfaceMap>();
  if (state->cref<switch_state_tags::aclTableGroupMap>()) {
    // set multi map if acl table group map exists
    state->fromThrift<
        switch_state_tags::aclTableGroupMaps,
        switch_state_tags::aclTableGroupMap>();
  }
  state
      ->fromThrift<switch_state_tags::dsfNodesMap, switch_state_tags::dsfNodes>(
          true /*emptyMnpuMapOk*/);
  state->fromThrift<
      switch_state_tags::remoteInterfaceMaps,
      switch_state_tags::remoteInterfaceMap>();
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
  auto defaultVlan = getSwitchSettings()->getDefaultVlan();
  if (defaultVlan.has_value()) {
    return VlanID(defaultVlan.value());
  }
  return VlanID(cref<switch_state_tags::defaultVlan>()->toThrift());
}

SwitchID SwitchState::getAssociatedSwitchID(PortID portID) const {
  auto port = getPort(portID);
  if (port->getInterfaceIDs().size() != 1) {
    throw FbossError(
        "Unexpected number of interfaces associated with port: ",
        port->getName(),
        " expected: 1 got: ",
        port->getInterfaceIDs().size());
  }
  auto intf = getInterfaces()->getInterface(port->getInterfaceID());

  if (!intf || intf->getType() != cfg::InterfaceType::SYSTEM_PORT) {
    throw FbossError("TODO: figure out switch id for non VOQ switch ports");
  }
  auto systemPortID = intf->getSystemPortID();
  CHECK(systemPortID.has_value());
  return getSystemPorts()->getNode(*systemPortID)->getSwitchId();
}

std::optional<cfg::Range64> SwitchState::getAssociatedSystemPortRangeIf(
    InterfaceID intfID) const {
  auto intf = getInterfaces()->getInterfaceIf(intfID);
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
  auto port = getPorts()->getPortIf(portID);
  if (!port || port->getInterfaceIDs().size() != 1) {
    return std::nullopt;
  }
  return getAssociatedSystemPortRangeIf(port->getInterfaceID());
}

InterfaceID SwitchState::getInterfaceIDForPort(PortID portID) const {
  auto port = getPorts()->getPort(portID);
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
  if (auto switchSettings =
          cref<switch_state_tags::switchSettingsMap>()->getSwitchSettings()) {
    data.switchSettings() = switchSettings->toThrift();
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
  if (auto obj = toThrift(cref<switch_state_tags::mirrorMaps>())) {
    data.mirrorMap() = *obj;
  }
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
  if (auto obj = toThrift(cref<switch_state_tags::transceiverMaps>())) {
    data.transceiverMap() = *obj;
  }
  if (auto obj = toThrift(cref<switch_state_tags::bufferPoolCfgMaps>())) {
    data.bufferPoolCfgMap() = *obj;
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
  for (const auto& port : std::as_const(*getPorts())) {
    if (port.second->getPfc().has_value() &&
        port.second->getPfc()->watchdog().has_value()) {
      auto pfcWd = port.second->getPfc()->watchdog().value();
      if (!recoveryAction.has_value()) {
        return *pfcWd.recoveryAction();
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

template class ThriftStructNode<SwitchState, state::SwitchState>;

} // namespace facebook::fboss
