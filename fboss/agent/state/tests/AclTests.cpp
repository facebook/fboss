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
using facebook::fboss::InterfaceID;
using folly::MacAddress;
using std::make_pair;
using std::make_shared;
using std::shared_ptr;

DECLARE_bool(enable_acl_table_group);

namespace {
HwSwitchMatcher scope() {
  return HwSwitchMatcher{std::unordered_set<SwitchID>{SwitchID(0)}};
}
} // namespace

TEST(Acl, applyConfig) {
  FLAGS_enable_acl_table_group = false;
  auto platform = createMockPlatform();
  auto stateV0 = make_shared<SwitchState>();
  auto aclEntry = make_shared<AclEntry>(0, std::string("acl0"));
  auto multiSwitchAcls = stateV0->getMultiSwitchAcls()->modify(&stateV0);
  multiSwitchAcls->addNode(aclEntry, scope());
  auto MultiSwitchAclMap = stateV0->getMultiSwitchAcls();
  auto aclV0 = MultiSwitchAclMap->getNodeIf("acl0");
  EXPECT_EQ(0, aclV0->getGeneration());
  EXPECT_FALSE(aclV0->isPublished());
  EXPECT_EQ(0, aclV0->getPriority());
  stateV0->registerPort(PortID(1), "port1");

  aclV0->publish();
  EXPECT_TRUE(aclV0->isPublished());

  cfg::SwitchConfig config;
  config.ports()->resize(1);
  preparedMockPortConfig(config.ports()[0], 1);

  config.acls()->resize(1);
  *config.acls()[0].name() = "acl1";
  *config.acls()[0].actionType() = cfg::AclActionType::DENY;
  config.acls()[0].srcIp() = "192.168.0.1";
  config.acls()[0].dstIp() = "192.168.0.0/24";
  config.acls()[0].srcPort() = 5;
  config.acls()[0].dstPort() = 8;

  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  EXPECT_NE(nullptr, stateV1);
  auto aclV1 = stateV1->getAcl("acl1");
  ASSERT_NE(nullptr, aclV1);
  EXPECT_NE(aclV0, aclV1);

  EXPECT_EQ(0 + AclTable::kDataplaneAclMaxPriority, aclV1->getPriority());
  EXPECT_EQ(cfg::AclActionType::DENY, aclV1->getActionType());
  EXPECT_EQ(5, aclV1->getSrcPort());
  EXPECT_EQ(8, aclV1->getDstPort());

  EXPECT_FALSE(aclV1->isPublished());

  config.acls()[0].dstIp() = "invalid address";
  EXPECT_THROW(
      publishAndApplyConfig(stateV1, &config, platform.get()),
      folly::IPAddressFormatException);

  *config.acls()[0].name() = "acl2";
  config.acls()[0].dstIp().reset();
  auto stateV2 = publishAndApplyConfig(stateV1, &config, platform.get());
  EXPECT_NE(nullptr, stateV2);
  auto aclV2 = stateV2->getAcl("acl2");
  // We should handle the field removal correctly
  EXPECT_NE(nullptr, aclV2);
  EXPECT_FALSE(aclV2->getDstIp().first);

  // Nothing references this permit acl, so it shouldn't be added yet
  config.acls()->resize(2);
  *config.acls()[1].name() = "acl22";
  *config.acls()[1].actionType() = cfg::AclActionType::PERMIT;
  auto stateV22 = publishAndApplyConfig(stateV2, &config, platform.get());
  EXPECT_EQ(nullptr, stateV22);

  // Non-existent entry
  auto acl2V2 = stateV2->getAcl("something");
  ASSERT_EQ(nullptr, acl2V2);

  cfg::SwitchConfig configV1;
  configV1.ports()->resize(1);
  preparedMockPortConfig(configV1.ports()[0], 1);

  configV1.acls()->resize(1);
  *configV1.acls()[0].name() = "acl3";

  configV1.acls()[0].l4SrcPort() = 1;
  configV1.acls()[0].l4DstPort() = 3;
  // Make sure it's used so that it isn't ignored
  configV1.dataPlaneTrafficPolicy() = cfg::TrafficPolicyConfig();
  configV1.dataPlaneTrafficPolicy()->matchToAction()->resize(
      1, cfg::MatchToAction());
  *configV1.dataPlaneTrafficPolicy()->matchToAction()[0].matcher() = "acl3";
  *configV1.dataPlaneTrafficPolicy()->matchToAction()[0].action() =
      cfg::MatchAction();
  configV1.dataPlaneTrafficPolicy()
      ->matchToAction()[0]
      .action()
      ->sendToQueue() = cfg::QueueMatchAction();
  *configV1.dataPlaneTrafficPolicy()
       ->matchToAction()[0]
       .action()
       ->sendToQueue()
       ->queueId() = 1;

  auto stateV3 = publishAndApplyConfig(stateV2, &configV1, platform.get());
  EXPECT_NE(nullptr, stateV3);
  auto acls = stateV3->getMultiSwitchAcls();
  validateThriftMapMapSerialization(*stateV3->getMultiSwitchAcls());
  auto aclV3 = stateV3->getAcl("acl3");
  ASSERT_NE(nullptr, aclV3);
  EXPECT_NE(aclV0, aclV3);
  EXPECT_EQ(0 + AclTable::kDataplaneAclMaxPriority, aclV3->getPriority());
  EXPECT_EQ(cfg::AclActionType::PERMIT, aclV3->getActionType());
  EXPECT_FALSE(!aclV3->getL4SrcPort());
  EXPECT_EQ(aclV3->getL4SrcPort().value(), 1);
  EXPECT_FALSE(!aclV3->getL4DstPort());
  EXPECT_EQ(aclV3->getL4DstPort().value(), 3);

  // test max > 65535 case
  configV1.acls()[0].l4SrcPort() = 65536;
  EXPECT_THROW(
      publishAndApplyConfig(stateV3, &configV1, platform.get()), FbossError);

  cfg::SwitchConfig configV2;
  configV2.ports()->resize(1);
  preparedMockPortConfig(configV2.ports()[0], 1);
  configV2.acls()->resize(1);
  *configV2.acls()[0].name() = "acl3";
  // Make sure it's used so that it isn't ignored
  configV2.dataPlaneTrafficPolicy() = cfg::TrafficPolicyConfig();
  configV2.dataPlaneTrafficPolicy()->matchToAction()->resize(
      1, cfg::MatchToAction());
  *configV2.dataPlaneTrafficPolicy()->matchToAction()[0].matcher() = "acl3";
  *configV2.dataPlaneTrafficPolicy()->matchToAction()[0].action() =
      cfg::MatchAction();
  configV2.dataPlaneTrafficPolicy()
      ->matchToAction()[0]
      .action()
      ->sendToQueue() = cfg::QueueMatchAction();
  *configV2.dataPlaneTrafficPolicy()
       ->matchToAction()[0]
       .action()
       ->sendToQueue()
       ->queueId() = 1;

  // set the ip frag option
  configV2.acls()[0].ipFrag() = cfg::IpFragMatch::MATCH_NOT_FRAGMENTED;

  auto stateV5 = publishAndApplyConfig(stateV3, &configV2, platform.get());
  EXPECT_NE(nullptr, stateV5);
  auto aclV5 = stateV5->getAcl("acl3");
  EXPECT_NE(nullptr, aclV5);
  EXPECT_EQ(aclV5->getIpFrag().value(), cfg::IpFragMatch::MATCH_NOT_FRAGMENTED);

  // set dst Mac
  auto dstMacStr = "01:01:01:01:01:01";
  configV2.acls()[0].dstMac() = dstMacStr;

  auto stateV6 = publishAndApplyConfig(stateV5, &configV2, platform.get());
  EXPECT_NE(nullptr, stateV6);
  auto aclV6 = stateV6->getAcl("acl3");
  EXPECT_NE(nullptr, aclV6);

  EXPECT_EQ(MacAddress(dstMacStr), aclV6->getDstMac());
  // Remove src, dst Mac
  configV2.acls()[0].dstMac().reset();

  auto stateV7 = publishAndApplyConfig(stateV6, &configV2, platform.get());
  EXPECT_NE(nullptr, stateV7);
  auto aclV7 = stateV7->getAcl("acl3");
  EXPECT_NE(nullptr, aclV7);

  EXPECT_FALSE(aclV7->getDstMac());

  // set packet lookup result matching
  configV2.acls()[0].packetLookupResult() =
      cfg::PacketLookupResultType::PACKET_LOOKUP_RESULT_MPLS_NO_MATCH;
  auto stateV8 = publishAndApplyConfig(stateV7, &configV2, platform.get());
  EXPECT_NE(nullptr, stateV5);
  auto aclV8 = stateV8->getAcl("acl3");
  EXPECT_NE(nullptr, aclV8);
  EXPECT_EQ(
      aclV8->getPacketLookupResult().value(),
      cfg::PacketLookupResultType::PACKET_LOOKUP_RESULT_MPLS_NO_MATCH);

  // remove packet lookup result matching
  configV2.acls()[0].packetLookupResult().reset();
  auto stateV9 = publishAndApplyConfig(stateV8, &configV2, platform.get());
  EXPECT_NE(nullptr, stateV9);
  auto aclV9 = stateV9->getAcl("acl3");
  EXPECT_NE(nullptr, aclV9);
  EXPECT_FALSE(aclV9->getPacketLookupResult());

  // set vlan_id matching
  configV2.acls()[0].vlanID() = 2001;
  auto stateV10 = publishAndApplyConfig(stateV9, &configV2, platform.get());
  EXPECT_NE(nullptr, stateV10);
  auto aclV10 = stateV10->getAcl("acl3");
  EXPECT_NE(nullptr, aclV10);
  EXPECT_EQ(aclV10->getVlanID().value(), 2001);

  // remove vlan_id matching
  configV2.acls()[0].vlanID().reset();
  auto stateV11 = publishAndApplyConfig(stateV10, &configV2, platform.get());
  EXPECT_NE(nullptr, stateV11);
  auto aclV11 = stateV11->getAcl("acl3");
  EXPECT_NE(nullptr, aclV11);
  EXPECT_FALSE(aclV11->getVlanID());
}

