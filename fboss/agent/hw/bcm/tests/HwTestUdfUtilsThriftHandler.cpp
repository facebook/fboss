/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmFieldProcessorUtils.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/hw/bcm/BcmUdfManager.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/hw/test/HwTestAclUtils.h"
#include "fboss/agent/hw/test/HwTestThriftHandler.h"
#include "fboss/agent/packet/IPProto.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/utils/AclTestUtils.h"
#include "fboss/agent/test/utils/UdfTestUtils.h"

#include <gtest/gtest.h>

namespace facebook::fboss::utility {

bool HwTestThriftHandler::validateUdfConfig(
    std::unique_ptr<::std::string> udfGroupName,
    std::unique_ptr<::std::string> udfPacketMatchName) {
  auto bcmSwitch = static_cast<const BcmSwitch*>(hwSwitch_);

  int udfGroupId;
  udfGroupId = bcmSwitch->getUdfMgr()->getBcmUdfGroupId(*udfGroupName.get());
  /* get udf info */
  bcm_udf_t udfInfo;
  bcm_udf_t_init(&udfInfo);
  auto rv = bcm_udf_get(bcmSwitch->getUnit(), udfGroupId, &udfInfo);
  if (rv < 0) {
    XLOG(ERR) << "Unable to get udfInfo for udfGroupId: " << udfGroupId;
    return false;
  }

  if (udfInfo.layer != bcmUdfLayerL4OuterHeader) {
    XLOG(ERR) << "udfInfo.layer: Actual: " << udfInfo.layer
              << " Expected: " << bcmUdfLayerL4OuterHeader;
    return false;
  }
  if (*udfGroupName.get() == utility::kUdfHashDstQueuePairGroupName) {
    if (udfInfo.start != utility::kUdfHashDstQueuePairStartOffsetInBytes * 8) {
      XLOG(ERR) << "udfInfo.start: Actual: " << udfInfo.start << " Expected "
                << utility::kUdfHashDstQueuePairStartOffsetInBytes * 8;
      return false;
    }

    if (udfInfo.width != utility::kUdfHashDstQueuePairFieldSizeInBytes * 8) {
      XLOG(ERR) << "udfInfo.width: Actual: " << udfInfo.width << " Expected "
                << utility::kUdfHashDstQueuePairFieldSizeInBytes * 8;
      return false;
    }
  } else if (*udfGroupName.get() == utility::kUdfAclRoceOpcodeGroupName) {
    if (udfInfo.start != utility::kUdfAclRoceOpcodeStartOffsetInBytes * 8) {
      XLOG(ERR) << "udfInfo.start: Actual: " << udfInfo.start << " Expected "
                << utility::kUdfAclRoceOpcodeStartOffsetInBytes * 8;
      return false;
    }
    if (udfInfo.width != utility::kUdfAclRoceOpcodeFieldSizeInBytes * 8) {
      XLOG(ERR) << "udfInfo.width: Actual: " << udfInfo.width << " Expected "
                << utility::kUdfAclRoceOpcodeFieldSizeInBytes * 8;
      return false;
    }
  }

  const int udfPacketMatcherId =
      bcmSwitch->getUdfMgr()->getBcmUdfPacketMatcherId(
          *udfPacketMatchName.get());
  /* get udf pkt info */
  bcm_udf_pkt_format_info_t pktFormat;
  bcm_udf_pkt_format_info_t_init(&pktFormat);
  rv = bcm_udf_pkt_format_info_get(
      bcmSwitch->getUnit(), udfPacketMatcherId, &pktFormat);
  if (rv < 0) {
    XLOG(ERR) << "Unable to get pkt_format for udfPacketMatcherId: "
              << udfPacketMatcherId;
    return false;
  }

  if (pktFormat.ip_protocol != static_cast<int>(IP_PROTO::IP_PROTO_UDP)) {
    XLOG(ERR) << "pktFormat.ip_protocol: Actual: " << pktFormat.ip_protocol
              << " Expected: " << static_cast<int>(IP_PROTO::IP_PROTO_UDP);
    return false;
  }
  if (pktFormat.l4_dst_port != utility::kUdfL4DstPort) {
    XLOG(ERR) << "pktFormat.l4_dst_port: Actual: " << pktFormat.l4_dst_port
              << " Expected: " << utility::kUdfL4DstPort;
    return false;
  }
  return true;
}

bool HwTestThriftHandler::validateRemoveUdfGroup(
    std::unique_ptr<::std::string> udfGroupName,
    int udfGroupId) {
  auto bcmSwitch = static_cast<const BcmSwitch*>(hwSwitch_);

  try {
    getHwUdfGroupId(std::move(udfGroupName));
    XLOG(ERR) << "getHwUdfGroupId: Expected to throw an error but did not";
    return false;
  } catch (const FbossError&) {
    return true;
  }
  // Verify SDK removed udf group
  bcm_udf_t udfInfo;
  bcm_udf_t_init(&udfInfo);
  auto rv = bcm_udf_get(bcmSwitch->getUnit(), udfGroupId, &udfInfo);
  if (hwSwitch_->getPlatform()->getAsic()->getAsicType() !=
      cfg::AsicType::ASIC_TYPE_FAKE) {
    // expect bcmCheckError to throw an error
    try {
      bcmCheckError(rv, "Unable to get udfInfo for udfGroupId: ", udfGroupId);
      XLOG(ERR) << "bcmCheckError: Expected to throw an error but did not";
      return false;
    } catch (const FbossError&) {
      return true;
    }
  }
  return false;
}

bool HwTestThriftHandler::validateRemoveUdfPacketMatcher(
    std::unique_ptr<::std::string> udfPackeMatchName,
    int32_t udfPacketMatcherId) {
  // ensure that the udf packet matcher is removed from the switch
  // and getHwUdfPacketMatcherId throws an error
  try {
    getHwUdfPacketMatcherId(std::move(udfPackeMatchName));
    XLOG(ERR)
        << "getHwUdfPacketMatcherId: Expected to throw an error but did not";
    return false;
  } catch (const FbossError&) {
    return true;
  }
  // Verify SDK removed udf packet matcher
  bcm_udf_pkt_format_info_t pktFormat;
  bcm_udf_pkt_format_info_t_init(&pktFormat);
  auto rv = bcm_udf_pkt_format_info_get(
      static_cast<const BcmSwitch*>(hwSwitch_)->getUnit(),
      udfPacketMatcherId,
      &pktFormat);
  if (hwSwitch_->getPlatform()->getAsic()->getAsicType() !=
      cfg::AsicType::ASIC_TYPE_FAKE) {
    // expect bcmCheckError to throw an error
    try {
      bcmCheckError(
          rv,
          "Unable to get pkt_format for udfPacketMatcherId: ",
          udfPacketMatcherId);
      XLOG(ERR) << "bcmCheckError: Expected to throw an error but did not";
      return false;
    } catch (const FbossError&) {
      return true;
    }
  }
  return false;
}

int32_t HwTestThriftHandler::getHwUdfGroupId(
    std::unique_ptr<::std::string> udfGroupName) {
  return static_cast<const BcmSwitch*>(hwSwitch_)
      ->getUdfMgr()
      ->getBcmUdfGroupId(*udfGroupName.get());
}

int32_t HwTestThriftHandler::getHwUdfPacketMatcherId(
    std::unique_ptr<::std::string> udfPackeMatchName) {
  return static_cast<const BcmSwitch*>(hwSwitch_)
      ->getUdfMgr()
      ->getBcmUdfPacketMatcherId(*udfPackeMatchName.get());
}

bool HwTestThriftHandler::validateUdfIdsInQset(int aclGroupId, bool isSet) {
  const auto& udf_ids = getUdfQsetIds(
      static_cast<const BcmSwitch*>(hwSwitch_)->getUnit(), aclGroupId);
  if (isSet) {
    if (udf_ids.size() == 0) {
      XLOG(ERR) << "udf_ids.size(): Actual: " << udf_ids.size()
                << " Expected: " << " > 0";
      return false;
    }
  } else {
    if (udf_ids.size() != 0) {
      XLOG(ERR) << "udf_ids.size(): Actual: " << udf_ids.size()
                << " Expected: " << " 0";
      return false;
    }
  }
  return true;
}

bool HwTestThriftHandler::validateUdfAclRoceOpcodeConfig(
    std::unique_ptr<::facebook::fboss::state::SwitchState> curState) {
  if (!HwTestThriftHandler::isDefaultAclTableEnabled()) {
    XLOG(ERR) << "isDefaultAclTableEnabled: Actual: " << false
              << " Expected: " << true;
    return false;
  }
  std::shared_ptr<SwitchState> state = SwitchState::fromThrift(*curState.get());
  utility::checkSwHwAclMatch(hwSwitch_, state, utility::kUdfAclRoceOpcodeName);

  std::vector<cfg::CounterType> types =
      utility::getAclCounterTypes({hwSwitch_->getPlatform()->getAsic()});
  isStatProgrammedInDefaultAclTable(
      std::make_unique<std::vector<std::string>>(
          std::initializer_list<std::string>{"kUdfAclRoceOpcodeName"}),
      std::make_unique<std::string>(utility::kUdfAclRoceOpcodeStats),
      std::make_unique<std::vector<cfg::CounterType>>(types));

  // Verify that UdfGroupIds are there in Qset
  validateUdfIdsInQset(
      hwSwitch_->getPlatform()->getAsic()->getDefaultACLGroupID(), true);
  return true;
}

} // namespace facebook::fboss::utility
