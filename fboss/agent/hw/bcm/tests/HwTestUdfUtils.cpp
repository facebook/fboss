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

} // namespace facebook::fboss::utility
