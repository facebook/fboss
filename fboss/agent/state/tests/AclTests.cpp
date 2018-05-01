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
#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/mock/MockPlatform.h"
#include "fboss/agent/state/AclMap.h"
#include "fboss/agent/state/AclEntry.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/SwitchState.h"

#include <folly/IPAddress.h>
#include <folly/MacAddress.h>
#include <gtest/gtest.h>

using namespace facebook::fboss;
using std::make_pair;
using std::make_shared;
using std::shared_ptr;
using folly::MacAddress;

namespace {
// We offset the start point in ApplyThriftConfig
constexpr auto kAclStartPriority = 100000;
}

TEST(Acl, applyConfig) {
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
  config.ports.resize(1);
  config.ports[0].logicalID = 1;
  config.ports[0].name = "port1";
  config.ports[0].state = cfg::PortState::ENABLED;

  config.acls.resize(1);
  config.acls[0].name = "acl1";
  config.acls[0].actionType = cfg::AclActionType::DENY;
  config.acls[0].__isset.srcIp = true;
  config.acls[0].__isset.dstIp = true;
  config.acls[0].srcIp = "192.168.0.1";
  config.acls[0].dstIp = "192.168.0.0/24";
  config.acls[0].__isset.srcPort = true;
  config.acls[0].srcPort = 5;
  config.acls[0].__isset.dstPort = true;
  config.acls[0].dstPort = 8;

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

  config.acls[0].dstIp = "invalid address";
  EXPECT_THROW(publishAndApplyConfig(
    stateV1, &config, platform.get()), folly::IPAddressFormatException);

  config.acls[0].name = "acl2";
  config.acls[0].__isset.dstIp = false;
  auto stateV2 = publishAndApplyConfig(stateV1, &config, platform.get());
  EXPECT_NE(nullptr, stateV2);
  auto aclV2 = stateV2->getAcl("acl2");
  // We should handle the field removal correctly
  EXPECT_NE(nullptr, aclV2);
  EXPECT_FALSE(aclV2->getDstIp().first);

  // Nothing references this permit acl, so it shouldn't be added yet
  config.acls.resize(2);
  config.acls[1].name = "acl22";
  config.acls[1].actionType = cfg::AclActionType::PERMIT;
  auto stateV22 = publishAndApplyConfig(stateV2, &config, platform.get());
  EXPECT_EQ(nullptr, stateV22);

  // Non-existent entry
  auto acl2V2 = stateV2->getAcl("something");
  ASSERT_EQ(nullptr, acl2V2);

  cfg::SwitchConfig configV1;
  configV1.ports.resize(1);
  configV1.ports[0].logicalID = 1;
  configV1.ports[0].name = "port1";
  configV1.ports[0].state = cfg::PortState::ENABLED;

  configV1.acls.resize(1);
  configV1.acls[0].name = "acl3";

  // set ranges
  configV1.acls[0].srcL4PortRange.min = 1;
  configV1.acls[0].srcL4PortRange.max = 2;
  configV1.acls[0].__isset.srcL4PortRange = true;
  configV1.acls[0].dstL4PortRange.min = 3;
  configV1.acls[0].dstL4PortRange.max = 4;
  configV1.acls[0].__isset.dstL4PortRange = true;
  // Make sure it's used so that it isn't ignored
  configV1.globalEgressTrafficPolicy = cfg::TrafficPolicyConfig();
  configV1.__isset.globalEgressTrafficPolicy = true;
  configV1.globalEgressTrafficPolicy.matchToAction.resize(1,
      cfg::MatchToAction());
  configV1.globalEgressTrafficPolicy.matchToAction[0].matcher = "acl3";
  configV1.globalEgressTrafficPolicy.matchToAction[0].action =
      cfg::MatchAction();
  configV1.globalEgressTrafficPolicy.matchToAction[0].action.sendToQueue =
      cfg::QueueMatchAction();
  configV1.globalEgressTrafficPolicy.matchToAction[0]
      .action.sendToQueue.queueId = 1;

  auto stateV3 = publishAndApplyConfig(stateV2, &configV1, platform.get());
  EXPECT_NE(nullptr, stateV3);
  auto acls = stateV3->getAcls();
  auto aclV3 = stateV3->getAcl("system:acl3");
  ASSERT_NE(nullptr, aclV3);
  EXPECT_NE(aclV0, aclV3);
  EXPECT_EQ(0 + kAclStartPriority, aclV3->getPriority());
  EXPECT_EQ(cfg::AclActionType::PERMIT, aclV3->getActionType());
  EXPECT_FALSE(!aclV3->getSrcL4PortRange());
  EXPECT_EQ(aclV3->getSrcL4PortRange().value().getMin(), 1);
  EXPECT_EQ(aclV3->getSrcL4PortRange().value().getMax(), 2);
  EXPECT_FALSE(!aclV3->getDstL4PortRange());
  EXPECT_EQ(aclV3->getDstL4PortRange().value().getMin(), 3);
  EXPECT_EQ(aclV3->getDstL4PortRange().value().getMax(), 4);

  // test min > max case
  configV1.acls[0].srcL4PortRange.min = 3;
  EXPECT_THROW(publishAndApplyConfig(stateV3, &configV1, platform.get()),
    FbossError);
  // test max > 65535 case
  configV1.acls[0].srcL4PortRange.max = 65536;
  EXPECT_THROW(publishAndApplyConfig(stateV3, &configV1, platform.get()),
    FbossError);
  // set packet length rangeJson
  cfg::SwitchConfig configV2;
  configV2.ports.resize(1);
  configV2.ports[0].logicalID = 1;
  configV2.ports[0].name = "port1";
  configV2.ports[0].state = cfg::PortState::ENABLED;
  configV2.acls.resize(1);
  configV2.acls[0].name = "acl3";
  // Make sure it's used so that it isn't ignored
  configV2.globalEgressTrafficPolicy = cfg::TrafficPolicyConfig();
  configV2.__isset.globalEgressTrafficPolicy = true;
  configV2.globalEgressTrafficPolicy.matchToAction.resize(1,
      cfg::MatchToAction());
  configV2.globalEgressTrafficPolicy.matchToAction[0].matcher = "acl3";
  configV2.globalEgressTrafficPolicy.matchToAction[0].action =
      cfg::MatchAction();
  configV2.globalEgressTrafficPolicy.matchToAction[0].action.sendToQueue =
      cfg::QueueMatchAction();
  configV2.globalEgressTrafficPolicy.matchToAction[0]
      .action.sendToQueue.queueId = 1;

  // set pkt length range
  configV2.acls[0].pktLenRange.min = 34;
  configV2.acls[0].pktLenRange.max = 1500;
  configV2.acls[0].__isset.pktLenRange = true;

  auto stateV4 = publishAndApplyConfig(stateV3, &configV2, platform.get());
  EXPECT_NE(nullptr, stateV4);
  auto aclV4 = stateV4->getAcl("system:acl3");
  ASSERT_NE(nullptr, aclV4);
  EXPECT_TRUE(aclV4->getPktLenRange());
  EXPECT_EQ(aclV4->getPktLenRange().value().getMin(), 34);
  EXPECT_EQ(aclV4->getPktLenRange().value().getMax(), 1500);

  // set the ip frag option
  configV2.acls[0].ipFrag = cfg::IpFragMatch::MATCH_NOT_FRAGMENTED;
  configV2.acls[0].__isset.ipFrag = true;

  auto stateV5 = publishAndApplyConfig(stateV4, &configV2, platform.get());
  EXPECT_NE(nullptr, stateV5);
  auto aclV5 = stateV5->getAcl("system:acl3");
  EXPECT_NE(nullptr, aclV5);
  EXPECT_EQ(aclV5->getIpFrag().value(), cfg::IpFragMatch::MATCH_NOT_FRAGMENTED);

  // set dst Mac
  auto dstMacStr = "01:01:01:01:01:01";
  configV2.acls[0].__isset.dstMac = true;
  configV2.acls[0].dstMac = dstMacStr;

  auto stateV6 = publishAndApplyConfig(stateV5, &configV2, platform.get());
  EXPECT_NE(nullptr, stateV6);
  auto aclV6 = stateV6->getAcl("system:acl3");
  EXPECT_NE(nullptr, aclV6);

  EXPECT_EQ(MacAddress(dstMacStr), aclV6->getDstMac());
  // Remove src, dst Mac
  configV2.acls[0].__isset.dstMac = false;

  auto stateV7 = publishAndApplyConfig(stateV6, &configV2, platform.get());
  EXPECT_NE(nullptr, stateV7);
  auto aclV7 = stateV7->getAcl("system:acl3");
  EXPECT_NE(nullptr, aclV7);

  EXPECT_FALSE(aclV7->getDstMac());
}