TEST(Acl, stateDelta) {
  FLAGS_enable_acl_table_group = false;
  auto platform = createMockPlatform();
  auto stateV0 = make_shared<SwitchState>();

  cfg::SwitchConfig config;
  config.acls()->resize(4);
  *config.acls()[0].name() = "acl0";
  *config.acls()[0].actionType() = cfg::AclActionType::DENY;
  config.acls()[0].srcIp() = "192.168.0.1";
  config.acls()[0].srcPort() = 1;
  *config.acls()[1].name() = "acl1";
  config.acls()[1].srcIp() = "192.168.0.2";
  *config.acls()[1].actionType() = cfg::AclActionType::DENY;
  *config.acls()[2].name() = "acl3";
  *config.acls()[2].actionType() = cfg::AclActionType::DENY;
  config.acls()[2].srcIp() = "192.168.0.3";
  config.acls()[2].srcPort() = 5;
  config.acls()[2].dstPort() = 8;
  *config.acls()[3].name() = "acl4";
  *config.acls()[3].actionType() = cfg::AclActionType::DENY;
  config.acls()[3].srcIp() = "192.168.0.4";
  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  auto stateV2 = publishAndApplyConfig(stateV1, &config, platform.get());
  EXPECT_EQ(stateV2, nullptr);

  // Only change one action
  config.acls()[0].srcPort() = 2;
  auto stateV3 = publishAndApplyConfig(stateV1, &config, platform.get());
  StateDelta delta13(stateV1, stateV3);
  auto aclDelta13 = delta13.getAclsDelta();
  auto iter = aclDelta13.begin();
  EXPECT_EQ(iter->getOld()->getSrcPort(), 1);
  EXPECT_EQ(iter->getNew()->getSrcPort(), 2);
  ++iter;
  EXPECT_EQ(iter, aclDelta13.end());

  // Remove tail element
  config.acls()->pop_back();
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
  config.acls()->pop_back();
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
  config.acls()->resize(1);
  *config.acls()[0].name() = "acl1";
  *config.acls()[0].actionType() = cfg::AclActionType::DENY;
  config.acls()[0].proto() = 58;
  config.acls()[0].icmpType() = 128;
  config.acls()[0].icmpCode() = 0;

  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  EXPECT_NE(nullptr, stateV1);
  auto aclV1 = stateV1->getAcl("acl1");
  ASSERT_NE(nullptr, aclV1);
  EXPECT_EQ(0 + AclTable::kDataplaneAclMaxPriority, aclV1->getPriority());
  EXPECT_EQ(cfg::AclActionType::DENY, aclV1->getActionType());
  EXPECT_EQ(128, aclV1->getIcmpType().value());
  EXPECT_EQ(0, aclV1->getIcmpCode().value());

  // test config exceptions
  config.acls()[0].proto() = 4;
  EXPECT_THROW(
      publishAndApplyConfig(stateV1, &config, platform.get()), FbossError);
  config.acls()[0].proto().reset();
  EXPECT_THROW(
      publishAndApplyConfig(stateV1, &config, platform.get()), FbossError);
  config.acls()[0].proto() = 58;
  config.acls()[0].icmpType().reset();
  EXPECT_THROW(
      publishAndApplyConfig(stateV1, &config, platform.get()), FbossError);
}

