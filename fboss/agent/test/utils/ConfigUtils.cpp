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
#include "fboss/agent/AsicUtils.h"
#include "fboss/agent/FbossError.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/test/TestEnsembleIf.h"
#include "fboss/agent/test/utils/AclTestUtils.h"
#include "fboss/agent/test/utils/PortTestUtils.h"
#include "fboss/agent/test/utils/VoqTestUtils.h"
#include "fboss/lib/config/PlatformConfigUtils.h"

#include <folly/Format.h>
#include "folly/testing/TestUtil.h"

DEFINE_bool(nodeZ, false, "Setup test config as node Z");
DECLARE_string(mode);

namespace facebook::fboss::utility {

namespace {
void removePort(
    facebook::fboss::cfg::SwitchConfig& config,
    facebook::fboss::PortID port,
    bool supportsAddRemovePort) {
  auto cfgPort = facebook::fboss::utility::findCfgPortIf(config, port);
  if (cfgPort == config.ports()->end()) {
    return;
  }
  if (supportsAddRemovePort) {
    config.ports()->erase(cfgPort);
    auto removed = std::remove_if(
        config.vlanPorts()->begin(),
        config.vlanPorts()->end(),
        [port](auto vlanPort) {
          return facebook::fboss::PortID(*vlanPort.logicalPort()) == port;
        });
    config.vlanPorts()->erase(removed, config.vlanPorts()->end());
  } else {
    cfgPort->state() = facebook::fboss::cfg::PortState::DISABLED;
  }
}

int getRdswSysPortBlockSize(
    std::optional<PlatformType> platformType = std::nullopt) {
  // For dual stage 3/2q mode, sys ports are allocated in 2 blocks of 28 while
  // for single state we allocate a single block of 44
  // For PLATFORM_JANGA800BIC, use Prod range
  if (platformType.has_value() &&
      platformType.value() == PlatformType::PLATFORM_JANGA800BIC) {
    return 22;
  }
  return isDualStage3Q2QMode() ? 28 : 44;
}
int getEdswSysPortBlockSize() {
  // For dual stage 3/2q mode, sys ports are allocated in 2 blocks of 14 while
  // for single state we allocate a single block of 26
  return isDualStage3Q2QMode() ? 14 : 26;
}

int getPerNodeSysPortsBlockSize(
    const HwAsic& asic,
    int remoteSwitchId,
    std::optional<PlatformType> platformType = std::nullopt) {
  if (remoteSwitchId < getMaxRdsw(platformType) * asic.getNumCores()) {
    return getRdswSysPortBlockSize(platformType);
  }
  return getEdswSysPortBlockSize();
}

int getSysPortIdsAllocated(
    const HwAsic& asic,
    int remoteSwitchId,
    int64_t firstSwitchIdMin,
    std::optional<PlatformType> platformType = std::nullopt) {
  auto portsConsumed = firstSwitchIdMin;
  auto deviceIndex = remoteSwitchId / asic.getNumCores();
  CHECK(asic.getAsicType() == cfg::AsicType::ASIC_TYPE_JERICHO3);
  if (deviceIndex < getMaxRdsw(platformType)) {
    portsConsumed += deviceIndex * getRdswSysPortBlockSize(platformType) - 1;
  } else {
    portsConsumed +=
        getMaxRdsw(platformType) * getRdswSysPortBlockSize(platformType) +
        (deviceIndex - getMaxRdsw(platformType)) * getEdswSysPortBlockSize() -
        1;
  }
  return portsConsumed;
}

cfg::InterfaceType getInterfaceType(const HwAsic& asic) {
  if (asic.getAsicType() == cfg::AsicType::ASIC_TYPE_CHENAB) {
    return cfg::InterfaceType::PORT;
  }
  return cfg::InterfaceType::VLAN;
}
} // namespace
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
      {cfg::PortType::RECYCLE_PORT, cfg::PortLoopbackMode::NONE},
      {cfg::PortType::HYPER_PORT, cfg::PortLoopbackMode::NONE},
      {cfg::PortType::HYPER_PORT_MEMBER, cfg::PortLoopbackMode::NONE},
      {cfg::PortType::EVENTOR_PORT, cfg::PortLoopbackMode::NONE}};
  return kLoopbackMap;
}

