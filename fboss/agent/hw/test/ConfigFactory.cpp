/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/test/ConfigFactory.h"

#include "fboss/agent/FbossError.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/switch_asics/EbroAsic.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/hw/switch_asics/Jericho2Asic.h"
#include "fboss/agent/hw/switch_asics/Jericho3Asic.h"
#include "fboss/agent/hw/switch_asics/Ramon3Asic.h"
#include "fboss/agent/hw/switch_asics/RamonAsic.h"
#include "fboss/agent/hw/test/HwPortUtils.h"
#include "fboss/agent/hw/test/HwSwitchEnsemble.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/PortMap.h"
#include "fboss/lib/config/PlatformConfigUtils.h"
#include "fboss/lib/platforms/PlatformMode.h"

#include <folly/Format.h>
#include <folly/logging/xlog.h>
#include <thrift/lib/cpp/util/EnumUtils.h>

using namespace facebook::fboss;
using namespace facebook::fboss::utility;

DEFINE_bool(nodeZ, false, "Setup test config as node Z");

namespace {

std::string getLocalCpuMacStr() {
  return kLocalCpuMac().toString();
}

void removePort(
    cfg::SwitchConfig& config,
    PortID port,
    bool supportsAddRemovePort) {
  auto cfgPort = findCfgPortIf(config, port);
  if (cfgPort == config.ports()->end()) {
    return;
  }
  if (supportsAddRemovePort) {
    config.ports()->erase(cfgPort);
    auto removed = std::remove_if(
        config.vlanPorts()->begin(),
        config.vlanPorts()->end(),
        [port](auto vlanPort) {
          return PortID(*vlanPort.logicalPort()) == port;
        });
    config.vlanPorts()->erase(removed, config.vlanPorts()->end());
  } else {
    cfgPort->state() = cfg::PortState::DISABLED;
  }
}

void removeSubsumedPorts(
    cfg::SwitchConfig& config,
    const cfg::PlatformPortConfig& profile,
    bool supportsAddRemovePort) {
  if (auto subsumedPorts = profile.subsumedPorts()) {
    for (auto& subsumedPortID : subsumedPorts.value()) {
      removePort(config, PortID(subsumedPortID), supportsAddRemovePort);
    }
  }
}

bool isRswPlatform(PlatformType type) {
  std::set rswPlatforms = {
      PlatformType::PLATFORM_WEDGE,
      PlatformType::PLATFORM_WEDGE100,
      PlatformType::PLATFORM_WEDGE400,
      PlatformType::PLATFORM_WEDGE400C};
  return rswPlatforms.find(type) != rswPlatforms.end();
}

} // unnamed namespace

