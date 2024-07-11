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
#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmFieldProcessorUtils.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/hw/bcm/BcmUdfManager.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/hw/test/HwTest.h"
#include "fboss/agent/hw/test/HwTestAclUtils.h"
#include "fboss/agent/packet/IPProto.h"

#include <gtest/gtest.h>

namespace facebook::fboss::utility {

void validateUdfConfig(
    const HwSwitch* hw,
    const std::string& udfGroupName,
    const std::string& udfPacketMatcherName) {
  int udfGroupId;
  udfGroupId = static_cast<const BcmSwitch*>(hw)->getUdfMgr()->getBcmUdfGroupId(
      udfGroupName);
  /* get udf info */
  bcm_udf_t udfInfo;
  bcm_udf_t_init(&udfInfo);
  auto rv = bcm_udf_get(
      static_cast<const BcmSwitch*>(hw)->getUnit(), udfGroupId, &udfInfo);
  bcmCheckError(rv, "Unable to get udfInfo for udfGroupId: ", udfGroupId);
  EXPECT_EQ(udfInfo.layer, bcmUdfLayerL4OuterHeader);
  if (udfGroupName == utility::kUdfHashDstQueuePairGroupName) {
    EXPECT_EQ(
        udfInfo.start, utility::kUdfHashDstQueuePairStartOffsetInBytes * 8);
    EXPECT_EQ(udfInfo.width, utility::kUdfHashDstQueuePairFieldSizeInBytes * 8);
  } else if (udfGroupName == utility::kUdfAclRoceOpcodeGroupName) {
    EXPECT_EQ(udfInfo.start, utility::kUdfAclRoceOpcodeStartOffsetInBytes * 8);
    EXPECT_EQ(udfInfo.width, utility::kUdfAclRoceOpcodeFieldSizeInBytes * 8);
  }

  const int udfPacketMatcherId =
      static_cast<const BcmSwitch*>(hw)->getUdfMgr()->getBcmUdfPacketMatcherId(
          udfPacketMatcherName);
  /* get udf pkt info */
  bcm_udf_pkt_format_info_t pktFormat;
  bcm_udf_pkt_format_info_t_init(&pktFormat);
  rv = bcm_udf_pkt_format_info_get(
      static_cast<const BcmSwitch*>(hw)->getUnit(),
      udfPacketMatcherId,
      &pktFormat);
  bcmCheckError(
      rv,
      "Unable to get pkt_format for udfPacketMatcherId: ",
      udfPacketMatcherId);
  EXPECT_EQ(pktFormat.ip_protocol, static_cast<int>(IP_PROTO::IP_PROTO_UDP));
  EXPECT_EQ(pktFormat.l4_dst_port, utility::kUdfL4DstPort);
}

void validateRemoveUdfGroup(
    const HwSwitch* hw,
    const std::string& udfGroupName,
    int udfGroupId) {
  EXPECT_THROW(getHwUdfGroupId(hw, udfGroupName), FbossError);

  // Verify SDK removed udf group
  bcm_udf_t udfInfo;
  bcm_udf_t_init(&udfInfo);
  auto rv = bcm_udf_get(
      static_cast<const BcmSwitch*>(hw)->getUnit(), udfGroupId, &udfInfo);
  if (hw->getPlatform()->getAsic()->getAsicType() !=
      cfg::AsicType::ASIC_TYPE_FAKE) {
    EXPECT_THROW(
        bcmCheckError(rv, "Unable to get udfInfo for udfGroupId: ", udfGroupId),
        FbossError);
  }
}

void validateRemoveUdfPacketMatcher(
    const HwSwitch* hw,
    const std::string& udfPackeMatchName,
    int udfPacketMatcherId) {
  EXPECT_THROW(getHwUdfPacketMatcherId(hw, udfPackeMatchName), FbossError);

  // Verify SDK removed udf packet matcher
  bcm_udf_pkt_format_info_t pktFormat;
  bcm_udf_pkt_format_info_t_init(&pktFormat);
  auto rv = bcm_udf_pkt_format_info_get(
      static_cast<const BcmSwitch*>(hw)->getUnit(),
      udfPacketMatcherId,
      &pktFormat);
  if (hw->getPlatform()->getAsic()->getAsicType() !=
      cfg::AsicType::ASIC_TYPE_FAKE) {
    EXPECT_THROW(
        bcmCheckError(
            rv,
            "Unable to get pkt_format for udfPacketMatcherId: ",
            udfPacketMatcherId),
        FbossError);
  }
}

int getHwUdfGroupId(const HwSwitch* hw, const std::string& udfGroupName) {
  return static_cast<const BcmSwitch*>(hw)->getUdfMgr()->getBcmUdfGroupId(
      udfGroupName);
}

int getHwUdfPacketMatcherId(
    const HwSwitch* hw,
    const std::string& udfPackeMatchName) {
  return static_cast<const BcmSwitch*>(hw)
      ->getUdfMgr()
      ->getBcmUdfPacketMatcherId(udfPackeMatchName);
}

void validateUdfIdsInQset(
    const HwSwitch* hw,
    const int aclGroupId,
    const bool isSet) {
  const auto& udf_ids =
      getUdfQsetIds(static_cast<const BcmSwitch*>(hw)->getUnit(), aclGroupId);
  if (isSet) {
    EXPECT_TRUE(udf_ids.size() > 0);
  } else {
    EXPECT_TRUE(udf_ids.size() == 0);
  }
}

cfg::SwitchConfig addUdfAclRoceOpcodeConfig(cfg::SwitchConfig& cfg) {
  cfg.udfConfig() = utility::addUdfAclConfig();
  auto acl = utility::addAcl(&cfg, utility::kUdfAclRoceOpcodeName);
  acl->udfGroups() = {utility::kUdfAclRoceOpcodeGroupName};
  acl->roceOpcode() = utility::kUdfRoceOpcodeAck;

  // Add AclStat configuration
  utility::addAclStat(
      &cfg, utility::kUdfAclRoceOpcodeName, utility::kUdfAclRoceOpcodeStats);

  return cfg;
}

void validateUdfAclRoceOpcodeConfig(
    const HwSwitch* hw,
    std::shared_ptr<SwitchState> curState) {
  ASSERT_TRUE(utility::isAclTableEnabled(hw));

  utility::checkSwHwAclMatch(hw, curState, utility::kUdfAclRoceOpcodeName);

  utility::checkAclStat(
      hw,
      curState,
      {utility::kUdfAclRoceOpcodeName},
      utility::kUdfAclRoceOpcodeStats,
      utility::getAclCounterTypes({hw->getPlatform()->getAsic()}));

  // Verify that UdfGroupIds are there in Qset
  utility::validateUdfIdsInQset(
      hw, hw->getPlatform()->getAsic()->getDefaultACLGroupID(), true);
}

} // namespace facebook::fboss::utility
