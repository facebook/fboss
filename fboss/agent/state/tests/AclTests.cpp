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
#include "fboss/agent/FbossError.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/mock/MockPlatform.h"
#include "fboss/agent/state/AclEntry.h"
#include "fboss/agent/state/AclMap.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/TestUtils.h"
#include "folly/IPAddress.h"

#include <folly/IPAddress.h>
#include <folly/MacAddress.h>
#include <gtest/gtest.h>

using namespace facebook::fboss;
using folly::MacAddress;
using std::make_pair;
using std::make_shared;
using std::shared_ptr;

DECLARE_bool(enable_acl_table_group);

namespace {
// We offset the start point in ApplyThriftConfig
constexpr auto kAclStartPriority = 100000;
} // namespace

TEST(Acl, applyConfig) {
  FLAGS_enable_acl_table_group = false;
  auto platform = createMockPlatform();
  auto stateV0 = make_shared<SwitchState>();
  auto aclEntry = make_shared<AclEntry>(0, "acl0");
  stateV0->addAcl(aclEntry);
  auto aclV0 = stateV0->getAcl("acl0");
  EXPECT_EQ(0, aclV0->getGeneration());
  EXPECT_FALSE(aclV0->isPublished());
  EXPECT_EQ(0, aclV0->getPriority());
  stateV0->registerPort(PortID(1), "port1");

  aclV0->publish();
  EXPECT_TRUE(aclV0->isPublished());

  cfg::SwitchConfig config;
  config.ports_ref()->resize(1);
  preparedMockPortConfig(config.ports_ref()[0], 1);

  config.acls_ref()->resize(1);
  *config.acls_ref()[0].name_ref() = "acl1";
  *config.acls_ref()[0].actionType_ref() = cfg::AclActionType::DENY;
  config.acls_ref()[0].srcIp_ref() = "192.168.0.1";
  config.acls_ref()[0].dstIp_ref() = "192.168.0.0/24";
  config.acls_ref()[0].srcPort_ref() = 5;
  config.acls_ref()[0].dstPort_ref() = 8;

  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  EXPECT_NE(nullptr, stateV1);
  auto aclV1 = stateV1->getAcl("acl1");
  ASSERT_NE(nullptr, aclV1);
  EXPECT_NE(aclV0, aclV1);

  EXPECT_EQ(0 + kAclStartPriority, aclV1->getPriority());
  EXPECT_EQ(cfg::AclActionType::DENY, aclV1->getActionType());
  EXPECT_EQ(5, aclV1->getSrcPort());
  EXPECT_EQ(8, aclV1->getDstPort());

  EXPECT_FALSE(aclV1->isPublished());

  config.acls_ref()[0].dstIp_ref() = "invalid address";
  EXPECT_THROW(
      publishAndApplyConfig(stateV1, &config, platform.get()),
      folly::IPAddressFormatException);

  *config.acls_ref()[0].name_ref() = "acl2";
  config.acls_ref()[0].dstIp_ref().reset();
  auto stateV2 = publishAndApplyConfig(stateV1, &config, platform.get());
  EXPECT_NE(nullptr, stateV2);
  auto aclV2 = stateV2->getAcl("acl2");
  // We should handle the field removal correctly
  EXPECT_NE(nullptr, aclV2);
  EXPECT_FALSE(aclV2->getDstIp().first);

  // Nothing references this permit acl, so it shouldn't be added yet
  config.acls_ref()->resize(2);
  *config.acls_ref()[1].name_ref() = "acl22";
  *config.acls_ref()[1].actionType_ref() = cfg::AclActionType::PERMIT;
  auto stateV22 = publishAndApplyConfig(stateV2, &config, platform.get());
  EXPECT_EQ(nullptr, stateV22);

  // Non-existent entry
  auto acl2V2 = stateV2->getAcl("something");
  ASSERT_EQ(nullptr, acl2V2);

  cfg::SwitchConfig configV1;
  configV1.ports_ref()->resize(1);
  preparedMockPortConfig(configV1.ports_ref()[0], 1);

  configV1.acls_ref()->resize(1);
  *configV1.acls_ref()[0].name_ref() = "acl3";

  configV1.acls_ref()[0].l4SrcPort_ref() = 1;
  configV1.acls_ref()[0].l4DstPort_ref() = 3;
  // Make sure it's used so that it isn't ignored
  configV1.dataPlaneTrafficPolicy_ref() = cfg::TrafficPolicyConfig();
  configV1.dataPlaneTrafficPolicy_ref()->matchToAction_ref()->resize(
      1, cfg::MatchToAction());
  *configV1.dataPlaneTrafficPolicy_ref()->matchToAction_ref()[0].matcher_ref() =
      "acl3";
  *configV1.dataPlaneTrafficPolicy_ref()->matchToAction_ref()[0].action_ref() =
      cfg::MatchAction();
  configV1.dataPlaneTrafficPolicy_ref()
      ->matchToAction_ref()[0]
      .action_ref()
      ->sendToQueue_ref() = cfg::QueueMatchAction();
  *configV1.dataPlaneTrafficPolicy_ref()
       ->matchToAction_ref()[0]
       .action_ref()
       ->sendToQueue_ref()
       ->queueId_ref() = 1;

  auto stateV3 = publishAndApplyConfig(stateV2, &configV1, platform.get());
  EXPECT_NE(nullptr, stateV3);
  auto acls = stateV3->getAcls();
  auto aclV3 = stateV3->getAcl("acl3");
  ASSERT_NE(nullptr, aclV3);
  EXPECT_NE(aclV0, aclV3);
  EXPECT_EQ(0 + kAclStartPriority, aclV3->getPriority());
  EXPECT_EQ(cfg::AclActionType::PERMIT, aclV3->getActionType());
  EXPECT_FALSE(!aclV3->getL4SrcPort());
  EXPECT_EQ(aclV3->getL4SrcPort().value(), 1);
  EXPECT_FALSE(!aclV3->getL4DstPort());
  EXPECT_EQ(aclV3->getL4DstPort().value(), 3);

  // test max > 65535 case
  configV1.acls_ref()[0].l4SrcPort_ref() = 65536;
  EXPECT_THROW(
      publishAndApplyConfig(stateV3, &configV1, platform.get()), FbossError);

  cfg::SwitchConfig configV2;
  configV2.ports_ref()->resize(1);
  preparedMockPortConfig(configV2.ports_ref()[0], 1);
  configV2.acls_ref()->resize(1);
  *configV2.acls_ref()[0].name_ref() = "acl3";
  // Make sure it's used so that it isn't ignored
  configV2.dataPlaneTrafficPolicy_ref() = cfg::TrafficPolicyConfig();
  configV2.dataPlaneTrafficPolicy_ref()->matchToAction_ref()->resize(
      1, cfg::MatchToAction());
  *configV2.dataPlaneTrafficPolicy_ref()->matchToAction_ref()[0].matcher_ref() =
      "acl3";
  *configV2.dataPlaneTrafficPolicy_ref()->matchToAction_ref()[0].action_ref() =
      cfg::MatchAction();
  configV2.dataPlaneTrafficPolicy_ref()
      ->matchToAction_ref()[0]
      .action_ref()
      ->sendToQueue_ref() = cfg::QueueMatchAction();
  *configV2.dataPlaneTrafficPolicy_ref()
       ->matchToAction_ref()[0]
       .action_ref()
       ->sendToQueue_ref()
       ->queueId_ref() = 1;

  // set the ip frag option
  configV2.acls_ref()[0].ipFrag_ref() = cfg::IpFragMatch::MATCH_NOT_FRAGMENTED;

  auto stateV5 = publishAndApplyConfig(stateV3, &configV2, platform.get());
  EXPECT_NE(nullptr, stateV5);
  auto aclV5 = stateV5->getAcl("acl3");
  EXPECT_NE(nullptr, aclV5);
  EXPECT_EQ(aclV5->getIpFrag().value(), cfg::IpFragMatch::MATCH_NOT_FRAGMENTED);

  // set dst Mac
  auto dstMacStr = "01:01:01:01:01:01";
  configV2.acls_ref()[0].dstMac_ref() = dstMacStr;

  auto stateV6 = publishAndApplyConfig(stateV5, &configV2, platform.get());
  EXPECT_NE(nullptr, stateV6);
  auto aclV6 = stateV6->getAcl("acl3");
  EXPECT_NE(nullptr, aclV6);

  EXPECT_EQ(MacAddress(dstMacStr), aclV6->getDstMac());
  // Remove src, dst Mac
  configV2.acls_ref()[0].dstMac_ref().reset();

  auto stateV7 = publishAndApplyConfig(stateV6, &configV2, platform.get());
  EXPECT_NE(nullptr, stateV7);
  auto aclV7 = stateV7->getAcl("acl3");
  EXPECT_NE(nullptr, aclV7);

  EXPECT_FALSE(aclV7->getDstMac());

  // set packet lookup result matching
  configV2.acls_ref()[0].packetLookupResult_ref() =
      cfg::PacketLookupResultType::PACKET_LOOKUP_RESULT_MPLS_NO_MATCH;
  auto stateV8 = publishAndApplyConfig(stateV7, &configV2, platform.get());
  EXPECT_NE(nullptr, stateV5);
  auto aclV8 = stateV8->getAcl("acl3");
  EXPECT_NE(nullptr, aclV8);
  EXPECT_EQ(
      aclV8->getPacketLookupResult().value(),
      cfg::PacketLookupResultType::PACKET_LOOKUP_RESULT_MPLS_NO_MATCH);

  // remove packet lookup result matching
  configV2.acls_ref()[0].packetLookupResult_ref().reset();
  auto stateV9 = publishAndApplyConfig(stateV8, &configV2, platform.get());
  EXPECT_NE(nullptr, stateV9);
  auto aclV9 = stateV9->getAcl("acl3");
  EXPECT_NE(nullptr, aclV9);
  EXPECT_FALSE(aclV9->getPacketLookupResult());
}

