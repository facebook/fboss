/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/sai/switch/SaiUdfManager.h"
#include "fboss/agent/hw/sai/switch/tests/ManagerTestBase.h"
#include "fboss/agent/packet/Ethertype.h"
#include "fboss/agent/packet/IPProto.h"
#include "fboss/agent/state/UdfPacketMatcher.h"
#include "fboss/agent/types.h"

#include <string>

using namespace facebook::fboss;

class UdfManagerTest : public ManagerTestBase {
 public:
  void SetUp() override {
    ManagerTestBase::SetUp();
  }

  uint16_t kIpv4Type() {
    return static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV4);
  }

  uint16_t kIpv6Type() {
    return static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV6);
  }

  uint8_t kUdpType() {
    return static_cast<uint8_t>(IP_PROTO::IP_PROTO_UDP);
  }

  uint8_t kTcpType() {
    return static_cast<uint8_t>(IP_PROTO::IP_PROTO_TCP);
  }

  uint16_t kUdpPort() {
    return 1234;
  }

  int kOffset() {
    return 2;
  }

  int kFieldSize() {
    return 4;
  }

  std::string kUdfMatchName() {
    return "testUdfPacketMatch";
  }

  std::string kUdfGroupName() {
    return "testUdfGroupMatch";
  }

  std::shared_ptr<UdfPacketMatcher> createUdfPacketMatcher(
      const std::string& name) {
    auto swUdfMatch = std::make_shared<UdfPacketMatcher>(name);
    swUdfMatch->setUdfl2PktType(cfg::UdfMatchL2Type::UDF_L2_PKT_TYPE_ETH);
    swUdfMatch->setUdfl3PktType(cfg::UdfMatchL3Type::UDF_L3_PKT_TYPE_IPV6);
    swUdfMatch->setUdfl4PktType(cfg::UdfMatchL4Type::UDF_L4_PKT_TYPE_UDP);
    swUdfMatch->setUdfL4DstPort(kUdpPort());
    return swUdfMatch;
  }

  std::shared_ptr<UdfGroup> createUdfGroup(
      const std::string& name,
      const std::vector<std::string>& matcherIds) {
    auto swUdfGroup = std::make_shared<UdfGroup>(name);
    swUdfGroup->setUdfBaseHeader(cfg::UdfBaseHeaderType::UDF_L2_HEADER);
    swUdfGroup->setStartOffsetInBytes(kOffset());
    swUdfGroup->setFieldSizeInBytes(kFieldSize());
    swUdfGroup->setUdfPacketMatcherIds(matcherIds);
    return swUdfGroup;
  }

  void validateUdfMatcher(UdfMatchSaiId saiUdfMatchId) {
    auto& udfApi = saiApiTable->udfApi();
    EXPECT_EQ(
        udfApi.getAttribute(
            saiUdfMatchId, SaiUdfMatchTraits::Attributes::L2Type{}),
        AclEntryFieldU16(
            std::make_pair(kIpv6Type(), SaiUdfManager::kMaskDontCare)));
    EXPECT_EQ(
        udfApi.getAttribute(
            saiUdfMatchId, SaiUdfMatchTraits::Attributes::L3Type{}),
        AclEntryFieldU8(
            std::make_pair(kUdpType(), SaiUdfManager::kMaskDontCare)));
    EXPECT_EQ(
        udfApi.getAttribute(
            saiUdfMatchId, SaiUdfMatchTraits::Attributes::L4DstPortType{}),
        AclEntryFieldU16(
            std::make_pair(kUdpPort(), SaiUdfManager::kL4PortMask)));
  }

  void validateUdfGroup(UdfGroupSaiId saiUdfGroupId) {
    auto& udfApi = saiApiTable->udfApi();
    EXPECT_EQ(
        udfApi.getAttribute(
            saiUdfGroupId, SaiUdfGroupTraits::Attributes::Type{}),
        SAI_UDF_GROUP_TYPE_HASH);
    EXPECT_EQ(
        udfApi.getAttribute(
            saiUdfGroupId, SaiUdfGroupTraits::Attributes::Length{}),
        kFieldSize());
  }

  void validateUdf(
      UdfSaiId saiUdfId,
      UdfMatchSaiId saiUdfMatchId,
      UdfGroupSaiId saiUdfGroupId) {
    auto& udfApi = saiApiTable->udfApi();
    EXPECT_EQ(
        udfApi.getAttribute(saiUdfId, SaiUdfTraits::Attributes::UdfMatchId{}),
        saiUdfMatchId);
    EXPECT_EQ(
        udfApi.getAttribute(saiUdfId, SaiUdfTraits::Attributes::UdfGroupId{}),
        saiUdfGroupId);
    EXPECT_EQ(
        udfApi.getAttribute(saiUdfId, SaiUdfTraits::Attributes::Base{}),
        SAI_UDF_BASE_L2);
    EXPECT_EQ(
        udfApi.getAttribute(saiUdfId, SaiUdfTraits::Attributes::Offset{}),
        kOffset());
  }
};