TEST(Acl, stateDelta) {
  auto platform = createMockPlatform();
  auto stateV0 = make_shared<SwitchState>();

  cfg::SwitchConfig config;
  config.acls.resize(4);
  config.acls[0].name = "acl0";
  config.acls[0].actionType = cfg::AclActionType::DENY;
  config.acls[0].__isset.srcIp = true;
  config.acls[0].srcIp = "192.168.0.1";
  config.acls[0].__isset.srcPort = true;
  config.acls[0].srcPort = 1;
  config.acls[1].name = "acl1";
  config.acls[1].__isset.srcIp = true;
  config.acls[1].srcIp = "192.168.0.2";
  config.acls[1].actionType = cfg::AclActionType::DENY;
  config.acls[2].name = "acl3";
  config.acls[2].actionType = cfg::AclActionType::DENY;
  config.acls[2].__isset.srcIp = true;
  config.acls[2].srcIp = "192.168.0.3";
  config.acls[2].__isset.srcPort = true;
  config.acls[2].srcPort = 5;
  config.acls[2].__isset.dstPort = true;
  config.acls[2].dstPort = 8;
  config.acls[3].name = "acl4";
  config.acls[3].actionType = cfg::AclActionType::DENY;
  config.acls[3].__isset.srcIp = true;
  config.acls[3].srcIp = "192.168.0.4";
  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  auto stateV2 = publishAndApplyConfig(stateV1, &config, platform.get());
  EXPECT_EQ(stateV2, nullptr);

  // Only change one action
  config.acls[0].srcPort = 2;;
  auto stateV3 = publishAndApplyConfig(stateV1, &config, platform.get());
  StateDelta delta13(stateV1, stateV3);
  auto aclDelta13 = delta13.getAclsDelta();
  auto iter = aclDelta13.begin();
  EXPECT_EQ(iter->getOld()->getSrcPort(), 1);
  EXPECT_EQ(iter->getNew()->getSrcPort(), 2);
  ++iter;
  EXPECT_EQ(iter, aclDelta13.end());

  // Remove tail element
  config.acls.pop_back();
  auto stateV4 = publishAndApplyConfig(stateV3, &config, platform.get());
  StateDelta delta34(stateV3, stateV4);
  auto aclDelta34 = delta34.getAclsDelta();
  iter = aclDelta34.begin();
  EXPECT_EQ(iter->getOld()->getSrcIp(), folly::CIDRNetwork(
        folly::IPAddress("192.168.0.4"), 32));
  EXPECT_EQ(iter->getNew(), nullptr);
  ++iter;
  EXPECT_EQ(iter, aclDelta34.end());
  // Remove tail element
  config.acls.pop_back();
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
  auto platform = createMockPlatform();
  auto stateV0 = make_shared<SwitchState>();

  cfg::SwitchConfig config;
  config.acls.resize(1);
  config.acls[0].name = "acl1";
  config.acls[0].actionType = cfg::AclActionType::DENY;
  config.acls[0].proto = 58;
  config.acls[0].__isset.proto = true;
  config.acls[0].icmpType = 128;
  config.acls[0].__isset.icmpType = true;
  config.acls[0].icmpCode = 0;
  config.acls[0].__isset.icmpCode = true;

  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  EXPECT_NE(nullptr, stateV1);
  auto aclV1 = stateV1->getAcl("acl1");
  ASSERT_NE(nullptr, aclV1);
  EXPECT_EQ(0 + kAclStartPriority, aclV1->getPriority());
  EXPECT_EQ(cfg::AclActionType::DENY, aclV1->getActionType());
  EXPECT_EQ(128, aclV1->getIcmpType().value());
  EXPECT_EQ(0, aclV1->getIcmpCode().value());

  // test config exceptions
  config.acls[0].proto = 4;
  EXPECT_THROW(
    publishAndApplyConfig(stateV1, &config, platform.get()), FbossError);
  config.acls[0].__isset.proto = false;
  EXPECT_THROW(
    publishAndApplyConfig(stateV1, &config, platform.get()), FbossError);
  config.acls[0].proto = 58;
  config.acls[0].__isset.proto = true;
  config.acls[0].__isset.icmpType = false;
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
  auto platform = createMockPlatform();
  auto stateV0 = make_shared<SwitchState>();
  stateV0->registerPort(PortID(1), "port1");
  stateV0->registerPort(PortID(2), "port2");

  cfg::SwitchConfig config;
  config.ports.resize(2);
  config.ports[0].logicalID = 1;
  config.ports[0].name = "port1";
  config.ports[0].state = cfg::PortState::ENABLED;
  config.ports[1].logicalID = 2;
  config.ports[1].name = "port2";
  config.ports[1].state = cfg::PortState::ENABLED;

  config.acls.resize(4);
  config.acls[0].name = "acl1";
  config.acls[0].actionType = cfg::AclActionType::DENY;
  config.acls[0].__isset.srcIp = true;
  config.acls[0].__isset.dstIp = true;
  config.acls[0].srcIp = "192.168.0.1";
  config.acls[0].dstIp = "192.168.0.0/24";
  config.acls[0].__isset.srcPort = true;
  config.acls[0].srcPort = 5;
  config.acls[0].__isset.dstPort = true;
  config.acls[0].dstPort = 8;
  config.acls[1].name = "acl2";
  config.acls[1].dscp = 16;
  config.acls[1].__isset.dscp = true;
  config.acls[2].name = "acl3";
  config.acls[2].dscp = 16;
  config.acls[2].__isset.dscp = true;
  config.acls[3].name = "acl4";
  config.acls[3].actionType = cfg::AclActionType::DENY;

  config.__isset.globalEgressTrafficPolicy = true;
  config.globalEgressTrafficPolicy.matchToAction.resize(2,
      cfg::MatchToAction());
  config.globalEgressTrafficPolicy.matchToAction[0].matcher = "acl2";
  config.globalEgressTrafficPolicy.matchToAction[0].action =
      cfg::MatchAction();
  config.globalEgressTrafficPolicy.matchToAction[0].action.sendToQueue =
      cfg::QueueMatchAction();
  config.globalEgressTrafficPolicy.matchToAction[0]
      .action.sendToQueue.queueId = 1;
  config.globalEgressTrafficPolicy.matchToAction[1].matcher = "acl3";
  config.globalEgressTrafficPolicy.matchToAction[1].action =
      cfg::MatchAction();
  config.globalEgressTrafficPolicy.matchToAction[1].action.sendToQueue =
      cfg::QueueMatchAction();
  config.globalEgressTrafficPolicy.matchToAction[1]
      .action.sendToQueue.queueId = 2;

  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  EXPECT_NE(stateV1, nullptr);
  auto acls = stateV1->getAcls();
  EXPECT_NE(acls, nullptr);
  EXPECT_NE(acls->getEntryIf("acl1"), nullptr);
  EXPECT_NE(acls->getEntryIf("system:acl2"), nullptr);
  EXPECT_NE(acls->getEntryIf("system:acl3"), nullptr);

  EXPECT_EQ(acls->getEntryIf("acl1")->getPriority(), kAclStartPriority);
  EXPECT_EQ(acls->getEntryIf("acl4")->getPriority(), kAclStartPriority + 1);
  EXPECT_EQ(acls->getEntryIf("system:acl2")->getPriority(),
      kAclStartPriority + 2);
  EXPECT_EQ(acls->getEntryIf("system:acl3")->getPriority(),
      kAclStartPriority + 3);
}