TEST(Acl, stateDelta) {
  FLAGS_enable_acl_table_group = false;
  auto platform = createMockPlatform();
  auto stateV0 = make_shared<SwitchState>();

  cfg::SwitchConfig config;
  config.acls_ref()->resize(4);
  *config.acls_ref()[0].name_ref() = "acl0";
  *config.acls_ref()[0].actionType_ref() = cfg::AclActionType::DENY;
  config.acls_ref()[0].srcIp_ref() = "192.168.0.1";
  config.acls_ref()[0].srcPort_ref() = 1;
  *config.acls_ref()[1].name_ref() = "acl1";
  config.acls_ref()[1].srcIp_ref() = "192.168.0.2";
  *config.acls_ref()[1].actionType_ref() = cfg::AclActionType::DENY;
  *config.acls_ref()[2].name_ref() = "acl3";
  *config.acls_ref()[2].actionType_ref() = cfg::AclActionType::DENY;
  config.acls_ref()[2].srcIp_ref() = "192.168.0.3";
  config.acls_ref()[2].srcPort_ref() = 5;
  config.acls_ref()[2].dstPort_ref() = 8;
  *config.acls_ref()[3].name_ref() = "acl4";
  *config.acls_ref()[3].actionType_ref() = cfg::AclActionType::DENY;
  config.acls_ref()[3].srcIp_ref() = "192.168.0.4";
  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  auto stateV2 = publishAndApplyConfig(stateV1, &config, platform.get());
  EXPECT_EQ(stateV2, nullptr);

  // Only change one action
  config.acls_ref()[0].srcPort_ref() = 2;
  auto stateV3 = publishAndApplyConfig(stateV1, &config, platform.get());
  StateDelta delta13(stateV1, stateV3);
  auto aclDelta13 = delta13.getAclsDelta();
  auto iter = aclDelta13.begin();
  EXPECT_EQ(iter->getOld()->getSrcPort(), 1);
  EXPECT_EQ(iter->getNew()->getSrcPort(), 2);
  ++iter;
  EXPECT_EQ(iter, aclDelta13.end());

  // Remove tail element
  config.acls_ref()->pop_back();
  auto stateV4 = publishAndApplyConfig(stateV3, &config, platform.get());
  StateDelta delta34(stateV3, stateV4);
  auto aclDelta34 = delta34.getAclsDelta();
  iter = aclDelta34.begin();
  EXPECT_EQ(
      iter->getOld()->getSrcIp(),
      folly::CIDRNetwork(folly::IPAddress("192.168.0.4"), 32));
  EXPECT_EQ(iter->getNew(), nullptr);
  ++iter;
  EXPECT_EQ(iter, aclDelta34.end());
  // Remove tail element
  config.acls_ref()->pop_back();
  auto stateV5 = publishAndApplyConfig(stateV4, &config, platform.get());
  StateDelta delta45(stateV4, stateV5);
  auto aclDelta45 = delta45.getAclsDelta();
  iter = aclDelta45.begin();
  EXPECT_EQ(
      iter->getOld()->getSrcIp(),
      folly::CIDRNetwork(folly::IPAddress("192.168.0.3"), 32));
  EXPECT_EQ(iter->getOld()->getSrcPort(), 5);
  EXPECT_EQ(iter->getOld()->getDstPort(), 8);
  EXPECT_EQ(iter->getOld()->getActionType(), cfg::AclActionType::DENY);
  EXPECT_EQ(iter->getNew(), nullptr);
  ++iter;
  EXPECT_EQ(iter, aclDelta45.end());
}