bool isEnabledPortWithSubnet(
    const cfg::Port& port,
    const cfg::SwitchConfig& config) {
  auto ingressVlan = folly::copy(port.ingressVlan().value());
  for (const auto& intf : *config.interfaces()) {
    if (cfg::InterfaceType::VLAN == intf.type()) {
      if (folly::copy(intf.vlanID().value()) == ingressVlan) {
        return (
            !intf.ipAddresses().value().empty() &&
            folly::copy(port.state().value()) == cfg::PortState::ENABLED);
      }
    } else if (cfg::InterfaceType::PORT == intf.type()) {
      if (folly::copy(intf.portID().to_optional().value()) ==
          port.logicalID().value()) {
        return (
            !intf.ipAddresses().value().empty() &&
            folly::copy(port.state().value()) == cfg::PortState::ENABLED);
      }
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
      ? folly::sformat("{}:{}::2/128", firstOctet, secondOctet)
      : folly::sformat("{}:{}::1/128", firstOctet, secondOctet);
  auto v4 = FLAGS_nodeZ
      ? folly::sformat("{}.{}.0.2/32", firstOctet, secondOctet)
      : folly::sformat("{}.{}.0.1/32", firstOctet, secondOctet);
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
      std::string portSetStr;
      for (auto portID : ports) {
        portSetStr = folly::to<std::string>(portSetStr, portID, ", ");
      }
      throw FbossError("Can't find safe profiles for ports:", portSetStr);
    }

    auto asicType = asic->getAsicType();

    auto bestSpeed = cfg::PortSpeed::DEFAULT;
    auto bestProfile = cfg::PortProfileID::PROFILE_DEFAULT;
    if (asicType == cfg::AsicType::ASIC_TYPE_JERICHO3 &&
        FLAGS_dual_stage_rdsw_3q_2q) {
      // When using dual_stage_rdsw_3q_2q mapping. Pick NIF port
      // speed to be 400G, since that's what we have in chip config
      // and J3 does not support dynamic port speed change yet.
      auto portId = group.first;
      auto platPortItr = platformMapping->getPlatformPorts().find(portId);
      if (platPortItr == platformMapping->getPlatformPorts().end()) {
        throw FbossError("Can't find platform port for:", portId);
      }
      switch (*platPortItr->second.mapping()->portType()) {
        case cfg::PortType::INTERFACE_PORT:
          bestSpeed = cfg::PortSpeed::FOURHUNDREDG;
          break;
        case cfg::PortType::FABRIC_PORT:
        case cfg::PortType::MANAGEMENT_PORT:
        case cfg::PortType::RECYCLE_PORT:
        case cfg::PortType::EVENTOR_PORT:
        case cfg::PortType::CPU_PORT:
        case cfg::PortType::HYPER_PORT:
        case cfg::PortType::HYPER_PORT_MEMBER:
          break;
      }
    } else if (asicType == cfg::AsicType::ASIC_TYPE_CHENAB) {
      if (FLAGS_mode == "yangra") {
        // For yangra pick both profile and speed to be 400G, since that's what
        // is expected in production and chenab does not support dynamic port
        // profile change, as it may lead to recreation of ports by delete and
        // add. the usecase of recreating ports by delete and add is not
        // supported in chenab. Minipack3n has max port speed of 400G only
        bestSpeed = cfg::PortSpeed::FOURHUNDREDG;
        bestProfile = cfg::PortProfileID::PROFILE_400G_4_PAM4_RS544X2N_OPTICAL;
      }
    }
    // If bestSpeed is default - pick the largest speed from the safe profiles
    auto pickMaxSpeed = bestSpeed == cfg::PortSpeed::DEFAULT;
    auto pickBestProfile = bestProfile == cfg::PortProfileID::PROFILE_DEFAULT;
    if (pickBestProfile) {
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
    } else {
      if (getSpeed(bestProfile) != bestSpeed) {
        throw FbossError(
            "Invalid profile:", bestProfile, " for speed ", bestSpeed);
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
  defaultConfig.ingressVlan() = kDefaultVlanId4094;
  defaultConfig.state() = cfg::PortState::DISABLED;
  defaultConfig.portType() = *entry.mapping()->portType();
  if (asic->getSwitchType() == cfg::SwitchType::VOQ ||
      asic->getSwitchType() == cfg::SwitchType::FABRIC) {
    defaultConfig.ingressVlan() = 0;
  } else {
    defaultConfig.ingressVlan() = kDefaultVlanId4094;
  }
  defaultConfig.scope() = *entry.mapping()->scope();
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

int getMaxRdsw(std::optional<PlatformType> platformType) {
  if (isDualStage3Q2QMode()) {
    return 512;
  } else if (
      platformType.has_value() &&
      platformType.value() == PlatformType::PLATFORM_JANGA800BIC) {
    return 256; // Janga gets 256 max RDSWs for single-stage
  } else {
    return 128; // Others get 128 for single-stage (default)
  }
}

int getMaxEdsw() {
  return isDualStage3Q2QMode() ? 128 : 16;
}

int getLocalSystemPortOffset(const cfg::SystemPortRanges& ranges) {
  // For 2q/3q mode, local sys port offset is -32K
  // FIXME 2-stage-MTIA, offsets may be higher for 2nd ASIC
  return isDualStage3Q2QMode() ? -32 * 1024
                               : *ranges.systemPortRanges()->begin()->minimum();
}

cfg::DsfNode dsfNodeConfig(
    const HwAsic& firstAsic,
    int64_t otherSwitchId,
    const std::optional<PlatformType> platformType,
    const std::optional<int> clusterId,
    const std::string& switchNamePrefix) {
  auto getPlatformType = [](const auto& asic, auto& platformType) {
    if (platformType.has_value()) {
      return *platformType;
    }
    switch (asic.getAsicType()) {
      case cfg::AsicType::ASIC_TYPE_JERICHO2:
        return PlatformType::PLATFORM_MERU400BIU;
      case cfg::AsicType::ASIC_TYPE_JERICHO3:
        return PlatformType::PLATFORM_MERU800BIA;
      case cfg::AsicType::ASIC_TYPE_RAMON:
        return PlatformType::PLATFORM_MERU400BFU;
      case cfg::AsicType::ASIC_TYPE_RAMON3:
        return PlatformType::PLATFORM_MERU800BFA;
      default:
        break;
    }
    throw FbossError("Unexpected asic type: ", asic.getAsicTypeStr());
  };
  auto getSystemPortRanges = [](const HwAsic& fromAsic,
                                int64_t otherSwitchId,
                                std::optional<PlatformType> platformType =
                                    std::nullopt) {
    cfg::SystemPortRanges sysPortRanges;
    CHECK(fromAsic.getSystemPortRanges().systemPortRanges()->size());
    CHECK(fromAsic.getSwitchId().has_value());
    CHECK_EQ(*fromAsic.getSwitchId(), 0) << " For VOQ switches input asic "
                                         << " must be of the first VOQ switch";
    auto firstDsfNodeSysPortRanges =
        *fromAsic.getSystemPortRanges().systemPortRanges();
    for (const auto& firstNodeRange : firstDsfNodeSysPortRanges) {
      cfg::Range64 systemPortRange;
      // Already allocated + 1
      systemPortRange.minimum() = getSysPortIdsAllocated(
                                      fromAsic,
                                      otherSwitchId,
                                      *firstNodeRange.minimum(),
                                      platformType) +
          1;
      systemPortRange.maximum() = *systemPortRange.minimum() +
          getPerNodeSysPortsBlockSize(fromAsic, otherSwitchId, platformType) -
          1;
      XLOG(DBG2) << " For switch Id: " << otherSwitchId
                 << " allocating range, min: " << *systemPortRange.minimum()
                 << " max: " << *systemPortRange.maximum();

      sysPortRanges.systemPortRanges()->push_back(systemPortRange);
    }
    return sysPortRanges;
  };
  cfg::DsfNode dsfNode;
  dsfNode.switchId() = otherSwitchId;
  dsfNode.name() =
      folly::sformat("{}{}", switchNamePrefix, *dsfNode.switchId());
  auto resolvedPlatformType = getPlatformType(firstAsic, platformType);
  switch (firstAsic.getSwitchType()) {
    case cfg::SwitchType::VOQ: {
      dsfNode.type() = cfg::DsfNodeType::INTERFACE_NODE;
      auto sysPortRanges =
          getSystemPortRanges(firstAsic, otherSwitchId, resolvedPlatformType);
      dsfNode.systemPortRanges() = sysPortRanges;
      dsfNode.localSystemPortOffset() = getLocalSystemPortOffset(sysPortRanges);
      // In single-stage sys port ranges, 0th port is reserved for CPU.
      // For dual stage, no implicit reservation for 0th port, hence offset
      // should be 1 less.
      dsfNode.globalSystemPortOffset() = isDualStage3Q2QMode()
          ? *sysPortRanges.systemPortRanges()->begin()->minimum() - 1
          : *sysPortRanges.systemPortRanges()->begin()->minimum();
      CHECK(firstAsic.getInbandPortId().has_value());
      dsfNode.inbandPortId() = *firstAsic.getInbandPortId();
      dsfNode.nodeMac() = kLocalCpuMac().toString();
      dsfNode.loopbackIps() = getLoopbackIps(SwitchID(*dsfNode.switchId()));
    } break;
    case cfg::SwitchType::FABRIC:
      dsfNode.type() = cfg::DsfNodeType::FABRIC_NODE;
      break;
    case cfg::SwitchType::NPU:
    case cfg::SwitchType::PHY:
      throw FbossError("Unexpected switch type: ", firstAsic.getSwitchType());
  }
  dsfNode.asicType() = firstAsic.getAsicType();
  dsfNode.platformType() = getPlatformType(firstAsic, platformType);
  if (clusterId.has_value()) {
    dsfNode.clusterId() = clusterId.value();
  }
  return dsfNode;
}

cfg::SwitchConfig onePortPerInterfaceConfigImpl(
    const SwSwitch* swSwitch,
    const std::vector<PortID>& ports,
    bool interfaceHasSubnet,
    bool setInterfaceMac,
    int baseIntfId,
    bool enableFabricPorts,
    cfg::InterfaceType intfType) {
  std::vector<const HwAsic*> asics;
  auto switchIds = swSwitch->getHwAsicTable()->getSwitchIDs();
  for (const auto& switchId : switchIds) {
    asics.push_back(swSwitch->getHwAsicTable()->getHwAsic(switchId));
  }
  auto asic = checkSameAndGetAsic(asics);
  return onePortPerInterfaceConfigImpl(
      swSwitch->getPlatformMapping(),
      asic,
      ports,
      swSwitch->getPlatformSupportsAddRemovePort(),
      asic->desiredLoopbackModes(),
      interfaceHasSubnet,
      setInterfaceMac,
      baseIntfId,
      enableFabricPorts,
      // Use SwitchInfo from --config if SwSwitch is provided
      swSwitch->getSwitchInfoTable().getSwitchIdToSwitchInfo(),
      swSwitch->getHwAsicTable()->getHwAsics(),
      swSwitch->getPlatformType(),
      intfType);
}

cfg::SwitchConfig onePortPerInterfaceConfigImpl(
    const PlatformMapping* platformMapping,
    const HwAsic* asic,
    const std::vector<PortID>& ports,
    bool supportsAddRemovePort,
    const std::map<cfg::PortType, cfg::PortLoopbackMode>& lbModeMap,
    bool interfaceHasSubnet,
    bool setInterfaceMac,
    int baseIntfId,
    bool enableFabricPorts,
    const std::optional<std::map<SwitchID, cfg::SwitchInfo>>&
        switchIdToSwitchInfo,
    const std::optional<std::map<SwitchID, const HwAsic*>>& hwAsicTable,
    const std::optional<PlatformType> platformType,
    cfg::InterfaceType intfType) {
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
      enableFabricPorts,
      switchIdToSwitchInfo,
      hwAsicTable,
      platformType,
      intfType);
}

cfg::SwitchConfig onePortPerInterfaceConfigImpl(
    const TestEnsembleIf* ensemble,
    const std::vector<PortID>& ports,
    bool interfaceHasSubnet,
    bool setInterfaceMac,
    int baseIntfId,
    bool enableFabricPorts,
    cfg::InterfaceType intfType) {
  auto platformMapping = ensemble->getPlatformMapping();
  auto switchIds = ensemble->getHwAsicTable()->getSwitchIDs();
  CHECK_GE(switchIds.size(), 1);
  auto asic = ensemble->getHwAsicTable()->getHwAsic(*switchIds.cbegin());
  auto supportsAddRemovePort = ensemble->supportsAddRemovePort();

  return onePortPerInterfaceConfigImpl(
      platformMapping,
      asic,
      ports,
      supportsAddRemovePort,
      asic->desiredLoopbackModes(),
      interfaceHasSubnet,
      setInterfaceMac,
      baseIntfId,
      enableFabricPorts,
      std::nullopt, // switchIdToSwitchInfo
      std::nullopt, // hwAsicTable
      std::nullopt, // platformType
      intfType);
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
    bool enableFabricPorts,
    const std::optional<std::map<SwitchID, cfg::SwitchInfo>>&
        switchIdToSwitchInfo,
    const std::optional<std::map<SwitchID, const HwAsic*>>& hwAsicTable,
    const std::optional<PlatformType> platformType,
    cfg::InterfaceType intfType) {
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
    if (switchType == cfg::SwitchType::NPU &&
        intfType == cfg::InterfaceType::VLAN) {
      port2vlan[port] = VlanID(vlan);
      idx++;
      // If current vlan has portsPerIntf port,
      // then add a new vlan
      if (idx % portsPerIntf == 0) {
        vlans.emplace_back(vlan);
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
      enableFabricPorts,
      switchIdToSwitchInfo,
      hwAsicTable,
      platformType);
  auto addInterface = [&config](
                          int32_t intfId,
                          int32_t vlanId,
                          cfg::InterfaceType type,
                          bool setMac,
                          bool hasSubnet,
                          std::optional<std::vector<std::string>> subnets,
                          cfg::Scope scope,
                          std::optional<int32_t> port = std::nullopt) {
    auto i = config.interfaces()->size();
    config.interfaces()->emplace_back();
    config.interfaces()[i].name() = folly::to<std::string>(intfId);
    *config.interfaces()[i].intfID() = intfId;
    *config.interfaces()[i].vlanID() = vlanId;
    *config.interfaces()[i].routerID() = 0;
    *config.interfaces()[i].type() = type;
    *config.interfaces()[i].scope() = scope;
    if (setMac) {
      config.interfaces()[i].mac() = getLocalCpuMacStr();
    }
    if (type == cfg::InterfaceType::PORT) {
      CHECK(port.has_value());
      config.interfaces()[i].portID() = *port;
    }
    config.interfaces()[i].mtu() = 9000;
    if (hasSubnet) {
      if (subnets) {
        config.interfaces()[i].ipAddresses() = *subnets;
      } else {
        auto ipDecimal = i + 1;
        auto v4Mask = 24;
        auto v6Mask = 64;
        bool isV4 = true;
        config.interfaces()[i].ipAddresses()->resize(2);
        config.interfaces()[i].ipAddresses()[0] = FLAGS_nodeZ
            ? genInterfaceAddress(ipDecimal, isV4, 2, v4Mask)
            : genInterfaceAddress(ipDecimal, isV4, 1, v4Mask);
        config.interfaces()[i].ipAddresses()[1] = FLAGS_nodeZ
            ? genInterfaceAddress(ipDecimal, !isV4, 1, v6Mask)
            : genInterfaceAddress(ipDecimal, !isV4, 0, v6Mask);
      }
    }
  };
  if (cfg::InterfaceType::VLAN == intfType) {
    for (auto i = 0; i < vlans.size(); ++i) {
      addInterface(
          vlans[i],
          vlans[i],
          intfType,
          setInterfaceMac,
          interfaceHasSubnet,
          std::nullopt,
          cfg::Scope::LOCAL);
    }
  } else if (cfg::InterfaceType::PORT == intfType) {
    for (auto i = 0; i < ports.size(); ++i) {
      addInterface(
          kBaseVlanId + i,
          0,
          intfType,
          setInterfaceMac,
          interfaceHasSubnet,
          std::nullopt,
          cfg::Scope::LOCAL,
          ports[i]);
    }
  }
  // Create interfaces for local sys ports on VOQ switches
  if (switchType == cfg::SwitchType::VOQ) {
    CHECK(config.switchSettings()->switchIdToSwitchInfo()->size());
    auto scopeResolver =
        SwitchIdScopeResolver(*config.switchSettings()->switchIdToSwitchInfo());
    CHECK_EQ(portsPerIntf, 1) << " For VOQ switches sys port to interface "
                                 "mapping must by 1:1";
    const std::set<cfg::PortType> kCreateIntfsFor = {
        cfg::PortType::INTERFACE_PORT,
        cfg::PortType::RECYCLE_PORT,
        cfg::PortType::MANAGEMENT_PORT,
        cfg::PortType::EVENTOR_PORT,
        cfg::PortType::HYPER_PORT};
    for (const auto& port : *config.ports()) {
      if (kCreateIntfsFor.find(*port.portType()) == kCreateIntfsFor.end()) {
        continue;
      }
      auto mySwitchId = scopeResolver.scope(port).switchId();
      auto sysPortId = getSystemPortID(
          PortID(*port.logicalID()),
          *port.scope(),
          *config.switchSettings()->switchIdToSwitchInfo(),
          SwitchID(mySwitchId));
      auto intfId = static_cast<uint32_t>(sysPortId);
      std::optional<std::vector<std::string>> subnets;
      auto portScope = *platformMapping->getPlatformPort(*port.logicalID())
                            .mapping()
                            ->scope();
      if (*port.portType() == cfg::PortType::RECYCLE_PORT &&
          portScope == cfg::Scope::GLOBAL) {
        // only set IP for global recycle ports
        subnets = getLoopbackIps(SwitchID(mySwitchId));
      }
      addInterface(
          intfId,
          0,
          cfg::InterfaceType::SYSTEM_PORT,
          setInterfaceMac,
          interfaceHasSubnet,
          subnets,
          portScope);
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
    bool enableFabricPorts,
    const std::optional<std::map<SwitchID, cfg::SwitchInfo>>&
        switchIdToSwitchInfo,
    const std::optional<std::map<SwitchID, const HwAsic*>>& hwAsicTable,
    const std::optional<PlatformType> platformType) {
  cfg::SwitchConfig config;
  if (switchIdToSwitchInfo.has_value() && hwAsicTable.has_value()) {
    populateSwitchInfo(
        config,
        switchIdToSwitchInfo.value(),
        hwAsicTable.value(),
        platformType);
  } else {
    std::map<SwitchID, cfg::SwitchInfo> defaultSwitchIdToSwitchInfo;
    std::map<SwitchID, const HwAsic*> defaultHwAsicTable;
    auto switchType = asic->getSwitchType();
    auto asicType = asic->getAsicType();
    int64_t switchId{0};
    if (asic->getSwitchId().has_value()) {
      switchId = *asic->getSwitchId();
    }
    defaultHwAsicTable.insert({SwitchID(switchId), asic});
    cfg::SwitchInfo switchInfo;
    cfg::Range64 portIdRange;
    portIdRange.minimum() =
        cfg::switch_config_constants::DEFAULT_PORT_ID_RANGE_MIN();
    portIdRange.maximum() = cfg::switch_config_constants::
        DEFAULT_DUAL_STAGE_3Q_2Q_PORT_ID_RANGE_MAX();
    switchInfo.portIdRange() = portIdRange;
    switchInfo.switchIndex() = 0;
    switchInfo.switchType() = switchType;
    switchInfo.asicType() = asicType;
    // TODO: Instead of using hard codings for connection handle and
    // src mac, get the configs from AgentConfig
    if (asicType == cfg::AsicType::ASIC_TYPE_RAMON) {
      switchInfo.connectionHandle() = "0c:00";
    } else if (asicType == cfg::AsicType::ASIC_TYPE_RAMON3) {
      switchInfo.connectionHandle() = "15:00";
    } else if (asicType == cfg::AsicType::ASIC_TYPE_JERICHO2) {
      switchInfo.switchMac() = "02:00:00:00:00:01";
      switchInfo.connectionHandle() = "68:00";
    } else if (asicType == cfg::AsicType::ASIC_TYPE_JERICHO3) {
      switchInfo.switchMac() = "02:00:00:00:00:01";
      switchInfo.connectionHandle() = "15:00";
    } else if (
        asicType == cfg::AsicType::ASIC_TYPE_EBRO ||
        asicType == cfg::AsicType::ASIC_TYPE_YUBA) {
      switchInfo.connectionHandle() = "/dev/uio0";
    }
    switchInfo.systemPortRanges() = asic->getSystemPortRanges();
    if (asic->getLocalSystemPortOffset().has_value()) {
      switchInfo.localSystemPortOffset() = *asic->getLocalSystemPortOffset();
    }
    if (asic->getGlobalSystemPortOffset().has_value()) {
      switchInfo.globalSystemPortOffset() = *asic->getGlobalSystemPortOffset();
    }
    if (asic->getInbandPortId().has_value()) {
      switchInfo.inbandPortId() = *asic->getInbandPortId();
    }
    defaultSwitchIdToSwitchInfo.insert({SwitchID(switchId), switchInfo});
    populateSwitchInfo(
        config, defaultSwitchIdToSwitchInfo, defaultHwAsicTable, platformType);
  }
  if (FLAGS_enable_acl_table_group) {
    utility::setupDefaultAclTableGroups(config);
  }
  auto switchType = asic->getSwitchType();
  // VOQ config
  if (switchType == cfg::SwitchType::VOQ) {
    auto nameAndDefaultVoqCfg =
        getNameAndDefaultVoqCfg(cfg::PortType::INTERFACE_PORT);
    CHECK(nameAndDefaultVoqCfg.has_value());
    config.defaultVoqConfig() = nameAndDefaultVoqCfg->queueConfig;
    auto cpuVoqConfigAndName = getNameAndDefaultVoqCfg(cfg::PortType::CPU_PORT);
    if (cpuVoqConfigAndName) {
      config.cpuVoqs() = cpuVoqConfigAndName->queueConfig;
    }
  }

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
    auto iter = lbModeMap.find(folly::copy(portCfg->portType().value()));
    if (iter == lbModeMap.end()) {
      throw FbossError(
          "Unable to find the desired loopback mode for port type: ",
          folly::copy(portCfg->portType().value()));
    }
    portCfg->loopbackMode() = iter->second;
    if (portCfg->portType() == cfg::PortType::FABRIC_PORT) {
      portCfg->ingressVlan() = 0;
      portCfg->maxFrameSize() = 0;
      portCfg->state() = enableFabricPorts ? cfg::PortState::ENABLED
                                           : cfg::PortState::DISABLED;

      if (asic->isSupported(HwAsic::Feature::FABRIC_TX_QUEUES)) {
        portCfg->portQueueConfigName() = kFabricTxQueueConfig;
      }
    } else {
      portCfg->state() = cfg::PortState::ENABLED;
      portCfg->ingressVlan() = port2vlan.find(portID)->second;
      portCfg->maxFrameSize() = kPortMTU;
      if (switchType == cfg::SwitchType::VOQ &&
          *portCfg->portType() != cfg::PortType::INTERFACE_PORT) {
        auto nameAndVoqConfig = getNameAndDefaultVoqCfg(*portCfg->portType());
        if (nameAndVoqConfig.has_value()) {
          config.portQueueConfigs()[nameAndVoqConfig->name] =
              nameAndVoqConfig->queueConfig;
          portCfg->portVoqConfigName() = nameAndVoqConfig->name;
        }
      }
    }
    portCfg->routable() = true;
    portCfg->parserType() = cfg::ParserType::L3;
  }

  if (switchType == cfg::SwitchType::NPU) {
    // Vlan config
    for (auto vlanID : vlans) {
      cfg::Vlan vlan;
      vlan.id() = vlanID;
      vlan.name() = "vlan" + std::to_string(vlanID);
      vlan.routable() = true;
      config.vlans()->push_back(vlan);
    }

    auto defaultVlanId =
        (asic->getAsicType() != cfg::AsicType::ASIC_TYPE_CHENAB)
        ? kDefaultVlanId4094
        : kDefaultVlanId1;
    cfg::Vlan defaultVlan;
    defaultVlan.id() = defaultVlanId;
    defaultVlan.name() = folly::sformat("vlan{}", defaultVlanId);
    defaultVlan.intfID() = 10;
    defaultVlan.routable() = true;
    config.vlans()->push_back(defaultVlan);
    config.defaultVlan() = defaultVlanId;

    // Vlan port config
    for (auto vlanPortPair : port2vlan) {
      cfg::VlanPort vlanPort;
      vlanPort.logicalPort() = vlanPortPair.first;
      vlanPort.vlanID() = vlanPortPair.second;
      vlanPort.spanningTreeState() = cfg::SpanningTreeState::FORWARDING;
      vlanPort.emitTags() = false;
      config.vlanPorts()->push_back(vlanPort);
    }
    if (asic->getAsicType() == cfg::AsicType::ASIC_TYPE_CHENAB) {
      /*
       * TODO(pshaikh): Chenab-Hack pipeline lookup for traffic injected by cpu
       * requires vlan rif in default vlan.
       */
      cfg::Interface intf1;
      intf1.intfID() = *defaultVlan.intfID();
      intf1.name() = "default_vlan_rif";
      intf1.vlanID() = kDefaultVlanId1;
      intf1.mac() = getLocalCpuMacStr();
      intf1.type() = cfg::InterfaceType::VLAN;
      intf1.routerID() = 0;
      intf1.mtu() = 9000;
      intf1.isVirtual() = true;
      auto ipDecimal = config.interfaces()->size() + 1;
      intf1.ipAddresses()->emplace_back(
          genInterfaceAddress(ipDecimal, true /* v4 */, 1, 24));
      intf1.ipAddresses()->emplace_back(
          genInterfaceAddress(ipDecimal, false /* v4 */, 0, 64));
      config.interfaces()->push_back(intf1);
    }
  }
  return config;
}

void populateSwitchInfo(
    cfg::SwitchConfig& config,
    const std::map<SwitchID, cfg::SwitchInfo>& switchIdToSwitchInfo,
    const std::map<SwitchID, const HwAsic*>& hwAsicTable,
    const std::optional<PlatformType> platformType) {
  std::map<long, cfg::SwitchInfo> newSwitchIdToSwitchInfo;
  std::map<long, cfg::DsfNode> newDsfNodes;
  // save the firsthwAsic to create dsfNodeConfig
  const auto& firstHwAsic = [&hwAsicTable]() {
    SwitchID switchId{0};
    auto firstHwAsicTableItr = hwAsicTable.find(switchId);
    if (firstHwAsicTableItr == hwAsicTable.end()) {
      throw FbossError("HwAsic not found for SwitchID: ", switchId);
    }
    return firstHwAsicTableItr->second;
  }();
  for (const auto& [switchId, switchInfo] : switchIdToSwitchInfo) {
    newSwitchIdToSwitchInfo.insert({switchId, switchInfo});
    auto hwAsicTableItr = hwAsicTable.find(switchId);
    if (hwAsicTableItr == hwAsicTable.end()) {
      throw FbossError("HwAsic not found for SwitchID: ", switchId);
    }
    const auto& hwAsic = hwAsicTableItr->second;
    if (hwAsic->getSwitchType() == cfg::SwitchType::VOQ ||
        hwAsic->getSwitchType() == cfg::SwitchType::FABRIC) {
      newDsfNodes.insert(
          {switchId, dsfNodeConfig(*firstHwAsic, switchId, platformType)});
    }
  }
  config.switchSettings()->switchIdToSwitchInfo() = newSwitchIdToSwitchInfo;
  config.dsfNodes() = newDsfNodes;
}

cfg::SwitchConfig
oneL3IntfTwoPortConfig(const SwSwitch* sw, PortID port1, PortID port2) {
  std::vector<PortID> ports{port1, port2};
  auto asic = checkSameAndGetAsic(sw->getHwAsicTable()->getL3Asics());
  return oneL3IntfTwoPortConfig(
      sw->getPlatformMapping(),
      asic,
      port1,
      port2,
      sw->getPlatformSupportsAddRemovePort(),
      asic->desiredLoopbackModes());
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
  config.interfaces()[0].type() = cfg::InterfaceType::VLAN;
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

cfg::SwitchConfig twoL3IntfConfig(
    SwSwitch* swSwitch,
    PortID port1,
    PortID port2,
    const std::map<cfg::PortType, cfg::PortLoopbackMode>& lbModeMap,
    cfg::InterfaceType intfType) {
  return twoL3IntfConfig(
      swSwitch->getPlatformMapping(),
      swSwitch->getHwAsicTable()->getL3Asics(),
      swSwitch->getPlatformSupportsAddRemovePort(),
      port1,
      port2,
      lbModeMap,
      intfType);
}

cfg::SwitchConfig twoL3IntfConfig(
    const PlatformMapping* platformMapping,
    const std::vector<const HwAsic*>& asics,
    bool supportsAddRemovePort,
    PortID port1,
    PortID port2,
    const std::map<cfg::PortType, cfg::PortLoopbackMode>& lbModeMap,
    cfg::InterfaceType intfType) {
  auto asic = checkSameAndGetAsic(asics);
  std::map<PortID, VlanID> port2vlan;
  std::vector<PortID> ports{port1, port2};
  std::vector<VlanID> vlans;
  auto vlan = kBaseVlanId;
  auto switchType = asic->getSwitchType();
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
        platformMapping->getPlatformPort(port).mapping()->portType();
    CHECK(portType != cfg::PortType::FABRIC_PORT);
    // For non NPU switch type vendor SAI impls don't support
    // tagging packet at port ingress.
    if (switchType == cfg::SwitchType::NPU) {
      port2vlan[port] = VlanID(kBaseVlanId);
      vlans.emplace_back(vlan++);
    } else {
      port2vlan[port] = VlanID(0);
    }
  }
  auto config = genPortVlanCfg(
      platformMapping,
      asic,
      ports,
      port2vlan,
      vlans,
      lbModeMap,
      supportsAddRemovePort);

  auto computeIntfId = [&config, &platformMapping, &ports, &switchType, &vlans](
                           auto idx) {
    if (switchType == cfg::SwitchType::NPU) {
      return static_cast<int64_t>(vlans[idx]);
    }
    auto mySwitchId =
        config.switchSettings()->switchIdToSwitchInfo()->begin()->first;

    auto port = ports[idx];
    auto portScope = *platformMapping->getPlatformPort(port).mapping()->scope();
    auto sysPortId = getSystemPortID(
        port,
        portScope,
        *config.switchSettings()->switchIdToSwitchInfo(),
        SwitchID(mySwitchId));
    return static_cast<int64_t>(sysPortId);
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
    if (switchType == cfg::SwitchType::NPU) {
      if (intfType == cfg::InterfaceType::SYSTEM_PORT) {
        throw FbossError("System port not supported for NPU switch type");
      }
    }
    intf.type() = switchType == cfg::SwitchType::NPU
        ? intfType
        : cfg::InterfaceType::SYSTEM_PORT;
    if (switchType == cfg::SwitchType::VOQ) {
      auto portScope =
          *platformMapping->getPlatformPort(ports[i]).mapping()->scope();
      intf.scope() = portScope;
    }
    config.interfaces()->push_back(intf);
  }
  return config;
}

bool isRswPlatform(PlatformType type) {
  switch (type) {
    case PlatformType::PLATFORM_WEDGE:
    case PlatformType::PLATFORM_WEDGE100:
    case PlatformType::PLATFORM_WEDGE400:
    case PlatformType::PLATFORM_WEDGE400_GRANDTETON:
    case PlatformType::PLATFORM_WEDGE400C:
      return true;
    default:
      return false;
  };
  return false;
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
    auto logId = folly::copy(port.logicalID().value());
    if (folly::copy(port.state().value()) != cfg::PortState::ENABLED) {
      continue;
    }

    auto portId = PortID(logId);
    auto vlanId = folly::copy(port.ingressVlan().value());
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
    auto logId = folly::copy(port.logicalID().value());
    if (folly::copy(port.state().value()) != cfg::PortState::ENABLED) {
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
      if (pgName.find("downlink") != std::string::npos) {
        downlinks.push_back(portId);
      } else if (
          (pgName.find("uplink") != std::string::npos) &&
          uplinks.size() < ecmpWidth) {
        uplinks.push_back(portId);
      }
    }
  }

  XLOG(DBG2) << "Uplinks : " << uplinks.size()
             << " downlinks: " << downlinks.size();
  return std::pair(uplinks, downlinks);
}

/*
 * Generic function to separate uplinks and downlinks, given a HwSwitch* and a
 * SwitchConfig.
 *
 * Calls already-defined RSW helper functions if HwSwitch* is an RSW platform,
 * otherwise separates the config's ports.
 */
UplinkDownlinkPair getAllUplinkDownlinkPorts(
    PlatformType platformType,
    const cfg::SwitchConfig& config,
    const int ecmpWidth,
    const bool mmu_lossless) {
  if (mmu_lossless) {
    return getRtswUplinkDownlinkPorts(config, ecmpWidth);
  } else if (isRswPlatform(platformType)) {
    return getRswUplinkDownlinkPorts(config, ecmpWidth);
  }

  // If the platform is not an RSW, consider the first ecmpWidth-many ports to
  // be uplinks and the rest to be downlinks.
  // First populate masterPorts with all ports, analogous to
  // masterLogicalPortIds, then slice uplinks/downlinks according to ecmpWidth.

  std::vector<PortID> uplinks, downlinks;
  std::set<PortID> aggports;

  // Get all aggregate ports
  for (auto aggrPort : *config.aggregatePorts()) {
    for (auto memberPort : *aggrPort.memberPorts()) {
      aggports.insert(folly::copy(PortID(memberPort.memberPortID().value())));
    }
  }

  for (const auto& port : *config.ports()) {
    if (isEnabledPortWithSubnet(port, config)) {
      if (aggports.find(PortID(port.logicalID().value())) == aggports.end()) {
        if (uplinks.size() < ecmpWidth) {
          uplinks.emplace_back(folly::copy(port.logicalID().value()));
        } else {
          downlinks.emplace_back(folly::copy(port.logicalID().value()));
        }
      } else {
        downlinks.emplace_back(folly::copy(port.logicalID().value()));
      }
    }
  }
  CHECK_GE(uplinks.size() + downlinks.size(), ecmpWidth)
      << "Not enough ports with subnet in config. Need  " << ecmpWidth
      << " ports, but found only " << uplinks.size() + downlinks.size();
  return std::pair(uplinks, downlinks);
}
// Set any ports in this port group to use the specified speed,
// and disables any ports that don't support this speed.
void configurePortGroup(
    const PlatformMapping* platformMapping,
    bool supportsAddRemovePort,
    cfg::SwitchConfig& config,
    cfg::PortSpeed speed,
    std::vector<PortID> allPortsInGroup) {
  for (auto portID : allPortsInGroup) {
    // We might have removed a subsumed port already in a previous
    // iteration of the loop.
    auto cfgPort = findCfgPortIf(config, portID);
    if (cfgPort == config.ports()->end()) {
      continue;
    }

    const auto& platPortEntry = platformMapping->getPlatformPort(portID);
    auto profileID = platformMapping->getProfileIDBySpeedIf(portID, speed);
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
      throw std::runtime_error(
          folly::to<std::string>(
              "No profile ", profileID.value(), " found for port ", portID));
    }

    cfgPort->profileID() = profileID.value();
    cfgPort->speed() = speed;
    cfgPort->state() = cfg::PortState::ENABLED;
    removeSubsumedPorts(config, profile->second, supportsAddRemovePort);
  }
}

std::vector<PortID> getAllPortsInGroup(
    const PlatformMapping* platformMapping,
    PortID portID) {
  std::vector<PortID> allPortsinGroup;
  if (const auto& platformPorts = platformMapping->getPlatformPorts();
      !platformPorts.empty()) {
    const auto& portList =
        utility::getPlatformPortsByControllingPort(platformPorts, portID);
    for (const auto& port : portList) {
      allPortsinGroup.emplace_back(*port.mapping()->id());
    }
  }
  return allPortsinGroup;
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

bool checkConfigHasAclEntry(
    const cfg::SwitchConfig& config,
    std::string aclName) {
  if (isSaiConfig(config)) {
    for (const auto& aclTableGroup : *config.aclTableGroups()) {
      for (const auto& aclTable : *aclTableGroup.aclTables()) {
        for (const auto& acl : *aclTable.aclEntries()) {
          if (acl.name().value() == aclName) {
            return true;
          }
        }
      }
    }
  } else {
    auto acls = *config.acls();
    for (const auto& acl : acls) {
      if (acl.name().value() == aclName) {
        return true;
      }
    }
  }
  return false;
}

void configurePortProfile(
    const PlatformMapping* platformMapping,
    bool supportsAddRemovePort,
    cfg::SwitchConfig& config,
    cfg::PortProfileID profileID,
    std::vector<PortID> allPortsInGroup,
    PortID controllingPortID) {
  for (auto portID : allPortsInGroup) {
    // We might have removed a subsumed port already in a previous
    // iteration of the loop.
    auto cfgPort = findCfgPortIf(config, portID);
    if (cfgPort == config.ports()->end()) {
      continue;
    }

    const auto& platPortEntry = platformMapping->getPlatformPort(portID);
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
    cfgPort->state() = cfg::PortState::ENABLED;
    removeSubsumedPorts(config, profile->second, supportsAddRemovePort);
  }
}

void setupMultipleEgressPoolAndQueueConfigs(
    cfg::SwitchConfig& config,
    const std::vector<int>& losslessQueueIds,
    const uint64_t mmuSizeBytes) {
  const std::string kLosslessPoolName{"egress_lossless_pool"};
  const std::string kLossyPoolName{"egress_lossy_pool"};

  // Create pool configs for 2 egress buffer pools. The buffer pool
  // sizing here is very specific to YUBA. A different allocation
  // might be needed for other ASICs.
  cfg::BufferPoolConfig losslessPoolCfg;
  losslessPoolCfg.sharedBytes() = mmuSizeBytes;
  cfg::BufferPoolConfig lossyPoolCfg;
  lossyPoolCfg.sharedBytes() = mmuSizeBytes * 0.3; // 30% of MMU!
  std::map<std::string, cfg::BufferPoolConfig> bufferPoolCfgMap =
      config.bufferPoolConfigs().ensure();
  bufferPoolCfgMap.insert(std::make_pair(kLosslessPoolName, losslessPoolCfg));
  bufferPoolCfgMap.insert(std::make_pair(kLossyPoolName, lossyPoolCfg));
  config.bufferPoolConfigs() = std::move(bufferPoolCfgMap);

  // Attach lossless pool to lossless queues and lossy pool to the rest
  std::vector<cfg::PortQueue> queues;
  for (int qid = 0; qid < 8; qid++) {
    cfg::PortQueue queueCfg;
    queueCfg.id() = qid;
    queueCfg.name() = folly::to<std::string>("queue", qid);
    queueCfg.streamType() = cfg::StreamType::UNICAST;
    queueCfg.scheduling() = cfg::QueueScheduling::STRICT_PRIORITY;
    if (std::find(losslessQueueIds.begin(), losslessQueueIds.end(), qid) !=
        losslessQueueIds.end()) {
      queueCfg.bufferPoolName() = kLosslessPoolName;
    } else {
      queueCfg.bufferPoolName() = kLossyPoolName;
    }
    queues.push_back(queueCfg);
  }
  const std::string kPortQueueName{"egress_port_queue_config"};
  config.portQueueConfigs()[kPortQueueName] = std::move(queues);
  for (auto& portCfg : *config.ports()) {
    if (portCfg.portType() == cfg::PortType::INTERFACE_PORT) {
      portCfg.portQueueConfigName() = kPortQueueName;
    }
  }
}

bool isSaiConfig(const cfg::SwitchConfig& config) {
  return config.sdkVersion().has_value() &&
      config.sdkVersion()->saiSdk().has_value() &&
      !config.sdkVersion()->saiSdk()->empty();
}

void modifyPlatformConfig(
    cfg::PlatformConfig& config,
    const std::function<void(std::string&)>& modifyYamlFunc,
    const std::function<void(std::map<std::string, std::string>&)>&
        modifyMapFunc) {
  auto& chip = *config.chip();
  if (chip.getType() == cfg::ChipConfig::Type::bcm) {
    auto& bcm = chip.mutable_bcm();
    if (!bcm.yamlConfig().value_or("").empty()) {
      // yamlConfig used for TH4
      modifyYamlFunc(*bcm.yamlConfig());
    } else {
      modifyMapFunc(*bcm.config());
    }
  } else if (chip.getType() == cfg::ChipConfig::Type::asicConfig) {
    auto& common = *(chip.mutable_asicConfig().common());
    if (common.getType() == cfg::AsicConfigEntry::Type::yamlConfig) {
      // yamlConfig used for TH4
      modifyYamlFunc(common.mutable_yamlConfig());
    } else if (common.getType() == cfg::AsicConfigEntry::Type::config) {
      modifyMapFunc(common.mutable_config());
    }
  }
}

cfg::SwitchConfig onePortPerInterfaceConfig(
    const SwSwitch* swSwitch,
    const std::vector<PortID>& ports,
    bool interfaceHasSubnet,
    bool setInterfaceMac,
    int baseIntfId,
    bool enableFabricPorts,
    const std::optional<cfg::InterfaceType>& intfType) {
  auto switchID = swSwitch->getScopeResolver()->scope(ports[0]).switchId();
  auto asic = swSwitch->getHwAsicTable()->getHwAsicIf(switchID);
  cfg::InterfaceType intfTypeVal = getInterfaceType(*asic);
  if (intfType) {
    intfTypeVal = intfType.value();
  }
  return onePortPerInterfaceConfigImpl(
      swSwitch,
      ports,
      interfaceHasSubnet,
      setInterfaceMac,
      baseIntfId,
      enableFabricPorts,
      intfTypeVal);
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
    bool enableFabricPorts,
    const std::optional<std::map<SwitchID, cfg::SwitchInfo>>&
        switchIdToSwitchInfo,
    const std::optional<std::map<SwitchID, const HwAsic*>>& hwAsicTable,
    const std::optional<PlatformType> platformType,
    const std::optional<cfg::InterfaceType>& intfType) {
  cfg::InterfaceType intfTypeVal = getInterfaceType(*asic);
  if (intfType) {
    intfTypeVal = *intfType;
  }
  return onePortPerInterfaceConfigImpl(
      platformMapping,
      asic,
      ports,
      supportsAddRemovePort,
      lbModeMap,
      interfaceHasSubnet,
      setInterfaceMac,
      baseIntfId,
      enableFabricPorts,
      switchIdToSwitchInfo,
      hwAsicTable,
      platformType,
      intfTypeVal);
}

cfg::SwitchConfig onePortPerInterfaceConfig(
    const TestEnsembleIf* ensemble,
    const std::vector<PortID>& ports,
    bool interfaceHasSubnet,
    bool setInterfaceMac,
    int baseIntfId,
    bool enableFabricPorts,
    const std::optional<cfg::InterfaceType>& intfType) {
  auto switchID = ensemble->scopeResolver().scope(ports[0]).switchId();
  auto asic = ensemble->getHwAsicTable()->getHwAsicIf(switchID);
  cfg::InterfaceType intfTypeVal = getInterfaceType(*asic);
  if (intfType) {
    intfTypeVal = intfType.value();
  }
  return onePortPerInterfaceConfigImpl(
      ensemble,
      ports,
      interfaceHasSubnet,
      setInterfaceMac,
      baseIntfId,
      enableFabricPorts,
      intfTypeVal);
}

void runCintScript(TestEnsembleIf* ensemble, const std::string& cintStr) {
  folly::test::TemporaryFile file;
  XLOG(INFO) << " Cint file " << file.path().c_str();
  folly::writeFull(file.fd(), cintStr.c_str(), cintStr.size());
  auto cmd = folly::sformat("cint {}\n", file.path().c_str());
  std::string out;
  ensemble->runDiagCommand(cmd, out, std::nullopt);
}

std::string
genInterfaceAddress(int ipDecimal, bool isV4, int host, int subnetMask) {
  /* 224.x.x.x onwards are multicast */
  auto ipDecimal1 = folly::sformat("{}", ipDecimal % 224);
  auto ipDecimal2 = folly::sformat("{}", ipDecimal / 224);

  auto addr = isV4
      ? folly::IPAddress(
            folly::sformat("{}.{}.0.{}", ipDecimal1, ipDecimal2, host))
      : folly::IPAddress(
            folly::sformat("{}:{}::{}", ipDecimal1, ipDecimal2, host));
  return folly::sformat("{}/{}", addr.str(), subnetMask);
}
} // namespace facebook::fboss::utility