TEST(Acl, SerializeAclEntry) {
  auto entry = std::make_unique<AclEntry>(0, "dscp1");
  entry->setDscp(1);
  MatchAction action = MatchAction();
  cfg::QueueMatchAction queueAction = cfg::QueueMatchAction();
  queueAction.queueId = 3;
  action.setSendToQueue(make_pair(queueAction, false));
  entry->setAclAction(action);

  auto serialized = entry->toFollyDynamic();
  auto entryBack = AclEntry::fromFollyDynamic(serialized);

  EXPECT_TRUE(*entry == *entryBack);
  EXPECT_TRUE(entryBack->getAclAction());
  auto aclAction = entryBack->getAclAction().value();
  EXPECT_TRUE(aclAction.getSendToQueue());
  EXPECT_EQ(aclAction.getSendToQueue().value().second, false);
  EXPECT_EQ(aclAction.getSendToQueue().value().first.queueId, 3);

  // change to sendToCPU = true
  action.setSendToQueue(make_pair(queueAction, true));
  EXPECT_EQ(action.getSendToQueue().value().second, true);
  entry->setAclAction(action);
  serialized = entry->toFollyDynamic();
  entryBack = AclEntry::fromFollyDynamic(serialized);

  EXPECT_TRUE(*entry == *entryBack);
  EXPECT_TRUE(entryBack->getAclAction());
  aclAction = entryBack->getAclAction().value();
  EXPECT_TRUE(aclAction.getSendToQueue());
  EXPECT_EQ(aclAction.getSendToQueue().value().second, true);
  EXPECT_EQ(aclAction.getSendToQueue().value().first.queueId, 3);

  // Test PacketCounterAction
  entry = std::make_unique<AclEntry>(0, "stat0");
  action = MatchAction();
  auto packetCounterAction = cfg::PacketCounterMatchAction();
  packetCounterAction.counterName = "stat0.c";
  action.setPacketCounter(packetCounterAction);
  entry->setAclAction(action);

  serialized = entry->toFollyDynamic();
  entryBack = AclEntry::fromFollyDynamic(serialized);

  EXPECT_TRUE(*entry == *entryBack);
  EXPECT_TRUE(entryBack->getAclAction());
  aclAction = entryBack->getAclAction().value();
  EXPECT_TRUE(aclAction.getPacketCounter());
  EXPECT_EQ(aclAction.getPacketCounter().value().counterName, "stat0.c");
}