TEST(Acl, Icmp) {
  FLAGS_enable_acl_table_group = false;
  auto platform = createMockPlatform();
  auto stateV0 = make_shared<SwitchState>();

  cfg::SwitchConfig config;
  config.acls_ref()->resize(1);
  *config.acls_ref()[0].name_ref() = "acl1";
  *config.acls_ref()[0].actionType_ref() = cfg::AclActionType::DENY;
  config.acls_ref()[0].proto_ref() = 58;
  config.acls_ref()[0].icmpType_ref() = 128;
  config.acls_ref()[0].icmpCode_ref() = 0;

  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  EXPECT_NE(nullptr, stateV1);
  auto aclV1 = stateV1->getAcl("acl1");
  ASSERT_NE(nullptr, aclV1);
  EXPECT_EQ(0 + kAclStartPriority, aclV1->getPriority());
  EXPECT_EQ(cfg::AclActionType::DENY, aclV1->getActionType());
  EXPECT_EQ(128, aclV1->getIcmpType().value());
  EXPECT_EQ(0, aclV1->getIcmpCode().value());

  // test config exceptions
  config.acls_ref()[0].proto_ref() = 4;
  EXPECT_THROW(
      publishAndApplyConfig(stateV1, &config, platform.get()), FbossError);
  config.acls_ref()[0].proto_ref().reset();
  EXPECT_THROW(
      publishAndApplyConfig(stateV1, &config, platform.get()), FbossError);
  config.acls_ref()[0].proto_ref() = 58;
  config.acls_ref()[0].icmpType_ref().reset();
  EXPECT_THROW(
      publishAndApplyConfig(stateV1, &config, platform.get()), FbossError);
}

