/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <glog/logging.h>
#include <gtest/gtest.h>

#include <optional>
#include "fboss/agent/FbossError.h"

#include "fboss/agent/hw/bcm/BcmFieldProcessorFBConvertors.h"
#include "fboss/agent/hw/bcm/tests/BcmUnitTestUtils.h"

extern "C" {
#include <bcm/field.h>
}

extern "C" {
struct ibde_t;
ibde_t* bde;
}

namespace facebook::fboss {

using namespace facebook::fboss::utility;

TEST(FPBcmConvertors, priorityToFromBcm) {
  for (auto pri : {10, 99, 1001}) {
    EXPECT_EQ(pri, hwPriorityToSwPriority(swPriorityToHwPriority(pri)));
  }
}

TEST(FPBcmConvertors, cfgIpFragToFromBcm) {
  for (auto cfgIpFrag : apache::thrift::TEnumTraits<cfg::IpFragMatch>::values) {
    auto bcmIpFrag = cfgIpFragToBcmIpFrag(cfgIpFrag);
    EXPECT_EQ(cfgIpFrag, bcmIpFragToCfgIpFrag(bcmIpFrag));
  }
  EXPECT_THROW(bcmIpFragToCfgIpFrag(bcmFieldIpFragCount), FbossError);
}

TEST(FPBcmConvertors, ipTypeToFromBcm) {
  for (auto cfgIpType :
       {cfg::IpType::ANY,
        cfg::IpType::IP,
        cfg::IpType::IP4,
        cfg::IpType::IP6}) {
    auto bcmIpType = cfgIpTypeToBcmIpType(cfgIpType);
    EXPECT_EQ(cfgIpType, bcmIpTypeToCfgIpType(bcmIpType));
  }
  EXPECT_THROW(bcmIpTypeToCfgIpType(bcmFieldIpTypeNonIp), FbossError);
}

TEST(FPBcmConvertors, icmpTypeNoCode) {
  std::array<uint8_t, 4> icmpTypes = {
      0, // v4 echo
      8, // v4 echo reply
      128, // v6 echo
      129 // v6 echo reply
  };
  for (auto icmpType : icmpTypes) {
    std::optional<uint8_t> type, code;
    type = icmpType;
    uint16_t bcmCode, bcmMask;
    cfgIcmpTypeCodeToBcmIcmpCodeMask(type, code, &bcmCode, &bcmMask);
    EXPECT_EQ(0xFF00, bcmMask);
    EXPECT_EQ(static_cast<uint16_t>(*type) << 8, bcmCode);
    std::optional<uint8_t> fromBcmType, fromBcmCode;
    bcmIcmpTypeCodeToCfgIcmpTypeAndCode(
        bcmCode, bcmMask, &fromBcmType, &fromBcmCode);
    EXPECT_FALSE(fromBcmCode.has_value());
    EXPECT_EQ(icmpType, *fromBcmType);
  }
}

TEST(FPBcmConvertors, icmpTypeAndCode) {
  std::array<std::pair<uint8_t, uint8_t>, 4> icmpTypeAndCodes = {{
      {3, 0}, // v4 dst unreachable, net unreachable
      {3, 1}, // v4 dst unreachable, host unreachable
      {1, 3}, // v6 dst unreachable, address unreachable
      {1, 4}, // v6 dst unreachable, port unreachable
  }};
  for (auto icmpTypeAndCode : icmpTypeAndCodes) {
    std::optional<uint8_t> type, code;
    std::tie(type, code) =
        std::make_tuple(icmpTypeAndCode.first, icmpTypeAndCode.second);
    uint16_t bcmCode, bcmMask;
    cfgIcmpTypeCodeToBcmIcmpCodeMask(type, code, &bcmCode, &bcmMask);
    EXPECT_EQ(0xFFFF, bcmMask);
    EXPECT_EQ(static_cast<uint16_t>(*type) << 8 | *code, bcmCode);
    std::optional<uint8_t> fromBcmType, fromBcmCode;
    bcmIcmpTypeCodeToCfgIcmpTypeAndCode(
        bcmCode, bcmMask, &fromBcmType, &fromBcmCode);
    EXPECT_TRUE(fromBcmCode.has_value());
    EXPECT_EQ(icmpTypeAndCode.first, *fromBcmType);
    EXPECT_EQ(icmpTypeAndCode.second, *fromBcmCode);
  }
}

TEST(FPBcmConvertors, cfgCountertypeToFromBcm) {
  for (auto cfgCounterType :
       apache::thrift::TEnumTraits<cfg::CounterType>::values) {
    auto bcmCounterType = cfgCounterTypeToBcmCounterType(cfgCounterType);
    EXPECT_EQ(cfgCounterType, bcmCounterTypeToCfgCounterType(bcmCounterType));
  }
  EXPECT_THROW(bcmCounterTypeToCfgCounterType(bcmFieldStatCount), FbossError);
}

} // namespace facebook::fboss