TEST(Acl, aclModifyUnpublished) {
  auto state = make_shared<SwitchState>();
  auto mulitSwitchAclMap = state->getMultiSwitchAcls();
  EXPECT_EQ(mulitSwitchAclMap.get(), mulitSwitchAclMap->modify(&state));
}

TEST(Acl, aclModifyPublished) {
  auto state = make_shared<SwitchState>();
  state->publish();
  auto mulitSwitchAclMap = state->getMultiSwitchAcls();
  validateThriftMapMapSerialization(*mulitSwitchAclMap);
  EXPECT_NE(mulitSwitchAclMap.get(), mulitSwitchAclMap->modify(&state));
}

TEST(Acl, aclEntryModifyUnpublished) {
  auto state = make_shared<SwitchState>();
  auto aclEntry = make_shared<AclEntry>(0, std::string("acl0"));
  state->getMultiSwitchAcls()->addNode(aclEntry, scope());
  EXPECT_EQ(aclEntry.get(), aclEntry->modify(&state, scope()));
}

TEST(Acl, aclEntryModifyPublished) {
  auto state = make_shared<SwitchState>();
  auto aclEntry = make_shared<AclEntry>(0, std::string("acl0"));
  state->getMultiSwitchAcls()->addNode(aclEntry, scope());
  state->publish();
  EXPECT_NE(aclEntry.get(), aclEntry->modify(&state, scope()));
}

