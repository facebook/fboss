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
#include "fboss/agent/FbossError.h"

DEFINE_int32(acl_gid, -1, "Content aware processor group ID for ACLs");
DEFINE_int32(teFlow_gid, -1, "Exact Match group ID for TeFlows");

namespace {
constexpr auto kDefaultACLGroupID = 128;
constexpr auto kDefaultTeFlowGroupID = 1;
constexpr auto kDefaultDropEgressID = 100000;
enum IntAsicType ASIC_TYPE_LIST;
std::vector<IntAsicType> getAsicTypeIntList() {
  return ASIC_TYPE_LIST;
}
} // namespace

namespace facebook::fboss {

HwAsic::HwAsic(
    cfg::SwitchType switchType,
    std::optional<int64_t> switchId,
    std::unordered_set<cfg::SwitchType> supportedModes)
    : switchType_(switchType), switchId_(switchId) {
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

std::vector<HwAsic::AsicType> HwAsic::getAllHwAsicList() {
  std::vector<HwAsic::AsicType> result{};
  for (int asic : getAsicTypeIntList()) {
    HwAsic::AsicType asicType = static_cast<HwAsic::AsicType>(asic);
    result.push_back(asicType);
  }
  return result;
}

std::string HwAsic::getAsicTypeStr() const {
  switch (getAsicType()) {
    case AsicType::ASIC_TYPE_FAKE:
      return "Fake";
    case AsicType::ASIC_TYPE_MOCK:
      return "Mock";
    case AsicType::ASIC_TYPE_TRIDENT2:
      return "TD2";
    case AsicType::ASIC_TYPE_TOMAHAWK:
      return "TH";
    case AsicType::ASIC_TYPE_TOMAHAWK3:
      return "TH3";
    case AsicType::ASIC_TYPE_TOMAHAWK4:
      return "TH4";
    case AsicType::ASIC_TYPE_ELBERT_8DD:
      return "Elbert_8DD";
    case AsicType::ASIC_TYPE_EBRO:
      return "Ebro";
    case AsicType::ASIC_TYPE_GARONNE:
      return "Garonne";
    case AsicType::ASIC_TYPE_SANDIA_PHY:
      return "Sandia_phy";
    case AsicType::ASIC_TYPE_INDUS:
      return "Indus";
    case AsicType::ASIC_TYPE_BEAS:
      return "Beas";
  }
  throw FbossError("Unhandled ASIC type: ", getAsicType());
}
} // namespace facebook::fboss
