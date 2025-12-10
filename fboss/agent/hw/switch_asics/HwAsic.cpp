/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include <thrift/lib/cpp/util/EnumUtils.h>
#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/switch_asics/Agera3PhyAsic.h"
#include "fboss/agent/hw/switch_asics/ChenabAsic.h"
#include "fboss/agent/hw/switch_asics/CredoPhyAsic.h"
#include "fboss/agent/hw/switch_asics/EbroAsic.h"
#include "fboss/agent/hw/switch_asics/FakeAsic.h"
#include "fboss/agent/hw/switch_asics/Jericho2Asic.h"
#include "fboss/agent/hw/switch_asics/Jericho3Asic.h"
#include "fboss/agent/hw/switch_asics/MockAsic.h"
#include "fboss/agent/hw/switch_asics/Ramon3Asic.h"
#include "fboss/agent/hw/switch_asics/RamonAsic.h"
#include "fboss/agent/hw/switch_asics/Tomahawk3Asic.h"
#include "fboss/agent/hw/switch_asics/Tomahawk4Asic.h"
#include "fboss/agent/hw/switch_asics/Tomahawk5Asic.h"
#include "fboss/agent/hw/switch_asics/Tomahawk6Asic.h"
#include "fboss/agent/hw/switch_asics/TomahawkAsic.h"
#include "fboss/agent/hw/switch_asics/Trident2Asic.h"
#include "fboss/agent/hw/switch_asics/YubaAsic.h"

DEFINE_int32(acl_gid, -1, "Content aware processor group ID for ACLs");
DEFINE_int32(teFlow_gid, -1, "Exact Match group ID for TeFlows");

namespace {
constexpr auto kDefaultACLGroupID = 128;
constexpr auto kDefaultTeFlowGroupID = 1;
constexpr auto kDefaultDropEgressID = 100000;
} // namespace

