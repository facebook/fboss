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

namespace {
constexpr auto kInterfaces = "interfaces";
constexpr auto kPorts = "ports";
constexpr auto kVlans = "vlans";
constexpr auto kDefaultVlan = "defaultVlan";
constexpr auto kAcls = "acls";
constexpr auto kSflowCollectors = "sFlowCollectors";
constexpr auto kControlPlane = "controlPlane";
constexpr auto kQosPolicies = "qosPolicies";
constexpr auto kArpTimeout = "arpTimeout";
constexpr auto kNdpTimeout = "ndpTimeout";
constexpr auto kArpAgerInterval = "arpAgerInterval";
constexpr auto kMaxNeighborProbes = "maxNeighborProbes";
constexpr auto kStaleEntryInterval = "staleEntryInterval";
constexpr auto kLoadBalancers = "loadBalancers";
constexpr auto kMirrors = "mirrors";
constexpr auto kAggregatePorts = "aggregatePorts";
constexpr auto kLabelForwardingInformationBase = "labelFib";
constexpr auto kSwitchSettings = "switchSettings";
constexpr auto kDefaultDataplaneQosPolicy = "defaultDataPlaneQosPolicy";
constexpr auto kQcmCfg = "qcmConfig";
constexpr auto kBufferPoolCfgs = "bufferPoolConfigs";
constexpr auto kFibs = "fibs";
constexpr auto kTransceivers = "transceivers";
constexpr auto kAclTableGroups = "aclTableGroups";
constexpr auto kSystemPorts = "systemPorts";
constexpr auto kTunnels = "ipTunnels";
constexpr auto kTeFlows = "teFlows";
constexpr auto kRemoteSystemPorts = "remoteSystemPorts";
constexpr auto kRemoteInterfaces = "remoteInterfaces";
constexpr auto kDsfNodes = "dsfNodes";
constexpr auto kUdfConfig = "udfConfig";
constexpr auto kFlowletSwitchingConfig = "flowletSwitchingConfig";
} // namespace

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
  ref<switch_state_tags::portMap>()->registerPort(id, name, portType);
}

void SwitchState::addPort(const std::shared_ptr<Port>& port) {
  ref<switch_state_tags::portMap>()->addPort(port);
}

void SwitchState::resetPorts(std::shared_ptr<PortMap> ports) {
  ref<switch_state_tags::portMap>() = ports;
}

void SwitchState::resetVlans(std::shared_ptr<VlanMap> vlans) {
  ref<switch_state_tags::vlanMap>() = vlans;
}

void SwitchState::addVlan(const std::shared_ptr<Vlan>& vlan) {
  if (cref<switch_state_tags::vlanMap>()->isPublished()) {
    // For ease-of-use, automatically clone the VlanMap if we are still
    // pointing to a published map.
    auto vlans = cref<switch_state_tags::vlanMap>()->clone();

    ref<switch_state_tags::vlanMap>() = vlans;
  }
  ref<switch_state_tags::vlanMap>()->addVlan(vlan);
}

void SwitchState::setDefaultVlan(const VlanID& id) {
  set<switch_state_tags::defaultVlan>(id);
}

void SwitchState::setArpTimeout(seconds timeout) {
  set<switch_state_tags::arpTimeout>(timeout.count());
}

void SwitchState::setNdpTimeout(seconds timeout) {
  set<switch_state_tags::ndpTimeout>(timeout.count());
}

void SwitchState::setArpAgerInterval(seconds interval) {
  set<switch_state_tags::arpAgerInterval>(interval.count());
}

void SwitchState::setMaxNeighborProbes(uint32_t maxNeighborProbes) {
  set<switch_state_tags::maxNeighborProbes>(maxNeighborProbes);
}

void SwitchState::setStaleEntryInterval(seconds interval) {
  set<switch_state_tags::staleEntryInterval>(interval.count());
}

void SwitchState::addIntf(const std::shared_ptr<Interface>& intf) {
  if (cref<switch_state_tags::interfaceMap>()->isPublished()) {
    // For ease-of-use, automatically clone the InterfaceMap if we are still
    // pointing to a published map.
    auto intfs = cref<switch_state_tags::interfaceMap>()->clone();
    ref<switch_state_tags::interfaceMap>() = intfs;
  }
  ref<switch_state_tags::interfaceMap>()->addInterface(intf);
}

void SwitchState::resetIntfs(std::shared_ptr<InterfaceMap> intfs) {
  ref<switch_state_tags::interfaceMap>() = intfs;
}

void SwitchState::resetRemoteIntfs(std::shared_ptr<InterfaceMap> intfs) {
  ref<switch_state_tags::remoteInterfaceMap>() = intfs;
}

void SwitchState::resetRemoteSystemPorts(
    std::shared_ptr<SystemPortMap> sysPorts) {
  ref<switch_state_tags::remoteSystemPortMap>() = sysPorts;
}