TEST(Acl, AclGeneration) {
  FLAGS_enable_acl_table_group = false;
  auto platform = createMockPlatform();
  auto stateV0 = make_shared<SwitchState>();
  stateV0->registerPort(PortID(1), "port1");
  stateV0->registerPort(PortID(2), "port2");

  cfg::SwitchConfig config;
  config.ports()->resize(2);
  preparedMockPortConfig(config.ports()[0], 1);
  preparedMockPortConfig(config.ports()[1], 2);

  config.acls()->resize(5);
  *config.acls()[0].name() = "acl1";
  *config.acls()[0].actionType() = cfg::AclActionType::DENY;
  config.acls()[0].srcIp() = "192.168.0.1";
  config.acls()[0].dstIp() = "192.168.0.0/24";
  config.acls()[0].srcPort() = 5;
  config.acls()[0].dstPort() = 8;
  *config.acls()[1].name() = "acl2";
  config.acls()[1].dscp() = 16;
  *config.acls()[2].name() = "acl3";
  config.acls()[2].dscp() = 16;
  *config.acls()[3].name() = "acl4";
  *config.acls()[3].actionType() = cfg::AclActionType::DENY;
  *config.acls()[4].name() = "acl5";
  config.acls()[4].srcIp() = "2401:db00:21:7147:face:0:7:0/128";
  config.acls()[4].srcPort() = 5;

  config.dataPlaneTrafficPolicy() = cfg::TrafficPolicyConfig();
  config.dataPlaneTrafficPolicy()->matchToAction()->resize(
      3, cfg::MatchToAction());
  *config.dataPlaneTrafficPolicy()->matchToAction()[0].matcher() = "acl2";
  *config.dataPlaneTrafficPolicy()->matchToAction()[0].action() =
      cfg::MatchAction();
  config.dataPlaneTrafficPolicy()->matchToAction()[0].action()->sendToQueue() =
      cfg::QueueMatchAction();
  *config.dataPlaneTrafficPolicy()
       ->matchToAction()[0]
       .action()
       ->sendToQueue()
       ->queueId() = 1;
  *config.dataPlaneTrafficPolicy()->matchToAction()[1].matcher() = "acl3";
  *config.dataPlaneTrafficPolicy()->matchToAction()[1].action() =
      cfg::MatchAction();
  config.dataPlaneTrafficPolicy()->matchToAction()[1].action()->sendToQueue() =
      cfg::QueueMatchAction();
  *config.dataPlaneTrafficPolicy()
       ->matchToAction()[1]
       .action()
       ->sendToQueue()
       ->queueId() = 2;
  *config.dataPlaneTrafficPolicy()->matchToAction()[2].matcher() = "acl5";
  *config.dataPlaneTrafficPolicy()->matchToAction()[2].action() =
      cfg::MatchAction();
  config.dataPlaneTrafficPolicy()->matchToAction()[2].action()->setDscp() =
      cfg::SetDscpMatchAction();
  *config.dataPlaneTrafficPolicy()
       ->matchToAction()[2]
       .action()
       ->setDscp()
       ->dscpValue() = 8;

  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  EXPECT_NE(stateV1, nullptr);
  auto acls = stateV1->getMultiSwitchAcls();
  validateThriftMapMapSerialization(*acls);
  EXPECT_NE(acls, nullptr);
  EXPECT_NE(acls->getNodeIf("acl1"), nullptr);
  EXPECT_NE(acls->getNodeIf("acl2"), nullptr);
  EXPECT_NE(acls->getNodeIf("acl3"), nullptr);
  EXPECT_NE(acls->getNodeIf("acl5"), nullptr);

  EXPECT_EQ(
      acls->getNodeIf("acl1")->getPriority(),
      AclTable::kDataplaneAclMaxPriority);
  EXPECT_EQ(
      acls->getNodeIf("acl4")->getPriority(),
      AclTable::kDataplaneAclMaxPriority + 1);
  EXPECT_EQ(
      acls->getNodeIf("acl2")->getPriority(),
      AclTable::kDataplaneAclMaxPriority + 2);
  EXPECT_EQ(
      acls->getNodeIf("acl3")->getPriority(),
      AclTable::kDataplaneAclMaxPriority + 3);
  EXPECT_EQ(
      acls->getNodeIf("acl5")->getPriority(),
      AclTable::kDataplaneAclMaxPriority + 4);

  // Ensure that the global actions in global traffic policy has been added to
  // the ACL entries
  EXPECT_TRUE(acls->getNodeIf("acl5")->getAclAction() != nullptr);
  EXPECT_EQ(
      8,
      acls->getNodeIf("acl5")
          ->getAclAction()
          ->cref<switch_state_tags::setDscp>()
          ->cref<switch_config_tags::dscpValue>()
          ->cref());
}

