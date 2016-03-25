/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/ApplyThriftConfig.h"
#include "fboss/agent/test/TestUtils.h"
#include "fboss/agent/hw/mock/MockPlatform.h"
#include "fboss/agent/state/AclMap.h"
#include "fboss/agent/state/AclEntry.h"
#include "fboss/agent/state/SwitchState.h"

#include <gtest/gtest.h>

using namespace facebook::fboss;
using std::make_pair;
using std::make_shared;
using std::shared_ptr;

TEST(Acl, applyConfig) {
  MockPlatform platform;
  auto stateV0 = make_shared<SwitchState>();
  auto aclEntry = make_shared<AclEntry>(AclEntryID(0));
  stateV0->addAcl(aclEntry);
  auto aclV0 = stateV0->getAcl(AclEntryID(0));
  EXPECT_EQ(0, aclV0->getGeneration());
  EXPECT_FALSE(aclV0->isPublished());
  EXPECT_EQ(AclEntryID(0), aclV0->getID());

  aclV0->publish();
  EXPECT_TRUE(aclV0->isPublished());

  cfg::SwitchConfig config;
  config.acls.resize(1);
  config.acls[0].id = 100;
  config.acls[0].action = cfg::AclAction::DENY;
  config.acls[0].__isset.srcIp = true;
  config.acls[0].__isset.dstIp = true;
  config.acls[0].srcIp = "192.168.0.1";
  config.acls[0].dstIp = "192.168.0.0/24";
  config.acls[0].__isset.srcPort = true;
  config.acls[0].srcPort = 5;
  config.acls[0].__isset.dstPort = true;
  config.acls[0].dstPort = 8;

  auto stateV1 = publishAndApplyConfig(stateV0, &config, &platform);
  EXPECT_NE(nullptr, stateV1);
  auto aclV1 = stateV1->getAcl(AclEntryID(100));
  ASSERT_NE(nullptr, aclV1);
  EXPECT_NE(aclV0, aclV1);

  EXPECT_EQ(AclEntryID(100), aclV1->getID());
  EXPECT_EQ(cfg::AclAction::DENY, aclV1->getAction());
  EXPECT_EQ(5, aclV1->getSrcPort());
  EXPECT_EQ(8, aclV1->getDstPort());
  EXPECT_FALSE(aclV1->isPublished());

  config.acls[0].dstIp = "invalid address";
  EXPECT_THROW(publishAndApplyConfig(
    stateV1, &config, &platform), folly::IPAddressFormatException);

  config.acls[0].id = 200;
  config.acls[0].__isset.dstIp = false;
  auto stateV2 = publishAndApplyConfig(stateV1, &config, &platform);
  EXPECT_NE(nullptr, stateV2);
  auto aclV2 = stateV2->getAcl(AclEntryID(200));
  // We should handle the field removal correctly
  EXPECT_NE(nullptr, aclV2);
  EXPECT_FALSE(aclV2->getDstIp().first);

  // Non-existent entry
  auto acl2V2 = stateV2->getAcl(AclEntryID(100));
  ASSERT_EQ(nullptr, acl2V2);
}

TEST(Acl, stateDelta) {
  MockPlatform platform;
  auto stateV0 = make_shared<SwitchState>();

  cfg::SwitchConfig config;
  config.acls.resize(3);
  config.acls[0].id = 100;
  config.acls[0].action = cfg::AclAction::DENY;
  config.acls[0].__isset.srcIp = true;
  config.acls[0].srcIp = "192.168.0.1";
  config.acls[1].id = 200;
  config.acls[1].action = cfg::AclAction::PERMIT;
  config.acls[1].__isset.srcIp = true;
  config.acls[1].srcIp = "192.168.0.2";
  config.acls[2].id = 300;
  config.acls[2].action = cfg::AclAction::DENY;
  config.acls[2].__isset.srcIp = true;
  config.acls[2].srcIp = "192.168.0.3";
  config.acls[2].__isset.l4SrcPort = true;
  config.acls[2].l4SrcPort = 80;
  config.acls[2].__isset.srcPort = true;
  config.acls[2].srcPort = 5;
  config.acls[2].__isset.dstPort = true;
  config.acls[2].dstPort = 8;

  auto stateV1 = publishAndApplyConfig(stateV0, &config, &platform);
  auto stateV2 = publishAndApplyConfig(stateV1, &config, &platform);
  EXPECT_EQ(stateV2, nullptr);

  // Only change one action
  config.acls[0].action = cfg::AclAction::PERMIT;
  auto stateV3 = publishAndApplyConfig(stateV1, &config, &platform);
  StateDelta delta13(stateV1, stateV3);
  auto aclDelta13 = delta13.getAclsDelta();
  auto iter = aclDelta13.begin();
  EXPECT_EQ(iter->getOld()->getAction(), cfg::AclAction::DENY);
  EXPECT_EQ(iter->getNew()->getAction(), cfg::AclAction::PERMIT);
  ++iter;
  EXPECT_EQ(iter, aclDelta13.end());

  // Remove tail element
  config.acls.pop_back();
  auto stateV4 = publishAndApplyConfig(stateV3, &config, &platform);
  StateDelta delta34(stateV3, stateV4);
  auto aclDelta34 = delta34.getAclsDelta();
  iter = aclDelta34.begin();
  EXPECT_EQ(iter->getOld()->getL4SrcPort(), 80);
  EXPECT_EQ(iter->getOld()->getSrcPort(), 5);
  EXPECT_EQ(iter->getOld()->getDstPort(), 8);
  EXPECT_EQ(iter->getNew(), nullptr);
  ++iter;
  EXPECT_EQ(iter, aclDelta34.end());
}