TEST(Acl, aclModifyUnpublished) {
  auto state = make_shared<SwitchState>();
  auto aclMap = state->getAcls();
  EXPECT_EQ(aclMap.get(), aclMap->modify(&state));
}

TEST(Acl, aclModifyPublished) {
  auto state = make_shared<SwitchState>();
  state->publish();
  auto aclMap = state->getAcls();
  EXPECT_NE(aclMap.get(), aclMap->modify(&state));
}

TEST(Acl, AclGeneration) {
  FLAGS_enable_acl_table_group = false;
  auto platform = createMockPlatform();
  auto stateV0 = make_shared<SwitchState>();
  stateV0->registerPort(PortID(1), "port1");
  stateV0->registerPort(PortID(2), "port2");

  cfg::SwitchConfig config;
  config.ports_ref()->resize(2);
  preparedMockPortConfig(config.ports_ref()[0], 1);
  preparedMockPortConfig(config.ports_ref()[1], 2);

  config.acls_ref()->resize(5);
  *config.acls_ref()[0].name_ref() = "acl1";
  *config.acls_ref()[0].actionType_ref() = cfg::AclActionType::DENY;
  config.acls_ref()[0].srcIp_ref() = "192.168.0.1";
  config.acls_ref()[0].dstIp_ref() = "192.168.0.0/24";
  config.acls_ref()[0].srcPort_ref() = 5;
  config.acls_ref()[0].dstPort_ref() = 8;
  *config.acls_ref()[1].name_ref() = "acl2";
  config.acls_ref()[1].dscp_ref() = 16;
  *config.acls_ref()[2].name_ref() = "acl3";
  config.acls_ref()[2].dscp_ref() = 16;
  *config.acls_ref()[3].name_ref() = "acl4";
  *config.acls_ref()[3].actionType_ref() = cfg::AclActionType::DENY;
  *config.acls_ref()[4].name_ref() = "acl5";
  config.acls_ref()[4].srcIp_ref() = "2401:db00:21:7147:face:0:7:0/128";
  config.acls_ref()[4].srcPort_ref() = 5;

  config.dataPlaneTrafficPolicy_ref() = cfg::TrafficPolicyConfig();
  config.dataPlaneTrafficPolicy_ref()->matchToAction_ref()->resize(
      3, cfg::MatchToAction());
  *config.dataPlaneTrafficPolicy_ref()->matchToAction_ref()[0].matcher_ref() =
      "acl2";
  *config.dataPlaneTrafficPolicy_ref()->matchToAction_ref()[0].action_ref() =
      cfg::MatchAction();
  config.dataPlaneTrafficPolicy_ref()
      ->matchToAction_ref()[0]
      .action_ref()
      ->sendToQueue_ref() = cfg::QueueMatchAction();
  *config.dataPlaneTrafficPolicy_ref()
       ->matchToAction_ref()[0]
       .action_ref()
       ->sendToQueue_ref()
       ->queueId_ref() = 1;
  *config.dataPlaneTrafficPolicy_ref()->matchToAction_ref()[1].matcher_ref() =
      "acl3";
  *config.dataPlaneTrafficPolicy_ref()->matchToAction_ref()[1].action_ref() =
      cfg::MatchAction();
  config.dataPlaneTrafficPolicy_ref()
      ->matchToAction_ref()[1]
      .action_ref()
      ->sendToQueue_ref() = cfg::QueueMatchAction();
  *config.dataPlaneTrafficPolicy_ref()
       ->matchToAction_ref()[1]
       .action_ref()
       ->sendToQueue_ref()
       ->queueId_ref() = 2;
  *config.dataPlaneTrafficPolicy_ref()->matchToAction_ref()[2].matcher_ref() =
      "acl5";
  *config.dataPlaneTrafficPolicy_ref()->matchToAction_ref()[2].action_ref() =
      cfg::MatchAction();
  config.dataPlaneTrafficPolicy_ref()
      ->matchToAction_ref()[2]
      .action_ref()
      ->setDscp_ref() = cfg::SetDscpMatchAction();
  *config.dataPlaneTrafficPolicy_ref()
       ->matchToAction_ref()[2]
       .action_ref()
       ->setDscp_ref()
       ->dscpValue_ref() = 8;

  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  EXPECT_NE(stateV1, nullptr);
  auto acls = stateV1->getAcls();
  EXPECT_NE(acls, nullptr);
  EXPECT_NE(acls->getEntryIf("acl1"), nullptr);
  EXPECT_NE(acls->getEntryIf("acl2"), nullptr);
  EXPECT_NE(acls->getEntryIf("acl3"), nullptr);
  EXPECT_NE(acls->getEntryIf("acl5"), nullptr);

  EXPECT_EQ(acls->getEntryIf("acl1")->getPriority(), kAclStartPriority);
  EXPECT_EQ(acls->getEntryIf("acl4")->getPriority(), kAclStartPriority + 1);
  EXPECT_EQ(acls->getEntryIf("acl2")->getPriority(), kAclStartPriority + 2);
  EXPECT_EQ(acls->getEntryIf("acl3")->getPriority(), kAclStartPriority + 3);
  EXPECT_EQ(acls->getEntryIf("acl5")->getPriority(), kAclStartPriority + 4);

  // Ensure that the global actions in global traffic policy has been added to
  // the ACL entries
  EXPECT_TRUE(acls->getEntryIf("acl5")->getAclAction().has_value());
  EXPECT_EQ(
      8,
      *acls->getEntryIf("acl5")
           ->getAclAction()
           ->getSetDscp()
           .value()
           .dscpValue_ref());
}