namespace facebook::fboss::utility {

std::pair<int, int> getRetryCountAndDelay(const HwAsic* asic) {
  if (asic->isSupported(HwAsic::Feature::SLOW_STAT_UPDATE)) {
    return std::make_pair(50, 5000);
  }
  // default
  return std::make_pair(30, 1000);
}

std::unordered_map<PortID, cfg::PortProfileID>& getPortToDefaultProfileIDMap() {
  static std::unordered_map<PortID, cfg::PortProfileID> portProfileIDMap;
  return portProfileIDMap;
}

cfg::DsfNode dsfNodeConfig(const HwAsic& myAsic, int64_t otherSwitchId) {
  auto createAsic =
      [](const HwAsic& fromAsic,
         int64_t switchId) -> std::pair<std::shared_ptr<HwAsic>, PlatformType> {
    std::optional<cfg::Range64> systemPortRange;
    auto fromAsicSystemPortRange = fromAsic.getSystemPortRange();
    if (fromAsicSystemPortRange.has_value()) {
      cfg::Range64 range;
      auto blockSize = *fromAsicSystemPortRange->maximum() -
          *fromAsicSystemPortRange->minimum();
      range.minimum() = 100 + switchId * blockSize;
      range.maximum() = *range.minimum() + blockSize;
      systemPortRange = range;
    }
    auto localMac = utility::kLocalCpuMac();
    switch (fromAsic.getAsicType()) {
      case cfg::AsicType::ASIC_TYPE_JERICHO2:
        return std::pair(
            std::make_unique<Jericho2Asic>(
                fromAsic.getSwitchType(), switchId, systemPortRange, localMac),
            PlatformType::PLATFORM_MERU400BIU);
      case cfg::AsicType::ASIC_TYPE_JERICHO3:
        return std::pair(
            std::make_unique<Jericho3Asic>(
                fromAsic.getSwitchType(), switchId, systemPortRange, localMac),
            PlatformType::PLATFORM_MERU800BIA);
      case cfg::AsicType::ASIC_TYPE_RAMON:
        return std::pair(
            std::make_unique<RamonAsic>(
                fromAsic.getSwitchType(), switchId, std::nullopt, localMac),
            PlatformType::PLATFORM_MERU400BFU);
      case cfg::AsicType::ASIC_TYPE_RAMON3:
        return std::pair(
            std::make_unique<Ramon3Asic>(
                fromAsic.getSwitchType(), switchId, std::nullopt, localMac),
            PlatformType::PLATFORM_MERU800BFA);
      case cfg::AsicType::ASIC_TYPE_EBRO:
        PlatformType platformType;
        if (fromAsic.getSwitchType() == cfg::SwitchType::FABRIC) {
          platformType = PlatformType::PLATFORM_WEDGE400C_FABRIC;
        } else {
          platformType = PlatformType::PLATFORM_WEDGE400C_VOQ;
        }
        return std::pair(
            std::make_unique<EbroAsic>(
                fromAsic.getSwitchType(), switchId, systemPortRange, localMac),
            platformType);
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
      dsfNode.nodeMac() = "02:00:00:00:0F:0B";
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

namespace {

cfg::Port createDefaultPortConfig(
    const Platform* platform,
    PortID id,
    cfg::PortProfileID defaultProfileID) {
  cfg::Port defaultConfig;
  const auto& entry = platform->getPlatformPort(id)->getPlatformPortEntry();
  defaultConfig.name() = *entry.mapping()->name();
  defaultConfig.speed() = getSpeed(defaultProfileID);
  defaultConfig.profileID() = defaultProfileID;

  defaultConfig.logicalID() = id;
  defaultConfig.ingressVlan() = kDefaultVlanId;
  defaultConfig.state() = cfg::PortState::DISABLED;
  defaultConfig.portType() = *entry.mapping()->portType();
  auto switchType = platform->getAsic()->getSwitchType();
  if (switchType == cfg::SwitchType::VOQ ||
      switchType == cfg::SwitchType::FABRIC) {
    defaultConfig.ingressVlan() = 0;
  } else {
    defaultConfig.ingressVlan() = kDefaultVlanId;
  }
  return defaultConfig;
}

std::unordered_map<PortID, cfg::PortProfileID> getSafeProfileIDs(
    const Platform* platform,
    const std::map<PortID, std::vector<PortID>>&
        controllingPortToSubsidaryPorts) {
  std::unordered_map<PortID, cfg::PortProfileID> portToProfileIDs;
  const auto& plarformEntries = platform->getPlatformPorts();
  for (const auto& group : controllingPortToSubsidaryPorts) {
    const auto& ports = group.second;
    // Find the safe profile to satisfy all the ports in the group
    std::set<cfg::PortProfileID> safeProfiles;
    for (const auto& portID : ports) {
      for (const auto& profile :
           *plarformEntries.at(portID).supportedProfiles()) {
        if (auto subsumedPorts = profile.second.subsumedPorts();
            subsumedPorts && !subsumedPorts->empty()) {
          // as long as subsumedPorts doesn't overlap with portSet, also safe
          if (std::none_of(
                  subsumedPorts->begin(),
                  subsumedPorts->end(),
                  [ports](auto subsumedPort) {
                    return std::find(
                               ports.begin(),
                               ports.end(),
                               PortID(subsumedPort)) != ports.end();
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

    bool isJericho2 =
        platform->getAsic()->getAsicType() == cfg::AsicType::ASIC_TYPE_JERICHO2;
    bool isJericho3 =
        platform->getAsic()->getAsicType() == cfg::AsicType::ASIC_TYPE_JERICHO3;

    auto bestSpeed = cfg::PortSpeed::DEFAULT;
    if (isJericho2 || isJericho3) {
      // For J2c we always want to choose the following
      // speeds since that's what we have in hw chip config
      // and J2c does not support dynamic port speed change yet.
      auto portId = group.first;
      auto platPortItr = platform->getPlatformPorts().find(portId);
      if (platPortItr == platform->getPlatformPorts().end()) {
        throw FbossError("Can't find platform port for:", portId);
      }
      switch (*platPortItr->second.mapping()->portType()) {
        case cfg::PortType::INTERFACE_PORT:
          bestSpeed =
              getDefaultInterfaceSpeed(platform->getAsic()->getAsicType());
          break;
        case cfg::PortType::FABRIC_PORT:
          bestSpeed = getDefaultFabricSpeed(platform->getAsic()->getAsicType());
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
      portToProfileIDs.emplace(portID, bestProfile);
    }
  }
  return portToProfileIDs;
}

void securePortsInConfig(
    const Platform* platform,
    cfg::SwitchConfig& config,
    const std::vector<PortID>& ports) {
  // This function is to secure all ports in the input `ports` vector will be
  // in the config. Usually there're two main cases:
  // 1) all the ports in ports vector are from different group, so we don't need
  // to worry about how to deal w/ the slave ports.
  // 2) some ports in the ports vector are in the same group. In this case, we
  // need to make sure these ports will be in the config, and what's the most
  // important, we also need to make sure these ports using a safe PortProfileID
  std::map<PortID, std::vector<PortID>> groupPortsByControllingPort;
  const auto& plarformEntries = platform->getPlatformPorts();
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
    for (const auto& [portID, profileID] :
         getSafeProfileIDs(platform, portGroups)) {
      auto portCfg = findCfgPortIf(config, portID);
      if (portCfg != config.ports()->end()) {
        portCfg->profileID() = profileID;
        portCfg->speed() = getSpeed(profileID);
      } else {
        config.ports()->push_back(
            createDefaultPortConfig(platform, portID, profileID));
      }
    }
  }
}

std::vector<cfg::PortQueue> getFabTxQueueConfig() {
  cfg::PortQueue fabQueue;
  fabQueue.id() = 0;
  fabQueue.name() = "fabric_q0";
  fabQueue.streamType() = cfg::StreamType::FABRIC_TX;
  fabQueue.scheduling() = cfg::QueueScheduling::INTERNAL;
  return {fabQueue};
}

cfg::SwitchConfig genPortVlanCfg(
    const HwSwitch* hwSwitch,
    const std::vector<PortID>& ports,
    const std::map<PortID, VlanID>& port2vlan,
    const std::vector<VlanID>& vlans,
    const std::map<cfg::PortType, cfg::PortLoopbackMode> lbModeMap,
    bool optimizePortProfile = true,
    bool enableFabricPorts = false) {
  cfg::SwitchConfig config;
  auto asic = hwSwitch->getPlatform()->getAsic();
  auto switchType = asic->getSwitchType();
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
  switchInfo.asicType() = asic->getAsicType();
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
  for (auto const& [portID, profileID] : portToDefaultProfileID) {
    config.ports()->push_back(
        createDefaultPortConfig(hwSwitch->getPlatform(), portID, profileID));
  }
  auto const kFabricTxQueueConfig = "FabricTxQueueConfig";
  config.portQueueConfigs()[kFabricTxQueueConfig] = getFabTxQueueConfig();

  // Secure all ports in `ports` vector in the config
  securePortsInConfig(hwSwitch->getPlatform(), config, ports);

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
      portCfg->maxFrameSize() = hwSwitch->getPlatform()->getAsic()->isSupported(
                                    HwAsic::Feature::FABRIC_PORT_MTU)
          ? kPortMTU
          : 0;
      portCfg->state() = enableFabricPorts ? cfg::PortState::ENABLED
                                           : cfg::PortState::DISABLED;

      if (hwSwitch->getPlatform()->getAsic()->isSupported(
              HwAsic::Feature::FABRIC_TX_QUEUES)) {
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
} // namespace

void setPortToDefaultProfileIDMap(
    const std::shared_ptr<MultiSwitchPortMap>& ports,
    const Platform* platform) {
  // Most of the platforms will have default ports created when the HW is
  // initialized. But for those who don't have any default port, we'll fall
  // back to use PlatformPort and the safe PortProfileID
  if (ports->numNodes() > 0) {
    for (const auto& portMap : std::as_const(*ports)) {
      for (const auto& port : std::as_const(*portMap.second)) {
        auto profileID = port.second->getProfileID();
        // In case the profileID learnt from HW is using default, then use speed
        // to get the real profileID
        if (profileID == cfg::PortProfileID::PROFILE_DEFAULT) {
          auto platformPort = platform->getPlatformPort(port.second->getID());
          profileID =
              platformPort->getProfileIDBySpeed(port.second->getSpeed());
        }
        getPortToDefaultProfileIDMap().emplace(port.second->getID(), profileID);
      }
    }
  } else {
    const auto& safeProfileIDs = getSafeProfileIDs(
        platform, getSubsidiaryPortIDs(platform->getPlatformPorts()));
    getPortToDefaultProfileIDMap().insert(
        safeProfileIDs.begin(), safeProfileIDs.end());
  }
  XLOG(DBG2) << "PortToDefaultProfileIDMap has "
             << getPortToDefaultProfileIDMap().size() << " ports";
}
folly::MacAddress kLocalCpuMac() {
  static const folly::MacAddress kLocalMac(
      FLAGS_nodeZ ? "02:00:00:00:00:02" : "02:00:00:00:00:01");
  return kLocalMac;
}

const std::map<cfg::PortType, cfg::PortLoopbackMode>& kDefaultLoopbackMap() {
  static const std::map<cfg::PortType, cfg::PortLoopbackMode> kLoopbackMap = {
      {cfg::PortType::INTERFACE_PORT, cfg::PortLoopbackMode::NONE},
      {cfg::PortType::FABRIC_PORT, cfg::PortLoopbackMode::NONE},
      {cfg::PortType::RECYCLE_PORT, cfg::PortLoopbackMode::NONE}};
  return kLoopbackMap;
}

std::vector<std::string> getLoopbackIps(SwitchID switchId) {
  auto switchIdVal = static_cast<int64_t>(switchId);
  CHECK_LT(switchIdVal, 10) << " Switch Id >= 10, not supported";

  auto v6 = FLAGS_nodeZ ? folly::sformat("20{}::2/64", switchIdVal)
                        : folly::sformat("20{}::1/64", switchIdVal);
  auto v4 = FLAGS_nodeZ ? folly::sformat("20{}.0.0.2/24", switchIdVal)
                        : folly::sformat("20{}.0.0.1/24", switchIdVal);
  return {v6, v4};
}

cfg::SwitchConfig oneL3IntfConfig(
    const HwSwitch* hwSwitch,
    PortID port,
    const std::map<cfg::PortType, cfg::PortLoopbackMode>& lbModeMap,
    int baseVlanId) {
  std::vector<PortID> ports{port};
  return oneL3IntfNPortConfig(hwSwitch, ports, lbModeMap, true, baseVlanId);
}

cfg::SwitchConfig oneL3IntfNoIPAddrConfig(
    const HwSwitch* hwSwitch,
    PortID port,
    const std::map<cfg::PortType, cfg::PortLoopbackMode>& lbModeMap) {
  std::vector<PortID> ports{port};
  return oneL3IntfNPortConfig(
      hwSwitch, ports, lbModeMap, false /*interfaceHasSubnet*/);
}

cfg::SwitchConfig oneL3IntfTwoPortConfig(
    const HwSwitch* hwSwitch,
    PortID port1,
    PortID port2,
    const std::map<cfg::PortType, cfg::PortLoopbackMode>& lbModeMap) {
  std::vector<PortID> ports{port1, port2};
  return oneL3IntfNPortConfig(hwSwitch, ports, lbModeMap);
}

cfg::SwitchConfig oneL3IntfNPortConfig(
    const HwSwitch* hwSwitch,
    const std::vector<PortID>& ports,
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
      hwSwitch, vlanPorts, port2vlan, vlans, lbModeMap, optimizePortProfile);

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

cfg::SwitchConfig onePortPerInterfaceConfig(
    const HwSwitch* hwSwitch,
    const std::vector<PortID>& ports,
    const std::map<cfg::PortType, cfg::PortLoopbackMode>& lbModeMap,
    bool interfaceHasSubnet,
    bool setInterfaceMac,
    int baseIntfId,
    bool enableFabricPorts) {
  return multiplePortsPerIntfConfig(
      hwSwitch,
      ports,
      lbModeMap,
      interfaceHasSubnet,
      setInterfaceMac,
      baseIntfId,
      1, /* portPerIntf*/
      enableFabricPorts);
}

cfg::SwitchConfig multiplePortsPerIntfConfig(
    const HwSwitch* hwSwitch,
    const std::vector<PortID>& ports,
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
  auto switchType = hwSwitch->getPlatform()->getAsic()->getSwitchType();
  for (auto port : ports) {
    vlanPorts.push_back(port);
    auto portType =
        hwSwitch->getPlatform()->getPlatformPort(port)->getPortType();
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
      hwSwitch,
      vlanPorts,
      port2vlan,
      vlans,
      lbModeMap,
      true /*optimizePortProfile*/,
      enableFabricPorts);
  auto addInterface = [&config, baseVlanId](
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
  if (hwSwitch->getPlatform()->getAsic()->getSwitchType() ==
      cfg::SwitchType::VOQ) {
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

cfg::SwitchConfig twoL3IntfConfig(
    const HwSwitch* hwSwitch,
    PortID port1,
    PortID port2,
    const std::map<cfg::PortType, cfg::PortLoopbackMode>& lbModeMap) {
  std::map<PortID, VlanID> port2vlan;
  std::vector<PortID> ports{port1, port2};
  std::vector<VlanID> vlans;
  auto vlan = kBaseVlanId;
  auto switchType = hwSwitch->getPlatform()->getAsic()->getSwitchType();
  CHECK(
      switchType == cfg::SwitchType::NPU || switchType == cfg::SwitchType::VOQ)
      << "twoL3IntfConfig is only supported for VOQ or NPU switch types";
  PortID kRecyclePort(1);
  if (switchType == cfg::SwitchType::VOQ && port1 != kRecyclePort &&
      port2 != kRecyclePort) {
    ports.push_back(kRecyclePort);
  }
  for (auto port : ports) {
    auto portType =
        hwSwitch->getPlatform()->getPlatformPort(port)->getPortType();
    CHECK(portType != cfg::PortType::FABRIC_PORT);
    // For non NPU switch type vendor SAI impls don't support
    // tagging packet at port ingress.
    if (switchType == cfg::SwitchType::NPU) {
      port2vlan[port] = VlanID(kBaseVlanId);
      vlans.push_back(VlanID(vlan++));
    } else {
      port2vlan[port] = VlanID(0);
    }
  }
  auto config = genPortVlanCfg(hwSwitch, ports, port2vlan, vlans, lbModeMap);

  auto computeIntfId = [&config, &ports, &switchType, &vlans](auto idx) {
    if (switchType == cfg::SwitchType::NPU) {
      return static_cast<int64_t>(vlans[idx]);
    }
    auto mySwitchId =
        apache::thrift::can_throw(*config.switchSettings()->switchId());
    auto sysportRangeBegin =
        *config.dsfNodes()[mySwitchId].systemPortRange()->minimum();
    return sysportRangeBegin + static_cast<int>(ports[idx]);
  };
  for (auto i = 0; i < ports.size(); ++i) {
    cfg::Interface intf;
    *intf.intfID() = computeIntfId(i);
    *intf.vlanID() = switchType == cfg::SwitchType::NPU ? vlans[i] : 0;
    *intf.routerID() = 0;
    intf.ipAddresses()->resize(2);
    auto ipOctet = i + 1;
    intf.ipAddresses()[0] =
        folly::sformat("{}.{}.{}.{}/24", ipOctet, ipOctet, ipOctet, ipOctet);
    intf.ipAddresses()[1] = folly::sformat("{}::{}/64", ipOctet, ipOctet);
    intf.mac() = getLocalCpuMacStr();
    intf.mtu() = 9000;
    intf.routerID() = 0;
    intf.type() = switchType == cfg::SwitchType::NPU
        ? cfg::InterfaceType::VLAN
        : cfg::InterfaceType::SYSTEM_PORT;
    config.interfaces()->push_back(intf);
  }
  return config;
}

void addMatcher(
    cfg::SwitchConfig* config,
    const std::string& matcherName,
    const cfg::MatchAction& matchAction) {
  cfg::MatchToAction action = cfg::MatchToAction();
  *action.matcher() = matcherName;
  *action.action() = matchAction;
  cfg::TrafficPolicyConfig egressTrafficPolicy;
  if (auto dataPlaneTrafficPolicy = config->dataPlaneTrafficPolicy()) {
    egressTrafficPolicy = *dataPlaneTrafficPolicy;
  }
  auto curNumMatchActions = egressTrafficPolicy.matchToAction()->size();
  egressTrafficPolicy.matchToAction()->resize(curNumMatchActions + 1);
  egressTrafficPolicy.matchToAction()[curNumMatchActions] = action;
  config->dataPlaneTrafficPolicy() = egressTrafficPolicy;
}

void delMatcher(cfg::SwitchConfig* config, const std::string& matcherName) {
  if (auto dataPlaneTrafficPolicy = config->dataPlaneTrafficPolicy()) {
    auto& matchActions = *dataPlaneTrafficPolicy->matchToAction();
    matchActions.erase(
        std::remove_if(
            matchActions.begin(),
            matchActions.end(),
            [&](cfg::MatchToAction const& matchAction) {
              if (*matchAction.matcher() == matcherName) {
                return true;
              }
              return false;
            }),
        matchActions.end());
  }
}

void updatePortSpeed(
    const HwSwitch& hwSwitch,
    cfg::SwitchConfig& cfg,
    PortID portID,
    cfg::PortSpeed speed) {
  auto cfgPort = findCfgPort(cfg, portID);
  auto platform = hwSwitch.getPlatform();
  auto supportsAddRemovePort = platform->supportsAddRemovePort();
  auto platformPort = platform->getPlatformPort(portID);
  const auto& platPortEntry = platformPort->getPlatformPortEntry();
  auto profileID = platformPort->getProfileIDBySpeed(speed);
  const auto& supportedProfiles = *platPortEntry.supportedProfiles();
  auto profile = supportedProfiles.find(profileID);
  if (profile == supportedProfiles.end()) {
    throw FbossError("No profile ", profileID, " found for port ", portID);
  }
  cfgPort->profileID() = profileID;
  cfgPort->speed() = speed;
  removeSubsumedPorts(cfg, profile->second, supportsAddRemovePort);
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

// Set any ports in this port group to use the specified speed,
// and disables any ports that don't support this speed.
void configurePortGroup(
    const HwSwitch& hwSwitch,
    cfg::SwitchConfig& config,
    cfg::PortSpeed speed,
    std::vector<PortID> allPortsInGroup) {
  auto platform = hwSwitch.getPlatform();
  auto supportsAddRemovePort = platform->supportsAddRemovePort();
  for (auto portID : allPortsInGroup) {
    // We might have removed a subsumed port already in a previous
    // iteration of the loop.
    auto cfgPort = findCfgPortIf(config, portID);
    if (cfgPort == config.ports()->end()) {
      continue;
    }

    auto platformPort = platform->getPlatformPort(portID);
    const auto& platPortEntry = platformPort->getPlatformPortEntry();
    auto profileID = platformPort->getProfileIDBySpeedIf(speed);
    if (!profileID.has_value()) {
      XLOG(WARNING) << "Port " << static_cast<int>(portID)
                    << "Doesn't support speed " << static_cast<int>(speed)
                    << ", disabling it instead";
      // Port doesn't support this speed, just disable it.
      cfgPort->state() = cfg::PortState::DISABLED;
      continue;
    }

    auto supportedProfiles = *platPortEntry.supportedProfiles();
    auto profile = supportedProfiles.find(profileID.value());
    if (profile == supportedProfiles.end()) {
      throw std::runtime_error(folly::to<std::string>(
          "No profile ", profileID.value(), " found for port ", portID));
    }

    cfgPort->profileID() = profileID.value();
    cfgPort->speed() = speed;
    cfgPort->state() = cfg::PortState::ENABLED;
    removeSubsumedPorts(config, profile->second, supportsAddRemovePort);
  }
}

void configurePortProfile(
    const HwSwitch& hwSwitch,
    cfg::SwitchConfig& config,
    cfg::PortProfileID profileID,
    std::vector<PortID> allPortsInGroup,
    PortID controllingPortID) {
  auto platform = hwSwitch.getPlatform();
  auto supportsAddRemovePort = platform->supportsAddRemovePort();
  auto controllingPort = findCfgPort(config, controllingPortID);
  for (auto portID : allPortsInGroup) {
    // We might have removed a subsumed port already in a previous
    // iteration of the loop.
    auto cfgPort = findCfgPortIf(config, portID);
    if (cfgPort == config.ports()->end()) {
      return;
    }

    auto platformPort = platform->getPlatformPort(portID);
    const auto& platPortEntry = platformPort->getPlatformPortEntry();
    auto supportedProfiles = *platPortEntry.supportedProfiles();
    auto profile = supportedProfiles.find(profileID);
    if (profile == supportedProfiles.end()) {
      XLOG(WARNING) << "Port " << static_cast<int>(portID)
                    << " doesn't support profile "
                    << apache::thrift::util::enumNameSafe(profileID)
                    << ", disabling it instead";
      // Port doesn't support this speed, just disable it.
      cfgPort->state() = cfg::PortState::DISABLED;
      continue;
    }
    cfgPort->profileID() = profileID;
    cfgPort->speed() = getSpeed(profileID);
    cfgPort->ingressVlan() = *controllingPort->ingressVlan();
    cfgPort->state() = cfg::PortState::ENABLED;
    removeSubsumedPorts(config, profile->second, supportsAddRemovePort);
  }
}

std::vector<PortID> getAllPortsInGroup(
    const HwSwitch* hwSwitch,
    PortID portID) {
  std::vector<PortID> allPortsinGroup;
  if (const auto& platformPorts = hwSwitch->getPlatform()->getPlatformPorts();
      !platformPorts.empty()) {
    const auto& portList =
        utility::getPlatformPortsByControllingPort(platformPorts, portID);
    for (const auto& port : portList) {
      allPortsinGroup.push_back(PortID(*port.mapping()->id()));
    }
  }
  return allPortsinGroup;
}

cfg::SwitchConfig createRtswUplinkDownlinkConfig(
    const HwSwitch* hwSwitch,
    HwSwitchEnsemble* ensemble,
    const std::vector<PortID>& masterLogicalPortIds,
    cfg::PortSpeed portSpeed,
    const std::map<cfg::PortType, cfg::PortLoopbackMode>& lbModeMap,
    std::vector<PortID>& uplinks,
    std::vector<PortID>& downlinks) {
  const int kTotalLinkCount = 32;
  std::vector<PortID> disabledLinks;
  ensemble->createEqualDistributedUplinkDownlinks(
      masterLogicalPortIds, uplinks, downlinks, disabledLinks, kTotalLinkCount);

  // make all logicalPorts have their own vlan id
  auto cfg = utility::onePortPerInterfaceConfig(
      hwSwitch, masterLogicalPortIds, lbModeMap, true, kUplinkBaseVlanId);
  for (auto portId : uplinks) {
    utility::updatePortSpeed(*hwSwitch, cfg, portId, portSpeed);
  }
  for (auto portId : downlinks) {
    utility::updatePortSpeed(*hwSwitch, cfg, portId, portSpeed);
  }

  // disable all ports other than uplinks/downlinks
  // this is needed to consume the guranteed buffer space same as
  // we do for RTSW in production
  for (auto& port : disabledLinks) {
    auto portCfg = utility::findCfgPort(cfg, port);
    portCfg->state() = cfg::PortState::DISABLED;
  }

  return cfg;
}

cfg::SwitchConfig createUplinkDownlinkConfig(
    const HwSwitch* hwSwitch,
    const std::vector<PortID>& masterLogicalPortIds,
    uint16_t uplinksCount,
    cfg::PortSpeed uplinkPortSpeed,
    cfg::PortSpeed downlinkPortSpeed,
    const std::map<cfg::PortType, cfg::PortLoopbackMode>& lbModeMap,
    bool interfaceHasSubnet) {
  auto platform = hwSwitch->getPlatform();
  /*
   * For platforms which are not rsw, its always onePortPerInterfaceConfig
   * config with all uplinks and downlinks in same speed. Use the
   * config factory utility to generate the config, update the port
   * speed and return the config.
   */
  if (!isRswPlatform(platform->getType())) {
    auto config = utility::onePortPerInterfaceConfig(
        hwSwitch,
        masterLogicalPortIds,
        lbModeMap,
        interfaceHasSubnet,
        true,
        kUplinkBaseVlanId);
    for (auto portId : masterLogicalPortIds) {
      utility::updatePortSpeed(*hwSwitch, config, portId, uplinkPortSpeed);
    }
    return config;
  }

  /*
   * Configure the top ports in the master logical port ids as uplinks
   * and remaining as downlinks based on the platform
   */
  std::vector<PortID> uplinkMasterPorts(
      masterLogicalPortIds.begin(),
      masterLogicalPortIds.begin() + uplinksCount);
  std::vector<PortID> downlinkMasterPorts(
      masterLogicalPortIds.begin() + uplinksCount, masterLogicalPortIds.end());
  /*
   * Prod uplinks are always onePortPerInterfaceConfig. Use the existing
   * utlity to generate one port per vlan config followed by port
   * speed update.
   */
  auto config = utility::onePortPerInterfaceConfig(
      hwSwitch,
      uplinkMasterPorts,
      lbModeMap,
      interfaceHasSubnet,
      true /*setInterfaceMac*/,
      kUplinkBaseVlanId);
  for (auto portId : uplinkMasterPorts) {
    utility::updatePortSpeed(*hwSwitch, config, portId, uplinkPortSpeed);
  }

  /*
   * downlinkMasterPorts are master logical port ids. Get all the ports in
   * a port group and add them to the config. Use configurePortGroup
   * to set the right speed and remove/disable the subsumed ports
   * based on the platform.
   */
  std::vector<PortID> allDownlinkPorts;
  for (auto masterDownlinkPort : downlinkMasterPorts) {
    auto allDownlinkPortsInGroup =
        utility::getAllPortsInGroup(hwSwitch, masterDownlinkPort);
    for (auto logicalPortId : allDownlinkPortsInGroup) {
      auto portConfig = findCfgPortIf(config, masterDownlinkPort);
      if (portConfig != config.ports()->end()) {
        allDownlinkPorts.push_back(logicalPortId);
      }
    }
    configurePortGroup(
        *hwSwitch, config, downlinkPortSpeed, allDownlinkPortsInGroup);
  }

  // Vlan config
  cfg::Vlan vlan;
  vlan.id() = kDownlinkBaseVlanId;
  vlan.name() = "vlan" + std::to_string(kDownlinkBaseVlanId);
  vlan.routable() = true;
  config.vlans()->push_back(vlan);

  // Vlan port config
  for (auto logicalPortId : allDownlinkPorts) {
    auto portConfig = utility::findCfgPortIf(config, logicalPortId);
    if (portConfig == config.ports()->end()) {
      continue;
    }
    auto iter = lbModeMap.find(portConfig->get_portType());
    if (iter == lbModeMap.end()) {
      throw FbossError(
          "Unable to find the desired loopback mode for port type: ",
          portConfig->get_portType());
    }
    portConfig->loopbackMode() = iter->second;
    portConfig->ingressVlan() = kDownlinkBaseVlanId;
    portConfig->routable() = true;
    portConfig->parserType() = cfg::ParserType::L3;
    portConfig->maxFrameSize() = 9412;
    cfg::VlanPort vlanPort;
    vlanPort.logicalPort() = logicalPortId;
    vlanPort.vlanID() = kDownlinkBaseVlanId;
    vlanPort.spanningTreeState() = cfg::SpanningTreeState::FORWARDING;
    vlanPort.emitTags() = false;
    config.vlanPorts()->push_back(vlanPort);
  }

  cfg::Interface interface;
  interface.intfID() = kDownlinkBaseVlanId;
  interface.vlanID() = kDownlinkBaseVlanId;
  interface.routerID() = 0;
  interface.mac() = utility::kLocalCpuMac().toString();
  interface.mtu() = 9000;
  if (interfaceHasSubnet) {
    interface.ipAddresses()->resize(2);
    interface.ipAddresses()[0] = "192.1.1.1/24";
    interface.ipAddresses()[1] = "2192::1/64";
  }
  config.interfaces()->push_back(interface);

  return config;
}

/*
 * Returns a pair of vectors of the form (uplinks, downlinks). Pertains
 * specifically to default RSW platforms, where all downlink ports are
 * configured to be on ingressVlan 2000.
 *
 * Created to verify load balancing based on the switch's config, but can be
 * used anywhere it's useful. Likely will also be used in verifying
 * queue-per-host QoS for MHNIC platforms.
 */
UplinkDownlinkPair getRswUplinkDownlinkPorts(
    const cfg::SwitchConfig& config,
    const int ecmpWidth) {
  std::vector<PortID> uplinks, downlinks;

  auto confPorts = *config.ports();
  XLOG_IF(WARN, confPorts.empty()) << "no ports found in config.ports_ref()";

  for (const auto& port : confPorts) {
    auto logId = port.get_logicalID();
    if (port.get_state() != cfg::PortState::ENABLED) {
      continue;
    }

    auto portId = PortID(logId);
    auto vlanId = port.get_ingressVlan();
    if (vlanId == kDownlinkBaseVlanId) {
      downlinks.push_back(portId);
    } else if (uplinks.size() < ecmpWidth) {
      uplinks.push_back(portId);
    }
  }

  XLOG(DBG2) << "Uplinks : " << uplinks.size()
             << " downlinks: " << downlinks.size();
  return std::pair(uplinks, downlinks);
}

UplinkDownlinkPair getRtswUplinkDownlinkPorts(
    const cfg::SwitchConfig& config,
    const int ecmpWidth) {
  std::vector<PortID> uplinks, downlinks;

  auto confPorts = *config.ports();
  XLOG_IF(WARN, confPorts.empty()) << "no ports found in config.ports_ref()";

  for (const auto& port : confPorts) {
    auto logId = port.get_logicalID();
    if (port.get_state() != cfg::PortState::ENABLED) {
      continue;
    }

    // for RTSW we can select uplinks/downlinks based on multiple conditions
    // we could use the vlanId as we used for Rsw, but that requires
    // substantial changes to the setup for RTSW. Avoiding that
    // used
    auto portId = PortID(logId);
    if (port.pfc().has_value()) {
      auto pfc = port.pfc().value();
      auto pgName = pfc.portPgConfigName().value();
      if (pgName.find("downlinks") != std::string::npos) {
        downlinks.push_back(portId);
      } else if (
          (pgName.find("uplinks") != std::string::npos) &&
          uplinks.size() < ecmpWidth) {
        uplinks.push_back(portId);
      }
    }
  }

  XLOG(DBG2) << "Uplinks : " << uplinks.size()
             << " downlinks: " << downlinks.size();
  return std::pair(uplinks, downlinks);
}

std::vector<PortDescriptor> getUplinksForEcmp(
    const HwSwitch* hwSwitch,
    const cfg::SwitchConfig& config,
    const int uplinkCount,
    const bool mmu_lossless_mode) {
  auto uplinks = getAllUplinkDownlinkPorts(
                     hwSwitch, config, uplinkCount, mmu_lossless_mode)
                     .first;
  std::vector<PortDescriptor> ecmpPorts;
  for (auto it = uplinks.begin(); it != uplinks.end(); it++) {
    ecmpPorts.push_back(PortDescriptor(*it));
  }
  return ecmpPorts;
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

/*
 * Generic function to separate uplinks and downlinks, given a HwSwitch* and a
 * SwitchConfig.
 *
 * Calls already-defined RSW helper functions if HwSwitch* is an RSW platform,
 * otherwise separates the config's ports.
 */
UplinkDownlinkPair getAllUplinkDownlinkPorts(
    const HwSwitch* hwSwitch,
    const cfg::SwitchConfig& config,
    const int ecmpWidth,
    const bool mmu_lossless) {
  auto platMode = hwSwitch->getPlatform()->getType();
  if (mmu_lossless) {
    return getRtswUplinkDownlinkPorts(config, ecmpWidth);
  } else if (isRswPlatform(platMode)) {
    return getRswUplinkDownlinkPorts(config, ecmpWidth);
  }

  // If the platform is not an RSW, consider the first ecmpWidth-many ports to
  // be uplinks and the rest to be downlinks.
  // First populate masterPorts with all ports, analogous to
  // masterLogicalPortIds, then slice uplinks/downlinks according to ecmpWidth.

  // just for brevity, mostly in return statement
  using PortList = std::vector<PortID>;
  PortList masterPorts;

  for (const auto& port : *config.ports()) {
    if (isEnabledPortWithSubnet(port, config)) {
      masterPorts.push_back(PortID(port.get_logicalID()));
    }
  }

  auto begin = masterPorts.begin();
  auto mid = masterPorts.begin() + ecmpWidth;
  auto end = masterPorts.end();
  return std::pair(PortList(begin, mid), PortList(mid, end));
}

/*
 * Takes a SwitchConfig and returns a map of queue IDs to DSCPs.
 * Particularly useful in verifyQueueMappings, where we don't have a guarantee
 * of what the QoS policies look like and we can't rely on something like
 * kOlympicQueueToDscp().
 */
std::map<int, std::vector<uint8_t>> getOlympicQosMaps(
    const cfg::SwitchConfig& config) {
  std::map<int, std::vector<uint8_t>> queueToDscp;

  for (const auto& qosPolicy : *config.qosPolicies()) {
    auto qosName = qosPolicy.get_name();
    XLOG(DBG2) << "Iterating over QoS policies: found qosPolicy " << qosName;

    // Optional thrift field access
    if (auto qosMap = qosPolicy.qosMap()) {
      auto dscpMaps = *qosMap->dscpMaps();

      for (const auto& dscpMap : dscpMaps) {
        auto queueId = dscpMap.get_internalTrafficClass();
        // Internally (i.e. in thrift), the mapping is implemented as a
        // map<int16_t, vector<int8_t>>; however, in functions like
        // verifyQueueMapping in HwTestQosUtils, the argument used is of the
        // form map<int, uint8_t>.
        // Trying to assign vector<uint8_t> to a vector<int8_t> makes the STL
        // unhappy, so we can just loop through and construct one on our own.
        std::vector<uint8_t> dscps;
        for (auto val : *dscpMap.fromDscpToTrafficClass()) {
          dscps.push_back((uint8_t)val);
        }
        queueToDscp[(int)queueId] = std::move(dscps);
      }
    } else {
      XLOG(ERR) << "qosMap not found in qosPolicy: " << qosName;
    }
  }

  return queueToDscp;
}

} // namespace facebook::fboss::utility