TEST(Acl, SerializeAclEntry) {
  auto entry = std::make_unique<AclEntry>(0, std::string("dscp1"));
  entry->setDscp(1);
  entry->setL4SrcPort(179);
  entry->setL4DstPort(179);
  MatchAction action = MatchAction();
  cfg::QueueMatchAction queueAction = cfg::QueueMatchAction();
  *queueAction.queueId() = 3;
  action.setSendToQueue(make_pair(queueAction, false));
  entry->setAclAction(action);

  auto serialized = entry->toThrift();
  auto entryBack = std::make_shared<AclEntry>(serialized);
  validateNodeSerialization(*entry);

  EXPECT_TRUE(*entry == *entryBack);
  EXPECT_TRUE(entryBack->getAclAction());
  auto aclAction = entryBack->getAclAction();
  EXPECT_TRUE(aclAction->cref<switch_state_tags::sendToQueue>() != nullptr);
  EXPECT_EQ(
      aclAction->cref<switch_state_tags::sendToQueue>()
          ->cref<switch_state_tags::sendToCPU>()
          ->cref(),
      false);
  EXPECT_EQ(
      aclAction->cref<switch_state_tags::sendToQueue>()
          ->cref<switch_state_tags::action>()
          ->cref<switch_config_tags::queueId>()
          ->cref(),
      3);
  // change to sendToCPU = true
  action.setSendToQueue(make_pair(queueAction, true));
  EXPECT_EQ(action.getSendToQueue().value().second, true);
  entry->setAclAction(action);
  serialized = entry->toThrift();
  entryBack = std::make_shared<AclEntry>(serialized);
  validateNodeSerialization(*entry);

  EXPECT_TRUE(*entry == *entryBack);
  EXPECT_TRUE(entryBack->getAclAction());
  aclAction = entryBack->getAclAction();
  EXPECT_TRUE(aclAction->cref<switch_state_tags::sendToQueue>() != nullptr);
  EXPECT_EQ(
      aclAction->cref<switch_state_tags::sendToQueue>()
          ->cref<switch_state_tags::sendToCPU>()
          ->cref(),
      true);
  EXPECT_EQ(
      aclAction->cref<switch_state_tags::sendToQueue>()
          ->cref<switch_state_tags::action>()
          ->cref<switch_config_tags::queueId>()
          ->cref(),
      3);
}

TEST(Acl, SerializeRedirectToNextHop) {
  auto entry = std::make_unique<AclEntry>(0, std::string("stat0"));
  MatchAction action = MatchAction();
  auto cfgRedirectToNextHop = cfg::RedirectToNextHopAction();
  std::vector<std::string> nexthops = {
      "2401:db00:e112:9103:1028::1b",
      "fe80:db00:e112:9103:2028::1b",
      "10.0.0.1",
      "10.0.0.2"};
  int intfID = 0;
  for (auto nh : nexthops) {
    cfg::RedirectNextHop nhop;
    nhop.ip_ref() = nh;
    nhop.intfID_ref() = ++intfID;
    cfgRedirectToNextHop.redirectNextHops()->push_back(nhop);
  }
  auto redirectToNextHop = MatchAction::RedirectToNextHopAction();
  redirectToNextHop.first = cfgRedirectToNextHop;

  // Only nexthops with V6 link local (fe80 subnet) address can be treated as
  // ResolvedNextHops
  std::vector<folly::IPAddress> nhAddrs = {
      folly::IPAddress("fe80:db00:e113:9103:1028::1b"),
      folly::IPAddress("fe80:db00:e113:9103:1028::1c"),
      folly::IPAddress("2401:db00:e112:9103:1028::1b"),
      folly::IPAddress("100.0.0.1"),
      folly::IPAddress("100.0.0.2")};
  auto nhset = MatchAction::NextHopSet();

  auto setNhAddrs = [](MatchAction::NextHopSet& nhset,
                       std::vector<folly::IPAddress>& nhAddrs) {
    int outIntfID = 0;
    int weight = 100;
    for (auto nhAddr : nhAddrs) {
      if (nhAddr.isV6() and nhAddr.isLinkLocal()) {
        nhset.insert(ResolvedNextHop(nhAddr, InterfaceID(outIntfID), weight));
        ++outIntfID;
      } else {
        nhset.insert(UnresolvedNextHop(nhAddr, weight));
      }
      ++weight;
    }
  };
  setNhAddrs(nhset, nhAddrs);

  redirectToNextHop.second = nhset;
  action.setRedirectToNextHop(redirectToNextHop);
  entry->setAclAction(action);
  entry->setEnabled(true);

  auto verifyEntries = [](AclEntry& entry,
                          std::vector<std::string>& nexthops,
                          MatchAction::NextHopSet& nhset) {
    auto serialized = entry.toThrift();
    auto entryBack = std::make_shared<AclEntry>(serialized);
    EXPECT_TRUE(entry == *entryBack);
    validateThriftStructNodeSerialization(entry);
    const auto& aclAction = entryBack->getAclAction();
    const auto& newRedirectToNextHop =
        aclAction->cref<switch_state_tags::redirectToNextHop>();
    int i = 0;
    int outIntfID = 0;
    const auto& redirectAction =
        newRedirectToNextHop->cref<switch_state_tags::action>();
    for (const auto& nh : std::as_const(
             *(redirectAction->cref<switch_config_tags::redirectNextHops>()))) {
      EXPECT_EQ(nh->cref<switch_config_tags::ip>()->cref(), nexthops[i]);
      EXPECT_EQ(nh->cref<switch_config_tags::intfID>()->cref(), ++outIntfID);
      ++i;
    }

    auto nhops = util::toRouteNextHopSet(
        newRedirectToNextHop->cref<switch_state_tags::resolvedNexthops>()
            ->toThrift());
    EXPECT_EQ(nhset, nhops);
    EXPECT_EQ(entry.isEnabled(), entryBack->isEnabled());
  };
  verifyEntries(*entry, nexthops, nhset);

  // Update nexthops
  nexthops.pop_back();
  nexthops.push_back("1000:db00:e112:9103:1028::1b");
  nexthops.push_back("10.0.0.3");
  redirectToNextHop.first.redirectNextHops()->clear();
  intfID = 0;
  for (auto nh : nexthops) {
    cfg::RedirectNextHop nhop;
    nhop.ip_ref() = nh;
    nhop.intfID_ref() = ++intfID;
    redirectToNextHop.first.redirectNextHops()->push_back(nhop);
  }
  action.setRedirectToNextHop(redirectToNextHop);
  entry->setAclAction(action);

  verifyEntries(*entry, nexthops, nhset);

  nhAddrs.pop_back();
  nhAddrs.pop_back();
  nhAddrs.push_back(folly::IPAddress("fe80:db00:e113:9103:1028::2a"));
  nhAddrs.push_back(folly::IPAddress("100.0.0.3"));
  nhset = MatchAction::NextHopSet();
  setNhAddrs(nhset, nhAddrs);

  // Update resoved nexthops
  redirectToNextHop.second = nhset;
  action.setRedirectToNextHop(redirectToNextHop);
  entry->setAclAction(action);

  verifyEntries(*entry, nexthops, nhset);
}