TEST(Acl, SerializeAclEntry) {
  auto entry = std::make_unique<AclEntry>(0, "dscp1");
  entry->setDscp(1);
  entry->setL4SrcPort(179);
  entry->setL4DstPort(179);
  MatchAction action = MatchAction();
  cfg::QueueMatchAction queueAction = cfg::QueueMatchAction();
  *queueAction.queueId_ref() = 3;
  action.setSendToQueue(make_pair(queueAction, false));
  entry->setAclAction(action);

  auto serialized = entry->toFollyDynamic();
  auto entryBack = AclEntry::fromFollyDynamic(serialized);
  validateThriftyMigration(*entry);

  EXPECT_TRUE(*entry == *entryBack);
  EXPECT_TRUE(entryBack->getAclAction());
  auto aclAction = entryBack->getAclAction().value();
  EXPECT_TRUE(aclAction.getSendToQueue());
  EXPECT_EQ(aclAction.getSendToQueue().value().second, false);
  EXPECT_EQ(*aclAction.getSendToQueue().value().first.queueId_ref(), 3);

  // change to sendToCPU = true
  action.setSendToQueue(make_pair(queueAction, true));
  EXPECT_EQ(action.getSendToQueue().value().second, true);
  entry->setAclAction(action);
  serialized = entry->toFollyDynamic();
  entryBack = AclEntry::fromFollyDynamic(serialized);
  validateThriftyMigration(*entry);

  EXPECT_TRUE(*entry == *entryBack);
  EXPECT_TRUE(entryBack->getAclAction());
  aclAction = entryBack->getAclAction().value();
  EXPECT_TRUE(aclAction.getSendToQueue());
  EXPECT_EQ(aclAction.getSendToQueue().value().second, true);
  EXPECT_EQ(*aclAction.getSendToQueue().value().first.queueId_ref(), 3);
}

TEST(Acl, SerializePacketCounter) {
  auto entry = std::make_unique<AclEntry>(0, "stat0");
  MatchAction action = MatchAction();
  auto counter = cfg::TrafficCounter();
  *counter.name_ref() = "stat0.c";
  action.setTrafficCounter(counter);
  entry->setAclAction(action);

  auto serialized = entry->toFollyDynamic();
  auto entryBack = AclEntry::fromFollyDynamic(serialized);
  validateThriftyMigration(*entry);

  EXPECT_TRUE(*entry == *entryBack);
  EXPECT_TRUE(entryBack->getAclAction());
  auto aclAction = entryBack->getAclAction().value();
  EXPECT_TRUE(aclAction.getTrafficCounter());
  EXPECT_EQ(*aclAction.getTrafficCounter()->name_ref(), "stat0.c");
  EXPECT_EQ(aclAction.getTrafficCounter()->types_ref()->size(), 1);
  EXPECT_EQ(
      aclAction.getTrafficCounter()->types_ref()[0], cfg::CounterType::PACKETS);

  // Test SetDscpMatchAction
  entry = std::make_unique<AclEntry>(0, "DspNew");
  action = MatchAction();
  auto setDscpMatchAction = cfg::SetDscpMatchAction();
  *setDscpMatchAction.dscpValue_ref() = 8;
  action.setSetDscp(setDscpMatchAction);
  entry->setAclAction(action);

  serialized = entry->toFollyDynamic();
  entryBack = AclEntry::fromFollyDynamic(serialized);
  validateThriftyMigration(*entry);

  EXPECT_TRUE(*entry == *entryBack);
  EXPECT_TRUE(entryBack->getAclAction());
  aclAction = entryBack->getAclAction().value();
  EXPECT_TRUE(aclAction.getSetDscp());
  EXPECT_EQ(*aclAction.getSetDscp().value().dscpValue_ref(), 8);

  // Set 2 counter types
  *counter.types_ref() = {cfg::CounterType::PACKETS, cfg::CounterType::BYTES};
  action.setTrafficCounter(counter);
  entry->setAclAction(action);

  serialized = entry->toFollyDynamic();
  entryBack = AclEntry::fromFollyDynamic(serialized);
  validateThriftyMigration(*entry);

  EXPECT_TRUE(*entry == *entryBack);
  aclAction = entryBack->getAclAction().value();
  EXPECT_EQ(aclAction.getTrafficCounter()->types_ref()->size(), 2);
  EXPECT_EQ(*aclAction.getTrafficCounter()->types_ref(), *counter.types_ref());
}

