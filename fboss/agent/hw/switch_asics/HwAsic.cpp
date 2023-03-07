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
#include "fboss/agent/hw/switch_asics/CredoPhyAsic.h"
#include "fboss/agent/hw/switch_asics/EbroAsic.h"
#include "fboss/agent/hw/switch_asics/FakeAsic.h"
#include "fboss/agent/hw/switch_asics/GaronneAsic.h"
#include "fboss/agent/hw/switch_asics/IndusAsic.h"
#include "fboss/agent/hw/switch_asics/MarvelPhyAsic.h"
#include "fboss/agent/hw/switch_asics/MockAsic.h"
#include "fboss/agent/hw/switch_asics/RamonAsic.h"
#include "fboss/agent/hw/switch_asics/Tomahawk3Asic.h"
#include "fboss/agent/hw/switch_asics/Tomahawk4Asic.h"
#include "fboss/agent/hw/switch_asics/Tomahawk5Asic.h"
#include "fboss/agent/hw/switch_asics/TomahawkAsic.h"
#include "fboss/agent/hw/switch_asics/Trident2Asic.h"

DEFINE_int32(acl_gid, -1, "Content aware processor group ID for ACLs");
DEFINE_int32(teFlow_gid, -1, "Exact Match group ID for TeFlows");

namespace {
constexpr auto kDefaultACLGroupID = 128;
constexpr auto kDefaultTeFlowGroupID = 1;
constexpr auto kDefaultDropEgressID = 100000;
} // namespace

namespace facebook::fboss {

HwAsic::HwAsic(
    cfg::SwitchType switchType,
    std::optional<int64_t> switchId,
    std::optional<cfg::Range64> systemPortRange,
    std::unordered_set<cfg::SwitchType> supportedModes)
    : switchType_(switchType),
      switchId_(switchId),
      systemPortRange_(systemPortRange) {
  if (supportedModes.find(switchType_) == supportedModes.end()) {
    throw std::runtime_error(
        folly::to<std::string>("Unsupported Mode: ", switchType_));
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
    cfg::AsicType asicType,
    cfg::SwitchType switchType,
    std::optional<int64_t> switchId,
    std::optional<cfg::Range64> systemPortRange) {
  switch (asicType) {
    case cfg::AsicType::ASIC_TYPE_FAKE:
      return std::make_unique<FakeAsic>(switchType, switchId, systemPortRange);
    case cfg::AsicType::ASIC_TYPE_MOCK:
      return std::make_unique<MockAsic>(switchType, switchId, systemPortRange);
    case cfg::AsicType::ASIC_TYPE_TRIDENT2:
      return std::make_unique<Trident2Asic>(
          switchType, switchId, systemPortRange);
    case cfg::AsicType::ASIC_TYPE_TOMAHAWK:
      return std::make_unique<TomahawkAsic>(
          switchType, switchId, systemPortRange);
    case cfg::AsicType::ASIC_TYPE_TOMAHAWK3:
      return std::make_unique<Tomahawk3Asic>(
          switchType, switchId, systemPortRange);
    case cfg::AsicType::ASIC_TYPE_TOMAHAWK4:
      return std::make_unique<Tomahawk4Asic>(
          switchType, switchId, systemPortRange);
    case cfg::AsicType::ASIC_TYPE_TOMAHAWK5:
      return std::make_unique<Tomahawk5Asic>(
          switchType, switchId, systemPortRange);
    case cfg::AsicType::ASIC_TYPE_ELBERT_8DD:
      return std::make_unique<CredoPhyAsic>(
          switchType, switchId, systemPortRange);
    case cfg::AsicType::ASIC_TYPE_EBRO:
      return std::make_unique<EbroAsic>(switchType, switchId, systemPortRange);
    case cfg::AsicType::ASIC_TYPE_GARONNE:
      return std::make_unique<GaronneAsic>(
          switchType, switchId, systemPortRange);
    case cfg::AsicType::ASIC_TYPE_SANDIA_PHY:
      return std::make_unique<MarvelPhyAsic>(
          switchType, switchId, systemPortRange);
    case cfg::AsicType::ASIC_TYPE_INDUS:
      return std::make_unique<IndusAsic>(switchType, switchId, systemPortRange);
    case cfg::AsicType::ASIC_TYPE_RAMON:
      return std::make_unique<RamonAsic>(switchType, switchId, systemPortRange);
  };
  throw FbossError("Unexcepted asic type: ", asicType);
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

HwAsic::RecyclePortInfo HwAsic::getRecyclePortInfo() const {
  throw FbossError("Recycle port info unimplemented for: ", getAsicTypeStr());
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
} // namespace facebook::fboss