TEST(Acl, SerializePacketCounter) {
  auto entry = std::make_unique<AclEntry>(0, std::string("stat0"));
  MatchAction action = MatchAction();
  auto counter = cfg::TrafficCounter();
  *counter.name() = "stat0.c";
  action.setTrafficCounter(counter);
  entry->setAclAction(action);

  auto serialized = entry->toThrift();
  auto entryBack = std::make_shared<AclEntry>(serialized);
  validateNodeSerialization(*entry);

  EXPECT_TRUE(*entry == *entryBack);
  EXPECT_TRUE(entryBack->getAclAction());
  const auto& aclAction = entryBack->getAclAction();

  EXPECT_TRUE(aclAction->cref<switch_state_tags::trafficCounter>() != nullptr);

  EXPECT_EQ(
      aclAction->cref<switch_state_tags::trafficCounter>()
          ->cref<switch_config_tags::name>()
          ->cref(),
      "stat0.c");
  EXPECT_EQ(
      aclAction->cref<switch_state_tags::trafficCounter>()
          ->cref<switch_config_tags::types>()
          ->size(),
      1);
  EXPECT_EQ(
      aclAction->cref<switch_state_tags::trafficCounter>()
          ->cref<switch_config_tags::types>()
          ->cref(0)
          ->toThrift(),
      cfg::CounterType::PACKETS);

  // Test SetDscpMatchAction
  entry = std::make_unique<AclEntry>(0, std::string("DspNew"));
  action = MatchAction();
  auto setDscpMatchAction = cfg::SetDscpMatchAction();
  *setDscpMatchAction.dscpValue() = 8;
  action.setSetDscp(setDscpMatchAction);
  entry->setAclAction(action);

  serialized = entry->toThrift();
  entryBack = std::make_shared<AclEntry>(serialized);
  validateNodeSerialization(*entry);

  EXPECT_TRUE(*entry == *entryBack);
  EXPECT_TRUE(entryBack->getAclAction());
  const auto& aclAction1 = entryBack->getAclAction();
  EXPECT_TRUE(aclAction1->cref<switch_state_tags::setDscp>() != nullptr);
  EXPECT_EQ(
      aclAction1->cref<switch_state_tags::setDscp>()
          ->cref<switch_config_tags::dscpValue>()
          ->toThrift(),
      8);

  // Set 2 counter types
  *counter.types() = {cfg::CounterType::PACKETS, cfg::CounterType::BYTES};
  action.setTrafficCounter(counter);
  entry->setAclAction(action);

  serialized = entry->toThrift();
  entryBack = std::make_shared<AclEntry>(serialized);
  validateNodeSerialization(*entry);

  EXPECT_TRUE(*entry == *entryBack);
  const auto& aclAction2 = entryBack->getAclAction();
  EXPECT_EQ(
      aclAction2->cref<switch_state_tags::trafficCounter>()
          ->cref<switch_config_tags::types>()
          ->size(),
      2);
  EXPECT_EQ(
      aclAction2->cref<switch_state_tags::trafficCounter>()
          ->cref<switch_config_tags::types>()
          ->toThrift(),
      *counter.types());
}

TEST(Acl, Ttl) {
  FLAGS_enable_acl_table_group = false;
  auto platform = createMockPlatform();
  auto stateV0 = make_shared<SwitchState>();

  cfg::SwitchConfig config;
  config.acls()->resize(1);
  *config.acls()[0].name() = "acl1";
  *config.acls()[0].actionType() = cfg::AclActionType::DENY;
  // set ttl
  config.acls()[0].ttl() = cfg::Ttl();
  *config.acls()[0].ttl()->value() = 42;
  *config.acls()[0].ttl()->mask() = 0xff;

  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  EXPECT_NE(nullptr, stateV1);
  auto aclV1 = stateV1->getAcl("acl1");
  ASSERT_NE(nullptr, aclV1);
  EXPECT_TRUE(aclV1->getTtl());
  EXPECT_EQ(aclV1->getTtl().value().getValue(), 42);
  EXPECT_EQ(aclV1->getTtl().value().getMask(), 0xff);

  // check invalid configs
  *config.acls()[0].ttl()->value() = 256;
  EXPECT_THROW(
      publishAndApplyConfig(stateV1, &config, platform.get()), FbossError);
  *config.acls()[0].ttl()->value() = -1;
  EXPECT_THROW(
      publishAndApplyConfig(stateV1, &config, platform.get()), FbossError);
  *config.acls()[0].ttl()->value() = 42;
  *config.acls()[0].ttl()->mask() = 256;
  EXPECT_THROW(
      publishAndApplyConfig(stateV1, &config, platform.get()), FbossError);
  *config.acls()[0].ttl()->mask() = -1;
  EXPECT_THROW(
      publishAndApplyConfig(stateV1, &config, platform.get()), FbossError);
}

