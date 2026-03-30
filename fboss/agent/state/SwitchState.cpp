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
#include "fboss/agent/AgentFeatures.h"
#include "fboss/agent/FbossError.h"
#include "fboss/agent/HwSwitchMatcher.h"
#include "fboss/agent/state/AclEntry.h"
#include "fboss/agent/state/AclMap.h"
#include "fboss/agent/state/AclTableGroupMap.h"
#include "fboss/agent/state/AggregatePort.h"
#include "fboss/agent/state/AggregatePortMap.h"
#include "fboss/agent/state/BufferPoolConfig.h"
#include "fboss/agent/state/BufferPoolConfigMap.h"
#include "fboss/agent/state/ControlPlane.h"
#include "fboss/agent/state/FibInfo.h"
#include "fboss/agent/state/FibInfoMap.h"
#include "fboss/agent/state/ForwardingInformationBaseMap.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/InterfaceMap.h"
#include "fboss/agent/state/IpTunnel.h"
#include "fboss/agent/state/IpTunnelMap.h"
#include "fboss/agent/state/LabelForwardingInformationBase.h"
#include "fboss/agent/state/MirrorOnDropReportMap.h"
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

using std::make_shared;
using std::shared_ptr;
using std::chrono::seconds;

DEFINE_bool(
    enable_acl_table_group,
    false,
    "Allow multiple acl tables (acl table group)");

/*
 * DEPRECATED: Migration to Interface neighbor tables is complete.
 * This flag is kept temporarily for compatibility with test configs
 * that pass --intf_nbr_tables=true. Will be removed after configerator cleanup.
 */
DEFINE_bool(
    intf_nbr_tables,
    true,
    "DEPRECATED: No longer used. Neighbor tables are always on interfaces.");

DEFINE_bool(
    emStatOnlyMode,
    false,
    "Flag to turn on EM entry programming with stat alone for FHTE");

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
  resetIntfs(std::make_shared<MultiSwitchInterfaceMap>());
  resetRemoteIntfs(std::make_shared<MultiSwitchInterfaceMap>());
  resetRemoteSystemPorts(std::make_shared<MultiSwitchSystemPortMap>());
  resetTransceivers(std::make_shared<MultiSwitchTransceiverMap>());
  resetControlPlane(std::make_shared<MultiControlPlane>());
  resetSwitchSettings(std::make_shared<MultiSwitchSettings>());
}

SwitchState::~SwitchState() = default;

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

const std::shared_ptr<MultiSwitchSystemPortMap>&
SwitchState::getFabricLinkMonitoringSystemPorts() const {
  return safe_cref<switch_state_tags::fabricLinkMonitoringSystemPortMaps>();
}