void SwitchState::addAcl(const std::shared_ptr<AclEntry>& acl) {
  // For ease-of-use, automatically clone the AclMap if we are still
  // pointing to a published map.
  if (cref<switch_state_tags::aclMap>()->isPublished()) {
    auto acls = cref<switch_state_tags::aclMap>()->clone();
    ref<switch_state_tags::aclMap>() = acls;
  }
  ref<switch_state_tags::aclMap>()->addEntry(acl);
}

std::shared_ptr<AclEntry> SwitchState::getAcl(const std::string& name) const {
  return getAcls()->getEntryIf(name);
}

void SwitchState::resetAcls(std::shared_ptr<AclMap> acls) {
  ref<switch_state_tags::aclMap>() = acls;
}

void SwitchState::resetAclTableGroups(
    std::shared_ptr<AclTableGroupMap> aclTableGroups) {
  ref<switch_state_tags::aclTableGroupMap>() = aclTableGroups;
}

void SwitchState::resetAggregatePorts(
    std::shared_ptr<AggregatePortMap> aggPorts) {
  ref<switch_state_tags::aggregatePortMap>() = aggPorts;
}

void SwitchState::resetSflowCollectors(
    const std::shared_ptr<SflowCollectorMap>& collectors) {
  ref<switch_state_tags::sflowCollectorMap>() = collectors;
}

void SwitchState::resetQosPolicies(std::shared_ptr<QosPolicyMap> qosPolicies) {
  ref<switch_state_tags::qosPolicyMap>() = qosPolicies;
}

void SwitchState::resetControlPlane(
    std::shared_ptr<ControlPlane> controlPlane) {
  ref<switch_state_tags::controlPlane>() = controlPlane;
}

void SwitchState::resetLoadBalancers(
    std::shared_ptr<LoadBalancerMap> loadBalancers) {
  ref<switch_state_tags::loadBalancerMap>() = loadBalancers;
}

void SwitchState::resetSwitchSettings(
    std::shared_ptr<SwitchSettings> switchSettings) {
  ref<switch_state_tags::switchSettings>() = switchSettings;
}

void SwitchState::resetQcmCfg(std::shared_ptr<QcmCfg> qcmCfg) {
  ref<switch_state_tags::qcmCfg>() = qcmCfg;
}

void SwitchState::resetBufferPoolCfgs(std::shared_ptr<BufferPoolCfgMap> cfgs) {
  ref<switch_state_tags::bufferPoolCfgMap>() = cfgs;
}

void SwitchState::resetFlowletSwitchingConfig(
    std::shared_ptr<FlowletSwitchingConfig> flowletSwitchingConfig) {
  ref<switch_state_tags::flowletSwitchingConfig>() = flowletSwitchingConfig;
}

const std::shared_ptr<LoadBalancerMap>& SwitchState::getLoadBalancers() const {
  return cref<switch_state_tags::loadBalancerMap>();
}

void SwitchState::resetMirrors(std::shared_ptr<MirrorMap> mirrors) {
  ref<switch_state_tags::mirrorMap>() = mirrors;
}

const std::shared_ptr<MirrorMap>& SwitchState::getMirrors() const {
  return cref<switch_state_tags::mirrorMap>();
}

const std::shared_ptr<ForwardingInformationBaseMap>& SwitchState::getFibs()
    const {
  return cref<switch_state_tags::fibs>();
}

void SwitchState::resetLabelForwardingInformationBase(
    std::shared_ptr<LabelForwardingInformationBase> labelFib) {
  ref<switch_state_tags::labelFib>() = labelFib;
}

void SwitchState::resetForwardingInformationBases(
    std::shared_ptr<ForwardingInformationBaseMap> fibs) {
  ref<switch_state_tags::fibs>() = fibs;
}

void SwitchState::addTransceiver(
    const std::shared_ptr<TransceiverSpec>& transceiver) {
  // For ease-of-use, automatically clone the TransceiverMap if we are still
  // pointing to a published map.
  if (cref<switch_state_tags::transceiverMap>()->isPublished()) {
    auto xcvrs = cref<switch_state_tags::transceiverMap>()->clone();
    ref<switch_state_tags::transceiverMap>() = xcvrs;
  }
  ref<switch_state_tags::transceiverMap>()->addTransceiver(transceiver);
}

void SwitchState::resetTransceivers(
    std::shared_ptr<TransceiverMap> transceivers) {
  ref<switch_state_tags::transceiverMap>() = transceivers;
}

void SwitchState::addSystemPort(const std::shared_ptr<SystemPort>& systemPort) {
  // For ease-of-use, automatically clone the SystemPortMap if we are still
  // pointing to a published map.
  if (cref<switch_state_tags::systemPortMap>()->isPublished()) {
    auto sysPortMap = cref<switch_state_tags::systemPortMap>()->clone();
    ref<switch_state_tags::systemPortMap>() = sysPortMap;
  }
  ref<switch_state_tags::systemPortMap>()->addSystemPort(systemPort);
}

void SwitchState::resetSystemPorts(std::shared_ptr<SystemPortMap> systemPorts) {
  ref<switch_state_tags::systemPortMap>() = systemPorts;
}