TEST_F(UdfManagerTest, createUdfMatch) {
  std::shared_ptr<UdfPacketMatcher> swUdfMatch =
      createUdfPacketMatcher(kUdfMatchName());
  auto saiUdfMatchId = saiManagerTable->udfManager().addUdfMatch(swUdfMatch);
  EXPECT_EQ(saiManagerTable->udfManager().getUdfMatchHandles().size(), 1);
  validateUdfMatcher(saiUdfMatchId);
}

TEST_F(UdfManagerTest, removeUdfMatch) {
  auto swUdfMatch = createUdfPacketMatcher(kUdfMatchName());
  auto saiUdfMatchId = saiManagerTable->udfManager().addUdfMatch(swUdfMatch);
  EXPECT_EQ(saiManagerTable->udfManager().getUdfMatchHandles().size(), 1);
  validateUdfMatcher(saiUdfMatchId);
  saiManagerTable->udfManager().removeUdfMatch(swUdfMatch);
  EXPECT_EQ(saiManagerTable->udfManager().getUdfMatchHandles().size(), 0);
}

TEST_F(UdfManagerTest, createUdfGroup) {
  auto swUdfMatch = createUdfPacketMatcher(kUdfMatchName());
  auto saiUdfMatchId = saiManagerTable->udfManager().addUdfMatch(swUdfMatch);
  EXPECT_EQ(saiManagerTable->udfManager().getUdfMatchHandles().size(), 1);
  validateUdfMatcher(saiUdfMatchId);

  auto swUdfGroup = createUdfGroup(kUdfGroupName(), {kUdfMatchName()});
  auto saiUdfGroupId = saiManagerTable->udfManager().addUdfGroup(swUdfGroup);
  EXPECT_EQ(saiManagerTable->udfManager().getUdfGroupHandles().size(), 1);
  validateUdfGroup(saiUdfGroupId);

  auto saiUdfId = saiManagerTable->udfManager()
                      .getUdfGroupHandles()
                      .at(kUdfGroupName())
                      ->udfs.at(kUdfMatchName())
                      ->udf->adapterKey();
  validateUdf(saiUdfId, saiUdfMatchId, saiUdfGroupId);
}

TEST_F(UdfManagerTest, removeUdfGroup) {
  auto swUdfMatch = createUdfPacketMatcher(kUdfMatchName());
  auto saiUdfMatchId = saiManagerTable->udfManager().addUdfMatch(swUdfMatch);
  EXPECT_EQ(saiManagerTable->udfManager().getUdfMatchHandles().size(), 1);
  validateUdfMatcher(saiUdfMatchId);

  auto swUdfGroup = createUdfGroup(kUdfGroupName(), {kUdfMatchName()});
  auto saiUdfGroupId = saiManagerTable->udfManager().addUdfGroup(swUdfGroup);
  EXPECT_EQ(saiManagerTable->udfManager().getUdfGroupHandles().size(), 1);
  validateUdfGroup(saiUdfGroupId);

  auto saiUdfId = saiManagerTable->udfManager()
                      .getUdfGroupHandles()
                      .at(kUdfGroupName())
                      ->udfs.at(kUdfMatchName())
                      ->udf->adapterKey();
  validateUdf(saiUdfId, saiUdfMatchId, saiUdfGroupId);

  saiManagerTable->udfManager().removeUdfMatch(swUdfMatch);
  EXPECT_EQ(saiManagerTable->udfManager().getUdfMatchHandles().size(), 0);
  // SaiUdf object should be removed once UdfMatch is removed.
  EXPECT_EQ(
      saiManagerTable->udfManager()
          .getUdfGroupHandles()
          .at(kUdfGroupName())
          ->udfs.size(),
      0);
  saiManagerTable->udfManager().removeUdfGroup(swUdfGroup);
  EXPECT_EQ(saiManagerTable->udfManager().getUdfGroupHandles().size(), 0);
}
