/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/test/HwTestUdfUtils.h"
#include "fboss/agent/hw/sai/switch/SaiSwitch.h"
#include "fboss/agent/hw/sai/switch/SaiUdfManager.h"
#include "fboss/agent/packet/IPProto.h"
#include "fboss/agent/test/utils/UdfTestUtils.h"

#include <gtest/gtest.h>

namespace facebook::fboss::utility {

void validateUdfConfig(
    const HwSwitch* hw,
    const std::string& udfGroupName,
    const std::string& udfPacketMatcherName) {
  const auto saiSwitch = static_cast<const SaiSwitch*>(hw);
  const facebook::fboss::SaiUdfManager& udfManager =
      saiSwitch->managerTable()->udfManager();
  const auto& udfApi = SaiApiTable::getInstance()->udfApi();
  const auto udfGroupIter = udfManager.getUdfGroupHandles().find(udfGroupName);
  EXPECT_TRUE(udfGroupIter != udfManager.getUdfGroupHandles().cend());
  if (udfGroupIter != udfManager.getUdfGroupHandles().cend()) {
    auto saiUdfGroup = udfGroupIter->second->udfGroup;
    if (udfGroupName == utility::kUdfHashDstQueuePairGroupName) {
      // Verify UdfGroup attributes
      EXPECT_EQ(
          udfApi.getAttribute(
              saiUdfGroup->adapterKey(), SaiUdfGroupTraits::Attributes::Type{}),
          SAI_UDF_GROUP_TYPE_HASH);
      if (hw->getPlatform()->getAsic()->getAsicType() ==
          cfg::AsicType::ASIC_TYPE_CHENAB) {
        EXPECT_EQ(
            udfApi.getAttribute(
                saiUdfGroup->adapterKey(),
                SaiUdfGroupTraits::Attributes::Length{}),
            utility::kChenabUdfHashDstQueuePairFieldSizeInBytes);
      } else {
        EXPECT_EQ(
            udfApi.getAttribute(
                saiUdfGroup->adapterKey(),
                SaiUdfGroupTraits::Attributes::Length{}),
            utility::kUdfHashDstQueuePairFieldSizeInBytes);
      }

      // Verify Udf attributes
      auto saiUdf = udfGroupIter->second->udfs[udfPacketMatcherName]->udf;
      if (hw->getPlatform()->getAsic()->getAsicType() ==
          cfg::AsicType::ASIC_TYPE_CHENAB) {
        EXPECT_EQ(
            udfApi.getAttribute(
                saiUdf->adapterKey(), SaiUdfTraits::Attributes::Offset{}),
            utility::kChenabUdfHashDstQueuePairStartOffsetInBytes);
      } else {
        EXPECT_EQ(
            udfApi.getAttribute(
                saiUdf->adapterKey(), SaiUdfTraits::Attributes::Offset{}),
            utility::kUdfHashDstQueuePairStartOffsetInBytes);
      }
      EXPECT_EQ(
          udfApi.getAttribute(
              saiUdf->adapterKey(), SaiUdfTraits::Attributes::Base{}),
          SAI_UDF_BASE_L4);
    } else if (udfGroupName == utility::kUdfAclRoceOpcodeGroupName) {
      // Verify UdfGroup attributes
      EXPECT_EQ(
          udfApi.getAttribute(
              saiUdfGroup->adapterKey(), SaiUdfGroupTraits::Attributes::Type{}),
          SAI_UDF_GROUP_TYPE_GENERIC);
      EXPECT_EQ(
          udfApi.getAttribute(
              saiUdfGroup->adapterKey(),
              SaiUdfGroupTraits::Attributes::Length{}),
          utility::kUdfAclRoceOpcodeFieldSizeInBytes);

      // Verify Udf attributes
      auto saiUdf = udfGroupIter->second->udfs[udfPacketMatcherName]->udf;
      EXPECT_EQ(
          udfApi.getAttribute(
              saiUdf->adapterKey(), SaiUdfTraits::Attributes::Offset{}),
          utility::kUdfAclRoceOpcodeStartOffsetInBytes);
      EXPECT_EQ(
          udfApi.getAttribute(
              saiUdf->adapterKey(), SaiUdfTraits::Attributes::Base{}),
          SAI_UDF_BASE_L4);
    } else {
      throw FbossError(
          "Unsupported UDF Group name " + udfGroupName + ". Cannot validate.");
    }
  }

  // Verify UdfMatch attributes
  const auto UdfMatchIter =
      udfManager.getUdfMatchHandles().find(udfPacketMatcherName);
  EXPECT_TRUE(UdfMatchIter != udfManager.getUdfMatchHandles().cend());
  if (UdfMatchIter != udfManager.getUdfMatchHandles().cend()) {
    auto saiUdfMatch = UdfMatchIter->second->udfMatch;
    EXPECT_EQ(
        udfApi.getAttribute(
            saiUdfMatch->adapterKey(), SaiUdfMatchTraits::Attributes::L3Type{}),
        AclEntryFieldU8(std::make_pair(
            static_cast<uint8_t>(IP_PROTO::IP_PROTO_UDP),
            static_cast<uint8_t>(SaiUdfManager::kMaskAny))));
#if SAI_API_VERSION >= SAI_VERSION(1, 12, 0)
    EXPECT_EQ(
        udfApi.getAttribute(
            saiUdfMatch->adapterKey(),
            SaiUdfMatchTraits::Attributes::L4DstPortType{}),
        AclEntryFieldU16(
            std::make_pair(kUdfL4DstPort, SaiUdfManager::kL4PortMask)));
#endif
  }
}

void validateRemoveUdfGroup(
    const HwSwitch* hw,
    const std::string& udfGroupName,
    int udfGroupId) {
  EXPECT_THROW(getHwUdfGroupId(hw, udfGroupName), FbossError);

  // Verify SDK removed udf group
  const auto& udfApi = SaiApiTable::getInstance()->udfApi();
  EXPECT_THROW(
      udfApi.getAttribute(
          static_cast<facebook::fboss::UdfGroupSaiId>(udfGroupId),
          SaiUdfGroupTraits::Attributes::Type{}),
      SaiApiError);
}

void validateRemoveUdfPacketMatcher(
    const HwSwitch* hw,
    const std::string& udfPackeMatchName,
    int udfPacketMatcherId) {
  EXPECT_THROW(getHwUdfPacketMatcherId(hw, udfPackeMatchName), FbossError);

  // Verify SDK removed udf packet matcher
  const auto& udfApi = SaiApiTable::getInstance()->udfApi();
  EXPECT_THROW(
      udfApi.getAttribute(
          static_cast<facebook::fboss::UdfMatchSaiId>(udfPacketMatcherId),
          SaiUdfMatchTraits::Attributes::L3Type{}),
      SaiApiError);
}

int getHwUdfGroupId(const HwSwitch* hw, const std::string& udfGroupName) {
  const auto& udfGroupHandles = static_cast<const SaiSwitch*>(hw)
                                    ->managerTable()
                                    ->udfManager()
                                    .getUdfGroupHandles();
  const auto udfGroupIter = udfGroupHandles.find(udfGroupName);
  if (udfGroupIter != udfGroupHandles.cend()) {
    return udfGroupIter->second->udfGroup->adapterKey();
  }
  throw FbossError("Cannot find UdfGroup " + udfGroupName + " in Sai Switch");
}

int getHwUdfPacketMatcherId(
    const HwSwitch* hw,
    const std::string& udfPackeMatchName) {
  const auto& udfMatchHandles = static_cast<const SaiSwitch*>(hw)
                                    ->managerTable()
                                    ->udfManager()
                                    .getUdfMatchHandles();
  const auto udfMatchIter = udfMatchHandles.find(udfPackeMatchName);
  if (udfMatchIter != udfMatchHandles.cend()) {
    return udfMatchIter->second->udfMatch->adapterKey();
  }
  throw FbossError(
      "Cannot find UdfMatch " + udfPackeMatchName + " in Sai Switch");
}

void validateUdfIdsInQset(
    const HwSwitch* /* unused */,
    const int /*aclGroupId*/,
    const bool /*isSet*/) {
  return;
}

cfg::SwitchConfig addUdfAclRoceOpcodeConfig(cfg::SwitchConfig& cfg) {
  // not supported on SAI yet.
  return cfg;
}

void validateUdfAclRoceOpcodeConfig(
    const HwSwitch* /*hw*/,
    std::shared_ptr<SwitchState> /*curState*/) {
  return;
}

} // namespace facebook::fboss::utility
