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
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/hw/bcm/BcmUdfManager.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/packet/IPProto.h"

#include <gtest/gtest.h>

namespace facebook::fboss::utility {

void validateUdfConfig(
    const HwSwitch* hw,
    const std::string& udfGroupName,
    const std::string& udfPacketMatcherName) {
  const int udfGroupId =
      static_cast<const BcmSwitch*>(hw)->getUdfMgr()->getBcmUdfGroupId(
          udfGroupName);
  /* get udf info */
  bcm_udf_t udfInfo;
  bcm_udf_t_init(&udfInfo);
  auto rv = bcm_udf_get(
      static_cast<const BcmSwitch*>(hw)->getUnit(), udfGroupId, &udfInfo);
  bcmCheckError(rv, "Unable to get udfInfo for udfGroupId: ", udfGroupId);
  EXPECT_EQ(udfInfo.layer, bcmUdfLayerL4OuterHeader);
  EXPECT_EQ(udfInfo.start, utility::kUdfStartOffsetInBytes * 8);
  EXPECT_EQ(udfInfo.width, utility::kUdfFieldSizeInBytes * 8);

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

} // namespace facebook::fboss::utility