TEST(Acl, Ttl) {
  auto platform = createMockPlatform();
  auto stateV0 = make_shared<SwitchState>();

  cfg::SwitchConfig config;
  config.acls.resize(1);
  config.acls[0].name = "acl1";
  config.acls[0].actionType = cfg::AclActionType::DENY;
  // set ttl
  config.acls[0].ttl.value = 42;
  config.acls[0].ttl.mask = 0xff;
  config.acls[0].__isset.ttl = true;

  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  EXPECT_NE(nullptr, stateV1);
  auto aclV1 = stateV1->getAcl("acl1");
  ASSERT_NE(nullptr, aclV1);
  EXPECT_TRUE(aclV1->getTtl());
  EXPECT_EQ(aclV1->getTtl().value().getValue(), 42);
  EXPECT_EQ(aclV1->getTtl().value().getMask(), 0xff);

  // check invalid configs
  config.acls[0].ttl.value = 256;
  EXPECT_THROW(publishAndApplyConfig(stateV1, &config, platform.get()),
    FbossError);
  config.acls[0].ttl.value = -1;
  EXPECT_THROW(publishAndApplyConfig(stateV1, &config, platform.get()),
    FbossError);
  config.acls[0].ttl.value = 42;
  config.acls[0].ttl.mask = 256;
  EXPECT_THROW(publishAndApplyConfig(stateV1, &config, platform.get()),
    FbossError);
  config.acls[0].ttl.mask = -1;
  EXPECT_THROW(publishAndApplyConfig(stateV1, &config, platform.get()),
    FbossError);
}