std::shared_ptr<AclEntry> SwitchState::getAcl(const std::string& name) const {
  if (FLAGS_enable_acl_table_group) {
    getAclTableGroups()
        ->getNode(cfg::AclStage::INGRESS)
        ->getAclTableMap()
        ->getNodeIf(name);
  }
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

void SwitchState::resetMirrorOnDropReports(
    const std::shared_ptr<MultiSwitchMirrorOnDropReportMap>& reports) {
  ref<switch_state_tags::mirrorOnDropReportMaps>() = reports;
}

const std::shared_ptr<MultiSwitchSflowCollectorMap>&
SwitchState::getSflowCollectors() const {
  return safe_cref<switch_state_tags::sflowCollectorMaps>();
}

const std::shared_ptr<MultiSwitchMirrorMap>& SwitchState::getMirrors() const {
  return safe_cref<switch_state_tags::mirrorMaps>();
}

const std::shared_ptr<MultiSwitchMirrorOnDropReportMap>&
SwitchState::getMirrorOnDropReports() const {
  return safe_cref<switch_state_tags::mirrorOnDropReportMaps>();
}

const std::shared_ptr<MultiSwitchQosPolicyMap>& SwitchState::getQosPolicies()
    const {
  return safe_cref<switch_state_tags::qosPolicyMaps>();
}

const std::shared_ptr<MultiSwitchFibInfoMap>& SwitchState::getFibsInfoMap()
    const {
  return safe_cref<switch_state_tags::fibsInfoMap>();
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

void SwitchState::resetFibsInfoMap(
    std::shared_ptr<MultiSwitchFibInfoMap> fibsInfoMap) {
  ref<switch_state_tags::fibsInfoMap>() = fibsInfoMap;
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

void SwitchState::resetRemoteSystemPorts(
    const std::shared_ptr<MultiSwitchSystemPortMap>& systemPorts) {
  ref<switch_state_tags::remoteSystemPortMaps>() = systemPorts;
}

void SwitchState::resetFabricLinkMonitoringSystemPorts(
    const std::shared_ptr<MultiSwitchSystemPortMap>& systemPorts) {
  ref<switch_state_tags::fabricLinkMonitoringSystemPortMaps>() = systemPorts;
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

void SwitchState::resetSrv6Tunnels(
    std::shared_ptr<MultiSwitchSrv6TunnelMap> tunnels) {
  ref<switch_state_tags::srv6TunnelMaps>() = tunnels;
}

const std::shared_ptr<MultiSwitchSrv6TunnelMap>& SwitchState::getSrv6Tunnels()
    const {
  return safe_cref<switch_state_tags::srv6TunnelMaps>();
}

void SwitchState::resetMySids(std::shared_ptr<MultiSwitchMySidMap> mySids) {
  ref<switch_state_tags::mySidMaps>() = mySids;
}

const std::shared_ptr<MultiSwitchMySidMap>& SwitchState::getMySids() const {
  return safe_cref<switch_state_tags::mySidMaps>();
}

void SwitchState::resetTeFlowTable(
    std::shared_ptr<MultiTeFlowTable> flowTable) {
  ref<switch_state_tags::teFlowTables>() = flowTable;
}

const std::shared_ptr<MultiTeFlowTable> SwitchState::getTeFlowTable() const {
  return get<switch_state_tags::teFlowTables>();
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
    auto aclMap =
        utility::getFirstMap(state->cref<switch_state_tags::aclMaps>());
    if (aclMap && aclMap->size()) {
      auto multiSwitchAclGroupMap =
          std::make_shared<MultiSwitchAclTableGroupMap>();
      auto matcher = HwSwitchMatcher(
          state->cref<switch_state_tags::aclMaps>()->cbegin()->first);
      multiSwitchAclGroupMap->addMapNode(
          AclTableGroupMap::createDefaultAclTableGroupMapFromThrift(
              aclMap->toThrift()),
          matcher);
      state->ref<switch_state_tags::aclTableGroupMaps>() =
          multiSwitchAclGroupMap;
      state->resetAcls(std::make_shared<MultiSwitchAclMap>());
    }
  }
  if (!FLAGS_enable_acl_table_group) {
    auto firstTableGroupMap = utility::getFirstMap(
        state->cref<switch_state_tags::aclTableGroupMaps>());
    auto aclMap = firstTableGroupMap
        ? AclTableGroupMap::getDefaultAclTableGroupMap(
              firstTableGroupMap->toThrift())
        : nullptr;
    if (aclMap && aclMap->size()) {
      auto multiSwitchAclMap = std::make_shared<MultiSwitchAclMap>();
      auto matcher = HwSwitchMatcher(
          state->cref<switch_state_tags::aclTableGroupMaps>()->cbegin()->first);
      multiSwitchAclMap->addMapNode(aclMap, matcher);
      state->set<switch_state_tags::aclMaps>(multiSwitchAclMap->toThrift());
    }
  }

  /*
   * FIB Migration: Four-stage transition from fibsMap to fibsInfoMap
   *
   * Stage 1 : Rollback safety - Clear fibsInfoMap when deserializing
   * to handle rollback from Stage 2 where both FIBs are
   * populated during warm boot exit.
   *
   * Stage 2 (Current): Migrate clients to new FIB while populating
   * both structures during warm boot exit. Forward migration (Stage
   * 2→3) clears fibsMap on init; rollback (Stage 2→1) clears fibsInfoMap during
   * init of stage 1.
   *
   * Stage 3: New FIB primary - All clients use fibsInfoMap, but both FIBs still
   * populated during warm boot exit to support direct Stage 1→3
   * transitions.
   *
   * Stage 4: Complete migration - Remove fibsMap entirely from codebase.
   */

  // Migrate old fibsMap to new fibsInfoMap
  // We need to populate fibsInfoMap with map<SwitchIdList, FibInfoFields>
  auto oldFibsMap = state->getFibs();
  if (oldFibsMap && !oldFibsMap->empty()) {
    auto fibsInfoMap = state->getFibsInfoMap();
    // Only convert if new fibsInfoMap doesn't exist or is empty
    if (!fibsInfoMap || fibsInfoMap->empty()) {
      auto newFibsInfoMap = std::make_shared<MultiSwitchFibInfoMap>();
      for (const auto& [switchIdListStr, fibMap] : std::as_const(*oldFibsMap)) {
        auto fibInfo = std::make_shared<FibInfo>();

        // Copy the old fibMap to the new FibInfo
        fibInfo->resetFibsMap(fibMap);

        newFibsInfoMap->updateFibInfo(
            fibInfo, HwSwitchMatcher(switchIdListStr));
      }

      // Update the state with the new fibsInfoMap
      state->resetFibsInfoMap(newFibsInfoMap);
    }
    // Clear the old fibsMap
    state->resetForwardingInformationBases(
        std::make_shared<MultiSwitchForwardingInformationBaseMap>());
  }

  // Migrate old ports field to new portsInfo field for warmboot compatibility
  for (auto& vlanMapEntry : *state->ref<switch_state_tags::vlanMaps>()) {
    for (auto& [vlanId, vlan] : *vlanMapEntry.second) {
      // If new portsInfo field is empty but old ports field has data, migrate
      // it
      if (vlan->getPortsInfo().empty()) {
        auto oldPorts = vlan->get<switch_state_tags::ports_DEPRECATED>();
        if (oldPorts && !oldPorts->empty()) {
          std::map<int16_t, state::VlanInfo> portsInfo;
          for (const auto& [portId, taggedOpt] : *oldPorts) {
            if (taggedOpt) {
              state::VlanInfo vlanInfo;
              *vlanInfo.tagged() = taggedOpt->cref();
              *vlanInfo.priorityTagged() = false;
              portsInfo.emplace(portId, vlanInfo);
            }
          }
          vlan->set<switch_state_tags::portsInfo>(portsInfo);
        }
      }
    }
  }

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
  // TODO - remove the default value return. Defaut vlan is
  // mandatory on broadcom NPU switches. Set it in config
  // for required switches.
  return VlanID(cfg::switch_config_constants::defaultVlanId());
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

cfg::SystemPortRanges SwitchState::getAssociatedSystemPortRangesIf(
    InterfaceID intfID) const {
  auto intf = getInterfaces()->getNodeIf(intfID);
  if (!intf || intf->getType() != cfg::InterfaceType::SYSTEM_PORT) {
    return cfg::SystemPortRanges();
  }
  auto systemPortID = intf->getSystemPortID();
  CHECK(systemPortID.has_value());
  auto switchId = getSystemPorts()->getNode(*systemPortID)->getSwitchId();
  auto dsfNode = getDsfNodes()->getNodeIf(switchId);
  return dsfNode->getSystemPortRanges();
}

cfg::SystemPortRanges SwitchState::getAssociatedSystemPortRangesIf(
    PortID portID) const {
  auto port = getPorts()->getNodeIf(portID);
  if (!port || port->getInterfaceIDs().size() != 1) {
    return cfg::SystemPortRanges();
  }
  return getAssociatedSystemPortRangesIf(port->getInterfaceID());
}

std::optional<int> SwitchState::getClusterId(SwitchID switchId) const {
  auto dsfNode = getDsfNodes()->getNodeIf(switchId);
  CHECK(dsfNode) << "invalid switch ID " << switchId;
  // TODO(daiweix): get clusterId from switch name
  return dsfNode->getClusterId();
  ;
}

std::vector<SwitchID> SwitchState::getIntraClusterSwitchIds(
    SwitchID switchId) const {
  std::map<int, std::vector<SwitchID>> clusterIdToSwitchIds;
  for (const auto& matcherAndNodes : std::as_const(*getDsfNodes())) {
    for (const auto& idAndNode : std::as_const(*matcherAndNodes.second)) {
      auto sid = static_cast<SwitchID>(idAndNode.first);
      auto cid = getClusterId(sid);
      if (cid &&
          idAndNode.second->getType() == cfg::DsfNodeType::INTERFACE_NODE) {
        clusterIdToSwitchIds[cid.value()].push_back(sid);
      }
    }
  }
  auto clusterId = getClusterId(switchId);
  if (!clusterId || clusterIdToSwitchIds.size() == 1) {
    return {};
  }
  return clusterIdToSwitchIds[clusterId.value()];
}

std::optional<InterfaceID> SwitchState::getInterfaceIDForPortIf(
    const PortDescriptor& port) const {
  switch (port.type()) {
    case PortDescriptor::PortType::PHYSICAL: {
      auto physicalPort = getPorts()->getNodeIf(port.phyPortID());
      if (!physicalPort) {
        XLOG(ERR) << "No port node found for port " << port.phyPortID();
        return std::nullopt;
      }
      // On VOQ/Fabric switches, port and interface have 1:1 relation.
      // For non VOQ/Fabric switches, in practice, a port is always part of a
      // single VLAN (and thus single interface).
      return physicalPort->getInterfaceID();
    }
    case PortDescriptor::PortType::AGGREGATE: {
      auto aggregatePort = getAggregatePorts()->getNodeIf(port.aggPortID());
      if (!aggregatePort || aggregatePort->getInterfaceIDs()->empty()) {
        XLOG(ERR) << "No aggregate port or interface found for "
                  << port.aggPortID();
        return std::nullopt;
      }
      // All aggregate member ports always belong to the same interface(s).
      // Thus, pick the interface for any member port.
      return InterfaceID(aggregatePort->getInterfaceIDs()->at(0)->cref());
    }
    case PortDescriptor::PortType::SYSTEM_PORT:
      XLOG(FATAL) << "Cannot get interface ID for system port: "
                  << port.sysPortID();
  }
  return std::nullopt;
}

InterfaceID SwitchState::getInterfaceIDForPort(
    const PortDescriptor& port) const {
  auto intfID = getInterfaceIDForPortIf(port);
  if (!intfID) {
    throw FbossError("No interface found for port ", port.str());
  }
  return intfID.value();
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
        !data.aclMaps().value().at(matcher).empty() &&
        (aclTableGroupMaps->empty() ||
         aclTableGroupMaps->find(matcher) == aclTableGroupMaps->end() ||
         aclTableGroupMaps->find(matcher)->second.empty())) {
      data.aclTableGroupMaps()->emplace(
          matcher,
          AclTableGroupMap::createDefaultAclTableGroupMapFromThrift(
              data.aclMaps().value().at(matcher))
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

  // Migrate new portsInfo field to old ports field for warmboot compatibility
  for (auto& [matcherKey, vlanMapThrift] : *data.vlanMaps()) {
    for (auto& [vlanId, vlanThrift] : vlanMapThrift) {
      // If new portsInfo field has data, populate old ports field
      if (!vlanThrift.portsInfo()->empty()) {
        std::map<int16_t, bool> ports;
        for (const auto& [portId, vlanInfo] : *vlanThrift.portsInfo()) {
          ports.emplace(portId, *vlanInfo.tagged());
        }
        vlanThrift.ports_DEPRECATED() = ports;
      }
    }
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
template MultiSwitchMirrorOnDropReportMap* SwitchState::modify<
    switch_state_tags::mirrorOnDropReportMaps>(std::shared_ptr<SwitchState>*);
template MultiSwitchIpTunnelMap* SwitchState::modify<
    switch_state_tags::ipTunnelMaps>(std::shared_ptr<SwitchState>*);
template MultiSwitchSrv6TunnelMap* SwitchState::modify<
    switch_state_tags::srv6TunnelMaps>(std::shared_ptr<SwitchState>*);
template MultiSwitchMySidMap* SwitchState::modify<switch_state_tags::mySidMaps>(
    std::shared_ptr<SwitchState>*);
template MultiSwitchSystemPortMap* SwitchState::modify<
    switch_state_tags::systemPortMaps>(std::shared_ptr<SwitchState>*);
template MultiSwitchSystemPortMap* SwitchState::modify<
    switch_state_tags::remoteSystemPortMaps>(std::shared_ptr<SwitchState>*);
template MultiSwitchSystemPortMap*
SwitchState::modify<switch_state_tags::fabricLinkMonitoringSystemPortMaps>(
    std::shared_ptr<SwitchState>*);
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

template struct ThriftStructNode<SwitchState, state::SwitchState>;

} // namespace facebook::fboss