void SwitchState::addTunnel(const std::shared_ptr<IpTunnel>& tunnel) {
  // For ease-of-use, automatically clone the TunnelMap if we are still
  // pointing to a published map.
  if (cref<switch_state_tags::ipTunnelMap>()->isPublished()) {
    auto ipTunnelMap = cref<switch_state_tags::ipTunnelMap>()->clone();
    ref<switch_state_tags::ipTunnelMap>() = ipTunnelMap;
  }
  ref<switch_state_tags::ipTunnelMap>()->addTunnel(tunnel);
}

void SwitchState::resetTunnels(std::shared_ptr<IpTunnelMap> tunnels) {
  ref<switch_state_tags::ipTunnelMap>() = tunnels;
}

void SwitchState::resetTeFlowTable(std::shared_ptr<TeFlowTable> flowTable) {
  ref<switch_state_tags::teFlowTable>() = flowTable;
}

void SwitchState::resetDsfNodes(std::shared_ptr<DsfNodeMap> dsfNodes) {
  ref<switch_state_tags::dsfNodes>() = dsfNodes;
}

void SwitchState::resetUdfConfig(std::shared_ptr<UdfConfig> udfConfig) {
  ref<switch_state_tags::udfConfig>() = udfConfig;
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
  auto mySwitchId = getSwitchSettings()->getSwitchId();
  return mySwitchId && SwitchID(*mySwitchId) == switchId;
}

std::shared_ptr<SystemPortMap> SwitchState::getSystemPorts(
    SwitchID switchId) const {
  auto sysPorts =
      isLocalSwitchId(switchId) ? getSystemPorts() : getRemoteSystemPorts();
  auto toRet = std::make_shared<SystemPortMap>();
  for (const auto& idAndSysPort : std::as_const(*sysPorts)) {
    const auto& sysPort = idAndSysPort.second;
    if (sysPort->getSwitchId() == switchId) {
      toRet->addSystemPort(sysPort);
    }
  }
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

void SwitchState::revertNewTeFlowEntry(
    const std::shared_ptr<TeFlowEntry>& newTeFlowEntry,
    const std::shared_ptr<TeFlowEntry>& oldTeFlowEntry,
    std::shared_ptr<SwitchState>* appliedState) {
  auto clonedTeFlowTable =
      (*appliedState)->getTeFlowTable()->modify(appliedState);
  if (oldTeFlowEntry) {
    clonedTeFlowTable->updateNode(oldTeFlowEntry);
  } else {
    clonedTeFlowTable->removeNode(newTeFlowEntry);
  }
}

std::unique_ptr<SwitchState> SwitchState::uniquePtrFromThrift(
    const state::SwitchState& switchState) {
  auto state = std::make_unique<SwitchState>();
  state->BaseT::fromThrift(switchState);
  if (FLAGS_enable_acl_table_group) {
    if (!switchState.aclMap()->empty()) {
      state->ref<switch_state_tags::aclTableGroupMap>() =
          AclTableGroupMap::createDefaultAclTableGroupMapFromThrift(
              switchState.get_aclMap());
      state->ref<switch_state_tags::aclMap>()->clear();
    }
  }
  if (!FLAGS_enable_acl_table_group) {
    auto aclTableGroupMap = switchState.aclTableGroupMap();
    if (aclTableGroupMap && aclTableGroupMap->size() > 0) {
      state->ref<switch_state_tags::aclMap>() =
          AclTableGroupMap::getDefaultAclTableGroupMap(*aclTableGroupMap);
      state->ref<switch_state_tags::aclTableGroupMap>()->clear();
    }
  }
  return state;
}

VlanID SwitchState::getDefaultVlan() const {
  return VlanID(cref<switch_state_tags::defaultVlan>()->toThrift());
}

std::shared_ptr<SwitchState> SwitchState::fromThrift(
    const state::SwitchState& data) {
  auto uniqState = uniquePtrFromThrift(data);
  std::shared_ptr<SwitchState> state = std::move(uniqState);
  return state;
}

state::SwitchState SwitchState::toThrift() const {
  auto data = BaseT::toThrift();
  auto aclMap = data.aclMap();
  auto aclTableGroupMap = data.aclTableGroupMap();
  if (FLAGS_enable_acl_table_group) {
    if (!aclMap->empty() && (!aclTableGroupMap || aclTableGroupMap->empty())) {
      data.aclTableGroupMap() =
          AclTableGroupMap::createDefaultAclTableGroupMapFromThrift(
              data.get_aclMap())
              ->toThrift();
    }
    aclMap->clear();
  } else {
    if (aclTableGroupMap && !aclTableGroupMap->empty() && aclMap->empty()) {
      if (auto aclMapPtr =
              AclTableGroupMap::getDefaultAclTableGroupMap(*aclTableGroupMap)) {
        aclMap = aclMapPtr->toThrift();
      }
      aclTableGroupMap->clear();
    }
  }
  return data;
}

// THRIFT_COPY
bool SwitchState::operator==(const SwitchState& other) const {
  return (toThrift() == other.toThrift());
}

template class ThriftStructNode<SwitchState, state::SwitchState>;

} // namespace facebook::fboss