TEST(Acl, TtlSerialization) {
  auto entry = std::make_unique<AclEntry>(0, "stat0");
  entry->setTtl(AclTtl(42, 0xff));
  auto action = MatchAction();
  auto packetCounterAction = cfg::PacketCounterMatchAction();
  packetCounterAction.counterName = "stat0.c";
  action.setPacketCounter(packetCounterAction);
  entry->setAclAction(action);

  auto serialized = entry->toFollyDynamic();
  auto entryBack = AclEntry::fromFollyDynamic(serialized);

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

TEST(Acl, IpType) {
  auto platform = createMockPlatform();
  auto stateV0 = make_shared<SwitchState>();

  cfg::SwitchConfig config;
  config.acls.resize(1);
  config.acls[0].name = "acl1";
  config.acls[0].actionType = cfg::AclActionType::DENY;
  // set IpType
  auto ipType = cfg::IpType::IP6;
  config.acls[0].__isset.ipType = true;
  config.acls[0].ipType = ipType;

  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  EXPECT_NE(nullptr, stateV1);
  auto aclV1 = stateV1->getAcl("acl1");
  EXPECT_NE(nullptr, aclV1);
  EXPECT_TRUE(aclV1->getIpType());
  EXPECT_EQ(aclV1->getIpType().value(), ipType);
}