TEST(Acl, Ttl) {
  FLAGS_enable_acl_table_group = false;
  auto platform = createMockPlatform();
  auto stateV0 = make_shared<SwitchState>();

  cfg::SwitchConfig config;
  config.acls_ref()->resize(1);
  *config.acls_ref()[0].name_ref() = "acl1";
  *config.acls_ref()[0].actionType_ref() = cfg::AclActionType::DENY;
  // set ttl
  config.acls_ref()[0].ttl_ref() = cfg::Ttl();
  *config.acls_ref()[0].ttl_ref()->value_ref() = 42;
  *config.acls_ref()[0].ttl_ref()->mask_ref() = 0xff;

  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  EXPECT_NE(nullptr, stateV1);
  auto aclV1 = stateV1->getAcl("acl1");
  ASSERT_NE(nullptr, aclV1);
  EXPECT_TRUE(aclV1->getTtl());
  EXPECT_EQ(aclV1->getTtl().value().getValue(), 42);
  EXPECT_EQ(aclV1->getTtl().value().getMask(), 0xff);

  // check invalid configs
  *config.acls_ref()[0].ttl_ref()->value_ref() = 256;
  EXPECT_THROW(
      publishAndApplyConfig(stateV1, &config, platform.get()), FbossError);
  *config.acls_ref()[0].ttl_ref()->value_ref() = -1;
  EXPECT_THROW(
      publishAndApplyConfig(stateV1, &config, platform.get()), FbossError);
  *config.acls_ref()[0].ttl_ref()->value_ref() = 42;
  *config.acls_ref()[0].ttl_ref()->mask_ref() = 256;
  EXPECT_THROW(
      publishAndApplyConfig(stateV1, &config, platform.get()), FbossError);
  *config.acls_ref()[0].ttl_ref()->mask_ref() = -1;
  EXPECT_THROW(
      publishAndApplyConfig(stateV1, &config, platform.get()), FbossError);
}

TEST(Acl, TtlSerialization) {
  auto entry = std::make_unique<AclEntry>(0, "stat0");
  entry->setTtl(AclTtl(42, 0xff));
  auto action = MatchAction();
  auto counter = cfg::TrafficCounter();
  *counter.name_ref() = "stat0.c";
  action.setTrafficCounter(counter);
  entry->setAclAction(action);

  auto serialized = entry->toFollyDynamic();
  auto entryBack = AclEntry::fromFollyDynamic(serialized);
  validateThriftyMigration(*entry);

  EXPECT_TRUE(*entry == *entryBack);
  EXPECT_TRUE(entryBack->getTtl());
  EXPECT_EQ(entryBack->getTtl().value().getValue(), 42);
  EXPECT_EQ(entryBack->getTtl().value().getMask(), 0xff);

  // check invalid configs
  auto ttl = AclTtl(42, 0xff);
  EXPECT_THROW(AclTtl(256, 0xff), FbossError);
  EXPECT_THROW(AclTtl(42, 256), FbossError);
  EXPECT_THROW(ttl.setValue(256), FbossError);
  EXPECT_THROW(ttl.setValue(-1), FbossError);
  EXPECT_THROW(ttl.setMask(256), FbossError);
  EXPECT_THROW(ttl.setMask(-1), FbossError);
}

TEST(Acl, PacketLookupResultSerialization) {
  auto entry = std::make_unique<AclEntry>(0, "stat0");
  entry->setPacketLookupResult(
      cfg::PacketLookupResultType::PACKET_LOOKUP_RESULT_MPLS_NO_MATCH);
  auto action = MatchAction();
  auto counter = cfg::TrafficCounter();
  counter.name_ref() = "stat0.c";
  action.setTrafficCounter(counter);
  entry->setAclAction(action);

  auto serialized = entry->toFollyDynamic();
  auto entryBack = AclEntry::fromFollyDynamic(serialized);
  validateThriftyMigration(*entry);

  EXPECT_TRUE(*entry == *entryBack);
  EXPECT_TRUE(entryBack->getPacketLookupResult());
  EXPECT_EQ(
      entryBack->getPacketLookupResult().value(),
      cfg::PacketLookupResultType::PACKET_LOOKUP_RESULT_MPLS_NO_MATCH);
}

TEST(Acl, IpType) {
  FLAGS_enable_acl_table_group = false;
  auto platform = createMockPlatform();
  auto stateV0 = make_shared<SwitchState>();

  cfg::SwitchConfig config;
  config.acls_ref()->resize(1);
  *config.acls_ref()[0].name_ref() = "acl1";
  *config.acls_ref()[0].actionType_ref() = cfg::AclActionType::DENY;
  // set IpType
  auto ipType = cfg::IpType::IP6;
  config.acls_ref()[0].ipType_ref() = ipType;

  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  EXPECT_NE(nullptr, stateV1);
  auto aclV1 = stateV1->getAcl("acl1");
  EXPECT_NE(nullptr, aclV1);
  EXPECT_TRUE(aclV1->getIpType());
  EXPECT_EQ(aclV1->getIpType().value(), ipType);
}

