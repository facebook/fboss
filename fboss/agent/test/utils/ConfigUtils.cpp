/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/test/utils/ConfigUtils.h"
#include <memory>

#include "fboss/agent/AgentFeatures.h"
#include "fboss/agent/FbossError.h"
#include "fboss/agent/SwSwitch.h"

#include "fboss/agent/test/utils/PortTestUtils.h"

#include <folly/Format.h>

DEFINE_bool(nodeZ, false, "Setup test config as node Z");

namespace {
constexpr auto kSysPortOffset = 100;
}

namespace facebook::fboss::utility {

folly::MacAddress kLocalCpuMac() {
  static const folly::MacAddress kLocalMac(
      FLAGS_nodeZ ? "02:00:00:00:00:02" : "02:00:00:00:00:01");
  return kLocalMac;
}

std::string getLocalCpuMacStr() {
  return kLocalCpuMac().toString();
}

const std::map<cfg::PortType, cfg::PortLoopbackMode>& kDefaultLoopbackMap() {
  static const std::map<cfg::PortType, cfg::PortLoopbackMode> kLoopbackMap = {
      {cfg::PortType::INTERFACE_PORT, cfg::PortLoopbackMode::NONE},
      {cfg::PortType::FABRIC_PORT, cfg::PortLoopbackMode::NONE},
      {cfg::PortType::MANAGEMENT_PORT, cfg::PortLoopbackMode::NONE},
      {cfg::PortType::RECYCLE_PORT, cfg::PortLoopbackMode::NONE}};
  return kLoopbackMap;
}

bool isEnabledPortWithSubnet(
    const cfg::Port& port,
    const cfg::SwitchConfig& config) {
  auto ingressVlan = port.get_ingressVlan();
  for (const auto& intf : *config.interfaces()) {
    if (intf.get_vlanID() == ingressVlan) {
      return (
          !intf.get_ipAddresses().empty() &&
          port.get_state() == cfg::PortState::ENABLED);
    }
  }
  return false;
}

std::vector<std::string> getLoopbackIps(SwitchID switchId) {
  auto switchIdVal = static_cast<int64_t>(switchId);
  // Use (200-255):(0-255) range for DSF node loopback IPs. Therefore, the max
  // number of switchId can be accomodated will be 56 * 256 = 14366, which is
  // more than what we need in both J2 and J3.
  const auto switchIdLimit = 14366;
  CHECK_LT(switchIdVal, switchIdLimit)
      << " Switch Id >=" << switchIdLimit << " , not supported";

  int firstOctet = 200 + switchIdVal / 256;
  int secondOctet = switchIdVal % 256;

  auto v6 = FLAGS_nodeZ
      ? folly::sformat("{}:{}::2/64", firstOctet, secondOctet)
      : folly::sformat("{}:{}::1/64", firstOctet, secondOctet);
  auto v4 = FLAGS_nodeZ
      ? folly::sformat("{}.{}.0.2/24", firstOctet, secondOctet)
      : folly::sformat("{}.{}.0.1/24", firstOctet, secondOctet);
  return {v6, v4};
}

std::vector<cfg::PortQueue> getFabTxQueueConfig() {
  cfg::PortQueue fabQueue;
  fabQueue.id() = 0;
  fabQueue.name() = "fabric_q0";
  fabQueue.streamType() = cfg::StreamType::FABRIC_TX;
  fabQueue.scheduling() = cfg::QueueScheduling::INTERNAL;
  return {fabQueue};
}

std::unordered_map<PortID, cfg::PortProfileID>& getPortToDefaultProfileIDMap() {
  static std::unordered_map<PortID, cfg::PortProfileID> portProfileIDMap;
  return portProfileIDMap;
}

std::unordered_map<PortID, cfg::PortProfileID> getSafeProfileIDs(
    const PlatformMapping* platformMapping,
    const HwAsic* asic,
    const std::map<PortID, std::vector<PortID>>&
        controllingPortToSubsidaryPorts,
    bool supportsAddRemovePort,
    std::optional<std::vector<PortID>> masterLogicalPortIds) {
  std::unordered_map<PortID, cfg::PortProfileID> portToProfileIDs;
  const auto& plarformEntries = platformMapping->getPlatformPorts();
  for (const auto& group : controllingPortToSubsidaryPorts) {
    const auto& ports = group.second;
    // Find the safe profile to satisfy all the ports in the group
    std::set<cfg::PortProfileID> safeProfiles;
    for (const auto& portID : ports) {
      for (const auto& profile :
           *plarformEntries.at(portID).supportedProfiles()) {
        if (auto subsumedPorts = profile.second.subsumedPorts();
            subsumedPorts && !subsumedPorts->empty()) {
          // Certain PortProfiles with higher speeds are safe, as long as
          // subsumedPorts doesn't overlap with portSet, or subsumed ports not
          // in masterLogicalPorts and platform supports add and remove ports
          if (std::none_of(
                  subsumedPorts->begin(),
                  subsumedPorts->end(),
                  [&](auto subsumedPort) {
                    return std::find(
                               ports.begin(),
                               ports.end(),
                               PortID(subsumedPort)) != ports.end() &&
                        (!supportsAddRemovePort ||
                         !masterLogicalPortIds.has_value() ||
                         std::find(
                             masterLogicalPortIds->begin(),
                             masterLogicalPortIds->end(),
                             PortID(subsumedPort)) !=
                             masterLogicalPortIds->end());
                  })) {
            safeProfiles.insert(profile.first);
          }
        } else {
          // no subsumed ports for this profile, safe
          safeProfiles.insert(profile.first);
        }
      }
    }
    if (safeProfiles.empty()) {
      std::string portSetStr = "";
      for (auto portID : ports) {
        portSetStr = folly::to<std::string>(portSetStr, portID, ", ");
      }
      throw FbossError("Can't find safe profiles for ports:", portSetStr);
    }

    auto asicType = asic->getAsicType();
    bool isJericho2 = asicType == cfg::AsicType::ASIC_TYPE_JERICHO2;

    auto bestSpeed = cfg::PortSpeed::DEFAULT;
    if (isJericho2) {
      // For J2c we always want to choose the following
      // speeds since that's what we have in hw chip config
      // and J2c does not support dynamic port speed change yet.
      auto portId = group.first;
      auto platPortItr = platformMapping->getPlatformPorts().find(portId);
      if (platPortItr == platformMapping->getPlatformPorts().end()) {
        throw FbossError("Can't find platform port for:", portId);
      }
      switch (*platPortItr->second.mapping()->portType()) {
        case cfg::PortType::INTERFACE_PORT:
        case cfg::PortType::MANAGEMENT_PORT:
          bestSpeed = getDefaultInterfaceSpeed(asicType);
          break;
        case cfg::PortType::FABRIC_PORT:
          bestSpeed = getDefaultFabricSpeed(asicType);
          break;
        case cfg::PortType::RECYCLE_PORT:
          bestSpeed = cfg::PortSpeed::XG;
          break;
        case cfg::PortType::CPU_PORT:
          break;
      }
    }

    auto bestProfile = cfg::PortProfileID::PROFILE_DEFAULT;
    // If bestSpeed is default - pick the largest speed from the safe profiles
    auto pickMaxSpeed = bestSpeed == cfg::PortSpeed::DEFAULT;
    for (auto profileID : safeProfiles) {
      auto speed = getSpeed(profileID);
      if (pickMaxSpeed) {
        if (static_cast<int>(bestSpeed) < static_cast<int>(speed)) {
          bestSpeed = speed;
          bestProfile = profileID;
        }
      } else if (speed == bestSpeed) {
        bestProfile = profileID;
      }
    }

    for (auto portID : ports) {
      if (supportsAddRemovePort && masterLogicalPortIds.has_value() &&
          std::find(
              masterLogicalPortIds->begin(),
              masterLogicalPortIds->end(),
              portID) == masterLogicalPortIds->end()) {
        continue;
      }
      portToProfileIDs.emplace(portID, bestProfile);
    }
  }
  return portToProfileIDs;
}

std::vector<cfg::Port>::iterator findCfgPort(
    cfg::SwitchConfig& cfg,
    PortID portID) {
  auto port = findCfgPortIf(cfg, portID);
  if (port == cfg.ports()->end()) {
    throw FbossError("No cfg found for port ", portID);
  }
  return port;
}

std::vector<cfg::Port>::iterator findCfgPortIf(
    cfg::SwitchConfig& cfg,
    PortID portID) {
  return std::find_if(
      cfg.ports()->begin(), cfg.ports()->end(), [&portID](auto& port) {
        return PortID(*port.logicalID()) == portID;
      });
}

cfg::Port createDefaultPortConfig(
    const PlatformMapping* platformMapping,
    const HwAsic* asic,
    PortID id,
    cfg::PortProfileID defaultProfileID) {
  cfg::Port defaultConfig;
  const auto& entry = platformMapping->getPlatformPort(id);
  defaultConfig.name() = *entry.mapping()->name();
  defaultConfig.speed() = getSpeed(defaultProfileID);
  defaultConfig.profileID() = defaultProfileID;

  defaultConfig.logicalID() = id;
  defaultConfig.ingressVlan() = kDefaultVlanId;
  defaultConfig.state() = cfg::PortState::DISABLED;
  defaultConfig.portType() = *entry.mapping()->portType();
  if (asic->getSwitchType() == cfg::SwitchType::VOQ ||
      asic->getSwitchType() == cfg::SwitchType::FABRIC) {
    defaultConfig.ingressVlan() = 0;
  } else {
    defaultConfig.ingressVlan() = kDefaultVlanId;
  }
  return defaultConfig;
}

void securePortsInConfig(
    const PlatformMapping* platformMapping,
    const HwAsic* asic,
    cfg::SwitchConfig& config,
    const std::vector<PortID>& ports,
    bool supportsAddRemovePort) {
  // This function is to secure all ports in the input `ports` vector will be
  // in the config. Usually there're two main cases:
  // 1) all the ports in ports vector are from different group, so we don't need
  // to worry about how to deal w/ the slave ports.
  // 2) some ports in the ports vector are in the same group. In this case, we
  // need to make sure these ports will be in the config, and what's the most
  // important, we also need to make sure these ports using a safe PortProfileID
  std::map<PortID, std::vector<PortID>> groupPortsByControllingPort;
  const auto& plarformEntries = platformMapping->getPlatformPorts();
  for (const auto& portID : ports) {
    if (const auto& entry = plarformEntries.find(portID);
        entry != plarformEntries.end()) {
      auto controllingPort =
          PortID(*entry->second.mapping()->controllingPort());
      groupPortsByControllingPort[controllingPort].push_back(portID);
    } else {
      throw FbossError("Port:", portID, " doesn't exist in PlatformMapping");
    }
  }

  // If the mandatory port from input `ports` is the only port from the same
  // port group, and it already exists in the config, we don't need to adjust
  // the profileID and speed for it.
  std::map<PortID, std::vector<PortID>> portGroups;
  for (auto group : groupPortsByControllingPort) {
    auto portSet = group.second;
    if (portSet.size() == 1 &&
        findCfgPortIf(config, *portSet.begin()) != config.ports()->end()) {
      continue;
    }
    portGroups.emplace(group.first, std::move(group.second));
  }

  // Make sure all the ports in portGroups use the safe profile in the config
  if (portGroups.size() > 0) {
    for (const auto& [portID, profileID] : getSafeProfileIDs(
             platformMapping, asic, portGroups, supportsAddRemovePort, ports)) {
      auto portCfg = findCfgPortIf(config, portID);
      if (portCfg != config.ports()->end()) {
        portCfg->profileID() = profileID;
        portCfg->speed() = getSpeed(profileID);
      } else {
        config.ports()->push_back(
            createDefaultPortConfig(platformMapping, asic, portID, profileID));
      }
    }
  }
}
cfg::DsfNode dsfNodeConfig(
    const HwAsic& myAsic,
    int64_t otherSwitchId,
    std::optional<int> systemPortMin) {
  auto createAsic = [&](const HwAsic& fromAsic, int64_t switchId)
      -> std::pair<std::shared_ptr<HwAsic>, PlatformType> {
    std::optional<cfg::Range64> systemPortRange;
    auto fromAsicSystemPortRange = fromAsic.getSystemPortRange();
    if (fromAsicSystemPortRange.has_value()) {
      cfg::Range64 range;
      auto blockSize = *fromAsicSystemPortRange->maximum() -
          *fromAsicSystemPortRange->minimum();
      range.minimum() = systemPortMin.has_value()
          ? kSysPortOffset + systemPortMin.value()
          : kSysPortOffset + switchId * blockSize;
      range.maximum() = *range.minimum() + blockSize;
      systemPortRange = range;
    }
    auto localMac = utility::kLocalCpuMac();
    switch (fromAsic.getAsicType()) {
      case cfg::AsicType::ASIC_TYPE_JERICHO2:
        return std::pair(
            std::make_unique<Jericho2Asic>(
                fromAsic.getSwitchType(),
                switchId,
                fromAsic.getSwitchIndex(),
                systemPortRange,
                localMac),
            PlatformType::PLATFORM_MERU400BIU);
      case cfg::AsicType::ASIC_TYPE_JERICHO3:
        return std::pair(
            std::make_unique<Jericho3Asic>(
                fromAsic.getSwitchType(),
                switchId,
                fromAsic.getSwitchIndex(),
                systemPortRange,
                localMac),
            PlatformType::PLATFORM_MERU800BIA);
      case cfg::AsicType::ASIC_TYPE_RAMON:
        return std::pair(
            std::make_unique<RamonAsic>(
                fromAsic.getSwitchType(),
                switchId,
                fromAsic.getSwitchIndex(),
                std::nullopt,
                localMac),
            PlatformType::PLATFORM_MERU400BFU);
      case cfg::AsicType::ASIC_TYPE_RAMON3:
        return std::pair(
            std::make_unique<Ramon3Asic>(
                fromAsic.getSwitchType(),
                switchId,
                fromAsic.getSwitchIndex(),
                std::nullopt,
                localMac),
            PlatformType::PLATFORM_MERU800BFA);
      default:
        throw FbossError("Unexpected asic type: ", fromAsic.getAsicTypeStr());
    }
  };
  auto [otherAsic, otherPlatformType] = createAsic(myAsic, otherSwitchId);
  cfg::DsfNode dsfNode;
  dsfNode.switchId() = *otherAsic->getSwitchId();
  dsfNode.name() = folly::sformat("hwTestSwitch{}", *dsfNode.switchId());
  switch (otherAsic->getSwitchType()) {
    case cfg::SwitchType::VOQ:
      dsfNode.type() = cfg::DsfNodeType::INTERFACE_NODE;
      CHECK(otherAsic->getSystemPortRange().has_value());
      dsfNode.systemPortRange() = *otherAsic->getSystemPortRange();
      dsfNode.nodeMac() = kLocalCpuMac().toString();
      dsfNode.loopbackIps() = getLoopbackIps(SwitchID(*dsfNode.switchId()));
      break;
    case cfg::SwitchType::FABRIC:
      dsfNode.type() = cfg::DsfNodeType::FABRIC_NODE;
      break;
    case cfg::SwitchType::NPU:
    case cfg::SwitchType::PHY:
      throw FbossError("Unexpected switch type: ", otherAsic->getSwitchType());
  }
  dsfNode.asicType() = otherAsic->getAsicType();
  dsfNode.platformType() = otherPlatformType;
  return dsfNode;
}

cfg::SwitchConfig onePortPerInterfaceConfig(
    const SwSwitch* swSwitch,
    const std::vector<PortID>& ports,
    bool interfaceHasSubnet,
    bool setInterfaceMac,
    int baseIntfId,
    bool enableFabricPorts) {
  // Before m-mpu agent test, use first Asic for initialization.
  auto switchIds = swSwitch->getHwAsicTable()->getSwitchIDs();
  CHECK_GE(switchIds.size(), 1);
  auto asic = swSwitch->getHwAsicTable()->getHwAsic(*switchIds.cbegin());
  return onePortPerInterfaceConfig(
      swSwitch->getPlatformMapping(),
      asic,
      ports,
      swSwitch->getPlatformSupportsAddRemovePort(),
      asic->desiredLoopbackModes(),
      interfaceHasSubnet,
      setInterfaceMac,
      baseIntfId,
      enableFabricPorts);
}

cfg::SwitchConfig onePortPerInterfaceConfig(
    const PlatformMapping* platformMapping,
    const HwAsic* asic,
    const std::vector<PortID>& ports,
    bool supportsAddRemovePort,
    const std::map<cfg::PortType, cfg::PortLoopbackMode>& lbModeMap,
    bool interfaceHasSubnet,
    bool setInterfaceMac,
    int baseIntfId,
    bool enableFabricPorts) {
  return multiplePortsPerIntfConfig(
      platformMapping,
      asic,
      ports,
      supportsAddRemovePort,
      lbModeMap,
      interfaceHasSubnet,
      setInterfaceMac,
      baseIntfId,
      1, /* portPerIntf*/
      enableFabricPorts);
}

cfg::SwitchConfig multiplePortsPerIntfConfig(
    const PlatformMapping* platformMapping,
    const HwAsic* asic,
    const std::vector<PortID>& ports,
    bool supportsAddRemovePort,
    const std::map<cfg::PortType, cfg::PortLoopbackMode>& lbModeMap,
    bool interfaceHasSubnet,
    bool setInterfaceMac,
    const int baseVlanId,
    const int portsPerIntf,
    bool enableFabricPorts) {
  std::map<PortID, VlanID> port2vlan;
  std::vector<VlanID> vlans;
  std::vector<PortID> vlanPorts;
  auto idx = 0;
  auto vlan = baseVlanId;
  auto switchType = asic->getSwitchType();
  for (auto port : ports) {
    vlanPorts.push_back(port);
    auto portType =
        *platformMapping->getPlatformPort(port).mapping()->portType();
    if (portType == cfg::PortType::FABRIC_PORT) {
      continue;
    }
    // For non NPU switch type vendor SAI impls don't support
    // tagging packet at port ingress.
    if (switchType == cfg::SwitchType::NPU) {
      port2vlan[port] = VlanID(vlan);
      idx++;
      // If current vlan has portsPerIntf port,
      // then add a new vlan
      if (idx % portsPerIntf == 0) {
        vlans.push_back(VlanID(vlan));
        vlan++;
      }
    } else {
      port2vlan[port] = VlanID(0);
    }
  }
  auto config = genPortVlanCfg(
      platformMapping,
      asic,
      vlanPorts,
      port2vlan,
      vlans,
      lbModeMap,
      supportsAddRemovePort,
      true /*optimizePortProfile*/,
      enableFabricPorts);
  auto addInterface = [&config](
                          int32_t intfId,
                          int32_t vlanId,
                          cfg::InterfaceType type,
                          bool setMac,
                          bool hasSubnet,
                          std::optional<std::vector<std::string>> subnets) {
    auto i = config.interfaces()->size();
    config.interfaces()->push_back(cfg::Interface{});
    *config.interfaces()[i].intfID() = intfId;
    *config.interfaces()[i].vlanID() = vlanId;
    *config.interfaces()[i].routerID() = 0;
    *config.interfaces()[i].type() = type;
    if (setMac) {
      config.interfaces()[i].mac() = getLocalCpuMacStr();
    }
    config.interfaces()[i].mtu() = 9000;
    if (hasSubnet) {
      if (subnets) {
        config.interfaces()[i].ipAddresses() = *subnets;
      } else {
        config.interfaces()[i].ipAddresses()->resize(2);
        auto ipDecimal = folly::sformat("{}", i + 1);
        config.interfaces()[i].ipAddresses()[0] = FLAGS_nodeZ
            ? folly::sformat("{}.0.0.2/24", ipDecimal)
            : folly::sformat("{}.0.0.1/24", ipDecimal);
        config.interfaces()[i].ipAddresses()[1] = FLAGS_nodeZ
            ? folly::sformat("{}::1/64", ipDecimal)
            : folly::sformat("{}::0/64", ipDecimal);
      }
    }
  };
  for (auto i = 0; i < vlans.size(); ++i) {
    addInterface(
        vlans[i],
        vlans[i],
        cfg::InterfaceType::VLAN,
        setInterfaceMac,
        interfaceHasSubnet,
        std::nullopt);
  }
  // Create interfaces for local sys ports on VOQ switches
  if (switchType == cfg::SwitchType::VOQ) {
    CHECK_EQ(portsPerIntf, 1) << " For VOQ switches sys port to interface "
                                 "mapping must by 1:1";
    const std::set<cfg::PortType> kCreateIntfsFor = {
        cfg::PortType::INTERFACE_PORT, cfg::PortType::RECYCLE_PORT};
    for (const auto& port : *config.ports()) {
      if (kCreateIntfsFor.find(*port.portType()) == kCreateIntfsFor.end()) {
        continue;
      }
      CHECK(config.switchSettings()->switchIdToSwitchInfo()->size());
      auto mySwitchId =
          config.switchSettings()->switchIdToSwitchInfo()->begin()->first;
      CHECK(config.dsfNodes()[mySwitchId].systemPortRange().has_value());
      auto sysportRangeBegin =
          *config.dsfNodes()[mySwitchId].systemPortRange()->minimum();
      auto intfId = sysportRangeBegin + *port.logicalID();
      std::optional<std::vector<std::string>> subnets;
      if (*port.portType() == cfg::PortType::RECYCLE_PORT) {
        subnets = getLoopbackIps(SwitchID(mySwitchId));
      }
      addInterface(
          intfId,
          0,
          cfg::InterfaceType::SYSTEM_PORT,
          setInterfaceMac,
          interfaceHasSubnet,
          subnets);
    }
  }
  return config;
}

cfg::SwitchConfig genPortVlanCfg(
    const PlatformMapping* platformMapping,
    const HwAsic* asic,
    const std::vector<PortID>& ports,
    const std::map<PortID, VlanID>& port2vlan,
    const std::vector<VlanID>& vlans,
    const std::map<cfg::PortType, cfg::PortLoopbackMode> lbModeMap,
    bool supportsAddRemovePort,
    bool optimizePortProfile,
    bool enableFabricPorts) {
  cfg::SwitchConfig config;
  auto switchType = asic->getSwitchType();
  auto asicType = asic->getAsicType();
  int64_t switchId{0};
  if (asic->getSwitchId().has_value()) {
    switchId = *asic->getSwitchId();
    if (switchType == cfg::SwitchType::VOQ ||
        switchType == cfg::SwitchType::FABRIC) {
      config.dsfNodes()->insert(
          {*asic->getSwitchId(), dsfNodeConfig(*asic, *asic->getSwitchId())});
    }
  }
  cfg::SwitchInfo switchInfo;
  cfg::Range64 portIdRange;
  portIdRange.minimum() =
      cfg::switch_config_constants::DEFAULT_PORT_ID_RANGE_MIN();
  portIdRange.maximum() =
      cfg::switch_config_constants::DEFAULT_PORT_ID_RANGE_MAX();
  switchInfo.portIdRange() = portIdRange;
  switchInfo.switchIndex() = 0;
  switchInfo.switchType() = switchType;
  switchInfo.asicType() = asicType;
  if (asicType == cfg::AsicType::ASIC_TYPE_RAMON) {
    switchInfo.connectionHandle() = "0c:00";
  } else if (asicType == cfg::AsicType::ASIC_TYPE_EBRO) {
    switchInfo.connectionHandle() = "/dev/uio0";
  }
  if (asic->getSystemPortRange().has_value()) {
    switchInfo.systemPortRange() = *asic->getSystemPortRange();
  }
  config.switchSettings()->switchIdToSwitchInfo() = {
      std::make_pair(switchId, switchInfo)};
  // Use getPortToDefaultProfileIDMap() to genetate the default config instead
  // of using PlatformMapping.
  // The main reason is to avoid using PlatformMapping is because some of the
  // platforms support adding and removing ports, and their ChipConfig might
  // only has the controlling ports, which means when the chip is initialized
  // the number of ports from the hardware might be different from the number
  // of total platform ports from PlatformMapping.
  // And if we try to use all ports from PlatformMapping to create default
  // config, it will have to trigger an unncessary PortGroup re-program to add
  // those new ports on the hardware.
  const auto& portToDefaultProfileID = getPortToDefaultProfileIDMap();
  CHECK_GT(portToDefaultProfileID.size(), 0);
  const auto& platformPorts = platformMapping->getPlatformPorts();
  for (auto const& [portID, profileID] : portToDefaultProfileID) {
    if (!FLAGS_hide_fabric_ports ||
        *platformPorts.find(static_cast<int32_t>(portID))
                ->second.mapping()
                ->portType() != cfg::PortType::FABRIC_PORT) {
      config.ports()->push_back(
          createDefaultPortConfig(platformMapping, asic, portID, profileID));
    }
  }
  auto const kFabricTxQueueConfig = "FabricTxQueueConfig";
  config.portQueueConfigs()[kFabricTxQueueConfig] = getFabTxQueueConfig();

  // Secure all ports in `ports` vector in the config
  securePortsInConfig(
      platformMapping, asic, config, ports, supportsAddRemovePort);

  // Port config
  auto kPortMTU = 9412;
  for (auto portID : ports) {
    auto portCfg = findCfgPort(config, portID);
    auto iter = lbModeMap.find(portCfg->get_portType());
    if (iter == lbModeMap.end()) {
      throw FbossError(
          "Unable to find the desired loopback mode for port type: ",
          portCfg->get_portType());
    }
    portCfg->loopbackMode() = iter->second;
    if (portCfg->portType() == cfg::PortType::FABRIC_PORT) {
      portCfg->ingressVlan() = 0;
      portCfg->maxFrameSize() =
          asic->isSupported(HwAsic::Feature::FABRIC_PORT_MTU) ? kPortMTU : 0;
      portCfg->state() = enableFabricPorts ? cfg::PortState::ENABLED
                                           : cfg::PortState::DISABLED;

      if (asic->isSupported(HwAsic::Feature::FABRIC_TX_QUEUES)) {
        portCfg->portQueueConfigName() = kFabricTxQueueConfig;
      }
    } else {
      portCfg->state() = cfg::PortState::ENABLED;
      portCfg->ingressVlan() = port2vlan.find(portID)->second;
      portCfg->maxFrameSize() = kPortMTU;
    }
    portCfg->routable() = true;
    portCfg->parserType() = cfg::ParserType::L3;
  }

  if (vlans.size()) {
    // Vlan config
    for (auto vlanID : vlans) {
      cfg::Vlan vlan;
      vlan.id() = vlanID;
      vlan.name() = "vlan" + std::to_string(vlanID);
      vlan.routable() = true;
      config.vlans()->push_back(vlan);
    }

    cfg::Vlan defaultVlan;
    defaultVlan.id() = kDefaultVlanId;
    defaultVlan.name() = folly::sformat("vlan{}", kDefaultVlanId);
    defaultVlan.routable() = true;
    config.vlans()->push_back(defaultVlan);
    config.defaultVlan() = kDefaultVlanId;

    // Vlan port config
    for (auto vlanPortPair : port2vlan) {
      cfg::VlanPort vlanPort;
      vlanPort.logicalPort() = vlanPortPair.first;
      vlanPort.vlanID() = vlanPortPair.second;
      vlanPort.spanningTreeState() = cfg::SpanningTreeState::FORWARDING;
      vlanPort.emitTags() = false;
      config.vlanPorts()->push_back(vlanPort);
    }
  }
  return config;
}

cfg::SwitchConfig oneL3IntfTwoPortConfig(
    const PlatformMapping* platformMapping,
    const HwAsic* asic,
    PortID port1,
    PortID port2,
    bool supportsAddRemovePort,
    const std::map<cfg::PortType, cfg::PortLoopbackMode>& lbModeMap) {
  std::vector<PortID> ports{port1, port2};
  return oneL3IntfNPortConfig(
      platformMapping, asic, ports, supportsAddRemovePort, lbModeMap);
}

cfg::SwitchConfig oneL3IntfNPortConfig(
    const PlatformMapping* platformMapping,
    const HwAsic* asic,
    const std::vector<PortID>& ports,
    bool supportsAddRemovePort,
    const std::map<cfg::PortType, cfg::PortLoopbackMode>& lbModeMap,
    bool interfaceHasSubnet,
    int baseVlanId,
    bool optimizePortProfile,
    bool setInterfaceMac) {
  std::map<PortID, VlanID> port2vlan;
  std::vector<VlanID> vlans{VlanID(baseVlanId)};
  std::vector<PortID> vlanPorts;

  for (auto port : ports) {
    port2vlan[port] = VlanID(baseVlanId);
    vlanPorts.push_back(port);
  }
  auto config = genPortVlanCfg(
      platformMapping,
      asic,
      vlanPorts,
      port2vlan,
      vlans,
      lbModeMap,
      supportsAddRemovePort,
      optimizePortProfile);

  config.interfaces()->resize(1);
  config.interfaces()[0].intfID() = baseVlanId;
  config.interfaces()[0].vlanID() = baseVlanId;
  *config.interfaces()[0].routerID() = 0;
  if (setInterfaceMac) {
    config.interfaces()[0].mac() = getLocalCpuMacStr();
  }
  config.interfaces()[0].mtu() = 9000;
  if (interfaceHasSubnet) {
    config.interfaces()[0].ipAddresses()->resize(2);
    config.interfaces()[0].ipAddresses()[0] =
        FLAGS_nodeZ ? "1.1.1.2/24" : "1.1.1.1/24";
    config.interfaces()[0].ipAddresses()[1] =
        FLAGS_nodeZ ? "1::1/64" : "1::/64";
  }
  return config;
}

} // namespace facebook::fboss::utility