TEST(Acl, TtlSerialization) {
  auto entry = std::make_unique<AclEntry>(0, std::string("stat0"));
  entry->setTtl(AclTtl(42, 0xff));
  auto action = MatchAction();
  auto counter = cfg::TrafficCounter();
  *counter.name() = "stat0.c";
  action.setTrafficCounter(counter);
  entry->setAclAction(action);

  auto serialized = entry->toThrift();
  auto entryBack = std::make_shared<AclEntry>(serialized);
  validateNodeSerialization(*entry);

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
  auto entry = std::make_unique<AclEntry>(0, std::string("stat0"));
  entry->setPacketLookupResult(
      cfg::PacketLookupResultType::PACKET_LOOKUP_RESULT_MPLS_NO_MATCH);
  auto action = MatchAction();
  auto counter = cfg::TrafficCounter();
  counter.name() = "stat0.c";
  action.setTrafficCounter(counter);
  entry->setAclAction(action);

  auto serialized = entry->toThrift();
  auto entryBack = std::make_shared<AclEntry>(serialized);
  validateNodeSerialization(*entry);

  EXPECT_TRUE(*entry == *entryBack);
  EXPECT_TRUE(entryBack->getPacketLookupResult());
  EXPECT_EQ(
      entryBack->getPacketLookupResult().value(),
      cfg::PacketLookupResultType::PACKET_LOOKUP_RESULT_MPLS_NO_MATCH);
}

TEST(Acl, VlanIDSerialization) {
  auto entry = std::make_unique<AclEntry>(0, std::string("stat0"));
  entry->setVlanID(2001);
  auto action = MatchAction();
  auto counter = cfg::TrafficCounter();
  counter.name() = "stat0.c";
  action.setTrafficCounter(counter);
  entry->setAclAction(action);

  auto serialized = entry->toThrift();
  auto entryBack = std::make_shared<AclEntry>(serialized);
  validateNodeSerialization(*entry);

  EXPECT_TRUE(*entry == *entryBack);
  EXPECT_TRUE(entryBack->getVlanID());
  EXPECT_EQ(entryBack->getVlanID().value(), 2001);
}

TEST(Acl, IpType) {
  FLAGS_enable_acl_table_group = false;
  auto platform = createMockPlatform();
  auto stateV0 = make_shared<SwitchState>();

  cfg::SwitchConfig config;
  config.acls()->resize(1);
  *config.acls()[0].name() = "acl1";
  *config.acls()[0].actionType() = cfg::AclActionType::DENY;
  // set IpType
  auto ipType = cfg::IpType::IP6;
  config.acls()[0].ipType() = ipType;

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
  config.acls()->resize(1);
  *config.acls()[0].name() = "acl1";
  *config.acls()[0].actionType() = cfg::AclActionType::DENY;

  // set lookupClassL2
  auto lookupClassL2 = cfg::AclLookupClass::DST_CLASS_L3_LOCAL_IP4;
  config.acls()[0].lookupClassL2() = lookupClassL2;

  // apply lookupClassL2 config and validate
  auto stateV2 = publishAndApplyConfig(stateV0, &config, platform.get());
  EXPECT_NE(nullptr, stateV2);
  auto aclV2 = stateV2->getAcl("acl1");
  ASSERT_NE(nullptr, aclV2);
  EXPECT_TRUE(aclV2->getLookupClassL2());
  EXPECT_EQ(aclV2->getLookupClassL2().value(), lookupClassL2);

  // set lookupClassNeighbor
  auto lookupClassNeighbor = cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0;
  config.acls()[0].lookupClassNeighbor() = lookupClassNeighbor;

  // apply lookupClassNeighbor config and validate
  auto stateV3 = publishAndApplyConfig(stateV0, &config, platform.get());
  EXPECT_NE(nullptr, stateV3);
  auto aclV3 = stateV3->getAcl("acl1");
  ASSERT_NE(nullptr, aclV3);
  EXPECT_TRUE(aclV3->getLookupClassNeighbor());
  EXPECT_EQ(aclV3->getLookupClassNeighbor().value(), lookupClassNeighbor);

  // set lookupClassRoute
  auto lookupClassRoute = cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0;
  config.acls()[0].lookupClassRoute() = lookupClassRoute;

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
  *counter.name() = "stat0.c";
  action.setTrafficCounter(counter);

  // test for lookupClassL2 serialization/de-serialization
  auto entryL2 = std::make_unique<AclEntry>(0, std::string("stat1"));
  auto lookupClassL2 = cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_7;
  entryL2->setLookupClassL2(lookupClassL2);
  entryL2->setAclAction(action);

  auto serialized = entryL2->toThrift();
  auto entryBackL2 = std::make_shared<AclEntry>(serialized);
  validateNodeSerialization(*entryL2);

  EXPECT_TRUE(*entryL2 == *entryBackL2);
  EXPECT_TRUE(entryBackL2->getLookupClassL2());
  EXPECT_EQ(entryBackL2->getLookupClassL2().value(), lookupClassL2);

  // test for lookupClassNeighbor serialization/de-serialization
  auto entryNeighbor = std::make_unique<AclEntry>(0, std::string("stat1"));
  auto lookupClassNeighbor = cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_7;
  entryNeighbor->setLookupClassNeighbor(lookupClassNeighbor);
  entryNeighbor->setAclAction(action);

  serialized = entryNeighbor->toThrift();
  auto entryBackNeighbor = std::make_shared<AclEntry>(serialized);
  validateNodeSerialization(*entryNeighbor);

  EXPECT_TRUE(*entryNeighbor == *entryBackNeighbor);
  EXPECT_TRUE(entryBackNeighbor->getLookupClassNeighbor());
  EXPECT_EQ(
      entryBackNeighbor->getLookupClassNeighbor().value(), lookupClassNeighbor);

  // test for lookupClassRoute serialization/de-serialization
  auto entryRoute = std::make_unique<AclEntry>(0, std::string("stat1"));
  auto lookupClassRoute = cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_7;
  entryRoute->setLookupClassRoute(lookupClassRoute);
  entryRoute->setAclAction(action);

  serialized = entryRoute->toThrift();
  auto entryBackRoute = std::make_shared<AclEntry>(serialized);
  validateNodeSerialization(*entryRoute);

  EXPECT_TRUE(*entryRoute == *entryBackRoute);
  EXPECT_TRUE(entryBackRoute->getLookupClassRoute());
  EXPECT_EQ(entryBackRoute->getLookupClassRoute().value(), lookupClassRoute);
}