TEST(Acl, LookupClass) {
  FLAGS_enable_acl_table_group = false;
  auto platform = createMockPlatform();
  auto stateV0 = make_shared<SwitchState>();

  cfg::SwitchConfig config;
  config.acls_ref()->resize(1);
  *config.acls_ref()[0].name_ref() = "acl1";
  *config.acls_ref()[0].actionType_ref() = cfg::AclActionType::DENY;

  // set lookupClassL2
  auto lookupClassL2 = cfg::AclLookupClass::DST_CLASS_L3_LOCAL_IP4;
  config.acls_ref()[0].lookupClassL2_ref() = lookupClassL2;

  // apply lookupClassL2 config and validate
  auto stateV2 = publishAndApplyConfig(stateV0, &config, platform.get());
  EXPECT_NE(nullptr, stateV2);
  auto aclV2 = stateV2->getAcl("acl1");
  ASSERT_NE(nullptr, aclV2);
  EXPECT_TRUE(aclV2->getLookupClassL2());
  EXPECT_EQ(aclV2->getLookupClassL2().value(), lookupClassL2);

  // set lookupClassNeighbor
  auto lookupClassNeighbor = cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0;
  config.acls_ref()[0].lookupClassNeighbor_ref() = lookupClassNeighbor;

  // apply lookupClassNeighbor config and validate
  auto stateV3 = publishAndApplyConfig(stateV0, &config, platform.get());
  EXPECT_NE(nullptr, stateV3);
  auto aclV3 = stateV3->getAcl("acl1");
  ASSERT_NE(nullptr, aclV3);
  EXPECT_TRUE(aclV3->getLookupClassNeighbor());
  EXPECT_EQ(aclV3->getLookupClassNeighbor().value(), lookupClassNeighbor);

  // set lookupClassRoute
  auto lookupClassRoute = cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0;
  config.acls_ref()[0].lookupClassRoute_ref() = lookupClassRoute;

  // apply lookupClassRoute config and validate
  auto stateV4 = publishAndApplyConfig(stateV0, &config, platform.get());
  EXPECT_NE(nullptr, stateV4);
  auto aclV4 = stateV4->getAcl("acl1");
  ASSERT_NE(nullptr, aclV4);
  EXPECT_TRUE(aclV4->getLookupClassRoute());
  EXPECT_EQ(aclV4->getLookupClassRoute().value(), lookupClassRoute);
}

TEST(Acl, LookupClassSerialization) {
  auto action = MatchAction();
  auto counter = cfg::TrafficCounter();
  *counter.name_ref() = "stat0.c";
  action.setTrafficCounter(counter);

  // test for lookupClassL2 serialization/de-serialization
  auto entryL2 = std::make_unique<AclEntry>(0, "stat1");
  auto lookupClassL2 = cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_7;
  entryL2->setLookupClassL2(lookupClassL2);
  entryL2->setAclAction(action);

  auto serialized = entryL2->toFollyDynamic();
  auto entryBackL2 = AclEntry::fromFollyDynamic(serialized);
  validateThriftyMigration(*entryL2);

  EXPECT_TRUE(*entryL2 == *entryBackL2);
  EXPECT_TRUE(entryBackL2->getLookupClassL2());
  EXPECT_EQ(entryBackL2->getLookupClassL2().value(), lookupClassL2);

  // test for lookupClassNeighbor serialization/de-serialization
  auto entryNeighbor = std::make_unique<AclEntry>(0, "stat1");
  auto lookupClassNeighbor = cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_7;
  entryNeighbor->setLookupClassNeighbor(lookupClassNeighbor);
  entryNeighbor->setAclAction(action);

  serialized = entryNeighbor->toFollyDynamic();
  auto entryBackNeighbor = AclEntry::fromFollyDynamic(serialized);
  validateThriftyMigration(*entryNeighbor);

  EXPECT_TRUE(*entryNeighbor == *entryBackNeighbor);
  EXPECT_TRUE(entryBackNeighbor->getLookupClassNeighbor());
  EXPECT_EQ(
      entryBackNeighbor->getLookupClassNeighbor().value(), lookupClassNeighbor);

  // test for lookupClassRoute serialization/de-serialization
  auto entryRoute = std::make_unique<AclEntry>(0, "stat1");
  auto lookupClassRoute = cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_7;
  entryRoute->setLookupClassRoute(lookupClassRoute);
  entryRoute->setAclAction(action);

  serialized = entryRoute->toFollyDynamic();
  auto entryBackRoute = AclEntry::fromFollyDynamic(serialized);
  validateThriftyMigration(*entryRoute);

  EXPECT_TRUE(*entryRoute == *entryBackRoute);
  EXPECT_TRUE(entryBackRoute->getLookupClassRoute());
  EXPECT_EQ(entryBackRoute->getLookupClassRoute().value(), lookupClassRoute);
}

TEST(Acl, InvalidTrafficCounter) {
  FLAGS_enable_acl_table_group = false;
  auto platform = createMockPlatform();
  auto stateV0 = make_shared<SwitchState>();

  cfg::SwitchConfig config;
  config.acls_ref()->resize(1);
  *config.acls_ref()[0].name_ref() = "acl0";
  *config.acls_ref()[0].actionType_ref() = cfg::AclActionType::PERMIT;
  config.dataPlaneTrafficPolicy_ref() = cfg::TrafficPolicyConfig();
  config.dataPlaneTrafficPolicy_ref()->matchToAction_ref()->resize(1);
  *config.dataPlaneTrafficPolicy_ref()->matchToAction_ref()[0].matcher_ref() =
      "acl0";
  config.dataPlaneTrafficPolicy_ref()
      ->matchToAction_ref()[0]
      .action_ref()
      ->counter_ref() = "stat0";

  EXPECT_THROW(
      publishAndApplyConfig(stateV0, &config, platform.get()), FbossError);
}