namespace facebook::fboss {

HwAsic::HwAsic(
    std::optional<int64_t> switchId,
    const cfg::SwitchInfo& switchInfo,
    std::optional<cfg::SdkVersion> sdkVersion,
    std::unordered_set<cfg::SwitchType> supportedModes)
    : switchType_(*switchInfo.switchType()),
      switchId_(switchId),
      switchIndex_(*switchInfo.switchIndex()),
      systemPortRanges_(*switchInfo.systemPortRanges()),
      sdkVersion_(sdkVersion) {
  CHECK(switchInfo.switchMac().has_value());
  asicMac_ = folly::MacAddress(*switchInfo.switchMac());
  if (supportedModes.find(switchType_) == supportedModes.end()) {
    throw std::runtime_error(
        folly::to<std::string>("Unsupported Mode: ", switchType_));
  }
  if (switchInfo.globalSystemPortOffset()) {
    globalSystemPortOffset_ = *switchInfo.globalSystemPortOffset();
  }
  if (switchInfo.localSystemPortOffset()) {
    localSystemPortOffset_ = *switchInfo.localSystemPortOffset();
  }
  if (switchInfo.inbandPortId()) {
    inbandPortId_ = *switchInfo.inbandPortId();
  }
}
/*
 * Default Content Aware Processor group ID for ACLs
 */
int HwAsic::getDefaultACLGroupID() const {
  if (FLAGS_acl_gid > 0) {
    return FLAGS_acl_gid;
  } else {
    return kDefaultACLGroupID;
  }
}

std::unique_ptr<HwAsic> HwAsic::makeAsic(
    int64_t switchId,
    const cfg::SwitchInfo& switchInfo,
    std::optional<cfg::SdkVersion> sdkVersion,
    std::optional<HwAsic::FabricNodeRole> fabricNodeRole) {
  switch (*switchInfo.asicType()) {
    case cfg::AsicType::ASIC_TYPE_FAKE:
      return std::make_unique<FakeAsic>(switchId, switchInfo, sdkVersion);
    case cfg::AsicType::ASIC_TYPE_MOCK:
      return std::make_unique<MockAsic>(switchId, switchInfo, sdkVersion);
    case cfg::AsicType::ASIC_TYPE_TRIDENT2:
      return std::make_unique<Trident2Asic>(switchId, switchInfo, sdkVersion);
    case cfg::AsicType::ASIC_TYPE_TOMAHAWK:
      return std::make_unique<TomahawkAsic>(switchId, switchInfo, sdkVersion);
    case cfg::AsicType::ASIC_TYPE_TOMAHAWK3:
      return std::make_unique<Tomahawk3Asic>(switchId, switchInfo, sdkVersion);
    case cfg::AsicType::ASIC_TYPE_TOMAHAWK4:
      return std::make_unique<Tomahawk4Asic>(switchId, switchInfo, sdkVersion);
    case cfg::AsicType::ASIC_TYPE_TOMAHAWK5:
      return std::make_unique<Tomahawk5Asic>(switchId, switchInfo, sdkVersion);
    case cfg::AsicType::ASIC_TYPE_TOMAHAWK6:
      return std::make_unique<Tomahawk6Asic>(switchId, switchInfo, sdkVersion);
    case cfg::AsicType::ASIC_TYPE_ELBERT_8DD:
      return std::make_unique<CredoPhyAsic>(switchId, switchInfo, sdkVersion);
    case cfg::AsicType::ASIC_TYPE_EBRO:
      return std::make_unique<EbroAsic>(switchId, switchInfo, sdkVersion);
    case cfg::AsicType::ASIC_TYPE_YUBA:
      return std::make_unique<YubaAsic>(switchId, switchInfo, sdkVersion);
    case cfg::AsicType::ASIC_TYPE_CHENAB:
      return std::make_unique<ChenabAsic>(switchId, switchInfo, sdkVersion);
    case cfg::AsicType::ASIC_TYPE_JERICHO2:
      return std::make_unique<Jericho2Asic>(switchId, switchInfo, sdkVersion);
    case cfg::AsicType::ASIC_TYPE_JERICHO3:
      return std::make_unique<Jericho3Asic>(switchId, switchInfo, sdkVersion);
    case cfg::AsicType::ASIC_TYPE_RAMON:
      return std::make_unique<RamonAsic>(switchId, switchInfo, sdkVersion);
    case cfg::AsicType::ASIC_TYPE_RAMON3:
      CHECK(fabricNodeRole.has_value());
      return std::make_unique<Ramon3Asic>(
          switchId, switchInfo, sdkVersion, fabricNodeRole.value());
    case cfg::AsicType::ASIC_TYPE_AGERA3:
      return std::make_unique<Agera3PhyAsic>(switchId, switchInfo, sdkVersion);
    case cfg::AsicType::ASIC_TYPE_GARONNE:
    case cfg::AsicType::ASIC_TYPE_SANDIA_PHY:
      throw FbossError("Unexcepted asic type: ", *switchInfo.asicType());
  };
  throw FbossError("Unexcepted asic type: ", *switchInfo.asicType());
}
/*
 * Default Content Aware Processor group ID for TeFlows
 */
int HwAsic::getDefaultTeFlowGroupID() const {
  if (FLAGS_teFlow_gid > 0) {
    return FLAGS_teFlow_gid;
  } else {
    return kDefaultTeFlowGroupID;
  }
}

/*
 * station entry id for vlan interface
 */
int HwAsic::getStationID(int intfID) const {
  return intfID;
}

int HwAsic::getDefaultDropEgressID() const {
  return kDefaultDropEgressID;
}

std::vector<cfg::AsicType> HwAsic::getAllHwAsicList() {
  std::vector<cfg::AsicType> result;
  for (auto asicType : apache::thrift::TEnumTraits<cfg::AsicType>::values) {
    result.push_back(asicType);
  }
  return result;
}

cfg::Range64 HwAsic::getReservedEncapIndexRange() const {
  throw FbossError(
      "Reserved encap idx range unimplemented for: ", getAsicTypeStr());
}

HwAsic::RecyclePortInfo HwAsic::getRecyclePortInfo(
    InterfaceNodeRole /* intfRole */) const {
  throw FbossError("Recycle port info unimplemented for: ", getAsicTypeStr());
}

uint32_t HwAsic::getMaxSwitchId() const {
  throw FbossError("Max switchId unimplemented for: ", getAsicTypeStr());
}

cfg::Range64 HwAsic::makeRange(int64_t min, int64_t max) {
  cfg::Range64 kRange;
  kRange.minimum() = min;
  kRange.maximum() = max;
  return kRange;
}
std::string HwAsic::getAsicTypeStr() const {
  return apache::thrift::util::enumNameSafe(getAsicType());
}

const std::map<cfg::PortType, cfg::PortLoopbackMode>&
HwAsic::desiredLoopbackModes() const {
  static const std::map<cfg::PortType, cfg::PortLoopbackMode> kLoopbackMode = {
      {cfg::PortType::INTERFACE_PORT, cfg::PortLoopbackMode::MAC}};
  return kLoopbackMode;
}

cfg::PortLoopbackMode HwAsic::getDesiredLoopbackMode(
    cfg::PortType portType) const {
  const auto loopbackModeMap = desiredLoopbackModes();
  auto itr = loopbackModeMap.find(portType);
  if (itr != loopbackModeMap.end()) {
    return itr->second;
  }
  throw FbossError("Unable to find the portType ", portType);
}

uint32_t HwAsic::getMaxPorts() const {
  return 2048;
}

uint32_t HwAsic::getVirtualDevices() const {
  return 1;
}

std::optional<uint32_t> HwAsic::computePortGroupSkew(
    const std::map<PortID, uint32_t>& portId2cableLen) const {
  throw FbossError(
      "Derived class must override getPortGroupSkew, where applicable");
}

HwAsic::FabricNodeRole HwAsic::getFabricNodeRole() const {
  throw FbossError(
      "Derived class must override getFabricNodeRole, where applicable");
}

const std::set<uint16_t>& HwAsic::getL1FabricPortsToConnectToL2() const {
  throw FbossError(
      "Derived class must override getL1FabricPortsToConnectToL2, where applicable");
}

uint64_t HwAsic::getCpuPortEgressPoolSize() const {
  throw FbossError("cpu port egress pool size is not available");
}

bool HwAsic::portMtuSupported(cfg::PortType portType) const {
  switch (portType) {
    case cfg::PortType::INTERFACE_PORT:
    case cfg::PortType::CPU_PORT:
    case cfg::PortType::RECYCLE_PORT:
    case cfg::PortType::MANAGEMENT_PORT:
    case cfg::PortType::EVENTOR_PORT:
    case cfg::PortType::HYPER_PORT:
      return true;
    case cfg::PortType::FABRIC_PORT:
    case cfg::PortType::HYPER_PORT_MEMBER:
      return false;
  };
  throw FbossError("Unexcepted port type: ", portType);
}
} // namespace facebook::fboss