TEST(Acl, InvalidTrafficCounter) {
  FLAGS_enable_acl_table_group = false;
  auto platform = createMockPlatform();
  auto stateV0 = make_shared<SwitchState>();

  cfg::SwitchConfig config;
  config.acls()->resize(1);
  *config.acls()[0].name() = "acl0";
  *config.acls()[0].actionType() = cfg::AclActionType::PERMIT;
  config.dataPlaneTrafficPolicy() = cfg::TrafficPolicyConfig();
  config.dataPlaneTrafficPolicy()->matchToAction()->resize(1);
  *config.dataPlaneTrafficPolicy()->matchToAction()[0].matcher() = "acl0";
  config.dataPlaneTrafficPolicy()->matchToAction()[0].action()->counter() =
      "stat0";

  EXPECT_THROW(
      publishAndApplyConfig(stateV0, &config, platform.get()), FbossError);
}

TEST(Acl, GetRequiredAclTableQualifiers) {
  cfg::SwitchConfig config;
  config.acls();
  config.acls()->resize(2);

  auto setAclQualifiers = [](std::string ip, std::string name) {
    cfg::AclEntry acl;
    acl.name() = name;
    acl.srcIp() = ip;
    acl.dstIp() = ip;
    acl.proto() = 50;
    acl.tcpFlagsBitMap() = 123;
    acl.srcPort() = 1001;
    acl.dstPort() = 2002;
    acl.ipFrag() = cfg::IpFragMatch::MATCH_ANY_FRAGMENT;
    acl.dscp() = 8;
    acl.ipType() = cfg::IpType::ANY;
    cfg::Ttl ttl;
    ttl.value() = 255;
    ttl.mask() = 0xff;
    acl.ttl() = ttl;
    acl.dstMac() = "a:b:c:d:e:f";
    acl.l4SrcPort() = 4005;
    acl.l4DstPort() = 4005;
    acl.lookupClassL2() = cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0;
    acl.lookupClassNeighbor() =
        cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_1;
    acl.lookupClassRoute() = cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_2;
    acl.actionType() = cfg::AclActionType::DENY;
    return acl;
  };
  config.acls()[0] = setAclQualifiers("10.0.0.1/32", "acl0");
  config.acls()[1] = setAclQualifiers("1::1/128", "acl1");

  config.dataPlaneTrafficPolicy() = cfg::TrafficPolicyConfig();
  config.dataPlaneTrafficPolicy()->matchToAction()->resize(2);
  config.dataPlaneTrafficPolicy()->matchToAction()[0].matcher() = "acl0";
  config.dataPlaneTrafficPolicy()->matchToAction()[0].matcher() = "acl1";

  auto platform = createMockPlatform();
  auto stateV0 = make_shared<SwitchState>();
  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  validateThriftMapMapSerialization(*stateV1->getMultiSwitchAcls());
  auto q0 = stateV1->getMultiSwitchAcls()
                ->getNodeIf("acl0")
                ->getRequiredAclTableQualifiers();
  auto q1 = stateV1->getMultiSwitchAcls()
                ->getNodeIf("acl1")
                ->getRequiredAclTableQualifiers();

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