TEST(Acl, GetRequiredAclTableQualifiers) {
  cfg::SwitchConfig config;
  config.acls_ref();
  config.acls_ref()->resize(2);

  auto setAclQualifiers = [](std::string ip, std::string name) {
    cfg::AclEntry acl;
    acl.name_ref() = name;
    acl.srcIp_ref() = ip;
    acl.dstIp_ref() = ip;
    acl.proto_ref() = 50;
    acl.tcpFlagsBitMap_ref() = 123;
    acl.srcPort_ref() = 1001;
    acl.dstPort_ref() = 2002;
    acl.ipFrag_ref() = cfg::IpFragMatch::MATCH_ANY_FRAGMENT;
    acl.dscp_ref() = 8;
    acl.ipType_ref() = cfg::IpType::ANY;
    cfg::Ttl ttl;
    ttl.value_ref() = 255;
    ttl.mask_ref() = 0xff;
    acl.ttl_ref() = ttl;
    acl.dstMac_ref() = "a:b:c:d:e:f";
    acl.l4SrcPort_ref() = 4005;
    acl.l4DstPort_ref() = 4005;
    acl.lookupClassL2_ref() = cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0;
    acl.lookupClassNeighbor_ref() =
        cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_1;
    acl.lookupClassRoute_ref() =
        cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_2;
    acl.actionType_ref() = cfg::AclActionType::DENY;
    return acl;
  };
  config.acls_ref()[0] = setAclQualifiers("10.0.0.1/32", "acl0");
  config.acls_ref()[1] = setAclQualifiers("1::1/128", "acl1");

  config.dataPlaneTrafficPolicy_ref() = cfg::TrafficPolicyConfig();
  config.dataPlaneTrafficPolicy_ref()->matchToAction_ref()->resize(2);
  config.dataPlaneTrafficPolicy_ref()->matchToAction_ref()[0].matcher_ref() =
      "acl0";
  config.dataPlaneTrafficPolicy_ref()->matchToAction_ref()[0].matcher_ref() =
      "acl1";

  auto platform = createMockPlatform();
  auto stateV0 = make_shared<SwitchState>();
  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());

  auto q0 =
      stateV1->getAcls()->getEntry("acl0")->getRequiredAclTableQualifiers();
  auto q1 =
      stateV1->getAcls()->getEntry("acl1")->getRequiredAclTableQualifiers();

  std::set<cfg::AclTableQualifier> qualifiers0{
      cfg::AclTableQualifier::SRC_IPV4,
      cfg::AclTableQualifier::DST_IPV4,
      cfg::AclTableQualifier::IP_PROTOCOL,
      cfg::AclTableQualifier::TCP_FLAGS,
      cfg::AclTableQualifier::IP_FRAG,
      cfg::AclTableQualifier::DSCP,
      cfg::AclTableQualifier::IP_TYPE,
      cfg::AclTableQualifier::TTL,
      cfg::AclTableQualifier::DST_MAC,
      cfg::AclTableQualifier::L4_SRC_PORT,
      cfg::AclTableQualifier::L4_DST_PORT,
      cfg::AclTableQualifier::SRC_PORT,
      cfg::AclTableQualifier::OUT_PORT,
      cfg::AclTableQualifier::LOOKUP_CLASS_L2,
      cfg::AclTableQualifier::LOOKUP_CLASS_NEIGHBOR,
      cfg::AclTableQualifier::LOOKUP_CLASS_ROUTE};

  std::set<cfg::AclTableQualifier> qualifiers1{
      cfg::AclTableQualifier::SRC_IPV6,
      cfg::AclTableQualifier::DST_IPV6,
      cfg::AclTableQualifier::IP_PROTOCOL,
      cfg::AclTableQualifier::TCP_FLAGS,
      cfg::AclTableQualifier::IP_FRAG,
      cfg::AclTableQualifier::DSCP,
      cfg::AclTableQualifier::IP_TYPE,
      cfg::AclTableQualifier::TTL,
      cfg::AclTableQualifier::DST_MAC,
      cfg::AclTableQualifier::L4_SRC_PORT,
      cfg::AclTableQualifier::L4_DST_PORT,
      cfg::AclTableQualifier::SRC_PORT,
      cfg::AclTableQualifier::OUT_PORT,
      cfg::AclTableQualifier::LOOKUP_CLASS_L2,
      cfg::AclTableQualifier::LOOKUP_CLASS_NEIGHBOR,
      cfg::AclTableQualifier::LOOKUP_CLASS_ROUTE};

  EXPECT_EQ(q0, qualifiers0);
  EXPECT_EQ(q1, qualifiers1);
}
