// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/bcm/BcmHostKey.h"
#include "fboss/agent/test/LabelForwardingUtils.h"

#include <gtest/gtest.h>

namespace {
auto constexpr kLinkLocal = "fe80::";
using namespace facebook::fboss;

void verifyUnlabeledHostKey(
    const HostKey& key,
    const BcmHostKey& hostKey,
    bool resolved) {
  EXPECT_EQ(key.getVrf(), hostKey.getVrf());
  EXPECT_EQ(key.addr(), hostKey.addr());
  EXPECT_EQ(key.intfID(), hostKey.intfID());
  EXPECT_EQ(key.hasLabel(), hostKey.hasLabel());
  EXPECT_THROW(key.getLabel(), FbossError);
  if (!resolved) {
    EXPECT_THROW(key.intfID().value(), std::bad_optional_access);
  } else {
    EXPECT_EQ(key.intfID(), hostKey.intf());
  }
  EXPECT_EQ(key.needsMplsTunnel(), hostKey.needsMplsTunnel());
  EXPECT_EQ(key.needsMplsTunnel(), false);
  EXPECT_THROW(key.tunnelLabelStack(), FbossError);
  EXPECT_THROW(hostKey.tunnelLabelStack(), FbossError);
}

void verifyLabeledHostKey(
    const HostKey& key,
    const BcmLabeledHostKey& hostKey) {
  EXPECT_EQ(key.getVrf(), hostKey.getVrf());
  EXPECT_EQ(key.addr(), hostKey.addr());
  EXPECT_EQ(key.intfID(), hostKey.intfID());
  EXPECT_EQ(key.hasLabel(), hostKey.hasLabel());
  EXPECT_EQ(key.getLabel(), hostKey.getLabel());
  EXPECT_EQ(key.intfID().value(), hostKey.intf());
  ASSERT_EQ(key.needsMplsTunnel(), hostKey.needsMplsTunnel());
  if (key.needsMplsTunnel()) {
    EXPECT_EQ(key.tunnelLabelStack(), hostKey.tunnelLabelStack());
  } else {
    EXPECT_THROW(key.tunnelLabelStack(), FbossError);
    EXPECT_THROW(hostKey.tunnelLabelStack(), FbossError);
  }
}

} // namespace

namespace facebook::fboss {

TEST(BcmHostKey, unResolvedNextHop) {
  auto nexthop = UnresolvedNextHop(folly::IPAddressV6("2401::1"), 0);
  auto bcmHostKey = BcmHostKey(0, nexthop);
  verifyUnlabeledHostKey(bcmHostKey, bcmHostKey, false /* unresolved */);
}

TEST(BcmHostKey, resolvedNextHop) {
  auto nexthop =
      ResolvedNextHop(folly::IPAddressV6(kLinkLocal), InterfaceID(1), 0);
  auto bcmHostKey = BcmHostKey(0, nexthop);
  verifyUnlabeledHostKey(bcmHostKey, bcmHostKey, true /* resolved */);
}

TEST(BcmHostKey, labeledNextHopWithSwap) {
  auto nexthop = ResolvedNextHop(
      folly::IPAddressV6(kLinkLocal),
      InterfaceID(1),
      0,
      util::getSwapAction(201));
  auto bcmLabeledHostKey = BcmLabeledHostKey(
      0,
      nexthop.labelForwardingAction()->swapWith().value(),
      nexthop.addr(),
      nexthop.intfID().value());
  verifyLabeledHostKey(bcmLabeledHostKey, bcmLabeledHostKey);
}

TEST(BcmHostKey, labeledNextHopWithPush) {
  auto nexthop = ResolvedNextHop(
      folly::IPAddressV6(kLinkLocal),
      InterfaceID(1),
      0,
      util::getPushAction(LabelForwardingAction::LabelStack{201, 202}));
  auto bcmLabeledHostKey = BcmLabeledHostKey(
      0,
      nexthop.labelForwardingAction()->pushStack().value(),
      nexthop.addr(),
      nexthop.intfID().value());

  verifyLabeledHostKey(bcmLabeledHostKey, bcmLabeledHostKey);
}

TEST(BcmHostKey, labeledNextHopWithPushOnlyOne) {
  auto nexthop = ResolvedNextHop(
      folly::IPAddressV6(kLinkLocal),
      InterfaceID(1),
      0,
      util::getPushAction(LabelForwardingAction::LabelStack{201}));
  auto bcmLabeledHostKey = BcmLabeledHostKey(
      0,
      nexthop.labelForwardingAction()->pushStack().value(),
      nexthop.addr(),
      nexthop.intfID().value());

  verifyLabeledHostKey(bcmLabeledHostKey, bcmLabeledHostKey);
}

TEST(BcmHostKey, unLabeledNextHopWitForLabelForwarding) {
  auto php = ResolvedNextHop(
      folly::IPAddressV6(kLinkLocal), InterfaceID(1), 0, util::getPhpAction());
  auto bcmHostKey = BcmHostKey(0, php);
  verifyUnlabeledHostKey(bcmHostKey, bcmHostKey, true /* resolved */);
}

TEST(BcmHostKey, LabeledKeyStr) {
  auto swapKey =
      BcmLabeledHostKey(0, 201, folly::IPAddress(kLinkLocal), InterfaceID(1));

  auto pushOneKey = BcmLabeledHostKey(
      0,
      LabelForwardingAction::LabelStack{301},
      folly::IPAddress(kLinkLocal),
      InterfaceID(2));

  auto pushManyKey = BcmLabeledHostKey(
      0,
      LabelForwardingAction::LabelStack{401, 402, 403, 404},
      folly::IPAddress(kLinkLocal),
      InterfaceID(3));

  EXPECT_EQ(
      swapKey.str(),
      folly::to<std::string>(
          "BcmLabeledHost: ", kLinkLocal, "@I", 1, "@label201"));
  EXPECT_EQ(
      pushOneKey.str(),
      folly::to<std::string>(
          "BcmLabeledHost: ", kLinkLocal, "@I", 2, "@stack[301]"));
  EXPECT_EQ(
      pushManyKey.str(),
      folly::to<std::string>(
          "BcmLabeledHost: ", kLinkLocal, "@I", 3, "@stack[401,402,403,404]"));
}

} // namespace facebook::fboss
