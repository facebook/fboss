/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/api/LagApi.h"
#include "fboss/agent/hw/sai/fake/FakeSai.h"

#include <folly/logging/xlog.h>

#include <gtest/gtest.h>

using namespace facebook::fboss;

class LagApiTest : public ::testing::Test {
 public:
  void SetUp() override {
    fs = FakeSai::getInstance();
    sai_api_initialize(0, nullptr);
    lagApi = std::make_unique<LagApi>();
  }
  void checkLag(const sai_object_id_t& lagId) const {
    EXPECT_EQ(lagId, fs->lm.get(lagId).id);
  }
  void checkLagMember(
      const sai_object_id_t& lagId,
      const sai_object_id_t& lagMemberId,
      const sai_object_id_t& portId) const {
    EXPECT_EQ(lagMemberId, fs->lm.getMember(lagMemberId).id);
    EXPECT_EQ(lagId, fs->lm.getMember(lagMemberId).lagId);
    EXPECT_EQ(portId, fs->lm.getMember(lagMemberId).portId);
  }
  std::shared_ptr<FakeSai> fs;
  std::unique_ptr<LagApi> lagApi;
};

TEST_F(LagApiTest, createLag) {
  auto lagId = lagApi->create({}, 0);
  checkLag(lagId);
}

TEST_F(LagApiTest, removeLag) {
  auto lagId = lagApi->create({}, 0);
  checkLag(lagId);
  lagApi->remove(lagId);
}

TEST_F(LagApiTest, createLagMember) {
  auto lagId = lagApi->create({}, 0);
  checkLag(lagId);
  auto portId = 21;
  LagTypes::MemberAttributeType lagIdAttribute =
      LagTypes::MemberAttributes::LagId(lagId);
  LagTypes::MemberAttributeType portIdAttribute =
      LagTypes::MemberAttributes::PortId(portId);
  auto lagMemberId = lagApi->createMember(
    {lagIdAttribute, portIdAttribute}, 0);
  checkLagMember(lagId, lagMemberId, portId);
}

TEST_F(LagApiTest, multipleLag) {
  auto lagId1 = lagApi->create({}, 0);
  auto lagId2 = lagApi->create({}, 0);
  checkLag(lagId1);
  checkLag(lagId2);
  EXPECT_NE(lagId1, lagId2);

  auto portId1 = 21;
  LagTypes::MemberAttributeType lagIdAttribute1 =
      LagTypes::MemberAttributes::LagId(lagId1);
  LagTypes::MemberAttributeType portIdAttribute1 =
      LagTypes::MemberAttributes::PortId(portId1);
  auto lagMemberId1 = lagApi->createMember(
    {lagIdAttribute1, portIdAttribute1}, 0);

  auto portId2 = 22;
  LagTypes::MemberAttributeType lagIdAttribute2 =
      LagTypes::MemberAttributes::LagId(lagId2);
  LagTypes::MemberAttributeType portIdAttribute2 =
      LagTypes::MemberAttributes::PortId(portId2);
  auto lagMemberId2 = lagApi->createMember(
    {lagIdAttribute2, portIdAttribute2}, 0);
  checkLagMember(lagId1, lagMemberId1, portId1);
  checkLagMember(lagId2, lagMemberId2, portId2);

}

TEST_F(LagApiTest, removeLagMember) {
  auto lagId = lagApi->create({}, 0);
  checkLag(lagId);
  auto portId = 21;
  LagTypes::MemberAttributeType lagIdAttribute =
      LagTypes::MemberAttributes::LagId(lagId);
  LagTypes::MemberAttributeType portIdAttribute =
      LagTypes::MemberAttributes::PortId(portId);
  auto lagMemberId = lagApi->createMember(
    {lagIdAttribute, portIdAttribute}, 0);
  checkLagMember(lagId, lagMemberId, portId);
  lagApi->removeMember(lagMemberId);
}

TEST_F(LagApiTest, multipleLagMembers) {
  auto lagId = lagApi->create({}, 0);
  checkLag(lagId);
  std::vector<int> portIds = {21, 22, 23, 24};

  LagTypes::MemberAttributeType lagIdAttribute =
      LagTypes::MemberAttributes::LagId(lagId);
  for (auto portId : portIds) {
    LagTypes::MemberAttributeType portIdAttribute =
        LagTypes::MemberAttributes::PortId(portId);
    auto lagMemberId = lagApi->createMember(
      {lagIdAttribute, portIdAttribute}, 0);
    checkLagMember(lagId, lagMemberId, portId);
  }
}

TEST_F(LagApiTest, getLagMemberAttribute) {
  auto lagId = lagApi->create({}, 0);
  checkLag(lagId);
  auto portId = 21;
  LagTypes::MemberAttributeType lagIdAttribute =
      LagTypes::MemberAttributes::LagId(lagId);
  LagTypes::MemberAttributeType portIdAttribute =
      LagTypes::MemberAttributes::PortId(portId);
  auto lagMemberId = lagApi->createMember(
    {lagIdAttribute, portIdAttribute}, 0);
  checkLagMember(lagId, lagMemberId, portId);

  auto lagIdGot = lagApi->getMemberAttribute(
      LagTypes::MemberAttributes::LagId(), lagMemberId);
  auto portIdGot = lagApi->getMemberAttribute(
      LagTypes::MemberAttributes::PortId(), lagMemberId);
  checkLagMember(lagIdGot, lagMemberId, portIdGot);
  EXPECT_EQ(lagId, lagIdGot);
  EXPECT_EQ(portId, portIdGot);
}
